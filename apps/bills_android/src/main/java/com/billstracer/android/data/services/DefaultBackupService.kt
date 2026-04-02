package com.billstracer.android.data.services

import android.content.Context
import android.net.Uri
import com.billstracer.android.data.nativebridge.WorkspaceNativeBindings
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.model.ExportedBackupBundleResult
import com.billstracer.android.model.ImportedBackupBundleResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

internal class DefaultBackupService(
    context: Context,
    private val runtime: AndroidWorkspaceRuntime,
) : BackupService {
    private val documentGateway = WorkspaceDocumentGateway(context)
    private val tempStorage = WorkspaceTempStorage(context.applicationContext.cacheDir)

    override suspend fun exportBackupBundle(
        targetDocumentUri: Uri,
    ): ExportedBackupBundleResult = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        val tempBundleFile = tempStorage.createTempBundleFile(prefix = "backup_bundle_export_")
        try {
            val exported = BackupNativeResultParser.parseExportedBackupBundleResult(
                rawJson = WorkspaceNativeBindings.exportBackupBundleNative(
                    workspace.configRoot.absolutePath,
                    workspace.recordsRoot.absolutePath,
                    tempBundleFile.absolutePath,
                ),
                destinationDisplayPath = documentGateway.displayPathForUri(
                    targetDocumentUri,
                    fallback = "selected backup file",
                ),
            )

            documentGateway.copyFileToUri(
                sourceFile = tempBundleFile,
                targetUri = targetDocumentUri,
                failureMessage = "Failed to open the selected backup export document.",
            )
            exported
        } catch (error: Throwable) {
            documentGateway.deleteSingleDocument(targetDocumentUri)
            throw error
        } finally {
            tempBundleFile.delete()
        }
    }

    override suspend fun importBackupBundle(
        sourceDocumentUri: Uri,
    ): ImportedBackupBundleResult = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        val tempBundleFile = tempStorage.createTempBundleFile(prefix = "backup_bundle_import_")
        try {
            documentGateway.copyUriToFile(
                sourceUri = sourceDocumentUri,
                destinationFile = tempBundleFile,
                failureMessage = "Failed to open the selected backup bundle.",
            )

            BackupNativeResultParser.parseImportedBackupBundleResult(
                rawJson = WorkspaceNativeBindings.importBackupBundleNative(
                    tempBundleFile.absolutePath,
                    workspace.configRoot.absolutePath,
                    workspace.recordsRoot.absolutePath,
                    workspace.dbFile.absolutePath,
                ),
                sourceDisplayPath = documentGateway.displayPathForUri(
                    sourceDocumentUri,
                    fallback = "selected backup bundle",
                ),
            )
        } finally {
            tempBundleFile.delete()
        }
    }
}
