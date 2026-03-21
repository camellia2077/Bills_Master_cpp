package com.billstracer.android.platform

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp

private val compactYearFieldWidth = 136.dp
private val compactMonthFieldWidth = 104.dp

@Composable
internal fun YearMonthDigitsRow(
    yearValue: String,
    monthValue: String,
    onYearValueChange: (String) -> Unit,
    onMonthValueChange: (String) -> Unit,
    enabled: Boolean,
    rowTag: String,
    yearFieldTag: String,
    monthFieldTag: String,
    separatorTag: String,
    modifier: Modifier = Modifier,
) {
    val monthFocusRequester = remember { FocusRequester() }
    val focusManager = LocalFocusManager.current

    Row(
        modifier = modifier.testTag(rowTag),
        horizontalArrangement = Arrangement.spacedBy(6.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        YearDigitsField(
            value = yearValue,
            onValueChange = onYearValueChange,
            enabled = enabled,
            label = "Year",
            placeholder = "YYYY",
            testTag = yearFieldTag,
            modifier = Modifier.width(compactYearFieldWidth),
            imeAction = ImeAction.Next,
            onCompleted = { monthFocusRequester.requestFocus() },
        )
        Text(
            text = "-",
            style = MaterialTheme.typography.titleLarge,
            fontFamily = FontFamily.Monospace,
            modifier = Modifier.testTag(separatorTag),
        )
        MonthDigitsField(
            value = monthValue,
            onValueChange = onMonthValueChange,
            enabled = enabled,
            label = "Month",
            placeholder = "MM",
            testTag = monthFieldTag,
            modifier = Modifier
                .width(compactMonthFieldWidth)
                .focusRequester(monthFocusRequester),
            onCompleted = { focusManager.clearFocus() },
        )
    }
}

@Composable
internal fun YearDigitsField(
    value: String,
    onValueChange: (String) -> Unit,
    enabled: Boolean,
    label: String,
    placeholder: String,
    testTag: String,
    modifier: Modifier = Modifier,
    imeAction: ImeAction = ImeAction.Done,
    onCompleted: (() -> Unit)? = null,
) {
    val focusManager = LocalFocusManager.current

    OutlinedTextField(
        value = value,
        onValueChange = { rawInput ->
            val sanitized = sanitizeYearInput(rawInput)
            onValueChange(sanitized)
            if (sanitized.length == 4) {
                onCompleted?.invoke() ?: focusManager.clearFocus()
            }
        },
        enabled = enabled,
        modifier = modifier.testTag(testTag),
        label = { Text(text = label, fontFamily = FontFamily.Monospace) },
        placeholder = { Text(text = placeholder, fontFamily = FontFamily.Monospace) },
        textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
        singleLine = true,
        keyboardOptions = KeyboardOptions(
            keyboardType = KeyboardType.Number,
            imeAction = imeAction,
        ),
        keyboardActions = KeyboardActions(
            onNext = { onCompleted?.invoke() },
            onDone = {
                onCompleted?.invoke() ?: focusManager.clearFocus()
            },
        ),
    )
}

@Composable
internal fun MonthDigitsField(
    value: String,
    onValueChange: (String) -> Unit,
    enabled: Boolean,
    label: String,
    placeholder: String,
    testTag: String,
    modifier: Modifier = Modifier,
    onCompleted: (() -> Unit)? = null,
) {
    val focusManager = LocalFocusManager.current

    OutlinedTextField(
        value = value,
        onValueChange = { rawInput ->
            val sanitized = sanitizeMonthInput(rawInput)
            val normalized = if (sanitized.length == 1 && sanitized[0] in '2'..'9') {
                normalizeMonthInputCompletion(sanitized)
            } else {
                sanitized
            }
            onValueChange(normalized)
            if (normalized.length == 2) {
                onCompleted?.invoke() ?: focusManager.clearFocus()
            }
        },
        enabled = enabled,
        modifier = modifier
            .onFocusChanged { focusState ->
                if (!focusState.isFocused) {
                    val normalized = normalizeMonthInputCompletion(value)
                    if (normalized != value) {
                        onValueChange(normalized)
                    }
                }
            }
            .testTag(testTag),
        label = { Text(text = label, fontFamily = FontFamily.Monospace) },
        placeholder = { Text(text = placeholder, fontFamily = FontFamily.Monospace) },
        textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
        singleLine = true,
        keyboardOptions = KeyboardOptions(
            keyboardType = KeyboardType.Number,
            imeAction = ImeAction.Done,
        ),
        keyboardActions = KeyboardActions(
            onDone = {
                val normalized = normalizeMonthInputCompletion(value)
                if (normalized != value) {
                    onValueChange(normalized)
                }
                onCompleted?.invoke() ?: focusManager.clearFocus()
            },
        ),
    )
}

internal fun sanitizeYearInput(input: String): String =
    input.filter(Char::isDigit).take(4)

internal fun sanitizeMonthInput(input: String): String =
    input.filter(Char::isDigit).take(2)

internal fun normalizeMonthInputCompletion(input: String): String {
    val sanitized = sanitizeMonthInput(input)
    if (sanitized.length != 1) {
        return sanitized
    }
    val monthValue = sanitized.toIntOrNull() ?: return sanitized
    return if (monthValue in 1..9) sanitized.padStart(2, '0') else sanitized
}

internal fun yearInputOrNull(yearInput: String): String? =
    yearInput.takeIf { it.length == 4 }

internal fun yearInputValidationMessage(yearInput: String): String? {
    if (yearInput.isBlank()) {
        return null
    }
    return if (yearInput.length == 4) null else "Year must be 4 digits."
}

internal fun joinYearMonthText(yearInput: String, monthInput: String): String =
    when {
        yearInput.isBlank() && monthInput.isBlank() -> ""
        monthInput.isBlank() -> yearInput
        yearInput.isBlank() -> "-$monthInput"
        else -> "$yearInput-$monthInput"
    }

internal fun splitYearMonth(period: String): Pair<String, String> {
    val trimmed = period.trim()
    val year = sanitizeYearInput(trimmed.substringBefore('-'))
    val month = sanitizeMonthInput(trimmed.substringAfter('-', ""))
    return year to month
}

internal fun yearMonthOrNull(
    yearInput: String,
    monthInput: String,
): String? {
    if (yearInput.length != 4 || monthInput.length != 2) {
        return null
    }
    val monthValue = monthInput.toIntOrNull() ?: return null
    if (monthValue !in 1..12) {
        return null
    }
    return "$yearInput-$monthInput"
}

internal fun yearMonthValidationMessage(
    yearInput: String,
    monthInput: String,
): String? {
    if (yearInput.isBlank() && monthInput.isBlank()) {
        return null
    }
    if (yearInput.length != 4) {
        return "Year must be 4 digits."
    }
    if (monthInput.length != 2) {
        return "Month must be 2 digits."
    }
    val monthValue = monthInput.toIntOrNull()
    if (monthValue == null || monthValue !in 1..12) {
        return "Month must be between 01 and 12."
    }
    return null
}
