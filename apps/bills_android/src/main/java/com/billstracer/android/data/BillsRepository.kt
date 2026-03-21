package com.billstracer.android.data

import android.net.Uri
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.ExportedRecordFilesResult
import com.billstracer.android.model.ImportResult
import com.billstracer.android.model.ListedRecordPeriodsResult
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewResult
import com.billstracer.android.model.ThemePreferences

interface BillsRepository {
    suspend fun initialize(): AppEnvironment

    suspend fun updateBundledConfig(fileName: String, rawText: String): List<BundledConfigFile>

    suspend fun updateThemePreferences(preferences: ThemePreferences): ThemePreferences

    suspend fun importBundledSample(): ImportResult

    suspend fun queryYear(isoYear: String): QueryResult

    suspend fun queryMonth(isoMonth: String): QueryResult

    suspend fun openRecordPeriod(period: String): RecordEditorDocument

    suspend fun saveRecordDocument(period: String, rawText: String): RecordEditorDocument

    suspend fun previewRecordDocument(period: String, rawText: String): RecordPreviewResult

    suspend fun listRecordPeriods(): ListedRecordPeriodsResult

    suspend fun clearRecordFiles(): Int

    suspend fun exportRecordFiles(targetDirectoryUri: Uri): ExportedRecordFilesResult

    suspend fun clearDatabase(): Boolean
}
