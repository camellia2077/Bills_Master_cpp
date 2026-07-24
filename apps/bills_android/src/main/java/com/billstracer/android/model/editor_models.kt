package com.billstracer.android.model

data class StructuredRecordEditorEntry(
    val amountExpression: String,
    val description: String,
    val comment: String,
)

data class StructuredRecordEditorSubSection(
    val title: String,
    val entries: List<StructuredRecordEditorEntry>,
)

data class StructuredRecordEditorParentSection(
    val title: String,
    val subSections: List<StructuredRecordEditorSubSection>,
)

data class StructuredRecordEditorDocument(
    val dateLine: String,
    val remarkLines: List<String>,
    val sections: List<StructuredRecordEditorParentSection>,
)

data class RecordEditorDocument(
    val period: String,
    val relativePath: String,
    val rawText: String,
    val persisted: Boolean,
    val structuredDocument: StructuredRecordEditorDocument? = null,
    val rawFallbackReason: String? = null,
)

data class RecordSaveResult(
    val ok: Boolean,
    val message: String,
    val document: RecordEditorDocument? = null,
    val errorMessage: String? = null,
    val rawJson: String,
)

data class EditorRecordSummary(
    val income: Double,
    val expense: Double,
)
