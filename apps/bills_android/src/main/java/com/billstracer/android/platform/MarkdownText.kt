package com.billstracer.android.platform

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
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
        is MarkdownBlock.TableBlock -> {
            MarkdownTable(block = block, style = style, color = color)
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

@Composable
private fun MarkdownTable(
    block: MarkdownBlock.TableBlock,
    style: TextStyle,
    color: Color,
) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .horizontalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(4.dp),
    ) {
        MarkdownTableRow(
            cells = block.headers,
            isHeader = true,
            style = style,
            color = color,
        )
        block.rows.forEach { row ->
            MarkdownTableRow(
                cells = row,
                isHeader = false,
                style = style,
                color = color,
            )
        }
    }
}

@Composable
private fun MarkdownTableRow(
    cells: List<String>,
    isHeader: Boolean,
    style: TextStyle,
    color: Color,
) {
    Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
        cells.forEach { cell ->
            Surface(
                shape = MaterialTheme.shapes.small,
                color = if (isHeader) {
                    MaterialTheme.colorScheme.primary.copy(alpha = 0.12f)
                } else {
                    MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.60f)
                },
            ) {
                Text(
                    text = buildAnnotatedString { parseInlineMarkdown(cell, this) },
                    modifier = Modifier
                        .widthIn(min = 104.dp)
                        .padding(horizontal = 10.dp, vertical = 8.dp),
                    style = if (isHeader) {
                        style.copy(fontWeight = FontWeight.SemiBold)
                    } else {
                        style
                    },
                    color = color,
                )
            }
        }
    }
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
