package com.billstracer.android.features.editor

import com.billstracer.android.model.StructuredRecordEditorDocument
import com.billstracer.android.model.StructuredRecordEditorEntry
import com.billstracer.android.model.StructuredRecordEditorParentSection
import com.billstracer.android.model.StructuredRecordEditorSubSection
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class EditorSectionAdapterTest {
    @Test
    fun toEditorStructuredDraftBuildsStableDraftShape() {
        val document = StructuredRecordEditorDocument(
            dateLine = "date:2026-03",
            remarkLines = listOf("first", "second"),
            sections = listOf(
                StructuredRecordEditorParentSection(
                    title = "meal",
                    subSections = listOf(
                        StructuredRecordEditorSubSection(
                            title = "meal_low",
                            entries = listOf(
                                StructuredRecordEditorEntry(
                                    amountExpression = "12",
                                    description = "lunch",
                                    comment = "cheap",
                                ),
                            ),
                        ),
                    ),
                ),
            ),
        )

        val draft = document.toEditorStructuredDraft()

        assertEquals("date:2026-03", draft.dateLine)
        assertEquals("first\nsecond", draft.remarkText)
        assertEquals("meal", draft.sections.first().title)
        assertEquals("meal_low", draft.sections.first().subSections.first().title)
        assertEquals("12", draft.sections.first().subSections.first().entries.first().amountExpression)
    }

    @Test
    fun toStructuredRecordEditorDocumentRebuildsSharedModel() {
        val draft = EditorStructuredDraftUiModel(
            dateLine = "date:2026-03",
            remarkText = "first\nsecond",
            sections = listOf(
                EditorParentSectionDraftUiModel(
                    title = "meal",
                    subSections = listOf(
                        EditorSubSectionDraftUiModel(
                            title = "meal_low",
                            entries = listOf(
                                EditorEntryDraftUiModel(
                                    id = "1",
                                    amountExpression = "12",
                                    description = "lunch",
                                    comment = "cheap",
                                ),
                            ),
                        ),
                    ),
                ),
            ),
        )

        val structured = draft.toStructuredRecordEditorDocument()

        assertEquals("date:2026-03", structured.dateLine)
        assertEquals(listOf("first", "second"), structured.remarkLines)
        assertEquals("cheap", structured.sections.first().subSections.first().entries.first().comment)
    }

    @Test
    fun addedBlankEntryIsReportedAsIncomplete() {
        val draft = StructuredRecordEditorDocument(
            dateLine = "date:2026-03",
            remarkLines = emptyList(),
            sections = listOf(
                StructuredRecordEditorParentSection(
                    title = "meal",
                    subSections = listOf(
                        StructuredRecordEditorSubSection(
                            title = "meal_low",
                            entries = emptyList(),
                        ),
                    ),
                ),
            ),
        ).toEditorStructuredDraft()

        val updated = draft.withAddedEntry("meal", "meal_low")

        assertTrue(updated.hasIncompleteEntries())
    }

    @Test
    fun completedEntryIsNotReportedAsIncomplete() {
        val draft = EditorStructuredDraftUiModel(
            dateLine = "date:2026-03",
            remarkText = "",
            sections = listOf(
                EditorParentSectionDraftUiModel(
                    title = "meal",
                    subSections = listOf(
                        EditorSubSectionDraftUiModel(
                            title = "meal_low",
                            entries = listOf(
                                EditorEntryDraftUiModel(
                                    id = "1",
                                    amountExpression = "12",
                                    description = "lunch",
                                    comment = "",
                                ),
                            ),
                        ),
                    ),
                ),
            ),
        )

        assertFalse(draft.hasIncompleteEntries())
    }
}
