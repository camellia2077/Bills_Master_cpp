package com.billstracer.android.data.services

import com.billstracer.android.model.ListedRecordPeriodsResult
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewResult

interface EditorService {
    suspend fun openRecordPeriod(period: String): RecordEditorDocument

    suspend fun saveRecordDocument(period: String, rawText: String): RecordEditorDocument

    suspend fun previewRecordDocument(period: String, rawText: String): RecordPreviewResult

    suspend fun listRecordPeriods(): ListedRecordPeriodsResult
}
