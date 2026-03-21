package com.billstracer.android.data.services

import android.content.Context
import android.net.Uri
import androidx.documentfile.provider.DocumentFile
import com.billstracer.android.BuildConfig
import com.billstracer.android.data.nativebridge.WorkspaceNativeBindings
import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.int
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.ExportedRecordFilesResult
import com.billstracer.android.model.ImportResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.jsonObject
import java.io.File
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter

internal class DefaultWorkspaceService(
    context: Context,
    private val runtime: AndroidWorkspaceRuntime,
) : WorkspaceService {
    private val applicationContext = context.applicationContext

    override suspend fun initializeEnvironment(): AppEnvironment {
        val workspace = runtime.initializeWorkspace()
        return AppEnvironment(
            bundledSampleInputPath = workspace.bundledSampleInputPath,
            bundledSampleLabel = BuildConfig.BUNDLED_SAMPLE_LABEL,
            bundledSampleYear = BuildConfig.BUNDLED_SAMPLE_YEAR,
            bundledSampleMonth = BuildConfig.BUNDLED_SAMPLE_MONTH,
            configRoot = workspace.configRoot,
            recordsRoot = workspace.recordsRoot,
            dbFile = workspace.dbFile,
        )
    }

    override suspend fun importBundledSample(): ImportResult = withContext(Dispatchers.IO) {
        val environment = initializeEnvironment()
        parseImportResult(
            WorkspaceNativeBindings.importBundledSampleNative(
                environment.bundledSampleInputPath.absolutePath,
                environment.configRoot.absolutePath,
                environment.dbFile.absolutePath,
            ),
        )
    }

    override suspend fun exportWorkspaceFiles(
        targetDirectoryUri: Uri,
    ): ExportedRecordFilesResult = withContext(Dispatchers.IO) {
        val environment = initializeEnvironment()
        val txtFiles = environment.recordsRoot.walkTopDown()
            .filter { file -> file.isFile && file.extension.equals("txt", ignoreCase = true) }
            .filterNot { file -> file.hasHiddenRelativePath(environment.recordsRoot) }
            .sortedBy { file -> file.relativeTo(environment.recordsRoot).invariantSeparatorsPath }
            .toList()
        val configFiles = environment.configRoot.walkTopDown()
            .filter { file -> file.isFile && file.extension.equals("toml", ignoreCase = true) }
            .filterNot { file -> file.hasHiddenRelativePath(environment.configRoot) }
            .sortedBy { file -> file.relativeTo(environment.configRoot).invariantSeparatorsPath }
            .toList()

        if (txtFiles.isEmpty() && configFiles.isEmpty()) {
            return@withContext ExportedRecordFilesResult(
                exportedRecordFiles = 0,
                exportedConfigFiles = 0,
            )
        }

        val selectedDirectory = DocumentFile.fromTreeUri(applicationContext, targetDirectoryUri)
            ?: error("Failed to open the selected export folder.")
        require(selectedDirectory.isDirectory) {
            "The selected export folder is not a directory."
        }
        require(selectedDirectory.canWrite()) {
            "The selected export folder is not writable."
        }

        val exportRoot = createUniqueChildDirectory(
            parent = selectedDirectory,
            baseName = buildExportFolderName(),
        )
        try {
            txtFiles.forEach { sourceFile ->
                exportWorkspaceFile(
                    sourceFile = sourceFile,
                    exportRoot = exportRoot,
                    bundleSection = "records",
                    relativePath = sourceFile.relativeTo(environment.recordsRoot).invariantSeparatorsPath,
                )
            }
            configFiles.forEach { sourceFile ->
                exportWorkspaceFile(
                    sourceFile = sourceFile,
                    exportRoot = exportRoot,
                    bundleSection = "config",
                    relativePath = sourceFile.relativeTo(environment.configRoot).invariantSeparatorsPath,
                )
            }
        } catch (error: Throwable) {
            exportRoot.deleteRecursively()
            throw error
        }

        ExportedRecordFilesResult(
            exportedRecordFiles = txtFiles.size,
            exportedConfigFiles = configFiles.size,
            destinationDisplayPath = buildDestinationDisplayPath(selectedDirectory, exportRoot),
        )
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

    private fun parseImportResult(rawJson: String): ImportResult {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        return ImportResult(
            ok = root.boolean("ok"),
            code = root.string("code"),
            message = root.string("message"),
            processed = data.int("processed"),
            success = data.int("success"),
            failure = data.int("failure"),
            imported = data.int("imported"),
            rawJson = rawJson,
        )
    }

    private fun buildExportFolderName(): String {
        val timestamp = DateTimeFormatter.ofPattern("yyyyMMdd_HHmmss")
            .format(LocalDateTime.now())
        return "workspace_export_$timestamp"
    }

    private fun buildDestinationDisplayPath(
        selectedDirectory: DocumentFile,
        exportRoot: DocumentFile,
    ): String {
        val selectedFolderLabel = selectedDirectory.name?.takeIf { it.isNotBlank() } ?: "selected folder"
        val exportFolderLabel = exportRoot.name?.takeIf { it.isNotBlank() } ?: "workspace export"
        return "$selectedFolderLabel/$exportFolderLabel"
    }

    private fun exportWorkspaceFile(
        sourceFile: File,
        exportRoot: DocumentFile,
        bundleSection: String,
        relativePath: String,
    ) {
        val resolver = applicationContext.contentResolver
        var destinationDirectory = ensureChildDirectory(exportRoot, bundleSection)
        val pathSegments = relativePath.split('/').filter { it.isNotBlank() }
        require(pathSegments.isNotEmpty()) {
            "Invalid export relative path: $relativePath"
        }
        pathSegments.dropLast(1).forEach { segment ->
            destinationDirectory = ensureChildDirectory(destinationDirectory, segment)
        }
        val destinationFileName = pathSegments.last()
        destinationDirectory.findFile(destinationFileName)?.let { existingFile ->
            require(!existingFile.isDirectory) {
                "Export destination already contains a directory named '$destinationFileName'."
            }
            require(existingFile.delete()) {
                "Failed to replace existing export file '$destinationFileName'."
            }
        }
        val destinationFile = destinationDirectory.createFile(
            exportMimeTypeFor(sourceFile),
            destinationFileName,
        ) ?: error("Failed to create export destination for $relativePath.")
        try {
            resolver.openOutputStream(destinationFile.uri)?.use { outputStream ->
                sourceFile.inputStream().use { inputStream ->
                    inputStream.copyTo(outputStream)
                }
            } ?: error("Failed to open export destination for $relativePath.")
        } catch (error: Throwable) {
            destinationFile.delete()
            throw error
        }
    }

    private fun createUniqueChildDirectory(
        parent: DocumentFile,
        baseName: String,
    ): DocumentFile {
        repeat(100) { index ->
            val candidateName = if (index == 0) baseName else "${baseName}_$index"
            val existing = parent.findFile(candidateName)
            if (existing == null) {
                return parent.createDirectory(candidateName)
                    ?: error("Failed to create export folder '$candidateName'.")
            }
            if (existing.isDirectory && existing.listFiles().isEmpty()) {
                return existing
            }
        }
        error("Failed to allocate a unique export folder in the selected directory.")
    }

    private fun ensureChildDirectory(
        parent: DocumentFile,
        directoryName: String,
    ): DocumentFile {
        val existing = parent.findFile(directoryName)
        if (existing != null) {
            require(existing.isDirectory) {
                "Export destination already contains a file named '$directoryName'."
            }
            return existing
        }
        return parent.createDirectory(directoryName)
            ?: error("Failed to create export subdirectory '$directoryName'.")
    }

    private fun DocumentFile.deleteRecursively() {
        if (isDirectory) {
            listFiles().forEach { child -> child.deleteRecursively() }
        }
        delete()
    }

    private fun File.hasHiddenRelativePath(rootDirectory: File): Boolean =
        relativeTo(rootDirectory)
            .invariantSeparatorsPath
            .split('/')
            .any { segment -> segment.startsWith(".") }

    private fun exportMimeTypeFor(sourceFile: File): String =
        when (sourceFile.extension.lowercase()) {
            "toml" -> "application/toml"
            else -> "text/plain"
        }
}
