package com.billstracer.android.data.services

import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordSaveResult
import com.billstracer.android.model.StructuredRecordEditorDocument

interface EditorService {
    suspend fun listPersistedRecordPeriods(): List<String>

    suspend fun openPersistedRecordPeriod(period: String): RecordEditorDocument

    suspend fun serializeStructuredRecordDocument(document: StructuredRecordEditorDocument): String

    suspend fun commitRecordDocument(period: String, rawText: String): RecordSaveResult
}
