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

data class ExportedParseBundleResult(
    val exportedRecordFiles: Int,
    val exportedConfigFiles: Int,
    val destinationDisplayPath: String? = null,
    val rawJson: String,
) {
    val exported: Int
        get() = exportedRecordFiles + exportedConfigFiles
}

data class ImportedParseBundleResult(
    val ok: Boolean,
    val code: String,
    val message: String,
    val importedRecordFiles: Int,
    val importedConfigFiles: Int,
    val importedBills: Int = 0,
    val failedPhase: String? = null,
    val sourceDisplayPath: String? = null,
    val rawJson: String,
) {
    val imported: Int
        get() = importedRecordFiles + importedConfigFiles
}

data class RecordDirectoryImportResult(
    val processed: Int,
    val imported: Int,
    val overwritten: Int,
    val failure: Int,
    val invalid: Int,
    val duplicatePeriodConflicts: Int,
    val firstFailureMessage: String? = null,
)
