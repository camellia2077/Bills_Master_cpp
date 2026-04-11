package com.billstracer.android

import com.billstracer.android.data.services.EditorService
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordSaveResult
import com.billstracer.android.model.StructuredRecordEditorDocument
import com.billstracer.android.model.StructuredRecordEditorEntry
import com.billstracer.android.model.StructuredRecordEditorParentSection
import com.billstracer.android.model.StructuredRecordEditorSubSection

private val fakeStructuredEntryRegex =
    Regex("""^([+-]?\s*\d+(?:\.\d+)?(?:\s*[+-]\s*\d+(?:\.\d+)?)*)\s*(.*)$""")

private fun fakeParseStructuredEntryLine(line: String): StructuredRecordEditorEntry? {
    val match = fakeStructuredEntryRegex.matchEntire(line.trim()) ?: return null
    val amountExpression = match.groupValues[1].trim()
    val trailingText = match.groupValues[2]
    val commentIndex = trailingText.indexOf("//")
    val description = if (commentIndex >= 0) {
        trailingText.substring(0, commentIndex).trim()
    } else {
        trailingText.trim()
    }
    val comment = if (commentIndex >= 0) {
        trailingText.substring(commentIndex + 2).trim()
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
