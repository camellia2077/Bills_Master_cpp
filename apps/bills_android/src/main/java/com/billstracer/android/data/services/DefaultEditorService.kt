package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.EditorNativeBindings
import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewResult
import com.billstracer.android.model.RecordSaveResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import java.io.File

internal class DefaultEditorService(
    private val runtime: AndroidWorkspaceRuntime,
) : EditorService {
    override suspend fun listPersistedRecordPeriods(): List<String> = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        parseDatabaseRecordPeriods(
            EditorNativeBindings.listDatabaseRecordPeriodsNative(
                workspace.dbFile.absolutePath,
            ),
        )
    }

    override suspend fun openPersistedRecordPeriod(period: String): RecordEditorDocument =
        withContext(Dispatchers.IO) {
            val workspace = runtime.initializeWorkspace()
            val persistedRecordFile = recordFileForPeriod(workspace.recordsRoot, period)
            if (!persistedRecordFile.isFile) {
                error(
                    "Database and TXT source are out of sync for $period. Missing " +
                        persistedRecordFile.relativeTo(workspace.recordsRoot).invariantSeparatorsPath +
                        ". Re-import or resync records/.",
                )
            }
            RecordEditorDocument(
                period = period,
                relativePath = persistedRecordFile.relativeTo(workspace.recordsRoot)
                    .invariantSeparatorsPath,
                rawText = persistedRecordFile.readText(Charsets.UTF_8),
                persisted = true,
            )
        }

    override suspend fun commitRecordDocument(
        period: String,
        rawText: String,
    ): RecordSaveResult = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        parseRecordSaveResult(
            EditorNativeBindings.commitRecordDocumentNative(
                period,
                rawText,
                workspace.configRoot.absolutePath,
                workspace.recordsRoot.absolutePath,
                workspace.dbFile.absolutePath,
            ),
        )
    }

    override suspend fun previewRecordDocument(
        period: String,
        rawText: String,
    ): RecordPreviewResult = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        val previewDir = File(workspace.recordsRoot, ".preview")
        previewDir.mkdirs()
        val previewFile = File.createTempFile(
            "record-preview-${period.replace("-", "_")}-",
            ".txt",
            previewDir,
        )
        try {
            previewFile.writeText(rawText, Charsets.UTF_8)
            parseRecordPreviewResult(
                EditorNativeBindings.previewRecordPathNative(
                    previewFile.absolutePath,
                    workspace.configRoot.absolutePath,
                ),
            )
        } finally {
            previewFile.delete()
        }
    }

    private fun parseDatabaseRecordPeriods(rawJson: String): List<String> {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        if (!root.boolean("ok")) {
            error(root.string("message"))
        }
        return data["periods"]?.jsonArray?.mapNotNull { item ->
            item.jsonPrimitive.contentOrNull
        }.orEmpty().distinct().sortedDescending()
    }

    private fun parseRecordSaveResult(rawJson: String): RecordSaveResult {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        val errorMessage = data["error_message"]?.jsonPrimitive?.contentOrNull
            ?: root.string("message").takeIf { !root.boolean("ok") }
        val document = if (root.boolean("ok")) {
            RecordEditorDocument(
                period = data.string("period"),
                relativePath = data.string("relative_path"),
                rawText = data.string("text"),
                persisted = data.boolean("persisted"),
            )
        } else {
            null
        }
        return RecordSaveResult(
            ok = root.boolean("ok"),
            message = root.string("message"),
            document = document,
            errorMessage = errorMessage,
            rawJson = rawJson,
        )
    }
}
