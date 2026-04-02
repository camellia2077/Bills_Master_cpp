package com.billstracer.android.data.services

import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordSaveResult

interface EditorService {
    suspend fun listPersistedRecordPeriods(): List<String>

    suspend fun openPersistedRecordPeriod(period: String): RecordEditorDocument

    suspend fun commitRecordDocument(period: String, rawText: String): RecordSaveResult
}
