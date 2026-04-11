package com.billstracer.android.features.editor

import com.billstracer.android.model.StructuredRecordEditorDocument
import com.billstracer.android.model.StructuredRecordEditorEntry
import com.billstracer.android.model.StructuredRecordEditorParentSection
import com.billstracer.android.model.StructuredRecordEditorSubSection
import java.util.UUID

internal enum class EditorMode {
    Structured,
    RawExpert,
}

internal data class EditorEntryDraftUiModel(
    val id: String,
    val amountExpression: String,
    val description: String,
    val comment: String,
)

internal data class EditorSubSectionDraftUiModel(
    val title: String,
    val entries: List<EditorEntryDraftUiModel>,
)

internal data class EditorParentSectionDraftUiModel(
    val title: String,
    val subSections: List<EditorSubSectionDraftUiModel>,
)

internal data class EditorStructuredDraftUiModel(
    val dateLine: String,
    val remarkText: String,
    val sections: List<EditorParentSectionDraftUiModel>,
)

internal fun StructuredRecordEditorDocument.toEditorStructuredDraft(): EditorStructuredDraftUiModel =
    EditorStructuredDraftUiModel(
        dateLine = dateLine,
        remarkText = remarkLines.joinToString("\n"),
        sections = sections.map { parent ->
            EditorParentSectionDraftUiModel(
                title = parent.title,
                subSections = parent.subSections.map { subSection ->
                    EditorSubSectionDraftUiModel(
                        title = subSection.title,
                        entries = subSection.entries.mapIndexed { index, entry ->
                            EditorEntryDraftUiModel(
                                id = "${parent.title}:${subSection.title}:$index",
                                amountExpression = entry.amountExpression,
                                description = entry.description,
                                comment = entry.comment,
                            )
                        },
                    )
                },
            )
        },
    )

internal fun EditorStructuredDraftUiModel.toStructuredRecordEditorDocument(): StructuredRecordEditorDocument =
    StructuredRecordEditorDocument(
        dateLine = dateLine.trim(),
        remarkLines = remarkText.split('\n')
            .map { line -> line.trimEnd('\r') },
        sections = sections.map { parent ->
            StructuredRecordEditorParentSection(
                title = parent.title,
                subSections = parent.subSections.map { subSection ->
                    StructuredRecordEditorSubSection(
                        title = subSection.title,
                        entries = subSection.entries.map { entry ->
                            StructuredRecordEditorEntry(
                                amountExpression = entry.amountExpression.trim(),
                                description = entry.description.trim(),
                                comment = entry.comment.trim(),
                            )
                        },
                    )
                },
            )
        },
    )

internal fun EditorStructuredDraftUiModel.hasIncompleteEntries(): Boolean =
    sections.any { parent ->
        parent.subSections.any { subSection ->
            subSection.entries.any { entry ->
                entry.amountExpression.trim().isEmpty()
            }
        }
    }

internal fun EditorStructuredDraftUiModel.withRemarkText(remarkText: String): EditorStructuredDraftUiModel =
    copy(remarkText = remarkText)

internal fun EditorStructuredDraftUiModel.withAddedEntry(
    parentTitle: String,
    subSectionTitle: String,
): EditorStructuredDraftUiModel = copy(
    sections = sections.map { parent ->
        if (parent.title != parentTitle) {
            parent
        } else {
            parent.copy(
                subSections = parent.subSections.map { subSection ->
                    if (subSection.title != subSectionTitle) {
                        subSection
                    } else {
                        subSection.copy(
                            entries = subSection.entries + EditorEntryDraftUiModel(
                                id = UUID.randomUUID().toString(),
                                amountExpression = "",
                                description = "",
                                comment = "",
                            ),
                        )
                    }
                },
            )
        }
    },
)

internal fun EditorStructuredDraftUiModel.withRemovedEntry(
    parentTitle: String,
    subSectionTitle: String,
    entryId: String,
): EditorStructuredDraftUiModel = copy(
    sections = sections.map { parent ->
        if (parent.title != parentTitle) {
            parent
        } else {
            parent.copy(
                subSections = parent.subSections.map { subSection ->
                    if (subSection.title != subSectionTitle) {
                        subSection
                    } else {
                        subSection.copy(
                            entries = subSection.entries.filterNot { entry -> entry.id == entryId },
                        )
                    }
                },
            )
        }
    },
)

internal fun EditorStructuredDraftUiModel.withUpdatedEntry(
    parentTitle: String,
    subSectionTitle: String,
    entryId: String,
    transform: (EditorEntryDraftUiModel) -> EditorEntryDraftUiModel,
): EditorStructuredDraftUiModel = copy(
    sections = sections.map { parent ->
        if (parent.title != parentTitle) {
            parent
        } else {
            parent.copy(
                subSections = parent.subSections.map { subSection ->
                    if (subSection.title != subSectionTitle) {
                        subSection
                    } else {
                        subSection.copy(
                            entries = subSection.entries.map { entry ->
                                if (entry.id != entryId) {
                                    entry
                                } else {
                                    transform(entry)
                                }
                            },
                        )
                    }
                },
            )
        }
    },
)

internal fun filterEditorSections(
    sections: List<EditorParentSectionDraftUiModel>,
    query: String,
): List<EditorParentSectionDraftUiModel> {
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

internal fun editorSectionTagSuffix(raw: String): String = raw
    .lowercase()
    .map { character ->
        if (character.isLetterOrDigit() || character == '_') {
            character
        } else {
            '_'
        }
    }
    .joinToString("")
