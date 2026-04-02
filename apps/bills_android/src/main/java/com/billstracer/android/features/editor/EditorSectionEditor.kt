package com.billstracer.android.features.editor

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.TextRange
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties

@Composable
internal fun EditorStructuredSectionContent(
    documentKey: String,
    rawText: String,
    persistedRawText: String,
    isWorking: Boolean,
    onRawTextChange: (String) -> Unit,
    onCommitRawText: (String) -> Unit,
    modifier: Modifier = Modifier,
) {
    val document = remember(rawText) { parseEditorSectionDocument(rawText) }
    var showRawTextDialog by rememberSaveable(documentKey) { mutableStateOf(false) }
    if (document.fallbackToRawEditor) {
        EditorRawFallbackContent(
            documentKey = documentKey,
            rawText = rawText,
            fallbackReason = document.fallbackReason,
            onRawTextChange = onRawTextChange,
            onShowRawText = { showRawTextDialog = true },
            modifier = modifier,
        )
        if (showRawTextDialog) {
            EditorRawTextDialog(
                documentKey = documentKey,
                rawText = rawText,
                persistedRawText = persistedRawText,
                isWorking = isWorking,
                onCommitRawText = onCommitRawText,
                onDismiss = { showRawTextDialog = false },
            )
        }
        return
    }

    var searchQuery by rememberSaveable(documentKey) { mutableStateOf("") }
    val expandedStates = remember(documentKey) { mutableStateMapOf<String, Boolean>() }
    val visibleSections = remember(document, searchQuery) {
        filterEditorSections(document.sections, searchQuery)
    }

    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        EditorHeaderCard(
            documentKey = documentKey,
            document = document,
            onRemarkChange = { updatedRemark ->
                val updatedDocument = updateEditorRemarkContent(document, updatedRemark)
                onRawTextChange(serializeEditorSectionDocument(updatedDocument))
            },
        )
        OutlinedButton(
            onClick = { showRawTextDialog = true },
            modifier = Modifier
                .fillMaxWidth()
                .testTag("editor_view_raw_button"),
        ) {
            Text("View Raw TXT")
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
                expandedStates[parentSection.title] ?: false
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
                            .clickable {
                                expandedStates[parentSection.title] = !parentExpanded
                            }
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
                            EditorSubSectionField(
                                parentTitle = parentSection.title,
                                subSection = subSection,
                                onContentChange = { updatedContent ->
                                    val updatedDocument = updateEditorSubSectionContent(
                                        document = document,
                                        parentTitle = parentSection.title,
                                        subSectionTitle = subSection.title,
                                        rawContent = updatedContent,
                                    )
                                    onRawTextChange(serializeEditorSectionDocument(updatedDocument))
                                },
                            )
                        }
                    }
                }
            }
        }
    }
    if (showRawTextDialog) {
        EditorRawTextDialog(
            documentKey = documentKey,
            rawText = rawText,
            persistedRawText = persistedRawText,
            isWorking = isWorking,
            onCommitRawText = onCommitRawText,
            onDismiss = { showRawTextDialog = false },
        )
    }
}

@Composable
private fun EditorHeaderCard(
    documentKey: String,
    document: EditorSectionDocumentUiModel,
    onRemarkChange: (String) -> Unit,
) {
    var headerExpanded by rememberSaveable(documentKey) { mutableStateOf(false) }
    val externalRemarkText = remember(document.remarkLines) {
        document.remarkLines.joinToString("\n")
    }
    var remarkFieldValue by rememberSaveable(
        document.dateLine,
        stateSaver = TextFieldValue.Saver,
    ) {
        mutableStateOf(TextFieldValue(externalRemarkText))
    }

    LaunchedEffect(externalRemarkText) {
        if (remarkFieldValue.text != externalRemarkText) {
            val selectionEnd = minOf(remarkFieldValue.selection.end, externalRemarkText.length)
            val selectionStart = minOf(remarkFieldValue.selection.start, selectionEnd)
            remarkFieldValue = TextFieldValue(
                text = externalRemarkText,
                selection = TextRange(selectionStart, selectionEnd),
            )
        }
    }

    Surface(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(18.dp),
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
                if (document.dateLine.isNullOrBlank()) {
                    Text(
                        text = "No date header was found in this TXT.",
                        style = MaterialTheme.typography.bodySmall,
                        fontFamily = FontFamily.Monospace,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                } else {
                    Text(
                        text = document.dateLine,
                        style = MaterialTheme.typography.bodyMedium,
                        fontFamily = FontFamily.Monospace,
                    )
                }
                OutlinedTextField(
                    value = remarkFieldValue,
                    onValueChange = { nextValue ->
                        remarkFieldValue = nextValue
                        onRemarkChange(nextValue.text)
                    },
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
private fun EditorSubSectionField(
    parentTitle: String,
    subSection: EditorSubSectionUiModel,
    onContentChange: (String) -> Unit,
) {
    val externalText = remember(subSection.contentLines) {
        subSection.contentLines.joinToString("\n")
    }
    var fieldValue by rememberSaveable(
        parentTitle,
        subSection.title,
        stateSaver = TextFieldValue.Saver,
    ) {
        mutableStateOf(TextFieldValue(externalText))
    }

    LaunchedEffect(externalText) {
        if (fieldValue.text != externalText) {
            val selectionEnd = minOf(fieldValue.selection.end, externalText.length)
            val selectionStart = minOf(fieldValue.selection.start, selectionEnd)
            fieldValue = TextFieldValue(
                text = externalText,
                selection = TextRange(selectionStart, selectionEnd),
            )
        }
    }

    OutlinedTextField(
        value = fieldValue,
        onValueChange = { nextValue ->
            fieldValue = nextValue
            onContentChange(nextValue.text)
        },
        enabled = true,
        modifier = Modifier
            .fillMaxWidth()
            .heightIn(min = 96.dp, max = 180.dp)
            .testTag("editor_subsection_field_${editorSectionTagSuffix(subSection.title)}"),
        label = { Text(subSection.title, fontFamily = FontFamily.Monospace) },
        minLines = 3,
        textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
    )
}

@Composable
private fun EditorRawFallbackContent(
    documentKey: String,
    rawText: String,
    fallbackReason: String?,
    onRawTextChange: (String) -> Unit,
    onShowRawText: () -> Unit,
    modifier: Modifier = Modifier,
) {
    var fieldValue by rememberSaveable(
        documentKey,
        stateSaver = TextFieldValue.Saver,
    ) {
        mutableStateOf(TextFieldValue(rawText))
    }

    LaunchedEffect(rawText) {
        if (fieldValue.text != rawText) {
            val selectionEnd = minOf(fieldValue.selection.end, rawText.length)
            val selectionStart = minOf(fieldValue.selection.start, selectionEnd)
            fieldValue = TextFieldValue(
                text = rawText,
                selection = TextRange(selectionStart, selectionEnd),
            )
        }
    }

    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        Surface(
            modifier = Modifier.fillMaxWidth(),
            shape = RoundedCornerShape(16.dp),
            color = MaterialTheme.colorScheme.secondaryContainer.copy(alpha = 0.72f),
        ) {
            Text(
                text = fallbackReason?.let {
                    "This TXT shape is not supported by the section editor yet. $it"
                } ?: "This TXT shape is not supported by the section editor yet.",
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(12.dp)
                    .testTag("editor_section_fallback_message"),
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
        }
        OutlinedButton(
            onClick = onShowRawText,
            modifier = Modifier
                .fillMaxWidth()
                .testTag("editor_view_raw_button"),
        ) {
            Text("View Raw TXT")
        }
        OutlinedTextField(
            value = fieldValue,
            onValueChange = { nextValue ->
                fieldValue = nextValue
                if (nextValue.text != rawText) {
                    onRawTextChange(nextValue.text)
                }
            },
            enabled = true,
            modifier = Modifier
                .fillMaxWidth()
                .heightIn(min = 280.dp, max = 460.dp)
                .testTag("editor_record_field"),
            textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
            label = { Text("Raw TXT", fontFamily = FontFamily.Monospace) },
            minLines = 14,
        )
    }
}

@Composable
private fun EditorRawTextDialog(
    documentKey: String,
    rawText: String,
    persistedRawText: String,
    isWorking: Boolean,
    onCommitRawText: (String) -> Unit,
    onDismiss: () -> Unit,
) {
    var fieldValue by rememberSaveable(
        documentKey,
        stateSaver = TextFieldValue.Saver,
    ) {
        mutableStateOf(TextFieldValue(rawText))
    }
    var pendingCommittedText by rememberSaveable(documentKey) { mutableStateOf<String?>(null) }

    LaunchedEffect(rawText, pendingCommittedText) {
        if (pendingCommittedText == null && fieldValue.text != rawText) {
            val selectionEnd = minOf(fieldValue.selection.end, rawText.length)
            val selectionStart = minOf(fieldValue.selection.start, selectionEnd)
            fieldValue = TextFieldValue(
                text = rawText,
                selection = TextRange(selectionStart, selectionEnd),
            )
        }
    }

    LaunchedEffect(isWorking, persistedRawText, pendingCommittedText) {
        val pendingText = pendingCommittedText ?: return@LaunchedEffect
        if (!isWorking && persistedRawText == pendingText) {
            pendingCommittedText = null
            onDismiss()
        }
    }

    Dialog(
        onDismissRequest = onDismiss,
        properties = DialogProperties(usePlatformDefaultWidth = false),
    ) {
        Surface(
            modifier = Modifier
                .fillMaxWidth()
                .fillMaxHeight(0.96f)
                .testTag("editor_raw_dialog"),
            shape = RoundedCornerShape(20.dp),
            color = MaterialTheme.colorScheme.surface,
        ) {
            Column(
                modifier = Modifier.padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp),
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                ) {
                    Text(
                        text = "Raw TXT",
                        style = MaterialTheme.typography.titleMedium,
                        fontFamily = FontFamily.Monospace,
                    )
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        OutlinedButton(
                            onClick = {
                                if (!isWorking) {
                                    pendingCommittedText = fieldValue.text
                                    onCommitRawText(fieldValue.text)
                                }
                            },
                            enabled = !isWorking && fieldValue.text != rawText,
                            modifier = Modifier.testTag("editor_raw_dialog_save_button"),
                        ) {
                            Text("✓")
                        }
                        OutlinedButton(
                            onClick = onDismiss,
                            enabled = !isWorking,
                        ) {
                            Text("Close")
                        }
                    }
                }
                OutlinedTextField(
                    value = fieldValue,
                    onValueChange = { nextValue ->
                        fieldValue = nextValue
                    },
                    modifier = Modifier
                        .fillMaxWidth()
                        .fillMaxHeight()
                        .testTag("editor_raw_dialog_content"),
                    textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
                    minLines = 18,
                )
            }
        }
    }
}

private fun filterEditorSections(
    sections: List<EditorParentSectionUiModel>,
    query: String,
): List<EditorParentSectionUiModel> {
    val normalizedQuery = query.trim()
    if (normalizedQuery.isEmpty()) {
        return sections
    }

    return sections.mapNotNull { parent ->
        val parentMatches = parent.title.contains(normalizedQuery, ignoreCase = true)
        val visibleSubSections = if (parentMatches) {
            parent.subSections
        } else {
            parent.subSections.filter { sub ->
                sub.title.contains(normalizedQuery, ignoreCase = true)
            }
        }
        if (!parentMatches && visibleSubSections.isEmpty()) {
            null
        } else {
            parent.copy(subSections = visibleSubSections)
        }
    }
}

private fun editorSectionTagSuffix(raw: String): String = raw
    .lowercase()
    .map { character ->
        if (character.isLetterOrDigit() || character == '_') {
            character
        } else {
            '_'
        }
    }
    .joinToString("")
