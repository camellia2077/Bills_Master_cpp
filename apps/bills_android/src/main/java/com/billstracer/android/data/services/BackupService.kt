package com.billstracer.android.data.services

import android.net.Uri
import com.billstracer.android.model.ExportedBackupBundleResult
import com.billstracer.android.model.ImportedBackupBundleResult

interface BackupService {
    suspend fun exportBackupBundle(targetDocumentUri: Uri): ExportedBackupBundleResult

    suspend fun importBackupBundle(sourceDocumentUri: Uri): ImportedBackupBundleResult
}
