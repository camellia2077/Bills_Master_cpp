package com.billstracer.android.platform

internal fun parseMarkdownSections(text: String): List<MarkdownSection> {
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

internal fun parseMarkdownBlocks(text: String): List<MarkdownBlock> {
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

    var index = 0
    while (index < lines.size) {
        val line = lines[index]
        if (line.trim().startsWith("```")) {
            flushList()
            if (inCodeBlock) {
                blocks.add(MarkdownBlock.CodeBlock(codeBlockBuffer.toString().trimEnd()))
                codeBlockBuffer.clear()
                inCodeBlock = false
            } else {
                inCodeBlock = true
            }
            index += 1
            continue
        }

        if (inCodeBlock) {
            codeBlockBuffer.append(line).append("\n")
            index += 1
            continue
        }

        val tableParse = parseMarkdownTable(lines = lines, startIndex = index)
        if (tableParse != null) {
            flushList()
            blocks.add(tableParse.block)
            index = tableParse.nextIndex
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
        index += 1
    }

    flushList()
    return blocks
}

private data class MarkdownTableParseResult(
    val block: MarkdownBlock.TableBlock,
    val nextIndex: Int,
)

private fun parseMarkdownTable(
    lines: List<String>,
    startIndex: Int,
): MarkdownTableParseResult? {
    if (startIndex + 1 >= lines.size) {
        return null
    }

    val headerLine = lines[startIndex].trim()
    val separatorLine = lines[startIndex + 1].trim()
    if (!isMarkdownTableLine(headerLine) || !isMarkdownTableSeparatorLine(separatorLine)) {
        return null
    }

    val headers = parseMarkdownTableCells(headerLine)
    if (headers.isEmpty()) {
        return null
    }

    val rows = mutableListOf<List<String>>()
    var index = startIndex + 2
    while (index < lines.size) {
        val rowLine = lines[index].trim()
        if (!isMarkdownTableLine(rowLine)) {
            break
        }
        val cells = parseMarkdownTableCells(rowLine)
        if (cells.isNotEmpty()) {
            rows += normalizeMarkdownTableRow(cells, headers.size)
        }
        index += 1
    }

    return MarkdownTableParseResult(
        block = MarkdownBlock.TableBlock(
            headers = headers,
            rows = rows,
        ),
        nextIndex = index,
    )
}

private fun isMarkdownListLine(line: String): Boolean {
    val trimmedStart = line.trimStart()
    return trimmedStart.startsWith("- ") || trimmedStart.startsWith("* ")
}

private fun isMarkdownTableLine(line: String): Boolean {
    val trimmed = line.trim()
    return trimmed.startsWith("|") &&
        trimmed.endsWith("|") &&
        trimmed.count { it == '|' } >= 2
}

private fun isMarkdownTableSeparatorLine(line: String): Boolean {
    if (!isMarkdownTableLine(line)) {
        return false
    }
    return parseMarkdownTableCells(line).all { cell ->
        cell.matches(Regex("^:?-{3,}:?$"))
    }
}

private fun parseMarkdownTableCells(line: String): List<String> =
    line.trim()
        .removePrefix("|")
        .removeSuffix("|")
        .split('|')
        .map { cell -> cell.trim() }

private fun normalizeMarkdownTableRow(
    cells: List<String>,
    expectedSize: Int,
): List<String> =
    when {
        cells.size == expectedSize -> cells
        cells.size > expectedSize -> cells.take(expectedSize)
        else -> cells + List(expectedSize - cells.size) { "" }
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
