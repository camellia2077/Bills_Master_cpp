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
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.TextRange
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.unit.dp
import com.billstracer.android.model.RecordPreviewResult
import com.billstracer.android.platform.PaneContent
import com.billstracer.android.platform.SectionGroupCard
import com.billstracer.android.platform.StatPill

@Composable
internal fun EditorScreen(
    state: EditorUiState,
    onSelectExistingRecordYear: (String) -> Unit,
    onSelectExistingRecordMonth: (String) -> Unit,
    onOpenSelectedExistingRecord: () -> Unit,
    onPreviewRecord: () -> Unit,
    onSaveRecord: () -> Unit,
    onRecordDraftChange: (String) -> Unit,
    onResetRecordDraft: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val activeRecord = state.activeRecordDocument
    val hasUnsavedChanges = activeRecord != null && state.recordDraftText != activeRecord.rawText
    val existingYears = state.persistedRecordPeriods
        .mapNotNull { period -> period.substringBefore('-').takeIf { it.length == 4 } }
        .distinct()
    val existingMonths = state.persistedRecordPeriods
        .filter { period ->
            state.selectedExistingRecordYear.isNotBlank() &&
                period.startsWith("${state.selectedExistingRecordYear}-") &&
                period.length == 7
        }
        .map { period -> period.substringAfter('-') }
        .distinct()
    val hasAvailablePeriods = state.persistedRecordPeriods.isNotEmpty()
    var yearSelectorExpanded by rememberSaveable { mutableStateOf(false) }
    var monthSelectorExpanded by rememberSaveable { mutableStateOf(false) }
    var isEditorFieldFocused by remember { mutableStateOf(false) }
    var draftFieldValue by rememberSaveable(
        activeRecord?.relativePath,
        stateSaver = TextFieldValue.Saver,
    ) {
        mutableStateOf(TextFieldValue(state.recordDraftText))
    }

    LaunchedEffect(activeRecord?.relativePath, state.recordDraftText) {
        if (draftFieldValue.text != state.recordDraftText) {
            val selectionEnd = minOf(draftFieldValue.selection.end, state.recordDraftText.length)
            val selectionStart = minOf(draftFieldValue.selection.start, selectionEnd)
            draftFieldValue = TextFieldValue(
                text = state.recordDraftText,
                selection = TextRange(selectionStart, selectionEnd),
            )
        }
    }
    LaunchedEffect(activeRecord?.relativePath) {
        isEditorFieldFocused = false
    }

    PaneContent(
        modifier = modifier,
        scrollEnabled = !isEditorFieldFocused,
    ) {
        if (state.statusMessage.isNotBlank()) {
            Text(
                text = state.statusMessage,
                style = MaterialTheme.typography.bodyMedium,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier.testTag("editor_status_message"),
            )
        }
        state.errorMessage?.takeIf { it.isNotBlank() }?.let { errorMessage ->
            Surface(
                color = MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.92f),
                shape = MaterialTheme.shapes.medium,
            ) {
                Text(
                    text = errorMessage,
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(12.dp)
                        .testTag("editor_error_message"),
                    color = MaterialTheme.colorScheme.onErrorContainer,
                    style = MaterialTheme.typography.bodyMedium,
                    fontFamily = FontFamily.Monospace,
                )
            }
        }
        SectionGroupCard(title = "Open Existing Period") {
            Text(
                text = "Editor periods come from imported SQLite months. Opening a period still reads the canonical TXT source under records/.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
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
            Button(
                onClick = onOpenSelectedExistingRecord,
                enabled = !state.isInitializing &&
                    !state.isWorking &&
                    state.selectedExistingRecordYear.isNotBlank() &&
                    state.selectedExistingRecordMonth.isNotBlank(),
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("editor_open_existing_button"),
            ) {
                Text("Open Existing")
            }
            if (state.isInitializing) {
                Text(
                    text = "Loading imported periods from SQLite...",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            } else if (!hasAvailablePeriods) {
                Text(
                    text = "No imported months found in SQLite yet. Import TXT files first.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                    modifier = Modifier.testTag("editor_empty_state_message"),
                )
            } else {
                Text(
                    text = "Select an imported year/month and open its canonical TXT source at records/YYYY/YYYY-MM.txt.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
        }
        if (activeRecord == null) {
            Text(
                text = "Choose an imported year/month above, then open its canonical TXT source to start editing.",
                style = MaterialTheme.typography.bodyMedium,
                fontFamily = FontFamily.Monospace,
            )
        } else {
            Text(
                text = "Editing persisted TXT source: ${activeRecord.relativePath}",
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
                value = draftFieldValue,
                onValueChange = { nextValue ->
                    draftFieldValue = nextValue
                    if (nextValue.text != state.recordDraftText) {
                        onRecordDraftChange(nextValue.text)
                    }
                },
                enabled = !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .heightIn(min = 280.dp, max = 460.dp)
                    .onFocusChanged { focusState ->
                        isEditorFieldFocused = focusState.isFocused
                    }
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
                    enabled = !state.isWorking && state.recordDraftText.isNotBlank(),
                    modifier = Modifier
                        .weight(1f)
                        .testTag("editor_preview_button"),
                ) {
                    Text("Preview")
                }
                Button(
                    onClick = onSaveRecord,
                    enabled = !state.isWorking && hasUnsavedChanges,
                    modifier = Modifier
                        .weight(1f)
                        .testTag("editor_save_button"),
                ) {
                    Text("Save Record")
                }
                OutlinedButton(
                    onClick = onResetRecordDraft,
                    enabled = !state.isWorking && hasUnsavedChanges,
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
