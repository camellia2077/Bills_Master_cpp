package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.EditorNativeBindings
import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.double
import com.billstracer.android.data.nativebridge.int
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.model.InvalidRecordFile
import com.billstracer.android.model.ListedRecordPeriodsResult
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewFile
import com.billstracer.android.model.RecordPreviewResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.intOrNull
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import java.io.File

internal class DefaultEditorService(
    private val runtime: AndroidWorkspaceRuntime,
) : EditorService {
    override suspend fun openRecordPeriod(period: String): RecordEditorDocument =
        withContext(Dispatchers.IO) {
            val workspace = runtime.initializeWorkspace()
            val persistedRecordFile = recordFileForPeriod(workspace.recordsRoot, period)
            if (persistedRecordFile.isFile) {
                return@withContext RecordEditorDocument(
                    period = period,
                    relativePath = persistedRecordFile.relativeTo(workspace.recordsRoot)
                        .invariantSeparatorsPath,
                    rawText = persistedRecordFile.readText(Charsets.UTF_8),
                    persisted = true,
                )
            }

            parseRecordEditorDocument(
                EditorNativeBindings.generateRecordTemplateNative(
                    workspace.configRoot.absolutePath,
                    period,
                ),
            )
        }

    override suspend fun saveRecordDocument(
        period: String,
        rawText: String,
    ): RecordEditorDocument = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        val targetFile = recordFileForPeriod(workspace.recordsRoot, period)
        targetFile.parentFile?.mkdirs()
        targetFile.writeText(rawText, Charsets.UTF_8)
        RecordEditorDocument(
            period = period,
            relativePath = targetFile.relativeTo(workspace.recordsRoot).invariantSeparatorsPath,
            rawText = rawText,
            persisted = true,
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

    override suspend fun listRecordPeriods(): ListedRecordPeriodsResult = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        parseListedRecordPeriodsResult(
            EditorNativeBindings.listRecordPeriodsNative(workspace.recordsRoot.absolutePath),
        )
    }

    private fun parseRecordEditorDocument(rawJson: String): RecordEditorDocument {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        if (!root.boolean("ok")) {
            error(root.string("message"))
        }
        return RecordEditorDocument(
            period = data.string("period"),
            relativePath = data.string("relative_path"),
            rawText = data.string("text"),
            persisted = data.boolean("persisted"),
        )
    }

    private fun parseRecordPreviewResult(rawJson: String): RecordPreviewResult {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        val files = data["files"]?.jsonArray?.map { item ->
            val entry = item.jsonObject
            RecordPreviewFile(
                path = entry.string("path"),
                ok = entry.boolean("ok"),
                period = entry["period"]?.jsonPrimitive?.contentOrNull,
                year = entry["year"]?.jsonPrimitive?.intOrNull,
                month = entry["month"]?.jsonPrimitive?.intOrNull,
                transactionCount = entry.int("transaction_count"),
                totalIncome = entry.double("total_income"),
                totalExpense = entry.double("total_expense"),
                balance = entry.double("balance"),
                error = entry["error"]?.jsonPrimitive?.contentOrNull,
            )
        }.orEmpty()

        return RecordPreviewResult(
            ok = root.boolean("ok"),
            code = root.string("code"),
            message = root.string("message"),
            processed = data.int("processed"),
            success = data.int("success"),
            failure = data.int("failure"),
            periods = data["periods"]?.jsonArray?.mapNotNull { item ->
                item.jsonPrimitive.contentOrNull
            }.orEmpty(),
            files = files,
            rawJson = rawJson,
        )
    }

    private fun parseListedRecordPeriodsResult(rawJson: String): ListedRecordPeriodsResult {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        val invalidFiles = data["invalid_files"]?.jsonArray?.map { item ->
            val entry = item.jsonObject
            InvalidRecordFile(
                path = entry.string("path"),
                error = entry.string("error"),
            )
        }.orEmpty()

        return ListedRecordPeriodsResult(
            ok = root.boolean("ok"),
            code = root.string("code"),
            message = root.string("message"),
            processed = data.int("processed"),
            valid = data.int("valid"),
            invalid = data.int("invalid"),
            periods = data["periods"]?.jsonArray?.mapNotNull { item ->
                item.jsonPrimitive.contentOrNull
            }.orEmpty(),
            invalidFiles = invalidFiles,
            rawJson = rawJson,
        )
    }

    private fun recordFileForPeriod(recordsRoot: File, period: String): File {
        val year = period.substringBefore('-', missingDelimiterValue = period)
        return File(File(recordsRoot, year), "$period.txt")
    }
}
