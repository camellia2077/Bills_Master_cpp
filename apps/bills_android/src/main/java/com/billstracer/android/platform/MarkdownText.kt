package com.billstracer.android.platform

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.unit.dp

@Composable
internal fun MarkdownText(
    markdown: String,
    modifier: Modifier = Modifier,
    style: TextStyle = MaterialTheme.typography.bodyMedium,
    color: Color = MaterialTheme.colorScheme.onSurface,
) {
    val sections = remember(markdown) { parseMarkdownSections(markdown) }
    val expandedStates = remember(markdown) {
        mutableStateListOf<Boolean>().apply {
            repeat(sections.size) { add(true) }
        }
    }

    SelectionContainer(modifier = modifier) {
        Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
            val parentExpansionStack = mutableListOf<Pair<Int, Boolean>>()
            sections.forEachIndexed { index, section ->
                val header = section.header
                if (header != null) {
                    while (parentExpansionStack.isNotEmpty() &&
                        parentExpansionStack.last().first >= header.level
                    ) {
                        parentExpansionStack.removeAt(parentExpansionStack.lastIndex)
                    }
                    val parentExpanded = parentExpansionStack.all { it.second }
                    val isExpanded = expandedStates.getOrNull(index) ?: true

                    if (parentExpanded) {
                        MarkdownHeader(
                            header = header,
                            expanded = isExpanded,
                            onToggle = { expandedStates[index] = !expandedStates[index] },
                        )
                        if (isExpanded) {
                            section.content.forEach { block ->
                                MarkdownBlockView(block = block, style = style, color = color)
                            }
                        }
                    }
                    parentExpansionStack.add(header.level to isExpanded)
                } else if (parentExpansionStack.all { it.second }) {
                    section.content.forEach { block ->
                        MarkdownBlockView(block = block, style = style, color = color)
                    }
                }
            }
        }
    }
}

@Composable
private fun MarkdownHeader(
    header: MarkdownBlock.Header,
    expanded: Boolean,
    onToggle: () -> Unit,
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(top = 4.dp)
            .clickable(onClick = onToggle),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            text = if (expanded) "▼" else "▶",
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.primary,
            modifier = Modifier.padding(end = 6.dp),
        )
        Text(
            text = header.text,
            style = markdownHeaderStyle(header.level),
            color = MaterialTheme.colorScheme.primary,
        )
    }
}

@Composable
private fun MarkdownBlockView(
    block: MarkdownBlock,
    style: TextStyle,
    color: Color,
) {
    when (block) {
        is MarkdownBlock.Header -> {
            Text(
                text = block.text,
                style = markdownHeaderStyle(block.level),
                color = MaterialTheme.colorScheme.primary,
                modifier = Modifier.padding(top = 4.dp),
            )
        }
        is MarkdownBlock.Paragraph -> {
            Text(
                text = buildAnnotatedString { parseInlineMarkdown(block.text, this) },
                style = style,
                color = color,
            )
        }
        is MarkdownBlock.CodeBlock -> {
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(
                        color = MaterialTheme.colorScheme.surfaceVariant,
                        shape = MaterialTheme.shapes.small,
                    )
                    .padding(12.dp),
            ) {
                Text(
                    text = block.content,
                    style = MaterialTheme.typography.bodyMedium.copy(
                        fontFamily = FontFamily.Monospace,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    ),
                )
            }
        }
        is MarkdownBlock.ListBlock -> {
            Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                block.items.forEach { item ->
                    Row(modifier = Modifier.padding(start = (item.level * 14).dp)) {
                        Text(
                            text = "• ",
                            style = style,
                            color = color,
                            fontWeight = FontWeight.Bold,
                        )
                        Text(
                            text = buildAnnotatedString { parseInlineMarkdown(item.text, this) },
                            style = style,
                            color = color,
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun markdownHeaderStyle(level: Int): TextStyle =
    when (level) {
        1 -> MaterialTheme.typography.titleMedium.copy(fontWeight = FontWeight.SemiBold)
        2 -> MaterialTheme.typography.bodyLarge.copy(fontWeight = FontWeight.SemiBold)
        else -> MaterialTheme.typography.bodyMedium.copy(fontWeight = FontWeight.Medium)
    }

private sealed class MarkdownBlock {
    data class Header(val text: String, val level: Int) : MarkdownBlock()
    data class Paragraph(val text: String) : MarkdownBlock()
    data class CodeBlock(val content: String) : MarkdownBlock()
    data class ListBlock(val items: List<MarkdownListItem>) : MarkdownBlock()
}

private data class MarkdownListItem(
    val text: String,
    val level: Int,
)

private data class MarkdownSection(
    val header: MarkdownBlock.Header?,
    val content: List<MarkdownBlock>,
)

private fun parseMarkdownSections(text: String): List<MarkdownSection> {
    val blocks = parseMarkdownBlocks(text)
    if (blocks.isEmpty()) {
        return emptyList()
    }

    val sections = mutableListOf<MarkdownSection>()
    var currentHeader: MarkdownBlock.Header? = null
    val currentContent = mutableListOf<MarkdownBlock>()

    fun flushSection() {
        if (currentHeader != null || currentContent.isNotEmpty()) {
            sections.add(MarkdownSection(currentHeader, currentContent.toList()))
            currentHeader = null
            currentContent.clear()
        }
    }

    blocks.forEach { block ->
        if (block is MarkdownBlock.Header) {
            flushSection()
            currentHeader = block
        } else {
            currentContent.add(block)
        }
    }
    flushSection()

    return sections
}

private fun parseMarkdownBlocks(text: String): List<MarkdownBlock> {
    val lines = text.lines()
    val blocks = mutableListOf<MarkdownBlock>()
    var inCodeBlock = false
    val codeBlockBuffer = StringBuilder()
    val listBuffer = mutableListOf<MarkdownListItem>()

    fun flushList() {
        if (listBuffer.isNotEmpty()) {
            blocks.add(MarkdownBlock.ListBlock(listBuffer.toList()))
            listBuffer.clear()
        }
    }

    for (line in lines) {
        if (line.trim().startsWith("```")) {
            flushList()
            if (inCodeBlock) {
                blocks.add(MarkdownBlock.CodeBlock(codeBlockBuffer.toString().trimEnd()))
                codeBlockBuffer.clear()
                inCodeBlock = false
            } else {
                inCodeBlock = true
            }
            continue
        }

        if (inCodeBlock) {
            codeBlockBuffer.append(line).append("\n")
            continue
        }

        val trimmed = line.trim()
        when {
            trimmed.startsWith("#") -> {
                flushList()
                val level = trimmed.takeWhile { it == '#' }.length
                val rawHeader = trimmed.drop(level).trim()
                val headerText = rawHeader.replace(Regex("\\s+#+\\s*$"), "").trim()
                blocks.add(MarkdownBlock.Header(headerText, level))
            }
            isMarkdownListLine(line) -> {
                parseMarkdownListItem(line)?.let { listBuffer.add(it) }
            }
            trimmed.isBlank() -> flushList()
            else -> {
                flushList()
                blocks.add(MarkdownBlock.Paragraph(trimmed))
            }
        }
    }

    flushList()
    return blocks
}

private fun isMarkdownListLine(line: String): Boolean {
    val trimmedStart = line.trimStart()
    return trimmedStart.startsWith("- ") || trimmedStart.startsWith("* ")
}

private fun parseMarkdownListItem(line: String): MarkdownListItem? {
    val match = Regex("^([ \\t]*)([-*])\\s+(.*)$").find(line) ?: return null
    val indentToken = match.groupValues[1]
    val text = match.groupValues[3].trim()
    val indentSpaces = indentToken.fold(0) { acc, ch ->
        acc + if (ch == '\t') 4 else 1
    }
    return MarkdownListItem(text = text, level = (indentSpaces / 2).coerceAtLeast(0))
}

private fun parseInlineMarkdown(
    text: String,
    builder: AnnotatedString.Builder,
) {
    val parts = text.split("**")
    parts.forEachIndexed { index, part ->
        if (index % 2 == 1) {
            builder.withStyle(SpanStyle(fontWeight = FontWeight.Bold)) {
                append(part)
            }
        } else {
            val subParts = part.split("`")
            subParts.forEachIndexed { subIndex, subPart ->
                if (subIndex % 2 == 1) {
                    builder.withStyle(
                        SpanStyle(
                            fontFamily = FontFamily.Monospace,
                            background = Color.LightGray.copy(alpha = 0.3f),
                        ),
                    ) {
                        append(subPart)
                    }
                } else {
                    builder.append(subPart)
                }
            }
        }
    }
}
