package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.int
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.model.ExportedBackupBundleResult
import com.billstracer.android.model.ImportedBackupBundleResult
import kotlinx.serialization.json.JsonObject
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
            sourceDisplayPath = sourceDisplayPath,
            rawJson = rawJson,
        )
    }
}
