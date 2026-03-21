package com.billstracer.android

import android.net.Uri
import com.billstracer.android.data.services.EditorService
import com.billstracer.android.data.services.QueryService
import com.billstracer.android.data.services.SettingsService
import com.billstracer.android.data.services.WorkspaceService
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ExportedRecordFilesResult
import com.billstracer.android.model.ImportResult
import com.billstracer.android.model.InvalidRecordFile
import com.billstracer.android.model.ListedRecordPeriodsResult
import com.billstracer.android.model.MonthlySummaryItem
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewFile
import com.billstracer.android.model.RecordPreviewResult
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
    var exportedResult = ExportedRecordFilesResult(
        exportedRecordFiles = 1,
        exportedConfigFiles = 3,
        destinationDisplayPath = "dest/folder",
    )
    var clearedRecordFiles = 1
    var clearedDatabase = true

    override suspend fun initializeEnvironment(): AppEnvironment = environment

    override suspend fun importBundledSample(): ImportResult = importResult

    override suspend fun exportWorkspaceFiles(targetDirectoryUri: Uri): ExportedRecordFilesResult =
        exportedResult

    override suspend fun clearRecordFiles(): Int = clearedRecordFiles

    override suspend fun clearDatabase(): Boolean = clearedDatabase
}

internal class FakeQueryService : QueryService {
    var lastQueriedYear: String? = null
    var lastQueriedMonth: String? = null

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

    override suspend fun openRecordPeriod(period: String): RecordEditorDocument {
        val persistedText = savedRecords[period]
        val year = period.substringBefore('-')
        return RecordEditorDocument(
            period = period,
            relativePath = "$year/$period.txt",
            rawText = persistedText ?: "date:$period\nremark:\n\nmeal\nmeal_low\n",
            persisted = persistedText != null,
        )
    }

    override suspend fun saveRecordDocument(period: String, rawText: String): RecordEditorDocument {
        savedRecords[period] = rawText
        val year = period.substringBefore('-')
        return RecordEditorDocument(
            period = period,
            relativePath = "$year/$period.txt",
            rawText = rawText,
            persisted = true,
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

    override suspend fun listRecordPeriods(): ListedRecordPeriodsResult =
        ListedRecordPeriodsResult(
            ok = true,
            code = "ok",
            message = if (savedRecords.isEmpty()) "No record files found yet." else "Record periods listed successfully.",
            processed = savedRecords.size,
            valid = savedRecords.size,
            invalid = 0,
            periods = savedRecords.keys.sorted(),
            invalidFiles = emptyList(),
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
            versionName = "0.1.1",
            versionCode = 2,
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

internal val emptyListedPeriods = ListedRecordPeriodsResult(
    ok = true,
    code = "ok",
    message = "No record files found yet.",
    processed = 0,
    valid = 0,
    invalid = 0,
    periods = emptyList(),
    invalidFiles = emptyList<InvalidRecordFile>(),
    rawJson = """{"ok":true}""",
)
