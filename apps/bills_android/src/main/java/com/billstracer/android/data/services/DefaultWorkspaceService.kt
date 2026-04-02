package com.billstracer.android.data.services

import android.content.Context
import android.net.Uri
import com.billstracer.android.data.nativebridge.WorkspaceNativeBindings
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.ExportedParseBundleResult
import com.billstracer.android.model.ImportedParseBundleResult
import com.billstracer.android.model.RecordDirectoryImportResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File

internal class DefaultWorkspaceService(
    context: Context,
    private val runtime: AndroidWorkspaceRuntime,
) : WorkspaceService {
    private val documentGateway = WorkspaceDocumentGateway(context)
    private val tempStorage = WorkspaceTempStorage(context.applicationContext.cacheDir)

    override suspend fun initializeEnvironment(): AppEnvironment {
        val workspace = runtime.initializeWorkspace()
        return AppEnvironment(
            configRoot = workspace.configRoot,
            recordsRoot = workspace.recordsRoot,
            dbFile = workspace.dbFile,
        )
    }

    override suspend fun importTxtDirectoryAndSyncDatabase(
        sourceDirectoryUri: Uri,
    ): RecordDirectoryImportResult = withContext(Dispatchers.IO) {
        val environment = initializeEnvironment()
        val sourceDocuments = documentGateway.loadSourceTxtDocuments(sourceDirectoryUri)
        if (sourceDocuments.isEmpty()) {
            return@withContext RecordDirectoryImportResult(
                processed = 0,
                imported = 0,
                overwritten = 0,
                failure = 0,
                invalid = 0,
                duplicatePeriodConflicts = 0,
            )
        }

        val tempRoot = tempStorage.createTempRecordImportDir()
        try {
            tempStorage.stageSourceTxtDocuments(tempRoot, sourceDocuments)
            WorkspaceNativeResultParser.parseRecordDirectoryImportResult(
                WorkspaceNativeBindings.importTxtDirectoryAndSyncDatabaseNative(
                    tempRoot.absolutePath,
                    environment.configRoot.absolutePath,
                    environment.recordsRoot.absolutePath,
                    environment.dbFile.absolutePath,
                ),
            )
        } finally {
            tempRoot.deleteRecursively()
        }
    }

    override suspend fun exportParseBundle(
        targetDocumentUri: Uri,
    ): ExportedParseBundleResult = withContext(Dispatchers.IO) {
        val environment = initializeEnvironment()
        val tempBundleFile = tempStorage.createTempBundleFile(prefix = "parse_bundle_export_")
        try {
            val exported = WorkspaceNativeResultParser.parseExportParseBundleResult(
                rawJson = WorkspaceNativeBindings.exportParseBundleNative(
                    environment.configRoot.absolutePath,
                    environment.recordsRoot.absolutePath,
                    tempBundleFile.absolutePath,
                ),
                destinationDisplayPath = documentGateway.displayPathForUri(
                    targetDocumentUri,
                    fallback = "selected file",
                ),
            )

            documentGateway.copyFileToUri(
                sourceFile = tempBundleFile,
                targetUri = targetDocumentUri,
                failureMessage = "Failed to open the selected export document.",
            )

            exported
        } catch (error: Throwable) {
            documentGateway.deleteSingleDocument(targetDocumentUri)
            throw error
        } finally {
            tempBundleFile.delete()
        }
    }

    override suspend fun importParseBundle(
        sourceDocumentUri: Uri,
    ): ImportedParseBundleResult = withContext(Dispatchers.IO) {
        val environment = initializeEnvironment()
        val tempBundleFile = tempStorage.createTempBundleFile(prefix = "parse_bundle_import_")
        try {
            documentGateway.copyUriToFile(
                sourceUri = sourceDocumentUri,
                destinationFile = tempBundleFile,
                failureMessage = "Failed to open the selected parse bundle.",
            )

            WorkspaceNativeResultParser.parseImportedParseBundleResult(
                rawJson = WorkspaceNativeBindings.importParseBundleNative(
                    tempBundleFile.absolutePath,
                    environment.configRoot.absolutePath,
                    environment.recordsRoot.absolutePath,
                    environment.dbFile.absolutePath,
                ),
                sourceDisplayPath = documentGateway.displayPathForUri(
                    sourceDocumentUri,
                    fallback = "selected parse bundle",
                ),
            )
        } finally {
            tempBundleFile.delete()
        }
    }

    override suspend fun clearRecordFiles(): Int = withContext(Dispatchers.IO) {
        val environment = initializeEnvironment()
        val txtFiles = environment.recordsRoot.walkTopDown()
            .filter { file -> file.isFile && file.extension.equals("txt", ignoreCase = true) }
            .toList()
        txtFiles.forEach { file -> file.delete() }
        environment.recordsRoot.walkBottomUp()
            .filter { file -> file.isDirectory && file != environment.recordsRoot }
            .forEach { directory ->
                if (directory.listFiles().isNullOrEmpty()) {
                    directory.delete()
                }
            }
        txtFiles.size
    }

    override suspend fun clearDatabase(): Boolean = runtime.clearDatabase()
}
