package com.billstracer.android.data.services

import android.net.Uri
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.ExportedRecordFilesResult
import com.billstracer.android.model.ImportResult

interface WorkspaceService {
    suspend fun initializeEnvironment(): AppEnvironment

    suspend fun importBundledSample(): ImportResult

    suspend fun exportWorkspaceFiles(targetDirectoryUri: Uri): ExportedRecordFilesResult

    suspend fun clearRecordFiles(): Int

    suspend fun clearDatabase(): Boolean
}
