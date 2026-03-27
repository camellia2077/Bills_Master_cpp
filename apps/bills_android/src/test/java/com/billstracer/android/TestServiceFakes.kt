package com.billstracer.android

import android.net.Uri
import com.billstracer.android.data.services.EditorService
import com.billstracer.android.data.services.QueryService
import com.billstracer.android.data.services.SettingsService
import com.billstracer.android.data.services.WorkspaceService
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ExportedParseBundleResult
import com.billstracer.android.model.ImportedParseBundleResult
import com.billstracer.android.model.ImportResult
import com.billstracer.android.model.MonthlySummaryItem
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import com.billstracer.android.model.RecordDirectoryImportResult
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewFile
import com.billstracer.android.model.RecordPreviewResult
import com.billstracer.android.model.RecordSaveResult
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePreferences
import com.billstracer.android.model.VersionInfo
import java.io.File

internal fun fakeAppEnvironment(): AppEnvironment = AppEnvironment(
    bundledSampleInputPath = File("samples/2025"),
    bundledSampleLabel = "2025 full-year sample",
    bundledSampleYear = "2025",
    bundledSampleMonth = "2025-01",
    configRoot = File("config"),
    recordsRoot = File("records"),
    dbFile = File("db.sqlite3"),
)

internal class FakeWorkspaceService : WorkspaceService {
    var environment: AppEnvironment = fakeAppEnvironment()
    var importResult = ImportResult(
        ok = true,
        code = "ok",
        message = "2025 full-year sample",
        processed = 12,
        success = 12,
        failure = 0,
        imported = 12,
        rawJson = """{"ok":true}""",
    )
    var exportedResult = ExportedParseBundleResult(
        exportedRecordFiles = 1,
        exportedConfigFiles = 3,
        destinationDisplayPath = "parse_bundle.zip",
        rawJson = """{"ok":true}""",
    )
    var importedBundleResult = ImportedParseBundleResult(
        ok = true,
        code = "ok",
        message = "Parse bundle import finished.",
        importedRecordFiles = 1,
        importedConfigFiles = 3,
        importedBills = 1,
        sourceDisplayPath = "parse_bundle.zip",
        rawJson = """{"ok":true}""",
    )
    var recordsImportResult = ImportResult(
        ok = true,
        code = "ok",
        message = "Record import finished.",
        processed = 2,
        success = 2,
        failure = 0,
        imported = 2,
        rawJson = """{"ok":true}""",
    )
    var recordDirectoryImportResult = RecordDirectoryImportResult(
        processed = 2,
        imported = 2,
        overwritten = 0,
        failure = 0,
        invalid = 0,
        duplicatePeriodConflicts = 0,
    )
    var clearedRecordFiles = 1
    var clearedDatabase = true

    override suspend fun initializeEnvironment(): AppEnvironment = environment

    override suspend fun importTxtDirectoryAndSyncDatabase(sourceDirectoryUri: Uri): RecordDirectoryImportResult =
        recordDirectoryImportResult

    override suspend fun importBundledSample(): ImportResult = importResult

    override suspend fun exportParseBundle(targetDocumentUri: Uri): ExportedParseBundleResult =
        exportedResult

    override suspend fun importParseBundle(sourceDocumentUri: Uri): ImportedParseBundleResult =
        importedBundleResult

    override suspend fun clearRecordFiles(): Int = clearedRecordFiles

    override suspend fun clearDatabase(): Boolean = clearedDatabase
}

internal class FakeQueryService : QueryService {
    var availablePeriods: List<String> = listOf("2026-03", "2026-02", "2025-12")
    var lastQueriedYear: String? = null
    var lastQueriedMonth: String? = null

    override suspend fun listAvailablePeriods(): List<String> = availablePeriods

    override suspend fun queryYear(isoYear: String): QueryResult {
        lastQueriedYear = isoYear
        return QueryResult(
            ok = true,
            message = isoYear,
            type = QueryType.YEAR,
            year = isoYear.toIntOrNull(),
            month = null,
            matchedBills = 1,
            totalIncome = 10.0,
            totalExpense = -5.0,
            balance = 5.0,
            monthlySummary = listOf(MonthlySummaryItem(month = 1, income = 10.0, expense = -5.0, balance = 5.0)),
            standardReportMarkdown = "# $isoYear",
            standardReportJson = "{}",
            rawJson = """{"ok":true}""",
        )
    }

    override suspend fun queryMonth(isoMonth: String): QueryResult {
        lastQueriedMonth = isoMonth
        return QueryResult(
            ok = true,
            message = isoMonth,
            type = QueryType.MONTH,
            year = isoMonth.substringBefore('-').toIntOrNull(),
            month = isoMonth.substringAfter('-').toIntOrNull(),
            matchedBills = 1,
            totalIncome = 10.0,
            totalExpense = -5.0,
            balance = 5.0,
            monthlySummary = emptyList(),
            standardReportMarkdown = "# $isoMonth",
            standardReportJson = """{"meta":{"report_type":"monthly"}}""",
            rawJson = """{"ok":true}""",
        )
    }
}

internal class FakeEditorService : EditorService {
    val savedRecords = linkedMapOf<String, String>()
    val persistedPeriods = linkedSetOf("2026-03", "2026-02")
    val missingPersistedPeriods = linkedSetOf<String>()
    val commitFailures = linkedMapOf<String, String>()
    val committedPeriods = mutableListOf<String>()

    init {
        persistedPeriods.forEach { period ->
            savedRecords[period] = "date:$period\nremark:\n\nmeal\nmeal_low\n"
        }
    }

    override suspend fun listPersistedRecordPeriods(): List<String> =
        persistedPeriods.toList().sortedDescending()

    override suspend fun openPersistedRecordPeriod(period: String): RecordEditorDocument {
        if (!persistedPeriods.contains(period)) {
            error("No imported month exists for $period.")
        }
        if (missingPersistedPeriods.contains(period)) {
            error(
                "Database and TXT source are out of sync for $period. Missing " +
                    "${period.substringBefore('-')}/$period.txt. Re-import or resync records/.",
            )
        }
        val persistedText = savedRecords[period]
            ?: error(
                "Database and TXT source are out of sync for $period. Missing " +
                    "${period.substringBefore('-')}/$period.txt. Re-import or resync records/.",
            )
        val year = period.substringBefore('-')
        return RecordEditorDocument(
            period = period,
            relativePath = "$year/$period.txt",
            rawText = persistedText,
            persisted = true,
        )
    }

    override suspend fun commitRecordDocument(period: String, rawText: String): RecordSaveResult {
        committedPeriods += period
        val explicitFailure = commitFailures[period]
        if (explicitFailure != null) {
            return RecordSaveResult(
                ok = false,
                message = explicitFailure,
                errorMessage = explicitFailure,
                rawJson = """{"ok":false}""",
            )
        }
        if (!rawText.startsWith("date:$period")) {
            val message = "TXT header period does not match selected period '$period'."
            return RecordSaveResult(
                ok = false,
                message = message,
                errorMessage = message,
                rawJson = """{"ok":false}""",
            )
        }

        savedRecords[period] = rawText
        persistedPeriods += period
        val year = period.substringBefore('-')
        return RecordSaveResult(
            ok = true,
            message = "Saved $year/$period.txt and synced it to the database.",
            document = RecordEditorDocument(
                period = period,
                relativePath = "$year/$period.txt",
                rawText = rawText,
                persisted = true,
            ),
            rawJson = """{"ok":true}""",
        )
    }

    override suspend fun previewRecordDocument(period: String, rawText: String): RecordPreviewResult =
        RecordPreviewResult(
            ok = true,
            code = "ok",
            message = "Record preview completed successfully.",
            processed = 1,
            success = 1,
            failure = 0,
            periods = listOf(period),
            files = listOf(
                RecordPreviewFile(
                    path = "records/${period.substringBefore('-')}/$period.txt",
                    ok = true,
                    period = period,
                    year = period.substringBefore('-').toInt(),
                    month = period.substringAfter('-').toInt(),
                    transactionCount = if (rawText.isBlank()) 0 else 1,
                    totalIncome = 0.0,
                    totalExpense = -12.0,
                    balance = -12.0,
                ),
            ),
            rawJson = """{"ok":true}""",
        )
}

internal class FakeSettingsService : SettingsService {
    val savedConfigs = linkedMapOf(
        "validator_config.toml" to "[[categories]]\n",
        "modifier_config.toml" to "metadata_prefixes = [\"date:\"]\n",
        "export_formats.toml" to "enabled_formats = [\"json\", \"md\"]\n",
    )
    var savedTheme = ThemePreferences()

    override suspend fun loadBundledConfigs(): List<BundledConfigFile> =
        savedConfigs.map { (fileName, rawText) -> BundledConfigFile(fileName, rawText) }

    override suspend fun updateBundledConfig(
        fileName: String,
        rawText: String,
    ): List<BundledConfigFile> {
        savedConfigs[fileName] = rawText
        return loadBundledConfigs()
    }

    override suspend fun loadThemePreferences(): ThemePreferences = savedTheme

    override suspend fun updateThemePreferences(preferences: ThemePreferences): ThemePreferences {
        savedTheme = preferences
        return savedTheme
    }

    override suspend fun loadBundledNotices(): BundledNotices =
        BundledNotices(
            markdownText = "# Open Source Notices\n",
            rawJson = """{"schema_version":"1"}""",
        )

    override suspend fun loadVersionInfo(): Pair<VersionInfo, VersionInfo> =
        VersionInfo(
            versionName = "0.4.2",
            lastUpdated = "2026-03-10",
        ) to VersionInfo(
            versionName = "0.1.3",
            versionCode = 3,
        )
}

internal class FailingSettingsService : SettingsService by FakeSettingsService() {
    override suspend fun loadVersionInfo(): Pair<VersionInfo, VersionInfo> {
        error("version load failed")
    }
}

internal val emeraldDarkTheme = ThemePreferences(
    color = ThemeColor.EMERALD,
    mode = ThemeMode.DARK,
)
