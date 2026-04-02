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
