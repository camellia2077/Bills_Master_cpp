package com.billstracer.android.data.services

import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.EditorRecordSummary
import com.billstracer.android.model.RecordSaveResult
import com.billstracer.android.model.StructuredRecordEditorDocument

interface EditorService {
    suspend fun listPersistedRecordPeriods(): List<String>

    suspend fun openPersistedRecordPeriod(period: String): RecordEditorDocument

    suspend fun serializeStructuredRecordDocument(document: StructuredRecordEditorDocument): String

    suspend fun previewRecordSummary(rawText: String): EditorRecordSummary =
        EditorRecordSummary(income = 0.0, expense = 0.0)

    suspend fun commitRecordDocument(period: String, rawText: String): RecordSaveResult
}
