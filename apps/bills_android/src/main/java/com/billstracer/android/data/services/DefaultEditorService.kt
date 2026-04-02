package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.EditorNativeBindings
import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordSaveResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

internal class DefaultEditorService(
    private val runtime: AndroidWorkspaceRuntime,
) : EditorService {
    override suspend fun listPersistedRecordPeriods(): List<String> = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        val periods = mutableSetOf<String>()
        workspace.recordsRoot.walkTopDown().forEach { file ->
            if (file.isFile && file.extension.equals("txt", ignoreCase = true)) {
                file.useLines { lines ->
                    val firstLine = lines.firstOrNull()?.trim()
                    if (firstLine != null && firstLine.startsWith("date:")) {
                        val period = firstLine.substringAfter("date:").trim()
                        if (period.length == 7) {
                            periods.add(period)
                        }
                    }
                }
            }
        }
        val currentPeriod = java.time.YearMonth.now().toString()
        periods.add(currentPeriod)
        periods.toList().sortedDescending()
    }

    override suspend fun openPersistedRecordPeriod(period: String): RecordEditorDocument =
        withContext(Dispatchers.IO) {
            val workspace = runtime.initializeWorkspace()
            val persistedRecordFile = findRecordFileForPeriod(workspace.recordsRoot, period)
            if (persistedRecordFile == null) {
                val rawJson = EditorNativeBindings.generateRecordTemplateJsonNative(
                    workspace.configRoot.absolutePath,
                    period,
                )
                val generatedText = parseRecordTemplateResult(rawJson)
                return@withContext RecordEditorDocument(
                    period = period,
                    relativePath = defaultRecordFileForPeriod(workspace.recordsRoot, period)
                        .relativeTo(workspace.recordsRoot).invariantSeparatorsPath,
                    rawText = generatedText,
                    persisted = false,
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
            EditorNativeBindings.commitRecordDocumentJsonNative(
                period,
                rawText,
                workspace.configRoot.absolutePath,
                workspace.recordsRoot.absolutePath,
                workspace.dbFile.absolutePath,
            ),
        )
    }

    private fun parseRecordTemplateResult(rawJson: String): String {
        val root = parseRoot(rawJson)
        if (!root.boolean("ok")) {
            error(root.string("message"))
        }
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        return data.string("text")
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
