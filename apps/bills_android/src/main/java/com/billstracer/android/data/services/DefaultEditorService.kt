package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.EditorNativeBindings
import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordSaveResult
import com.billstracer.android.model.StructuredRecordEditorDocument
import com.billstracer.android.model.StructuredRecordEditorEntry
import com.billstracer.android.model.StructuredRecordEditorParentSection
import com.billstracer.android.model.StructuredRecordEditorSubSection
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive
import kotlinx.serialization.json.buildJsonArray
import kotlinx.serialization.json.buildJsonObject
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
                val parsed = parseStructuredDocumentOrFallback(generatedText)
                return@withContext RecordEditorDocument(
                    period = period,
                    relativePath = defaultRecordFileForPeriod(workspace.recordsRoot, period)
                        .relativeTo(workspace.recordsRoot).invariantSeparatorsPath,
                    rawText = generatedText,
                    persisted = false,
                    structuredDocument = parsed.first,
                    rawFallbackReason = parsed.second,
                )
            }
            val rawText = persistedRecordFile.readText(Charsets.UTF_8)
            val parsed = parseStructuredDocumentOrFallback(rawText)
            RecordEditorDocument(
                period = period,
                relativePath = persistedRecordFile.relativeTo(workspace.recordsRoot)
                    .invariantSeparatorsPath,
                rawText = rawText,
                persisted = true,
                structuredDocument = parsed.first,
                rawFallbackReason = parsed.second,
            )
        }

    override suspend fun serializeStructuredRecordDocument(
        document: StructuredRecordEditorDocument,
    ): String = withContext(Dispatchers.IO) {
        parseStructuredDocumentSerializationResult(
            EditorNativeBindings.serializeRecordEditorDocumentJsonNative(
                structuredDocumentToJson(document).toString(),
            ),
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
            val rawText = data.string("text")
            val parsed = parseStructuredDocumentOrFallback(rawText)
            RecordEditorDocument(
                period = data.string("period"),
                relativePath = data.string("relative_path"),
                rawText = rawText,
                persisted = data.boolean("persisted"),
                structuredDocument = parsed.first,
                rawFallbackReason = parsed.second,
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

    private fun parseStructuredDocumentOrFallback(rawText: String): Pair<StructuredRecordEditorDocument?, String?> {
        val root = parseRoot(EditorNativeBindings.parseRecordEditorDocumentJsonNative(rawText))
        if (!root.boolean("ok")) {
            val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
            val fallbackReason = buildString {
                append(root.string("message"))
                val line = data["line"]?.jsonPrimitive?.contentOrNull
                if (!line.isNullOrBlank()) {
                    append(" (line ")
                    append(line)
                    append(')')
                }
            }.ifBlank { "This TXT shape is not supported by the structured editor yet." }
            return null to fallbackReason
        }
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        return parseStructuredDocument(data) to null
    }

    private fun parseStructuredDocumentSerializationResult(rawJson: String): String {
        val root = parseRoot(rawJson)
        if (!root.boolean("ok")) {
            error(root.string("message"))
        }
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        return data.string("text")
    }

    private fun parseStructuredDocument(data: JsonObject): StructuredRecordEditorDocument =
        StructuredRecordEditorDocument(
            dateLine = data.string("date_line"),
            remarkLines = data["remark_lines"]?.jsonArray?.map { remark ->
                remark.jsonPrimitive.contentOrNull.orEmpty()
            }.orEmpty(),
            sections = data["sections"]?.jsonArray?.map { parentElement ->
                val parentObject = parentElement.jsonObject
                StructuredRecordEditorParentSection(
                    title = parentObject.string("title"),
                    subSections = parentObject["sub_sections"]?.jsonArray?.map { subSectionElement ->
                        val subSectionObject = subSectionElement.jsonObject
                        StructuredRecordEditorSubSection(
                            title = subSectionObject.string("title"),
                            entries = subSectionObject["entries"]?.jsonArray?.map { entryElement ->
                                val entryObject = entryElement.jsonObject
                                StructuredRecordEditorEntry(
                                    amountExpression = entryObject.string("amount_expression"),
                                    description = entryObject.string("description"),
                                    comment = entryObject.string("comment"),
                                )
                            }.orEmpty(),
                        )
                    }.orEmpty(),
                )
            }.orEmpty(),
        )

    private fun structuredDocumentToJson(document: StructuredRecordEditorDocument): JsonObject =
        buildJsonObject {
            put("date_line", JsonPrimitive(document.dateLine))
            put(
                "remark_lines",
                buildJsonArray {
                    document.remarkLines.forEach { line ->
                        add(JsonPrimitive(line))
                    }
                },
            )
            put(
                "sections",
                buildJsonArray {
                    document.sections.forEach { parent ->
                        add(
                            buildJsonObject {
                                put("title", JsonPrimitive(parent.title))
                                put(
                                    "sub_sections",
                                    buildJsonArray {
                                        parent.subSections.forEach { subSection ->
                                            add(
                                                buildJsonObject {
                                                    put("title", JsonPrimitive(subSection.title))
                                                    put(
                                                        "entries",
                                                        buildJsonArray {
                                                            subSection.entries.forEach { entry ->
                                                                add(
                                                                    buildJsonObject {
                                                                        put(
                                                                            "amount_expression",
                                                                            JsonPrimitive(entry.amountExpression),
                                                                        )
                                                                        put(
                                                                            "description",
                                                                            JsonPrimitive(entry.description),
                                                                        )
                                                                        put(
                                                                            "comment",
                                                                            JsonPrimitive(entry.comment),
                                                                        )
                                                                    },
                                                                )
                                                            }
                                                        },
                                                    )
                                                },
                                            )
                                        }
                                    },
                                )
                            },
                        )
                    }
                },
            )
        }
}
