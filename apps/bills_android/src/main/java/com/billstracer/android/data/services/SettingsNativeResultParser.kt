package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.int
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.model.ConfigFileValidationResult
import com.billstracer.android.model.ConfigTextsValidationResult
import com.billstracer.android.model.ConfigValidationIssue
import com.billstracer.android.model.ConfigValidationReport
import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

internal object SettingsNativeResultParser {
    fun parseConfigTextsValidationResult(rawJson: String): ConfigTextsValidationResult {
        val root = parseRoot(rawJson)
        if (!root.boolean("ok") && root.string("code") != "business.validation_failed") {
            error(root.string("message").ifBlank { "Failed to validate config texts." })
        }
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        return ConfigTextsValidationResult(
            ok = root.boolean("ok"),
            code = root.string("code"),
            message = root.string("message"),
            configValidation = parseConfigValidationReport(
                data["config_validation"]?.jsonObject ?: JsonObject(emptyMap()),
            ),
            enabledExportFormats = data.stringList("enabled_export_formats"),
            availableExportFormats = data.stringList("available_export_formats"),
            rawJson = rawJson,
        )
    }

    private fun parseConfigValidationReport(data: JsonObject): ConfigValidationReport =
        ConfigValidationReport(
            processed = data.int("processed"),
            success = data.int("success"),
            failure = data.int("failure"),
            allValid = data.boolean("all_valid"),
            files = data.objectList("files").map(::parseConfigFileValidationResult),
            enabledExportFormats = data.stringList("enabled_export_formats"),
            availableExportFormats = data.stringList("available_export_formats"),
        )

    private fun parseConfigFileValidationResult(data: JsonObject): ConfigFileValidationResult =
        ConfigFileValidationResult(
            sourceKind = data.string("source_kind"),
            fileName = data.string("file_name"),
            path = data.string("path"),
            ok = data.boolean("ok"),
            issues = data.objectList("issues").map(::parseConfigValidationIssue),
        )

    private fun parseConfigValidationIssue(data: JsonObject): ConfigValidationIssue =
        ConfigValidationIssue(
            sourceKind = data.string("source_kind"),
            stage = data.string("stage"),
            code = data.string("code"),
            message = data.string("message"),
            path = data.string("path"),
            line = data.int("line"),
            column = data.int("column"),
            fieldPath = data.string("field_path"),
            severity = data.string("severity"),
        )

    private fun JsonObject.objectList(key: String): List<JsonObject> =
        (this[key] as? JsonArray)?.mapNotNull { element ->
            runCatching { element.jsonObject }.getOrNull()
        }.orEmpty()

    private fun JsonObject.stringList(key: String): List<String> =
        (this[key] as? JsonArray)?.mapNotNull { element ->
            element.jsonPrimitive.contentOrNull
        }.orEmpty()
}
