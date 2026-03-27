package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.int
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.model.ExportedParseBundleResult
import com.billstracer.android.model.ImportedParseBundleResult
import com.billstracer.android.model.ImportResult
import com.billstracer.android.model.RecordDirectoryImportResult
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

internal object WorkspaceNativeResultParser {
    fun parseImportResult(rawJson: String): ImportResult {
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

    fun parseRecordDirectoryImportResult(rawJson: String): RecordDirectoryImportResult {
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

    fun parseExportParseBundleResult(
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

    fun parseImportedParseBundleResult(
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
}
