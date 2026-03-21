package com.billstracer.android.features.editor

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import com.billstracer.android.model.RecordPreviewResult
import com.billstracer.android.platform.PaneContent
import com.billstracer.android.platform.SectionGroupCard
import com.billstracer.android.platform.StatPill
import com.billstracer.android.platform.YearMonthDigitsRow
import com.billstracer.android.platform.yearMonthOrNull
import com.billstracer.android.platform.yearMonthValidationMessage

@Composable
internal fun EditorScreen(
    state: EditorUiState,
    onSelectExistingRecordYear: (String) -> Unit,
    onSelectExistingRecordMonth: (String) -> Unit,
    onOpenSelectedExistingRecord: () -> Unit,
    onRecordPeriodYearChange: (String) -> Unit,
    onRecordPeriodMonthChange: (String) -> Unit,
    onOpenManualRecord: () -> Unit,
    onPreviewRecord: () -> Unit,
    onSaveRecord: () -> Unit,
    onRefreshRecordPeriods: () -> Unit,
    onRecordDraftChange: (String) -> Unit,
    onResetRecordDraft: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val activeRecord = state.activeRecordDocument
    val listedPeriods = state.listedRecordPeriods
    val hasUnsavedChanges = activeRecord != null && state.recordDraftText != activeRecord.rawText
    val existingYears = listedPeriods?.periods.orEmpty()
        .mapNotNull { period -> period.substringBefore('-').takeIf { it.length == 4 } }
        .distinct()
    val existingMonths = listedPeriods?.periods.orEmpty()
        .filter { period ->
            state.selectedExistingRecordYear.isNotBlank() &&
                period.startsWith("${state.selectedExistingRecordYear}-") &&
                period.length == 7
        }
        .map { period -> period.substringAfter('-') }
        .distinct()
    val manualPeriodError = yearMonthValidationMessage(
        yearInput = state.recordPeriodYearInput,
        monthInput = state.recordPeriodMonthInput,
    )
    val isManualPeriodValid = yearMonthOrNull(
        yearInput = state.recordPeriodYearInput,
        monthInput = state.recordPeriodMonthInput,
    ) != null
    var yearSelectorExpanded by rememberSaveable { mutableStateOf(false) }
    var monthSelectorExpanded by rememberSaveable { mutableStateOf(false) }

    PaneContent(modifier = modifier) {
        SectionGroupCard(title = "Open Saved TXT") {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                Box(modifier = Modifier.weight(1f)) {
                    OutlinedButton(
                        onClick = { yearSelectorExpanded = true },
                        enabled = !state.isInitializing && !state.isWorking && existingYears.isNotEmpty(),
                        modifier = Modifier
                            .fillMaxWidth()
                            .testTag("editor_year_selector_button"),
                    ) {
                        Text(
                            text = state.selectedExistingRecordYear.ifBlank { "Select year" },
                            fontFamily = FontFamily.Monospace,
                        )
                    }
                    DropdownMenu(
                        expanded = yearSelectorExpanded,
                        onDismissRequest = { yearSelectorExpanded = false },
                        modifier = Modifier.fillMaxWidth(0.9f),
                    ) {
                        existingYears.forEach { year ->
                            DropdownMenuItem(
                                text = { Text(text = year, fontFamily = FontFamily.Monospace) },
                                onClick = {
                                    yearSelectorExpanded = false
                                    onSelectExistingRecordYear(year)
                                },
                            )
                        }
                    }
                }
                Box(modifier = Modifier.weight(1f)) {
                    OutlinedButton(
                        onClick = { monthSelectorExpanded = true },
                        enabled = !state.isInitializing && !state.isWorking && existingMonths.isNotEmpty(),
                        modifier = Modifier
                            .fillMaxWidth()
                            .testTag("editor_month_selector_button"),
                    ) {
                        Text(
                            text = state.selectedExistingRecordMonth.ifBlank { "Select month" },
                            fontFamily = FontFamily.Monospace,
                        )
                    }
                    DropdownMenu(
                        expanded = monthSelectorExpanded,
                        onDismissRequest = { monthSelectorExpanded = false },
                        modifier = Modifier.fillMaxWidth(0.9f),
                    ) {
                        existingMonths.forEach { month ->
                            DropdownMenuItem(
                                text = { Text(text = month, fontFamily = FontFamily.Monospace) },
                                onClick = {
                                    monthSelectorExpanded = false
                                    onSelectExistingRecordMonth(month)
                                },
                            )
                        }
                    }
                }
            }
            if (listedPeriods != null) {
                Text(
                    text = listedPeriods.message,
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                Button(
                    onClick = onOpenSelectedExistingRecord,
                    enabled = !state.isInitializing &&
                        !state.isWorking &&
                        state.selectedExistingRecordYear.isNotBlank() &&
                        state.selectedExistingRecordMonth.isNotBlank(),
                    modifier = Modifier
                        .weight(1f)
                        .testTag("editor_open_existing_button"),
                ) {
                    Text("Open Existing")
                }
                OutlinedButton(
                    onClick = onRefreshRecordPeriods,
                    enabled = !state.isInitializing && !state.isWorking,
                    modifier = Modifier
                        .weight(1f)
                        .testTag("editor_refresh_button"),
                ) {
                    Text("Refresh Periods")
                }
            }
            if (listedPeriods == null || listedPeriods.periods.isEmpty()) {
                Text(
                    text = "No saved record periods yet. Existing year/month choices only come from list_periods over records/*.txt content.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
            listedPeriods?.invalidFiles?.forEach { invalidFile ->
                Text(
                    text = "[INVALID] ${invalidFile.path} | ${invalidFile.error}",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                    color = MaterialTheme.colorScheme.error,
                )
            }
        }
        SectionGroupCard(title = "Open Or Create Period") {
            Text(
                text = "Period (defaults to current month)",
                style = MaterialTheme.typography.bodyMedium,
                fontFamily = FontFamily.Monospace,
            )
            YearMonthDigitsRow(
                yearValue = state.recordPeriodYearInput,
                monthValue = state.recordPeriodMonthInput,
                onYearValueChange = onRecordPeriodYearChange,
                onMonthValueChange = onRecordPeriodMonthChange,
                enabled = !state.isInitializing && !state.isWorking,
                rowTag = "editor_manual_period_field",
                yearFieldTag = "editor_manual_period_year_field",
                monthFieldTag = "editor_manual_period_month_field",
                separatorTag = "editor_manual_period_separator",
                modifier = Modifier.fillMaxWidth(),
            )
            manualPeriodError?.let { validationMessage ->
                Text(
                    text = validationMessage,
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                    color = MaterialTheme.colorScheme.error,
                )
            }
            OutlinedButton(
                onClick = onOpenManualRecord,
                enabled = !state.isInitializing && !state.isWorking && isManualPeriodValid,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("editor_open_manual_button"),
            ) {
                Text("Open Or Create Period")
            }
        }
        if (activeRecord == null) {
            Text(
                text = "Open an existing TXT period from the selectors above, or open/create a TXT for the period entered here.",
                style = MaterialTheme.typography.bodyMedium,
                fontFamily = FontFamily.Monospace,
            )
        } else {
            Text(
                text = if (activeRecord.persisted) {
                    "Editing persisted TXT source: ${activeRecord.relativePath}"
                } else {
                    "Editing generated template: ${activeRecord.relativePath}"
                },
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            Text(
                text = if (hasUnsavedChanges) {
                    "Draft changes are local until you press Save Record."
                } else {
                    "Editor is synced with the currently opened TXT source."
                },
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            OutlinedTextField(
                value = state.recordDraftText,
                onValueChange = onRecordDraftChange,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .heightIn(min = 280.dp, max = 460.dp)
                    .testTag("editor_record_field"),
                textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
                label = { Text(text = activeRecord.relativePath, fontFamily = FontFamily.Monospace) },
                minLines = 14,
            )
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                Button(
                    onClick = onPreviewRecord,
                    enabled = !state.isInitializing && !state.isWorking && state.recordDraftText.isNotBlank(),
                    modifier = Modifier
                        .weight(1f)
                        .testTag("editor_preview_button"),
                ) {
                    Text("Preview")
                }
                Button(
                    onClick = onSaveRecord,
                    enabled = !state.isInitializing && !state.isWorking && hasUnsavedChanges,
                    modifier = Modifier
                        .weight(1f)
                        .testTag("editor_save_button"),
                ) {
                    Text("Save Record")
                }
                OutlinedButton(
                    onClick = onResetRecordDraft,
                    enabled = !state.isInitializing && !state.isWorking && hasUnsavedChanges,
                    modifier = Modifier
                        .weight(1f)
                        .testTag("editor_reset_button"),
                ) {
                    Text("Reset Draft")
                }
            }
        }
        state.recordPreviewResult?.let { preview ->
            EditorPreviewBlock(preview = preview)
        }
    }
}

@Composable
private fun EditorPreviewBlock(preview: RecordPreviewResult) {
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Text(
            text = "Preview",
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
        )
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState()),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            StatPill(label = "processed", value = preview.processed.toString())
            StatPill(label = "success", value = preview.success.toString())
            StatPill(label = "failure", value = preview.failure.toString())
        }
        if (preview.periods.isNotEmpty()) {
            Text(
                text = "Periods: ${preview.periods.joinToString(", ")}",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
        }
        preview.files.forEach { file ->
            Surface(
                modifier = Modifier.fillMaxWidth(),
                shape = RoundedCornerShape(18.dp),
                color = if (file.ok) {
                    MaterialTheme.colorScheme.secondaryContainer.copy(alpha = 0.78f)
                } else {
                    MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.9f)
                },
            ) {
                Column(
                    modifier = Modifier.padding(12.dp),
                    verticalArrangement = Arrangement.spacedBy(6.dp),
                ) {
                    Text(
                        text = file.path,
                        style = MaterialTheme.typography.bodySmall,
                        fontFamily = FontFamily.Monospace,
                        color = if (file.ok) {
                            MaterialTheme.colorScheme.onSecondaryContainer
                        } else {
                            MaterialTheme.colorScheme.onErrorContainer
                        },
                    )
                    Text(
                        text = if (file.ok) {
                            "period=${file.period} txns=${file.transactionCount} income=${file.totalIncome} expense=${file.totalExpense} balance=${file.balance}"
                        } else {
                            file.error ?: "Preview failed."
                        },
                        style = MaterialTheme.typography.bodyMedium,
                        fontFamily = FontFamily.Monospace,
                        color = if (file.ok) {
                            MaterialTheme.colorScheme.onSecondaryContainer
                        } else {
                            MaterialTheme.colorScheme.onErrorContainer
                        },
                    )
                }
            }
        }
    }
}
