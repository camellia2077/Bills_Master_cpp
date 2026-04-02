package com.billstracer.android.features.editor

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class EditorSectionAdapterTest {
    @Test
    fun parseEditorSectionDocumentReadsTemplateStyleSections() {
        val document = parseEditorSectionDocument(
            """
            date:2026-03
            remark:test

            meal

            meal_low
            12 lunch

            meal_high
            25 dinner
            """.trimIndent(),
        )

        assertFalse(document.fallbackToRawEditor)
        assertEquals("date:2026-03", document.dateLine)
        assertEquals(listOf("test"), document.remarkLines)
        assertEquals(listOf("meal"), document.sections.map { it.title })
        assertEquals(listOf("meal_low", "meal_high"), document.sections.first().subSections.map { it.title })
        assertEquals(listOf("12 lunch"), document.sections.first().subSections.first().contentLines)
    }

    @Test
    fun parseEditorSectionDocumentSupportsInlineSubTitleContent() {
        val document = parseEditorSectionDocument(
            """
            date:2026-03
            remark:test

            meal
            meal_low 12 lunch
            meal_high 25 dinner
            """.trimIndent(),
        )

        assertFalse(document.fallbackToRawEditor)
        assertEquals(listOf("12 lunch"), document.sections.first().subSections.first().contentLines)
        assertEquals(listOf("25 dinner"), document.sections.first().subSections.last().contentLines)
    }

    @Test
    fun parseEditorSectionDocumentCollectsRepeatedRemarkLines() {
        val document = parseEditorSectionDocument(
            """
            date:2026-03
            remark:first line
            remark:second line

            meal

            meal_low
            12 lunch
            """.trimIndent(),
        )

        assertEquals("date:2026-03", document.dateLine)
        assertEquals(listOf("first line", "second line"), document.remarkLines)
    }

    @Test
    fun serializeEditorSectionDocumentRebuildsNormalizedTxt() {
        val document = EditorSectionDocumentUiModel(
            dateLine = "date:2026-03",
            remarkLines = listOf("test"),
            sections = listOf(
                EditorParentSectionUiModel(
                    title = "meal",
                    subSections = listOf(
                        EditorSubSectionUiModel(
                            title = "meal_low",
                            contentLines = listOf("12 lunch", "18 noodles"),
                        ),
                    ),
                ),
            ),
            fallbackToRawEditor = false,
        )

        assertEquals(
            """
            date:2026-03
            remark:test

            meal

            meal_low
            12 lunch
            18 noodles
            """.trimIndent(),
            serializeEditorSectionDocument(document),
        )
    }

    @Test
    fun updateEditorSubSectionContentKeepsEmptySubTitle() {
        val original = parseEditorSectionDocument(
            """
            date:2026-03
            remark:test

            meal

            meal_low
            12 lunch
            """.trimIndent(),
        )

        val updated = updateEditorSubSectionContent(
            document = original,
            parentTitle = "meal",
            subSectionTitle = "meal_low",
            rawContent = "\n\n",
        )

        assertEquals(
            """
            date:2026-03
            remark:test

            meal

            meal_low
            """.trimIndent(),
            serializeEditorSectionDocument(updated),
        )
    }

    @Test
    fun updateEditorRemarkContentSupportsMultilineRemark() {
        val original = parseEditorSectionDocument(
            """
            date:2026-03
            remark:test

            meal

            meal_low
            12 lunch
            """.trimIndent(),
        )

        val updated = updateEditorRemarkContent(
            document = original,
            rawRemark = "first line\nsecond line",
        )

        assertEquals(
            """
            date:2026-03
            remark:first line
            remark:second line

            meal

            meal_low
            12 lunch
            """.trimIndent(),
            serializeEditorSectionDocument(updated),
        )
    }

    @Test
    fun parseEditorSectionDocumentFallsBackWhenContentAppearsBeforeSubTitle() {
        val document = parseEditorSectionDocument(
            """
            date:2026-03
            remark:test

            meal
            12 lunch
            """.trimIndent(),
        )

        assertTrue(document.fallbackToRawEditor)
        assertTrue(document.fallbackReason?.contains("Content appeared") == true)
    }
}
