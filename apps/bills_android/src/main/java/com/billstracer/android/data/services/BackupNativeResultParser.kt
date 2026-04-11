package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.int
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.model.ExportedBackupBundleResult
import com.billstracer.android.model.ImportedBackupBundleResult
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

internal object BackupNativeResultParser {
    fun parseExportedBackupBundleResult(
        rawJson: String,
        destinationDisplayPath: String,
    ): ExportedBackupBundleResult {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        return ExportedBackupBundleResult(
            exportedRecordFiles = data.int("exported_record_files"),
            exportedConfigFiles = data.int("exported_config_files"),
            destinationDisplayPath = destinationDisplayPath,
            rawJson = rawJson,
        )
    }

    fun parseImportedBackupBundleResult(
        rawJson: String,
        sourceDisplayPath: String,
    ): ImportedBackupBundleResult {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        return ImportedBackupBundleResult(
            ok = root.boolean("ok"),
            code = root.string("code"),
            message = root.string("message"),
            restoredRecordFiles = data.int("restored_record_files"),
            restoredConfigFiles = data.int("restored_config_files"),
            restoredBills = data.int("restored_bills"),
            failedPhase = data["failed_phase"]?.jsonPrimitive?.contentOrNull,
            firstErrorMessage = parseFirstErrorMessage(data),
            sourceDisplayPath = sourceDisplayPath,
            rawJson = rawJson,
        )
    }

    private fun parseFirstErrorMessage(data: JsonObject): String? {
        // Prefer the earliest failing stage so the UI shows the first actionable
        // restore error instead of a later downstream symptom.
        val configValidation = data["config_validation"]?.jsonObject
        val configIssueMessage = firstConfigIssueMessage(configValidation)
        if (!configIssueMessage.isNullOrBlank()) {
            return configIssueMessage
        }

        val recordValidation = data["record_validation"]?.jsonObject
        val recordErrorMessage = firstRecordErrorMessage(recordValidation)
        if (!recordErrorMessage.isNullOrBlank()) {
            return recordErrorMessage
        }

        val dbIngest = data["db_ingest"]?.jsonObject
        return firstRecordErrorMessage(dbIngest)
    }

    private fun firstConfigIssueMessage(data: JsonObject?): String? =
        data?.objectList("files")
            ?.firstOrNull { !it.boolean("ok") }
            ?.objectList("issues")
            ?.firstOrNull()
            ?.string("message")
            ?.takeIf { it.isNotBlank() }

    private fun firstRecordErrorMessage(data: JsonObject?): String? {
        val failedFile = data?.objectList("files")
            ?.firstOrNull { !it.boolean("ok") }
            ?: return null
        return failedFile.string("error").takeIf { it.isNotBlank() }
            ?: failedFile.objectList("issues")
                .firstOrNull()
                ?.string("message")
                ?.takeIf { it.isNotBlank() }
    }

    private fun JsonObject.objectList(key: String): List<JsonObject> =
        (this[key] as? JsonArray)?.mapNotNull { element ->
            runCatching { element.jsonObject }.getOrNull()
        }.orEmpty()
}
