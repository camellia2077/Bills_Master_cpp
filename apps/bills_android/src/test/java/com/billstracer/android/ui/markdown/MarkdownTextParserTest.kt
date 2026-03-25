package com.billstracer.android.platform

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class MarkdownTextParserTest {
    @Test
    fun parseMarkdownBlocksRecognizesYearlyMarkdownTable() {
        val markdown = """
            ## 2025年 总览
            - **年总收入:** 134453.55 CNY
            - **年总支出:** -16603.43 CNY
            - **年终结余:** 117850.12 CNY

            ## 每月明细

            | 月份 | 收入 (CNY) | 支出 (CNY) | 结余 (CNY) |
            | :--- | :--- | :--- | :--- |
            | 2025-01 | 9586.68 | -1511.57 | 8075.11 |
            | 2025-02 | 13962.87 | -1097.59 | 12865.28 |
        """.trimIndent()

        val blocks = parseMarkdownBlocks(markdown)

        assertTrue(blocks.any { it is MarkdownBlock.TableBlock })
        val table = blocks.filterIsInstance<MarkdownBlock.TableBlock>().single()
        assertEquals(listOf("月份", "收入 (CNY)", "支出 (CNY)", "结余 (CNY)"), table.headers)
        assertEquals(2, table.rows.size)
        assertEquals(listOf("2025-01", "9586.68", "-1511.57", "8075.11"), table.rows.first())
    }

    @Test
    fun parseMarkdownBlocksKeepsHeadersAndListsAroundTable() {
        val markdown = """
            # Markdown Report

            ## Year Overview
            - income
            - expense

            | month | income |
            | :--- | :--- |
            | 2025-01 | 100 |
        """.trimIndent()

        val blocks = parseMarkdownBlocks(markdown)

        assertTrue(blocks[0] is MarkdownBlock.Header)
        assertTrue(blocks[1] is MarkdownBlock.Header)
        assertTrue(blocks[2] is MarkdownBlock.ListBlock)
        assertTrue(blocks[3] is MarkdownBlock.TableBlock)
    }
}
