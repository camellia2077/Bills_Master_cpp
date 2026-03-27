package com.billstracer.android.data.services

import android.net.Uri
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.ExportedParseBundleResult
import com.billstracer.android.model.ImportedParseBundleResult
import com.billstracer.android.model.ImportResult
import com.billstracer.android.model.RecordDirectoryImportResult

interface WorkspaceService {
    suspend fun initializeEnvironment(): AppEnvironment

    suspend fun importTxtDirectoryAndSyncDatabase(sourceDirectoryUri: Uri): RecordDirectoryImportResult

    suspend fun importBundledSample(): ImportResult

    suspend fun exportParseBundle(targetDocumentUri: Uri): ExportedParseBundleResult

    suspend fun importParseBundle(sourceDocumentUri: Uri): ImportedParseBundleResult

    suspend fun clearRecordFiles(): Int

    suspend fun clearDatabase(): Boolean
}
