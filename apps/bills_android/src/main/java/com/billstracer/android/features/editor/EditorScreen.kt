package com.billstracer.android.features.editor

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import com.billstracer.android.platform.PaneContent
import com.billstracer.android.platform.SectionGroupCard

@Composable
internal fun EditorScreen(
    state: EditorUiState,
    onScreenShown: () -> Unit,
    onSelectExistingRecordYear: (String) -> Unit,
    onSelectExistingRecordMonth: (String) -> Unit,
    onSaveStructuredRecord: () -> Unit,
    onSaveRawRecordText: (String) -> Unit,
    onRecordDraftChange: (String) -> Unit,
    onStructuredRemarkChange: (String) -> Unit,
    onAddStructuredEntry: (String, String) -> Unit,
    onRemoveStructuredEntry: (String, String, String) -> Unit,
    onStructuredEntryAmountChange: (String, String, String, String) -> Unit,
    onStructuredEntryDescriptionChange: (String, String, String, String) -> Unit,
    onStructuredEntryCommentChange: (String, String, String, String) -> Unit,
    onEnterRawExpertMode: () -> Unit,
    onReturnToStructuredMode: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val activeRecord = state.activeRecordDocument
    val structuredBaseline = activeRecord?.structuredDocument?.toEditorStructuredDraft()
    val hasStructuredChanges = state.structuredDraft != null &&
        structuredBaseline != null &&
        state.structuredDraft != structuredBaseline
    val hasRawChanges = activeRecord != null && state.recordDraftText != activeRecord.rawText
    val hasUnsavedChanges = when (state.editorMode) {
        EditorMode.Structured -> hasStructuredChanges
        EditorMode.RawExpert -> hasRawChanges
    }
    val saveEnabled = !state.isWorking && hasUnsavedChanges &&
        (state.editorMode == EditorMode.RawExpert || !state.hasIncompleteEntries)

    val existingYears = state.persistedRecordPeriods
        .mapNotNull { period -> period.substringBefore('-').takeIf { it.length == 4 } }
        .distinct()
        .sorted()
    val existingMonths = state.persistedRecordPeriods
        .filter { period ->
            state.selectedExistingRecordYear.isNotBlank() &&
                period.startsWith("${state.selectedExistingRecordYear}-") &&
                period.length == 7
        }
        .map { period -> period.substringAfter('-') }
        .distinct()
        .sorted()
    val hasAvailablePeriods = state.persistedRecordPeriods.isNotEmpty()
    var yearSelectorExpanded by remember { mutableStateOf(false) }
    var monthSelectorExpanded by remember { mutableStateOf(false) }

    LaunchedEffect(state.isInitializing) {
        if (!state.isInitializing) {
            onScreenShown()
        }
    }

    PaneContent(modifier = modifier) {
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
                text = "Editor opens the canonical TXT source under records/. The current month is selected automatically and will be created if the TXT file does not exist yet.",
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
                    text = "Selecting a year/month opens records/YYYY/YYYY-MM.txt. Entering this page jumps to the current month automatically.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
        }

        if (activeRecord == null) {
            Text(
                text = "Current month will open automatically once the available periods finish loading.",
                style = MaterialTheme.typography.bodyMedium,
                fontFamily = FontFamily.Monospace,
            )
            return@PaneContent
        }

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
        ) {
            Text(
                text = "Editing persisted TXT source: ${activeRecord.relativePath}",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier.weight(1f),
            )
            Button(
                onClick = {
                    if (state.editorMode == EditorMode.Structured) {
                        onSaveStructuredRecord()
                    } else {
                        onSaveRawRecordText(state.recordDraftText)
                    }
                },
                enabled = saveEnabled,
                modifier = Modifier
                    .padding(start = 12.dp)
                    .testTag("editor_save_button"),
            ) {
                Text(
                    text = "✓",
                    fontFamily = FontFamily.Monospace,
                )
            }
        }

        val syncMessage = when {
            state.editorMode == EditorMode.Structured && state.hasIncompleteEntries ->
                "Complete or delete unfinished entries before saving."
            hasUnsavedChanges && state.editorMode == EditorMode.Structured ->
                "Structured draft changes are local until you press Save Record."
            hasUnsavedChanges && state.editorMode == EditorMode.RawExpert ->
                "Raw TXT edits are local until you press Save Record."
            state.editorMode == EditorMode.RawExpert ->
                "Expert Raw TXT mode is active."
            else ->
                "Editor is synced with the currently opened TXT source."
        }
        Text(
            text = syncMessage,
            style = MaterialTheme.typography.bodySmall,
            fontFamily = FontFamily.Monospace,
        )

        if (state.editorMode == EditorMode.Structured && state.structuredDraft != null) {
            EditorStructuredSectionContent(
                documentKey = activeRecord.relativePath,
                draft = state.structuredDraft,
                hasIncompleteEntries = state.hasIncompleteEntries,
                isWorking = state.isWorking,
                onRemarkChange = onStructuredRemarkChange,
                onAddEntry = onAddStructuredEntry,
                onRemoveEntry = onRemoveStructuredEntry,
                onEntryAmountChange = onStructuredEntryAmountChange,
                onEntryDescriptionChange = onStructuredEntryDescriptionChange,
                onEntryCommentChange = onStructuredEntryCommentChange,
                onEnterRawExpertMode = onEnterRawExpertMode,
                modifier = Modifier.fillMaxWidth(),
            )
        } else {
            EditorRawExpertContent(
                rawText = state.recordDraftText,
                fallbackReason = activeRecord.rawFallbackReason,
                canReturnToStructured = activeRecord.structuredDocument != null,
                onRawTextChange = onRecordDraftChange,
                onReturnToStructured = onReturnToStructuredMode,
                modifier = Modifier.fillMaxWidth(),
            )
        }
    }
}
