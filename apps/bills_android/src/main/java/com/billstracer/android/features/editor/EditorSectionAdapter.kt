package com.billstracer.android.features.editor

internal data class EditorSectionDocumentUiModel(
    val dateLine: String?,
    val remarkLines: List<String>,
    val sections: List<EditorParentSectionUiModel>,
    val fallbackToRawEditor: Boolean,
    val fallbackReason: String? = null,
)

internal data class EditorParentSectionUiModel(
    val title: String,
    val subSections: List<EditorSubSectionUiModel>,
)

internal data class EditorSubSectionUiModel(
    val title: String,
    val contentLines: List<String>,
)

internal fun parseEditorSectionDocument(rawText: String): EditorSectionDocumentUiModel {
    var dateLine: String? = null
    val remarkLines = mutableListOf<String>()
    val sections = mutableListOf<MutableEditorParentSection>()
    var currentParent: MutableEditorParentSection? = null
    var currentSubSection: MutableEditorSubSection? = null

    rawText.split('\n').forEach { rawLine ->
        val normalizedLine = rawLine.trimEnd('\r')
        val trimmedLine = normalizedLine.trim()

        if (trimmedLine.startsWith("date:")) {
            dateLine = trimmedLine
            return@forEach
        }
        if (trimmedLine.startsWith("remark:")) {
            remarkLines += trimmedLine.removePrefix("remark:")
            return@forEach
        }
        if (trimmedLine.isBlank()) {
            return@forEach
        }

        val firstWhitespaceIndex = trimmedLine.indexOfFirst { it.isWhitespace() }
        val firstToken = if (firstWhitespaceIndex >= 0) {
            trimmedLine.substring(0, firstWhitespaceIndex)
        } else {
            trimmedLine
        }
        val remainder = trimmedLine.removePrefix(firstToken).trimStart()
        val startsWithLetter = trimmedLine.firstOrNull()?.isLetter() == true
        val isPureParentTitle = startsWithLetter && remainder.isEmpty() && !firstToken.contains('_')
        val isPureSubTitle = startsWithLetter && remainder.isEmpty() && firstToken.contains('_')
        val isInlineSubTitle = startsWithLetter && firstToken.contains('_') && remainder.isNotEmpty()

        when {
            isPureParentTitle -> {
                currentParent = MutableEditorParentSection(title = firstToken)
                sections += currentParent!!
                currentSubSection = null
            }

            isPureSubTitle -> {
                val parent = currentParent ?: return fallbackSectionDocument(
                    dateLine = dateLine,
                    remarkLines = remarkLines,
                    reason = "A sub-title appeared before any parent title.",
                )
                currentSubSection = MutableEditorSubSection(title = firstToken)
                parent.subSections += currentSubSection!!
            }

            isInlineSubTitle -> {
                val parent = currentParent ?: return fallbackSectionDocument(
                    dateLine = dateLine,
                    remarkLines = remarkLines,
                    reason = "A sub-title with inline content appeared before any parent title.",
                )
                currentSubSection = MutableEditorSubSection(
                    title = firstToken,
                    contentLines = mutableListOf(remainder),
                )
                parent.subSections += currentSubSection!!
            }

            currentSubSection != null -> {
                currentSubSection!!.contentLines += trimmedLine
            }

            else -> {
                return fallbackSectionDocument(
                    dateLine = dateLine,
                    remarkLines = remarkLines,
                    reason = "Content appeared before a recognized sub-title.",
                )
            }
        }
    }

    val finalizedSections = sections.map { parent ->
        EditorParentSectionUiModel(
            title = parent.title,
            subSections = parent.subSections.map { sub ->
                EditorSubSectionUiModel(
                    title = sub.title,
                    contentLines = sub.contentLines.toList(),
                )
            },
        )
    }
    return EditorSectionDocumentUiModel(
        dateLine = dateLine,
        remarkLines = remarkLines,
        sections = finalizedSections,
        fallbackToRawEditor = false,
        fallbackReason = null,
    )
}

internal fun serializeEditorSectionDocument(document: EditorSectionDocumentUiModel): String {
    val lines = mutableListOf<String>()
    document.dateLine?.trim()?.takeIf { it.isNotEmpty() }?.let { lines += it }
    val serializedRemarkLines = if (document.remarkLines.isEmpty()) {
        listOf("remark:")
    } else {
        document.remarkLines.map { remarkLine -> "remark:${remarkLine.trimEnd()}" }
    }
    lines += serializedRemarkLines
    if ((document.dateLine != null || serializedRemarkLines.isNotEmpty()) && document.sections.isNotEmpty()) {
        lines += ""
    }

    document.sections.forEachIndexed { parentIndex, parent ->
        lines += parent.title.trim()
        lines += ""

        parent.subSections.forEachIndexed { subIndex, subSection ->
            lines += subSection.title.trim()
            val normalizedContentLines = subSection.contentLines.mapNotNull { line ->
                line.trim().takeIf { it.isNotEmpty() }
            }
            lines += normalizedContentLines
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

internal fun updateEditorSubSectionContent(
    document: EditorSectionDocumentUiModel,
    parentTitle: String,
    subSectionTitle: String,
    rawContent: String,
): EditorSectionDocumentUiModel = document.copy(
    sections = document.sections.map { parent ->
        if (parent.title != parentTitle) {
            parent
        } else {
            parent.copy(
                subSections = parent.subSections.map { subSection ->
                    if (subSection.title != subSectionTitle) {
                        subSection
                    } else {
                        subSection.copy(
                            contentLines = rawContent.split('\n')
                                .map { it.trimEnd('\r') }
                                .mapNotNull { line -> line.trim().takeIf { it.isNotEmpty() } },
                        )
                    }
                },
            )
        }
    },
)

internal fun updateEditorRemarkContent(
    document: EditorSectionDocumentUiModel,
    rawRemark: String,
): EditorSectionDocumentUiModel = document.copy(
    remarkLines = rawRemark.split('\n')
        .map { it.trimEnd('\r') },
)

private data class MutableEditorParentSection(
    val title: String,
    val subSections: MutableList<MutableEditorSubSection> = mutableListOf(),
)

private data class MutableEditorSubSection(
    val title: String,
    val contentLines: MutableList<String> = mutableListOf(),
)

private fun fallbackSectionDocument(
    dateLine: String?,
    remarkLines: List<String>,
    reason: String,
): EditorSectionDocumentUiModel = EditorSectionDocumentUiModel(
    dateLine = dateLine,
    remarkLines = remarkLines,
    sections = emptyList(),
    fallbackToRawEditor = true,
    fallbackReason = reason,
)
