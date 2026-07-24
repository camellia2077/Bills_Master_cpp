package com.billstracer.android

import com.billstracer.android.data.services.EditorService
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordSaveResult
import com.billstracer.android.model.StructuredRecordEditorDocument
import com.billstracer.android.model.StructuredRecordEditorEntry
import com.billstracer.android.model.StructuredRecordEditorParentSection
import com.billstracer.android.model.StructuredRecordEditorSubSection

private fun fakeFindCommentStart(text: String): Pair<Int, Int>? {
    text.forEachIndexed { index, character ->
        if (text.startsWith("//", startIndex = index)) {
            return index to 2
        }
        if ((character == '#' || character == ';') && index > 0 && text[index - 1].isWhitespace()) {
            return index to 1
        }
    }
    return null
}

private fun fakeParseAmountExpressionPrefix(line: String): String? {
    class FakeExpressionPrefixParser(private val source: String) {
        var index: Int = 0
            private set

        fun parse(): Boolean {
            index = 0
            if (!parseExpression()) {
                return false
            }
            skipWhitespace()
            return index > 0
        }

        private fun skipWhitespace() {
            while (index < source.length && source[index].isWhitespace()) {
                index += 1
            }
        }

        private fun parseExpression(): Boolean {
            if (!parseTerm()) {
                return false
            }
            while (true) {
                skipWhitespace()
                if (index >= source.length) {
                    return true
                }
                val current = source[index]
                if (current != '+' && current != '-') {
                    return true
                }
                index += 1
                if (!parseTerm()) {
                    return false
                }
            }
        }

        private fun parseTerm(): Boolean {
            if (!parseFactor()) {
                return false
            }
            while (true) {
                skipWhitespace()
                if (index >= source.length) {
                    return true
                }
                val current = source[index]
                if (current != '*' && current != '/' && current != '×') {
                    return true
                }
                index += 1
                if (!parseFactor()) {
                    return false
                }
            }
        }

        private fun parseFactor(): Boolean {
            skipWhitespace()
            while (index < source.length && (source[index] == '+' || source[index] == '-')) {
                index += 1
                skipWhitespace()
            }
            if (index >= source.length) {
                return false
            }
            if (source[index] == '(') {
                index += 1
                if (!parseExpression()) {
                    return false
                }
                skipWhitespace()
                if (index >= source.length || source[index] != ')') {
                    return false
                }
                index += 1
                return true
            }
            return parseNumber()
        }

        private fun parseNumber(): Boolean {
            skipWhitespace()
            val start = index
            var sawDigit = false
            var sawDot = false
            while (index < source.length) {
                val current = source[index]
                when {
                    current.isDigit() -> {
                        sawDigit = true
                        index += 1
                    }
                    current == '.' && !sawDot -> {
                        sawDot = true
                        index += 1
                    }
                    else -> break
                }
            }
            return sawDigit && index > start
        }
    }

    val parser = FakeExpressionPrefixParser(line)
    if (!parser.parse()) {
        return null
    }
    var endIndex = parser.index
    while (endIndex > 0 && line[endIndex - 1].isWhitespace()) {
        endIndex -= 1
    }
    return line.substring(0, endIndex).trim().takeIf { it.isNotEmpty() }
}

private fun fakeParseStructuredEntryLine(line: String): StructuredRecordEditorEntry? {
    val normalized = line.trim()
    val amountExpression = fakeParseAmountExpressionPrefix(normalized) ?: return null
    val trailingText = normalized.removePrefix(amountExpression)
    val commentStart = fakeFindCommentStart(trailingText)
    val description = if (commentStart != null) {
        trailingText.substring(0, commentStart.first).trim()
    } else {
        trailingText.trim()
    }
    val comment = if (commentStart != null) {
        trailingText.substring(commentStart.first + commentStart.second).trim()
    } else {
        ""
    }
    return StructuredRecordEditorEntry(
        amountExpression = amountExpression,
        description = description,
        comment = comment,
    )
}

private fun fakeParseStructuredDocument(rawText: String): Pair<StructuredRecordEditorDocument?, String?> {
    var dateLine = ""
    val remarkLines = mutableListOf<String>()
    val sections = mutableListOf<StructuredRecordEditorParentSection>()
    var currentParentIndex = -1
    var currentSubIndex = -1

    rawText.split('\n').forEachIndexed { index, rawLine ->
        val trimmedLine = rawLine.trimEnd('\r').trim()
        if (trimmedLine.startsWith("date:")) {
            dateLine = trimmedLine
            return@forEachIndexed
        }
        if (trimmedLine.startsWith("remark:")) {
            remarkLines += trimmedLine.removePrefix("remark:")
            return@forEachIndexed
        }
        if (trimmedLine.isBlank()) {
            return@forEachIndexed
        }

        val firstWhitespaceIndex = trimmedLine.indexOfFirst { it.isWhitespace() }
        val firstToken = if (firstWhitespaceIndex >= 0) {
            trimmedLine.substring(0, firstWhitespaceIndex)
        } else {
            trimmedLine
        }
        val remainder = trimmedLine.removePrefix(firstToken).trimStart()
        val isParentTitle = trimmedLine.firstOrNull()?.isLetter() == true &&
            remainder.isEmpty() &&
            !firstToken.contains('_')
        val isSubTitle = trimmedLine.firstOrNull()?.isLetter() == true &&
            remainder.isEmpty() &&
            firstToken.contains('_')
        val isInlineSubTitle = trimmedLine.firstOrNull()?.isLetter() == true &&
            remainder.isNotEmpty() &&
            firstToken.contains('_')

        when {
            isParentTitle -> {
                sections += StructuredRecordEditorParentSection(
                    title = firstToken,
                    subSections = emptyList(),
                )
                currentParentIndex = sections.lastIndex
                currentSubIndex = -1
            }
            isSubTitle -> {
                if (currentParentIndex < 0) {
                    return null to "A sub-title appeared before any parent title. (line ${index + 1})"
                }
                val parent = sections[currentParentIndex]
                sections[currentParentIndex] = parent.copy(
                    subSections = parent.subSections + StructuredRecordEditorSubSection(
                        title = firstToken,
                        entries = emptyList(),
                    ),
                )
                currentSubIndex = sections[currentParentIndex].subSections.lastIndex
            }
            isInlineSubTitle -> {
                if (currentParentIndex < 0) {
                    return null to "A sub-title with inline content appeared before any parent title. (line ${index + 1})"
                }
                val entry = fakeParseStructuredEntryLine(remainder)
                    ?: return null to "A content line is not supported by the structured editor. (line ${index + 1})"
                val parent = sections[currentParentIndex]
                sections[currentParentIndex] = parent.copy(
                    subSections = parent.subSections + StructuredRecordEditorSubSection(
                        title = firstToken,
                        entries = listOf(entry),
                    ),
                )
                currentSubIndex = sections[currentParentIndex].subSections.lastIndex
            }
            currentParentIndex >= 0 && currentSubIndex >= 0 -> {
                val entry = fakeParseStructuredEntryLine(trimmedLine)
                    ?: return null to "A content line is not supported by the structured editor. (line ${index + 1})"
                val parent = sections[currentParentIndex]
                val subSection = parent.subSections[currentSubIndex]
                val updatedSubSections = parent.subSections.toMutableList()
                updatedSubSections[currentSubIndex] = subSection.copy(
                    entries = subSection.entries + entry,
                )
                sections[currentParentIndex] = parent.copy(subSections = updatedSubSections)
            }
            else -> {
                return null to "Content appeared before a recognized sub-title. (line ${index + 1})"
            }
        }
    }

    return StructuredRecordEditorDocument(
        dateLine = dateLine,
        remarkLines = remarkLines,
        sections = sections,
    ) to null
}

private fun fakeSerializeStructuredDocument(document: StructuredRecordEditorDocument): String {
    val lines = mutableListOf<String>()
    lines += document.dateLine
    if (document.remarkLines.isEmpty()) {
        lines += "remark:"
    } else {
        document.remarkLines.forEach { line ->
            lines += "remark:$line"
        }
    }
    if (document.sections.isNotEmpty()) {
        lines += ""
    }
    document.sections.forEachIndexed { parentIndex, parent ->
        lines += parent.title
        lines += ""
        parent.subSections.forEachIndexed { subIndex, subSection ->
            lines += subSection.title
            subSection.entries.forEach { entry ->
                var line = entry.amountExpression.trim()
                if (entry.description.isNotBlank()) {
                    line += " ${entry.description.trim()}"
                }
                if (entry.comment.isNotBlank()) {
                    line += " // ${entry.comment.trim()}"
                }
                lines += line
            }
            if (subIndex < parent.subSections.lastIndex) {
                lines += ""
            }
        }
        if (parentIndex < document.sections.lastIndex) {
            lines += ""
        }
    }
    return lines.joinToString("\n")
}

internal class FakeEditorService : EditorService {
    val savedRecords = linkedMapOf<String, String>()
    val persistedPeriods = linkedSetOf("2026-03", "2026-02")
    val missingPersistedPeriods = linkedSetOf<String>()
    val commitFailures = linkedMapOf<String, String>()
    val committedPeriods = mutableListOf<String>()

    init {
        persistedPeriods.forEach { period ->
            savedRecords[period] = "date:$period\nremark:\n\nmeal\n\nmeal_low\n"
        }
    }

    override suspend fun listPersistedRecordPeriods(): List<String> =
        persistedPeriods.toList().sortedDescending()

    override suspend fun openPersistedRecordPeriod(period: String): RecordEditorDocument {
        if (missingPersistedPeriods.contains(period)) {
            error(
                "Database and TXT source are out of sync for $period. Missing " +
                    "${period.substringBefore('-')}/$period.txt. Re-import or resync records/.",
            )
        }
        if (!persistedPeriods.contains(period)) {
            val year = period.substringBefore('-')
            val rawText = "date:$period\nremark:\n\nmeal\n\nmeal_low\n"
            val parsed = fakeParseStructuredDocument(rawText)
            return RecordEditorDocument(
                period = period,
                relativePath = "$year/$period.txt",
                rawText = rawText,
                persisted = false,
                structuredDocument = parsed.first,
                rawFallbackReason = parsed.second,
            )
        }
        val persistedText = savedRecords[period]
            ?: error(
                "Database and TXT source are out of sync for $period. Missing " +
                    "${period.substringBefore('-')}/$period.txt. Re-import or resync records/.",
            )
        val year = period.substringBefore('-')
        val parsed = fakeParseStructuredDocument(persistedText)
        return RecordEditorDocument(
            period = period,
            relativePath = "$year/$period.txt",
            rawText = persistedText,
            persisted = true,
            structuredDocument = parsed.first,
            rawFallbackReason = parsed.second,
        )
    }

    override suspend fun serializeStructuredRecordDocument(document: StructuredRecordEditorDocument): String =
        fakeSerializeStructuredDocument(document)

    override suspend fun commitRecordDocument(period: String, rawText: String): RecordSaveResult {
        committedPeriods += period
        val explicitFailure = commitFailures[period]
        if (explicitFailure != null) {
            return RecordSaveResult(
                ok = false,
                message = explicitFailure,
                errorMessage = explicitFailure,
                rawJson = """{"ok":false}""",
            )
        }
        if (!rawText.startsWith("date:$period")) {
            val message = "TXT header period does not match selected period '$period'."
            return RecordSaveResult(
                ok = false,
                message = message,
                errorMessage = message,
                rawJson = """{"ok":false}""",
            )
        }

        savedRecords[period] = rawText
        persistedPeriods += period
        val year = period.substringBefore('-')
        val parsed = fakeParseStructuredDocument(rawText)
        return RecordSaveResult(
            ok = true,
            message = "Saved $year/$period.txt and synced it to the database.",
            document = RecordEditorDocument(
                period = period,
                relativePath = "$year/$period.txt",
                rawText = rawText,
                persisted = true,
                structuredDocument = parsed.first,
                rawFallbackReason = parsed.second,
            ),
            rawJson = """{"ok":true}""",
        )
    }
}
