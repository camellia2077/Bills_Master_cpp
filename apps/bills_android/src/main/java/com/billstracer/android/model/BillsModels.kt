package com.billstracer.android.model

import java.io.File

data class BundledConfigFile(
    val fileName: String,
    val rawText: String,
)

data class BundledNotices(
    val markdownText: String,
    val rawJson: String,
)

data class VersionInfo(
    val versionName: String,
    val versionCode: Int? = null,
    val lastUpdated: String? = null,
)

data class RecordEditorDocument(
    val period: String,
    val relativePath: String,
    val rawText: String,
    val persisted: Boolean,
)

data class RecordPreviewFile(
    val path: String,
    val ok: Boolean,
    val period: String? = null,
    val year: Int? = null,
    val month: Int? = null,
    val transactionCount: Int = 0,
    val totalIncome: Double = 0.0,
    val totalExpense: Double = 0.0,
    val balance: Double = 0.0,
    val error: String? = null,
)

data class RecordPreviewResult(
    val ok: Boolean,
    val code: String,
    val message: String,
    val processed: Int,
    val success: Int,
    val failure: Int,
    val periods: List<String>,
    val files: List<RecordPreviewFile>,
    val rawJson: String,
)

data class InvalidRecordFile(
    val path: String,
    val error: String,
)

data class ListedRecordPeriodsResult(
    val ok: Boolean,
    val code: String,
    val message: String,
    val processed: Int,
    val valid: Int,
    val invalid: Int,
    val periods: List<String>,
    val invalidFiles: List<InvalidRecordFile>,
    val rawJson: String,
)

enum class ThemeMode(
    val displayName: String,
) {
    SYSTEM("Follow system"),
    LIGHT("Light"),
    DARK("Dark"),
}

enum class ThemePalette(
    val displayName: String,
) {
    EMBER("Ember"),
    HARBOR("Harbor"),
    GROVE("Grove"),
    CANYON("Canyon"),
}

data class ThemePreferences(
    val mode: ThemeMode = ThemeMode.SYSTEM,
    val palette: ThemePalette = ThemePalette.EMBER,
)

data class AppEnvironment(
    val bundledSampleFile: File,
    val bundledSampleLabel: String,
    val bundledSampleYear: String,
    val bundledSampleMonth: String,
    val configRoot: File,
    val recordsRoot: File,
    val dbFile: File,
    val coreVersion: VersionInfo,
    val androidVersion: VersionInfo,
    val bundledConfigs: List<BundledConfigFile>,
    val themePreferences: ThemePreferences,
    val bundledNotices: BundledNotices,
)

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

enum class QueryType {
    YEAR,
    MONTH,
}

data class MonthlySummaryItem(
    val month: Int,
    val income: Double,
    val expense: Double,
    val balance: Double,
)

data class QueryResult(
    val ok: Boolean,
    val message: String,
    val type: QueryType,
    val year: Int?,
    val month: Int?,
    val matchedBills: Int,
    val totalIncome: Double,
    val totalExpense: Double,
    val balance: Double,
    val monthlySummary: List<MonthlySummaryItem>,
    val standardReportMarkdown: String?,
    val standardReportJson: String?,
    val rawJson: String,
)

data class BillsUiState(
    val isInitializing: Boolean = true,
    val isWorking: Boolean = false,
    val statusMessage: String = "Preparing bundled samples and private workspace...",
    val bundledSampleLabel: String = "",
    val bundledSampleYear: String = "",
    val bundledSampleMonth: String = "",
    val coreVersion: VersionInfo? = null,
    val androidVersion: VersionInfo? = null,
    val bundledConfigs: List<BundledConfigFile> = emptyList(),
    val selectedConfigFileName: String = "",
    val configDrafts: Map<String, String> = emptyMap(),
    val selectedExistingRecordYear: String = "",
    val selectedExistingRecordMonth: String = "",
    val recordPeriodInput: String = "",
    val activeRecordDocument: RecordEditorDocument? = null,
    val recordDraftText: String = "",
    val listedRecordPeriods: ListedRecordPeriodsResult? = null,
    val recordPreviewResult: RecordPreviewResult? = null,
    val themePreferences: ThemePreferences = ThemePreferences(),
    val themeDraft: ThemePreferences = ThemePreferences(),
    val bundledNotices: BundledNotices? = null,
    val importResult: ImportResult? = null,
    val queryResult: QueryResult? = null,
    val errorMessage: String? = null,
)
