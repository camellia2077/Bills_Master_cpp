package com.billstracer.android.data.services

import android.content.Context
import android.net.Uri
import androidx.documentfile.provider.DocumentFile
import com.billstracer.android.data.nativebridge.WorkspaceNativeBindings
import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.int
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.ExportedParseBundleResult
import com.billstracer.android.model.ImportedParseBundleResult
import com.billstracer.android.model.ImportResult
import com.billstracer.android.model.RecordDirectoryImportResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import java.io.File
import java.util.Locale

internal class DefaultWorkspaceService(
    context: Context,
    private val runtime: AndroidWorkspaceRuntime,
) : WorkspaceService {
    private val applicationContext = context.applicationContext

    private data class SourceTxtDocument(
        val relativePath: String,
        val rawText: String,
    )

    private data class StagedTxtDocument(
        val relativePath: String,
        val rawText: String,
        val tempFile: File,
    )

    override suspend fun initializeEnvironment(): AppEnvironment {
        val workspace = runtime.initializeWorkspace()
        return AppEnvironment(
            bundledSampleInputPath = workspace.bundledSampleInputPath,
            bundledSampleLabel = workspace.bundledSampleSpec?.label,
            bundledSampleYear = workspace.bundledSampleSpec?.year,
            bundledSampleMonth = workspace.bundledSampleSpec?.month,
            configRoot = workspace.configRoot,
            recordsRoot = workspace.recordsRoot,
            dbFile = workspace.dbFile,
        )
    }

    override suspend fun importRecordFilesToDatabase(): ImportResult = withContext(Dispatchers.IO) {
        val environment = initializeEnvironment()
        parseImportResult(
            WorkspaceNativeBindings.importRecordsToDatabaseNative(
                environment.configRoot.absolutePath,
                environment.recordsRoot.absolutePath,
                environment.dbFile.absolutePath,
            ),
        )
    }

    override suspend fun importTxtDirectoryToRecords(
        sourceDirectoryUri: Uri,
    ): RecordDirectoryImportResult = withContext(Dispatchers.IO) {
        val environment = initializeEnvironment()
        val sourceDocuments = loadSourceTxtDocuments(sourceDirectoryUri)
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

        val tempRoot = createTempRecordImportDir()
        try {
            stageSourceTxtDocuments(tempRoot, sourceDocuments)
            parseRecordDirectoryImportResult(
                WorkspaceNativeBindings.importTxtDirectoryToRecordsNative(
                    tempRoot.absolutePath,
                    environment.configRoot.absolutePath,
                    environment.recordsRoot.absolutePath,
                ),
            )
        } finally {
            tempRoot.deleteRecursively()
        }
    }

    override suspend fun importBundledSample(): ImportResult = withContext(Dispatchers.IO) {
        val environment = initializeEnvironment()
        parseImportResult(BundledSampleImportSupport.importBundledSample(environment))
    }

    override suspend fun exportParseBundle(
        targetDocumentUri: Uri,
    ): ExportedParseBundleResult = withContext(Dispatchers.IO) {
        val environment = initializeEnvironment()
        val tempBundleFile = createTempBundleFile(prefix = "parse_bundle_export_")
        try {
            val exported = parseExportParseBundleResult(
                rawJson = WorkspaceNativeBindings.exportParseBundleNative(
                    environment.configRoot.absolutePath,
                    environment.recordsRoot.absolutePath,
                    tempBundleFile.absolutePath,
                ),
                destinationDisplayPath = displayPathForUri(targetDocumentUri, fallback = "selected file"),
            )

            applicationContext.contentResolver.openOutputStream(targetDocumentUri, "wt")?.use { output ->
                tempBundleFile.inputStream().use { input -> input.copyTo(output) }
            } ?: error("Failed to open the selected export document.")

            exported
        } catch (error: Throwable) {
            DocumentFile.fromSingleUri(applicationContext, targetDocumentUri)?.delete()
            throw error
        } finally {
            tempBundleFile.delete()
        }
    }

    override suspend fun importParseBundle(
        sourceDocumentUri: Uri,
    ): ImportedParseBundleResult = withContext(Dispatchers.IO) {
        val environment = initializeEnvironment()
        val tempBundleFile = createTempBundleFile(prefix = "parse_bundle_import_")
        try {
            applicationContext.contentResolver.openInputStream(sourceDocumentUri)?.use { input ->
                tempBundleFile.outputStream().use { output -> input.copyTo(output) }
            } ?: error("Failed to open the selected parse bundle.")

            parseImportedParseBundleResult(
                rawJson = WorkspaceNativeBindings.importParseBundleNative(
                    tempBundleFile.absolutePath,
                    environment.configRoot.absolutePath,
                    environment.recordsRoot.absolutePath,
                    environment.dbFile.absolutePath,
                ),
                sourceDisplayPath = displayPathForUri(sourceDocumentUri, fallback = "selected parse bundle"),
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

    private fun parseRecordDirectoryImportResult(rawJson: String): RecordDirectoryImportResult {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        if (!root.boolean("ok") && root.string("code") != "business.import_failed") {
            error(root.string("message"))
        }
        return RecordDirectoryImportResult(
            processed = data.int("processed"),
            imported = data.int("imported"),
            overwritten = data.int("overwritten"),
            failure = data.int("failure"),
            invalid = data.int("invalid"),
            duplicatePeriodConflicts = data.int("duplicate_period_conflicts"),
            firstFailureMessage = data["first_failure_message"]?.jsonPrimitive?.contentOrNull,
        )
    }

    private fun parseExportParseBundleResult(
        rawJson: String,
        destinationDisplayPath: String,
    ): ExportedParseBundleResult {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        return ExportedParseBundleResult(
            exportedRecordFiles = data.int("exported_record_files"),
            exportedConfigFiles = data.int("exported_config_files"),
            destinationDisplayPath = destinationDisplayPath,
            rawJson = rawJson,
        )
    }

    private fun parseImportedParseBundleResult(
        rawJson: String,
        sourceDisplayPath: String,
    ): ImportedParseBundleResult {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        return ImportedParseBundleResult(
            ok = root.boolean("ok"),
            code = root.string("code"),
            message = root.string("message"),
            importedRecordFiles = data.int("imported_record_files"),
            importedConfigFiles = data.int("imported_config_files"),
            importedBills = data.int("imported_bills"),
            failedPhase = data["failed_phase"]?.jsonPrimitive?.contentOrNull,
            sourceDisplayPath = sourceDisplayPath,
            rawJson = rawJson,
        )
    }

    private fun createTempBundleFile(prefix: String): File {
        val tempRoot = File(applicationContext.cacheDir, "parse_bundle")
        tempRoot.mkdirs()
        return File.createTempFile(prefix, ".zip", tempRoot)
    }

    private fun createTempRecordImportDir(): File {
        val tempRoot = File(applicationContext.cacheDir, "record_directory_import")
        tempRoot.mkdirs()
        return File.createTempFile("record_directory_", "", tempRoot).apply {
            delete()
            mkdirs()
        }
    }

    private fun displayPathForUri(uri: Uri, fallback: String): String =
        DocumentFile.fromSingleUri(applicationContext, uri)
            ?.name
            ?.takeIf { it.isNotBlank() }
            ?: fallback

    private fun loadSourceTxtDocuments(sourceDirectoryUri: Uri): List<SourceTxtDocument> {
        val documents = if (sourceDirectoryUri.scheme == "file") {
            loadFileSchemeTxtDocuments(sourceDirectoryUri)
        } else {
            loadTreeSchemeTxtDocuments(sourceDirectoryUri)
        }
        return documents.sortedWith(
            compareBy<SourceTxtDocument>(
                { document -> document.relativePath.lowercase(Locale.ROOT) },
                { document -> document.relativePath },
            ),
        )
    }

    private fun loadFileSchemeTxtDocuments(sourceDirectoryUri: Uri): List<SourceTxtDocument> {
        val directoryPath = requireNotNull(sourceDirectoryUri.path) {
            "Failed to resolve the selected directory."
        }
        val rootDirectory = File(directoryPath)
        require(rootDirectory.isDirectory) { "Selected location is not a directory." }
        return rootDirectory.walkTopDown()
            .filter { file -> file.isFile && file.extension.equals("txt", ignoreCase = true) }
            .map { file ->
                SourceTxtDocument(
                    relativePath = file.relativeTo(rootDirectory).invariantSeparatorsPath,
                    rawText = file.readText(Charsets.UTF_8),
                )
            }
            .toList()
    }

    private fun loadTreeSchemeTxtDocuments(sourceDirectoryUri: Uri): List<SourceTxtDocument> {
        val rootDocument = DocumentFile.fromTreeUri(applicationContext, sourceDirectoryUri)
            ?: error("Failed to open the selected directory.")
        if (!rootDocument.isDirectory) {
            error("Selected location is not a directory.")
        }
        val documents = mutableListOf<SourceTxtDocument>()
        collectTreeTxtDocuments(rootDocument, emptyList(), documents)
        return documents
    }

    private fun collectTreeTxtDocuments(
        document: DocumentFile,
        pathSegments: List<String>,
        output: MutableList<SourceTxtDocument>,
    ) {
        document.listFiles().forEach { child ->
            val childName = child.name ?: return@forEach
            if (child.isDirectory) {
                collectTreeTxtDocuments(child, pathSegments + childName, output)
                return@forEach
            }
            if (!child.isFile || !childName.endsWith(".txt", ignoreCase = true)) {
                return@forEach
            }
            val relativePath = (pathSegments + childName).joinToString("/")
            output += SourceTxtDocument(
                relativePath = relativePath,
                rawText = readTextFromUri(child.uri, relativePath),
            )
        }
    }

    private fun readTextFromUri(uri: Uri, label: String): String =
        applicationContext.contentResolver.openInputStream(uri)?.use { input ->
            input.readBytes().toString(Charsets.UTF_8)
        } ?: error("Failed to open TXT file '$label'.")

    private fun stageSourceTxtDocuments(
        tempRoot: File,
        sourceDocuments: List<SourceTxtDocument>,
    ): List<StagedTxtDocument> = sourceDocuments.map { document ->
        val tempFile = File(tempRoot, document.relativePath)
        tempFile.parentFile?.mkdirs()
        tempFile.writeText(document.rawText, Charsets.UTF_8)
        StagedTxtDocument(
            relativePath = document.relativePath,
            rawText = document.rawText,
            tempFile = tempFile,
        )
    }
}
