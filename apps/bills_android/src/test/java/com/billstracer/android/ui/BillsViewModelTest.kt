package com.billstracer.android.ui

import android.net.Uri
import com.billstracer.android.data.BillsRepository
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ExportedRecordFilesResult
import com.billstracer.android.model.ImportResult
import com.billstracer.android.model.InvalidRecordFile
import com.billstracer.android.model.ListedRecordPeriodsResult
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewFile
import com.billstracer.android.model.RecordPreviewResult
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePreferences
import com.billstracer.android.model.VersionInfo
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.advanceUntilIdle
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import java.io.File
import java.time.YearMonth

@OptIn(ExperimentalCoroutinesApi::class)
class BillsViewModelTest {
    private val dispatcher = StandardTestDispatcher()

    @Before
    fun setUp() {
        Dispatchers.setMain(dispatcher)
    }

    @After
    fun tearDown() {
        Dispatchers.resetMain()
    }

    @Test
    fun initializeLoadsBundledSample() = runTest {
        val viewModel = BillsViewModel(FakeBillsRepository())

        advanceUntilIdle()

        assertEquals(false, viewModel.uiState.value.isInitializing)
        assertEquals("2025 full-year sample", viewModel.uiState.value.bundledSampleLabel)
        assertEquals("2025", viewModel.uiState.value.bundledSampleYear)
        assertEquals("2025-01", viewModel.uiState.value.bundledSampleMonth)
        assertEquals(3, viewModel.uiState.value.bundledConfigs.size)
        assertEquals(
            "export_formats.toml",
            viewModel.uiState.value.bundledConfigs.last().fileName,
        )
        assertEquals(ThemeMode.SYSTEM, viewModel.uiState.value.themePreferences.mode)
        assertEquals(ThemeColor.SLATE, viewModel.uiState.value.themePreferences.color)
        assertEquals(true, viewModel.uiState.value.bundledNotices?.markdownText?.contains("Open Source Notices"))
        assertEquals("0.4.2", viewModel.uiState.value.coreVersion?.versionName)
        assertEquals("0.1.1", viewModel.uiState.value.androidVersion?.versionName)
        assertEquals(YearMonth.now().year.toString(), viewModel.uiState.value.recordPeriodYearInput)
        assertEquals("%02d".format(YearMonth.now().monthValue), viewModel.uiState.value.recordPeriodMonthInput)
        assertTrue(viewModel.uiState.value.recordPeriodInput.matches(Regex("""\d{4}-\d{2}""")))
        assertEquals("2025", viewModel.uiState.value.queryYearInput)
        assertEquals("2025", viewModel.uiState.value.queryPeriodYearInput)
        assertEquals("01", viewModel.uiState.value.queryPeriodMonthInput)
        assertNotNull(viewModel.uiState.value.statusMessage)
    }

    @Test
    fun importBundledSampleStoresResult() = runTest {
        val viewModel = BillsViewModel(FakeBillsRepository())
        advanceUntilIdle()

        viewModel.importBundledSample()
        advanceUntilIdle()

        assertEquals(12, viewModel.uiState.value.importResult?.imported)
    }

    @Test
    fun importFailureShowsDetailedMessage() = runTest {
        val viewModel = BillsViewModel(FailingBillsRepository())
        advanceUntilIdle()

        viewModel.importBundledSample()
        advanceUntilIdle()

        assertEquals(
            "insert_repository: UNIQUE constraint failed: bills.bill_date",
            viewModel.uiState.value.errorMessage,
        )
    }

    @Test
    fun runBundledMonthQueryStoresResult() = runTest {
        val repository = FakeBillsRepository()
        val viewModel = BillsViewModel(repository)
        advanceUntilIdle()

        viewModel.updateQueryPeriodYearInput("2025")
        viewModel.updateQueryPeriodMonthInput("03")
        viewModel.runBundledMonthQuery()
        advanceUntilIdle()

        assertEquals(QueryType.MONTH, viewModel.uiState.value.queryResult?.type)
        assertEquals("2025-03", repository.lastQueriedMonth)
    }

    @Test
    fun runBundledYearQueryUsesEditableInput() = runTest {
        val repository = FakeBillsRepository()
        val viewModel = BillsViewModel(repository)
        advanceUntilIdle()

        viewModel.updateQueryYearInput("2024")
        viewModel.runBundledYearQuery()
        advanceUntilIdle()

        assertEquals(QueryType.YEAR, viewModel.uiState.value.queryResult?.type)
        assertEquals("2024", repository.lastQueriedYear)
    }

    @Test
    fun invalidQueryYearDoesNotRun() = runTest {
        val repository = FakeBillsRepository()
        val viewModel = BillsViewModel(repository)
        advanceUntilIdle()

        viewModel.updateQueryYearInput("24")
        viewModel.runBundledYearQuery()
        advanceUntilIdle()

        assertEquals(null, repository.lastQueriedYear)
        assertEquals("Year query must use 4 digits.", viewModel.uiState.value.errorMessage)
    }

    @Test
    fun openRecordPeriodLoadsGeneratedTemplate() = runTest {
        val viewModel = BillsViewModel(FakeBillsRepository())
        advanceUntilIdle()

        viewModel.updateRecordPeriodYearInput("2026")
        viewModel.updateRecordPeriodMonthInput("03")
        viewModel.openRecordPeriod()
        advanceUntilIdle()

        assertEquals("2026-03", viewModel.uiState.value.activeRecordDocument?.period)
        assertEquals(false, viewModel.uiState.value.activeRecordDocument?.persisted)
        assertTrue(viewModel.uiState.value.recordDraftText.contains("date:2026-03"))
    }

    @Test
    fun invalidManualRecordMonthDoesNotOpenRecord() = runTest {
        val viewModel = BillsViewModel(FakeBillsRepository())
        advanceUntilIdle()

        viewModel.updateRecordPeriodYearInput("2026")
        viewModel.updateRecordPeriodMonthInput("13")
        viewModel.openRecordPeriod()
        advanceUntilIdle()

        assertEquals(null, viewModel.uiState.value.activeRecordDocument)
        assertEquals(
            "Manual period must use YYYY-MM, and month must be between 01 and 12.",
            viewModel.uiState.value.errorMessage,
        )
    }

    @Test
    fun initializeDerivesExistingYearMonthSelectorsFromListedPeriods() = runTest {
        val repository = FakeBillsRepository().apply {
            savedRecords["2025-01"] = "date:2025-01\nremark:\n"
            savedRecords["2025-03"] = "date:2025-03\nremark:\n"
            savedRecords["2026-02"] = "date:2026-02\nremark:\n"
        }
        val viewModel = BillsViewModel(repository)
        advanceUntilIdle()

        val expectedYear = if (repository.savedRecords.keys.any {
                it.startsWith("${YearMonth.now().year}-")
            }) {
            YearMonth.now().year.toString()
        } else {
            "2025"
        }
        val expectedMonth = repository.savedRecords.keys
            .filter { it.startsWith("$expectedYear-") }
            .map { it.substringAfter('-') }
            .sorted()
            .first()

        assertEquals(expectedYear, viewModel.uiState.value.selectedExistingRecordYear)
        assertEquals(expectedMonth, viewModel.uiState.value.selectedExistingRecordMonth)

        viewModel.selectExistingRecordYear("2026")
        assertEquals("02", viewModel.uiState.value.selectedExistingRecordMonth)
    }

    @Test
    fun saveRecordDraftPersistsTxtSource() = runTest {
        val repository = FakeBillsRepository()
        val viewModel = BillsViewModel(repository)
        advanceUntilIdle()

        viewModel.updateRecordPeriodInput("2026-03")
        viewModel.openRecordPeriod()
        advanceUntilIdle()
        viewModel.updateRecordDraft("date:2026-03\nremark:test\n\nmeal\nmeal_low 12 lunch\n")
        viewModel.saveRecordDraft()
        advanceUntilIdle()

        assertEquals("Saved 2026/2026-03.txt.", viewModel.uiState.value.statusMessage)
        assertEquals(
            "date:2026-03\nremark:test\n\nmeal\nmeal_low 12 lunch\n",
            repository.savedRecords.getValue("2026-03"),
        )
        assertEquals(true, viewModel.uiState.value.activeRecordDocument?.persisted)
    }

    @Test
    fun previewRecordDraftStoresDryRunResult() = runTest {
        val viewModel = BillsViewModel(FakeBillsRepository())
        advanceUntilIdle()

        viewModel.updateRecordPeriodInput("2026-03")
        viewModel.openRecordPeriod()
        advanceUntilIdle()
        viewModel.previewRecordDraft()
        advanceUntilIdle()

        assertEquals(1, viewModel.uiState.value.recordPreviewResult?.processed)
        assertEquals(1, viewModel.uiState.value.recordPreviewResult?.success)
    }

    @Test
    fun clearDatabaseResetsVisibleResults() = runTest {
        val viewModel = BillsViewModel(FakeBillsRepository())
        advanceUntilIdle()
        viewModel.importBundledSample()
        advanceUntilIdle()

        viewModel.clearDatabase()
        advanceUntilIdle()

        assertEquals(null, viewModel.uiState.value.importResult)
        assertEquals("Database file cleared.", viewModel.uiState.value.statusMessage)
    }

    @Test
    fun clearRecordFilesRemovesSavedTxtAndResetsRecordUiState() = runTest {
        val repository = FakeBillsRepository().apply {
            savedRecords["2026-03"] = "date:2026-03\nremark:\n"
        }
        val viewModel = BillsViewModel(repository)
        advanceUntilIdle()

        viewModel.updateRecordPeriodInput("2026-03")
        viewModel.openRecordPeriod()
        advanceUntilIdle()
        viewModel.clearRecordFiles()
        advanceUntilIdle()

        assertEquals(0, repository.savedRecords.size)
        assertEquals(null, viewModel.uiState.value.activeRecordDocument)
        assertEquals("", viewModel.uiState.value.recordDraftText)
        assertEquals("Cleared 1 TXT record file(s).", viewModel.uiState.value.statusMessage)
        assertEquals(emptyList<String>(), viewModel.uiState.value.listedRecordPeriods?.periods)
    }

    @Test
    fun saveSelectedConfigPersistsEditedToml() = runTest {
        val repository = FakeBillsRepository()
        val viewModel = BillsViewModel(repository)
        advanceUntilIdle()

        viewModel.selectBundledConfig("export_formats.toml")
        viewModel.updateConfigDraft("enabled_formats = [\"json\"]\n")
        viewModel.saveSelectedConfig()
        advanceUntilIdle()

        assertEquals("Modified and persisted export_formats.toml.", viewModel.uiState.value.statusMessage)
        assertEquals(
            "enabled_formats = [\"json\"]\n",
            viewModel.uiState.value.bundledConfigs.last().rawText,
        )
        assertEquals(
            "enabled_formats = [\"json\"]\n",
            repository.savedConfigs.getValue("export_formats.toml"),
        )
    }

    @Test
    fun editingDraftDoesNotPersistBeforeModify() = runTest {
        val repository = FakeBillsRepository()
        val viewModel = BillsViewModel(repository)
        advanceUntilIdle()

        viewModel.selectBundledConfig("export_formats.toml")
        viewModel.updateConfigDraft("enabled_formats = [\"json\"]\n")

        assertEquals(
            "enabled_formats = [\"json\", \"md\"]\n",
            repository.savedConfigs.getValue("export_formats.toml"),
        )
        assertEquals(
            "enabled_formats = [\"json\"]\n",
            viewModel.uiState.value.configDrafts.getValue("export_formats.toml"),
        )
    }

    @Test
    fun resetSelectedConfigDraftRestoresPersistedToml() = runTest {
        val viewModel = BillsViewModel(FakeBillsRepository())
        advanceUntilIdle()

        viewModel.selectBundledConfig("export_formats.toml")
        viewModel.updateConfigDraft("enabled_formats = [\"json\"]\n")
        viewModel.resetSelectedConfigDraft()

        assertEquals(
            "enabled_formats = [\"json\", \"md\"]\n",
            viewModel.uiState.value.configDrafts.getValue("export_formats.toml"),
        )
    }

    @Test
    fun editingThemeDraftDoesNotPersistBeforeApply() = runTest {
        val repository = FakeBillsRepository()
        val viewModel = BillsViewModel(repository)
        advanceUntilIdle()

        viewModel.updateThemeModeDraft(ThemeMode.DARK)
        viewModel.updateThemeColorDraft(ThemeColor.EMERALD)

        assertEquals(ThemeMode.SYSTEM, repository.savedTheme.mode)
        assertEquals(ThemeColor.SLATE, repository.savedTheme.color)
        assertEquals(ThemeMode.DARK, viewModel.uiState.value.themeDraft.mode)
        assertEquals(ThemeColor.EMERALD, viewModel.uiState.value.themeDraft.color)
    }

    @Test
    fun applyThemeDraftPersistsThemeSelection() = runTest {
        val repository = FakeBillsRepository()
        val viewModel = BillsViewModel(repository)
        advanceUntilIdle()

        viewModel.updateThemeModeDraft(ThemeMode.DARK)
        viewModel.updateThemeColorDraft(ThemeColor.EMERALD)
        viewModel.applyThemeDraft()
        advanceUntilIdle()

        assertEquals("Applied Emerald in Dark mode.", viewModel.uiState.value.statusMessage)
        assertEquals(ThemeMode.DARK, repository.savedTheme.mode)
        assertEquals(ThemeColor.EMERALD, repository.savedTheme.color)
        assertEquals(repository.savedTheme, viewModel.uiState.value.themePreferences)
        assertEquals(repository.savedTheme, viewModel.uiState.value.themeDraft)
    }
}

private class FakeBillsRepository : BillsRepository {
    val savedConfigs = linkedMapOf(
        "validator_config.toml" to "[[categories]]\n",
        "modifier_config.toml" to "metadata_prefixes = [\"date:\"]\n",
        "export_formats.toml" to "enabled_formats = [\"json\", \"md\"]\n",
    )
    val savedRecords = linkedMapOf<String, String>()
    var savedTheme = ThemePreferences()
    var lastQueriedYear: String? = null
    var lastQueriedMonth: String? = null

    override suspend fun initialize(): AppEnvironment = AppEnvironment(
        bundledSampleInputPath = File("samples/2025"),
        bundledSampleLabel = "2025 full-year sample",
        bundledSampleYear = "2025",
        bundledSampleMonth = "2025-01",
        configRoot = File("config"),
        recordsRoot = File("records"),
        dbFile = File("db.sqlite3"),
        coreVersion = VersionInfo(
            versionName = "0.4.2",
            lastUpdated = "2026-03-10",
        ),
        androidVersion = VersionInfo(
            versionName = "0.1.1",
            versionCode = 2,
        ),
        bundledConfigs = savedConfigs.map { (fileName, rawText) ->
            BundledConfigFile(fileName, rawText)
        },
        themePreferences = savedTheme,
        bundledNotices = BundledNotices(
            markdownText = "# Open Source Notices\n",
            rawJson = """{"schema_version":"1"}""",
        ),
    )

    override suspend fun updateBundledConfig(
        fileName: String,
        rawText: String,
    ): List<BundledConfigFile> {
        savedConfigs[fileName] = rawText
        return savedConfigs.map { (name, text) -> BundledConfigFile(name, text) }
    }

    override suspend fun updateThemePreferences(preferences: ThemePreferences): ThemePreferences {
        savedTheme = preferences
        return savedTheme
    }

    override suspend fun importBundledSample(): ImportResult = ImportResult(
        ok = true,
        code = "ok",
        message = "2025 full-year sample",
        processed = 12,
        success = 12,
        failure = 0,
        imported = 12,
        rawJson = """{"ok":true}""",
    )

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
        monthlySummary = emptyList(),
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
        standardReportJson = "{}",
        rawJson = """{"ok":true}""",
    )
    }

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

    override suspend fun saveRecordDocument(
        period: String,
        rawText: String,
    ): RecordEditorDocument {
        savedRecords[period] = rawText
        val year = period.substringBefore('-')
        return RecordEditorDocument(
            period = period,
            relativePath = "$year/$period.txt",
            rawText = rawText,
            persisted = true,
        )
    }

    override suspend fun previewRecordDocument(
        period: String,
        rawText: String,
    ): RecordPreviewResult = RecordPreviewResult(
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

    override suspend fun listRecordPeriods(): ListedRecordPeriodsResult = ListedRecordPeriodsResult(
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

    override suspend fun clearRecordFiles(): Int {
        val removed = savedRecords.size
        savedRecords.clear()
        return removed
    }

    override suspend fun exportRecordFiles(targetDirectoryUri: Uri): ExportedRecordFilesResult =
        ExportedRecordFilesResult(
            exportedRecordFiles = savedRecords.size,
            exportedConfigFiles = savedConfigs.size,
            destinationDisplayPath = targetDirectoryUri.toString(),
        )

    override suspend fun clearDatabase(): Boolean = true
}

private class FailingBillsRepository : BillsRepository by FakeBillsRepository() {
    override suspend fun updateBundledConfig(
        fileName: String,
        rawText: String,
    ): List<BundledConfigFile> {
        error("save not expected")
    }

    override suspend fun updateThemePreferences(preferences: ThemePreferences): ThemePreferences {
        error("theme update not expected")
    }

    override suspend fun importBundledSample(): ImportResult = ImportResult(
        ok = false,
        code = "business.import_failed",
        message = "insert_repository: UNIQUE constraint failed: bills.bill_date",
        processed = 1,
        success = 0,
        failure = 1,
        imported = 0,
        rawJson = """{"ok":false}""",
    )

    override suspend fun queryYear(isoYear: String): QueryResult {
        error("year query not expected")
    }

    override suspend fun queryMonth(isoMonth: String): QueryResult {
        error("month query not expected")
    }

    override suspend fun openRecordPeriod(period: String): RecordEditorDocument {
        error("record open not expected")
    }

    override suspend fun saveRecordDocument(
        period: String,
        rawText: String,
    ): RecordEditorDocument {
        error("record save not expected")
    }

    override suspend fun previewRecordDocument(
        period: String,
        rawText: String,
    ): RecordPreviewResult {
        error("record preview not expected")
    }

    override suspend fun listRecordPeriods(): ListedRecordPeriodsResult = ListedRecordPeriodsResult(
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

    override suspend fun clearRecordFiles(): Int {
        error("clear record files not expected")
    }
}
