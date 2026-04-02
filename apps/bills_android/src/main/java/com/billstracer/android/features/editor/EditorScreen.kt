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
    onSaveRecord: () -> Unit,
    onSaveRawRecordText: (String) -> Unit,
    onRecordDraftChange: (String) -> Unit,
    modifier: Modifier = Modifier,
) {
    val activeRecord = state.activeRecordDocument
    val hasUnsavedChanges = activeRecord != null && state.recordDraftText != activeRecord.rawText
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
        } else {
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
                    onClick = onSaveRecord,
                    enabled = !state.isWorking && hasUnsavedChanges,
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
            Text(
                text = if (hasUnsavedChanges) {
                    "Draft changes are local until you press Save Record."
                } else {
                    "Editor is synced with the currently opened TXT source."
                },
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            EditorStructuredSectionContent(
                documentKey = activeRecord.relativePath,
                rawText = state.recordDraftText,
                persistedRawText = activeRecord.rawText,
                isWorking = state.isWorking,
                onRawTextChange = onRecordDraftChange,
                onCommitRawText = onSaveRawRecordText,
                modifier = Modifier.fillMaxWidth(),
            )
        }
    }
}
