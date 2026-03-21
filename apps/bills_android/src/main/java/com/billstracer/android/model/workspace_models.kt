package com.billstracer.android.model

data class ImportResult(
    val ok: Boolean,
    val code: String,
    val message: String,
    val processed: Int,
    val success: Int,
    val failure: Int,
    val imported: Int,
    val rawJson: String,
)

data class ExportedRecordFilesResult(
    val exportedRecordFiles: Int,
    val exportedConfigFiles: Int,
    val destinationDisplayPath: String? = null,
) {
    val exported: Int
        get() = exportedRecordFiles + exportedConfigFiles
}
