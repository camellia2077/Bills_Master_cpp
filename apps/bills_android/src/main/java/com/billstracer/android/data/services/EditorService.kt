package com.billstracer.android.data.services

import com.billstracer.android.model.RecordDatabaseSyncResult
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewResult

interface EditorService {
    suspend fun listDatabaseRecordPeriods(): List<String>

    suspend fun openPersistedRecordPeriod(period: String): RecordEditorDocument

    suspend fun saveRecordDocument(period: String, rawText: String): RecordEditorDocument

    suspend fun syncSavedRecordToDatabase(period: String): RecordDatabaseSyncResult

    suspend fun previewRecordDocument(period: String, rawText: String): RecordPreviewResult
}
