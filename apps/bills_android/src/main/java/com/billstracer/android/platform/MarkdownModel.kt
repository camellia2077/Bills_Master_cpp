package com.billstracer.android.platform

internal sealed class MarkdownBlock {
    data class Header(val text: String, val level: Int) : MarkdownBlock()
    data class Paragraph(val text: String) : MarkdownBlock()
    data class CodeBlock(val content: String) : MarkdownBlock()
    data class ListBlock(val items: List<MarkdownListItem>) : MarkdownBlock()
    data class TableBlock(
        val headers: List<String>,
        val rows: List<List<String>>,
    ) : MarkdownBlock()
}

internal data class MarkdownListItem(
    val text: String,
    val level: Int,
)

internal data class MarkdownSection(
    val header: MarkdownBlock.Header?,
    val content: List<MarkdownBlock>,
)
