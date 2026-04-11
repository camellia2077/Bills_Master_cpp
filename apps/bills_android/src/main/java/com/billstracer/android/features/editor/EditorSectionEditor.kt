package com.billstracer.android.features.editor

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp

@Composable
internal fun EditorStructuredSectionContent(
    documentKey: String,
    draft: EditorStructuredDraftUiModel,
    hasIncompleteEntries: Boolean,
    isWorking: Boolean,
    onRemarkChange: (String) -> Unit,
    onAddEntry: (String, String) -> Unit,
    onRemoveEntry: (String, String, String) -> Unit,
    onEntryAmountChange: (String, String, String, String) -> Unit,
    onEntryDescriptionChange: (String, String, String, String) -> Unit,
    onEntryCommentChange: (String, String, String, String) -> Unit,
    onEnterRawExpertMode: () -> Unit,
    modifier: Modifier = Modifier,
) {
    var searchQuery by rememberSaveable(documentKey) { mutableStateOf("") }
    val parentExpandedStates = remember(documentKey) { mutableStateMapOf<String, Boolean>() }
    val subSectionExpandedStates = remember(documentKey) { mutableStateMapOf<String, Boolean>() }
    val visibleSections = remember(draft.sections, searchQuery) {
        filterEditorSections(draft.sections, searchQuery)
    }

    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        EditorHeaderCard(
            documentKey = documentKey,
            draft = draft,
            onRemarkChange = onRemarkChange,
        )
        OutlinedButton(
            onClick = onEnterRawExpertMode,
            enabled = !isWorking && !hasIncompleteEntries,
            modifier = Modifier
                .fillMaxWidth()
                .testTag("editor_view_raw_button"),
        ) {
            Text("Expert Raw TXT")
        }
        if (hasIncompleteEntries) {
            Surface(
                modifier = Modifier.fillMaxWidth(),
                color = MaterialTheme.colorScheme.secondaryContainer.copy(alpha = 0.72f),
            ) {
                Text(
                    text = "Complete or delete unfinished entries before saving or opening Raw TXT.",
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(12.dp)
                        .testTag("editor_incomplete_entries_message"),
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
        }
        OutlinedTextField(
            value = searchQuery,
            onValueChange = { searchQuery = it },
            enabled = true,
            modifier = Modifier
                .fillMaxWidth()
                .testTag("editor_section_search_field"),
            label = { Text("Search titles", fontFamily = FontFamily.Monospace) },
            singleLine = true,
            textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
        )

        visibleSections.forEach { parentSection ->
            val parentExpanded = if (searchQuery.isNotBlank()) {
                true
            } else {
                parentExpandedStates[parentSection.title] ?: false
            }
            Surface(
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("editor_parent_section_${editorSectionTagSuffix(parentSection.title)}"),
                shape = RoundedCornerShape(18.dp),
                color = MaterialTheme.colorScheme.surfaceContainer,
            ) {
                Column(
                    modifier = Modifier.padding(12.dp),
                    verticalArrangement = Arrangement.spacedBy(10.dp),
                ) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .clickable { parentExpandedStates[parentSection.title] = !parentExpanded }
                            .padding(vertical = 4.dp)
                            .testTag("editor_parent_toggle_${editorSectionTagSuffix(parentSection.title)}"),
                        horizontalArrangement = Arrangement.SpaceBetween,
                    ) {
                        Text(
                            text = parentSection.title,
                            style = MaterialTheme.typography.titleSmall,
                            fontFamily = FontFamily.Monospace,
                        )
                        Text(
                            text = if (parentExpanded) "▼" else "▶",
                            style = MaterialTheme.typography.titleSmall,
                            fontFamily = FontFamily.Monospace,
                        )
                    }

                    if (parentExpanded) {
                        parentSection.subSections.forEach { subSection ->
                            EditorSubSectionEntriesCard(
                                parentTitle = parentSection.title,
                                subSection = subSection,
                                isExpanded = if (searchQuery.isNotBlank()) {
                                    true
                                } else {
                                    subSectionExpandedStates[subSectionKey(parentSection.title, subSection.title)] ?: true
                                },
                                onToggleExpanded = { nextExpanded ->
                                    subSectionExpandedStates[subSectionKey(parentSection.title, subSection.title)] = nextExpanded
                                },
                                onAddEntry = onAddEntry,
                                onRemoveEntry = onRemoveEntry,
                                onEntryAmountChange = onEntryAmountChange,
                                onEntryDescriptionChange = onEntryDescriptionChange,
                            )
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun EditorHeaderCard(
    documentKey: String,
    draft: EditorStructuredDraftUiModel,
    onRemarkChange: (String) -> Unit,
) {
    var headerExpanded by rememberSaveable(documentKey) { mutableStateOf(false) }

    Surface(
        modifier = Modifier.fillMaxWidth(),
        color = MaterialTheme.colorScheme.surfaceContainerHighest.copy(alpha = 0.65f),
    ) {
        Column(
            modifier = Modifier.padding(12.dp),
            verticalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .clickable { headerExpanded = !headerExpanded }
                    .padding(vertical = 4.dp)
                    .testTag("editor_header_toggle"),
                horizontalArrangement = Arrangement.SpaceBetween,
            ) {
                Text(
                    text = "Header",
                    style = MaterialTheme.typography.titleSmall,
                    fontFamily = FontFamily.Monospace,
                )
                Text(
                    text = if (headerExpanded) "▼" else "▶",
                    style = MaterialTheme.typography.titleSmall,
                    fontFamily = FontFamily.Monospace,
                )
            }

            if (headerExpanded) {
                Text(
                    text = draft.dateLine,
                    style = MaterialTheme.typography.bodyMedium,
                    fontFamily = FontFamily.Monospace,
                )
                OutlinedTextField(
                    value = draft.remarkText,
                    onValueChange = onRemarkChange,
                    modifier = Modifier
                        .fillMaxWidth()
                        .heightIn(min = 88.dp, max = 160.dp)
                        .testTag("editor_remark_field"),
                    label = { Text("Remark", fontFamily = FontFamily.Monospace) },
                    textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
                    minLines = 3,
                )
            }
        }
    }
}

@Composable
private fun EditorSubSectionEntriesCard(
    parentTitle: String,
    subSection: EditorSubSectionDraftUiModel,
    isExpanded: Boolean,
    onToggleExpanded: (Boolean) -> Unit,
    onAddEntry: (String, String) -> Unit,
    onRemoveEntry: (String, String, String) -> Unit,
    onEntryAmountChange: (String, String, String, String) -> Unit,
    onEntryDescriptionChange: (String, String, String, String) -> Unit,
) {
    Surface(
        modifier = Modifier.fillMaxWidth(),
        color = MaterialTheme.colorScheme.surface,
    ) {
        Column(
            modifier = Modifier.padding(12.dp),
            verticalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            Row(
                modifier = Modifier.testTag(
                    "editor_subsection_toggle_${editorSectionTagSuffix(parentTitle)}_${editorSectionTagSuffix(subSection.title)}",
                )
                    .fillMaxWidth()
                    .clickable { onToggleExpanded(!isExpanded) }
                    .padding(vertical = 4.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
            ) {
                Text(
                    text = subSection.title,
                    style = MaterialTheme.typography.titleSmall,
                    fontFamily = FontFamily.Monospace,
                    modifier = Modifier.testTag(
                        "editor_subsection_title_${editorSectionTagSuffix(subSection.title)}",
                    ),
                )
                Text(
                    text = if (isExpanded) "▼" else "▶",
                    style = MaterialTheme.typography.titleSmall,
                    fontFamily = FontFamily.Monospace,
                )
            }

            if (isExpanded) {
                subSection.entries.forEachIndexed { index, entry ->
                    EditorEntryRow(
                        parentTitle = parentTitle,
                        subSectionTitle = subSection.title,
                        entry = entry,
                        onRemoveEntry = onRemoveEntry,
                        onEntryAmountChange = onEntryAmountChange,
                        onEntryDescriptionChange = onEntryDescriptionChange,
                    )
                    if (index < subSection.entries.lastIndex) {
                        HorizontalDivider(color = MaterialTheme.colorScheme.outlineVariant)
                    }
                }
                OutlinedButton(
                    onClick = { onAddEntry(parentTitle, subSection.title) },
                    modifier = Modifier
                        .fillMaxWidth()
                        .testTag("editor_add_entry_${editorSectionTagSuffix(subSection.title)}"),
                ) {
                    Text("Add Item")
                }
            }
        }
    }
}

private fun subSectionKey(parentTitle: String, subSectionTitle: String): String =
    "$parentTitle::$subSectionTitle"

@Composable
private fun EditorEntryRow(
    parentTitle: String,
    subSectionTitle: String,
    entry: EditorEntryDraftUiModel,
    onRemoveEntry: (String, String, String) -> Unit,
    onEntryAmountChange: (String, String, String, String) -> Unit,
    onEntryDescriptionChange: (String, String, String, String) -> Unit,
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp),
    ) {
                OutlinedTextField(
                    value = entry.description,
                    onValueChange = { nextValue ->
                        onEntryDescriptionChange(parentTitle, subSectionTitle, entry.id, nextValue)
                    },
            modifier = Modifier
                .weight(1f)
                .testTag("editor_entry_description_${entry.id}"),
                    label = { Text("Item", fontFamily = FontFamily.Monospace) },
                    textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
                    singleLine = true,
                    shape = RoundedCornerShape(20.dp),
                )
                OutlinedTextField(
                    value = entry.amountExpression,
                    onValueChange = { nextValue ->
                        onEntryAmountChange(parentTitle, subSectionTitle, entry.id, nextValue)
            },
            modifier = Modifier
                .width(132.dp)
                .testTag("editor_entry_amount_${entry.id}"),
            label = { Text("Amount", fontFamily = FontFamily.Monospace) },
            singleLine = true,
                    textStyle = MaterialTheme.typography.bodyMedium.copy(
                        fontFamily = FontFamily.Monospace,
                        textAlign = TextAlign.End,
                    ),
                    shape = RoundedCornerShape(20.dp),
                )
        IconButton(
            onClick = { onRemoveEntry(parentTitle, subSectionTitle, entry.id) },
            modifier = Modifier
                .size(48.dp)
                .testTag("editor_remove_entry_${entry.id}"),
        ) {
            Icon(
                imageVector = Icons.Filled.Delete,
                contentDescription = "Delete item",
                tint = MaterialTheme.colorScheme.error,
            )
        }
    }
}

@Composable
internal fun EditorRawExpertContent(
    rawText: String,
    fallbackReason: String?,
    canReturnToStructured: Boolean,
    onRawTextChange: (String) -> Unit,
    onReturnToStructured: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        Surface(
            modifier = Modifier.fillMaxWidth(),
            color = MaterialTheme.colorScheme.secondaryContainer.copy(alpha = 0.72f),
        ) {
            Text(
                text = fallbackReason ?: "Expert Raw TXT mode is active.",
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(12.dp)
                    .testTag("editor_section_fallback_message"),
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
        }
        if (canReturnToStructured) {
            OutlinedButton(
                onClick = onReturnToStructured,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("editor_return_structured_button"),
            ) {
                Text("Back To Structured Editor")
            }
        }
        OutlinedTextField(
            value = rawText,
            onValueChange = onRawTextChange,
            enabled = true,
            modifier = Modifier
                .fillMaxWidth()
                .heightIn(min = 280.dp, max = 520.dp)
                .testTag("editor_record_field"),
            textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
            label = { Text("Raw TXT", fontFamily = FontFamily.Monospace) },
            minLines = 14,
        )
    }
}
