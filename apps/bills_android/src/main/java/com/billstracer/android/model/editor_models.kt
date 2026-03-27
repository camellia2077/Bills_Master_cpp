package com.billstracer.android.model

data class RecordEditorDocument(
    val period: String,
    val relativePath: String,
    val rawText: String,
    val persisted: Boolean,
)

data class RecordSaveResult(
    val ok: Boolean,
    val message: String,
    val document: RecordEditorDocument? = null,
    val errorMessage: String? = null,
    val rawJson: String,
)

data class RecordPreviewFile(
    val path: String,
    val ok: Boolean,
    val period: String? = null,
    val year: Int? = null,
    val month: Int? = null,
    val transactionCount: Int = 0,
    val totalIncome: Double = 0.0,
    val totalExpense: Double = 0.0,
    val balance: Double = 0.0,
    val error: String? = null,
)

data class RecordPreviewResult(
    val ok: Boolean,
    val code: String,
    val message: String,
    val processed: Int,
    val success: Int,
    val failure: Int,
    val periods: List<String>,
    val files: List<RecordPreviewFile>,
    val rawJson: String,
)
