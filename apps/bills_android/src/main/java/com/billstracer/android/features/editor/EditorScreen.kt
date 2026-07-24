package com.billstracer.android.features.editor

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.material3.Button
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
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
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.luminance
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Check
import java.util.Locale
import kotlin.math.abs
import com.billstracer.android.features.common.YearMonthPickerAvailability
import com.billstracer.android.features.common.YearMonthPickerSheet
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

    val hasAvailablePeriods = state.persistedRecordPeriods.isNotEmpty()
    val monthPickerAvailability = remember(state.persistedRecordPeriods) {
        YearMonthPickerAvailability.fromPeriods(state.persistedRecordPeriods)
    }
    val selectedPeriod = listOf(
        state.selectedExistingRecordYear,
        state.selectedExistingRecordMonth,
    ).filter { it.isNotBlank() }.joinToString("-")
    var monthPickerVisible by remember { mutableStateOf(false) }
    var rawExpertDialogRequested by remember { mutableStateOf(false) }

    LaunchedEffect(state.isInitializing) {
        if (!state.isInitializing) {
            onScreenShown()
        }
    }

    PaneContent(
        modifier = modifier,
        scrollEnabled = state.editorMode != EditorMode.RawExpert,
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
        if (state.editorMode == EditorMode.Structured) {
            SectionGroupCard(title = "Select Month") {
            OutlinedButton(
                onClick = { monthPickerVisible = true },
                enabled = !state.isInitializing && !state.isWorking && hasAvailablePeriods,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("editor_month_picker_button"),
            ) {
                Text(
                    text = selectedPeriod.ifBlank { "Select year and month" },
                    style = MaterialTheme.typography.titleMedium,
                    fontFamily = FontFamily.Monospace,
                )
            }
            }
        }

        if (monthPickerVisible) {
            YearMonthPickerSheet(
                selectedYearMonth = selectedPeriod,
                availability = monthPickerAvailability,
                title = "Select TXT month",
                currentText = "Current selection: ${selectedPeriod.ifBlank { "None" }}",
                onYearMonthSelected = { period ->
                    onSelectExistingRecordYear(period.substringBefore('-'))
                    onSelectExistingRecordMonth(period.substringAfter('-'))
                },
                onDismissRequest = { monthPickerVisible = false },
            )
        }

        if (activeRecord == null) {
            return@PaneContent
        }

        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween,
        ) {
            Column(
                verticalArrangement = Arrangement.spacedBy(4.dp),
            ) {
                EditorSummaryLine(
                    label = "收入",
                    amount = state.recordSummary?.income,
                    baseColor = GitDiffAddedGreen,
                )
                EditorSummaryLine(
                    label = "支出",
                    amount = state.recordSummary?.expense?.let(::abs),
                    baseColor = GitDiffRemovedRed,
                    modifier = Modifier.testTag("editor_income_expense_summary"),
                )
            }
            Button(
                onClick = {
                    if (state.editorMode == EditorMode.Structured) {
                        onSaveStructuredRecord()
                    } else {
                        onSaveRawRecordText(state.recordDraftText)
                    }
                },
                enabled = saveEnabled,
                modifier = Modifier.testTag("editor_save_button"),
            ) {
                Text(text = "✓", fontFamily = FontFamily.Monospace)
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
            else -> null
        }
        syncMessage?.let { message ->
            Text(
                text = message,
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
        }

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
                onEnterRawExpertMode = {
                    rawExpertDialogRequested = true
                    onEnterRawExpertMode()
                },
                modifier = Modifier.fillMaxWidth(),
            )
        } else if (activeRecord.structuredDocument == null) {
            EditorRawExpertContent(
                rawText = state.recordDraftText,
                fallbackReason = activeRecord.rawFallbackReason,
                canReturnToStructured = false,
                onRawTextChange = onRecordDraftChange,
                onReturnToStructured = onReturnToStructuredMode,
                modifier = if (state.editorMode == EditorMode.RawExpert) {
                    Modifier
                        .fillMaxWidth()
                        .weight(1f)
                } else {
                    Modifier.fillMaxWidth()
                },
            )
        }
    }

    val showRawExpertDialog = rawExpertDialogRequested ||
        (state.editorMode == EditorMode.RawExpert && activeRecord?.structuredDocument != null)
    if (showRawExpertDialog && activeRecord != null) {
        Dialog(
            onDismissRequest = {
                rawExpertDialogRequested = false
                onReturnToStructuredMode()
            },
            properties = DialogProperties(
                usePlatformDefaultWidth = false,
                decorFitsSystemWindows = false,
            ),
        ) {
            Surface(
                modifier = Modifier.fillMaxSize(),
                color = MaterialTheme.colorScheme.background,
            ) {
                Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .statusBarsPadding()
                        .navigationBarsPadding(),
                ) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 8.dp, vertical = 4.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween,
                    ) {
                        IconButton(
                            onClick = {
                                rawExpertDialogRequested = false
                                onReturnToStructuredMode()
                            },
                            modifier = Modifier.testTag("editor_raw_close_button"),
                        ) {
                            Icon(
                                imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                                contentDescription = "Back to editor",
                            )
                        }
                        Text(
                            text = "Raw TXT",
                            style = MaterialTheme.typography.titleLarge,
                            fontFamily = FontFamily.Monospace,
                        )
                        IconButton(
                            onClick = {
                                onSaveRawRecordText(state.recordDraftText)
                                rawExpertDialogRequested = false
                            },
                            enabled = !state.isWorking && hasRawChanges,
                            modifier = Modifier.testTag("editor_raw_save_button"),
                        ) {
                            Icon(
                                imageVector = Icons.Filled.Check,
                                contentDescription = "Save TXT",
                            )
                        }
                    }
                    val rawEditorBackground = if (
                        MaterialTheme.colorScheme.background.luminance() < 0.5f
                    ) {
                        Color.Black
                    } else {
                        Color.White
                    }
                    RawTextEditText(
                        value = state.recordDraftText,
                        onValueChange = onRecordDraftChange,
                        modifier = Modifier
                            .fillMaxWidth()
                            .weight(1f)
                            .testTag("editor_raw_fullscreen_field"),
                        backgroundColor = rawEditorBackground,
                        textColorOverride = if (rawEditorBackground == Color.Black) {
                            Color.White
                        } else {
                            Color.Black
                        },
                    )
                }
            }
        }
    }
}

@Composable
private fun EditorSummaryLine(
    label: String,
    amount: Double?,
    baseColor: androidx.compose.ui.graphics.Color,
    modifier: Modifier = Modifier,
) {
    Row(
        modifier = modifier,
        horizontalArrangement = Arrangement.spacedBy(6.dp),
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
            color = baseColor,
        )
        Text(
            text = amount?.let { String.format(Locale.US, "%.2f", it) } ?: "--",
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
            color = baseColor,
        )
    }
}

private val GitDiffAddedGreen = Color(0xFF1A7F37)
private val GitDiffRemovedRed = Color(0xFFCF222E)
