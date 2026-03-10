package com.billstracer.android.ui

import com.billstracer.android.data.BillsRepository
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ImportResult
import com.billstracer.android.model.InvalidRecordFile
import com.billstracer.android.model.ListedRecordPeriodsResult
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewFile
import com.billstracer.android.model.RecordPreviewResult
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePalette
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
        assertEquals("2025-01", viewModel.uiState.value.bundledSampleLabel)
        assertEquals("2025", viewModel.uiState.value.bundledSampleYear)
        assertEquals("2025-01", viewModel.uiState.value.bundledSampleMonth)
        assertEquals(3, viewModel.uiState.value.bundledConfigs.size)
        assertEquals(
            "export_formats.toml",
            viewModel.uiState.value.bundledConfigs.last().fileName,
        )
        assertEquals(ThemeMode.SYSTEM, viewModel.uiState.value.themePreferences.mode)
        assertEquals(ThemePalette.EMBER, viewModel.uiState.value.themePreferences.palette)
        assertEquals(true, viewModel.uiState.value.bundledNotices?.markdownText?.contains("Open Source Notices"))
        assertEquals("0.4.2", viewModel.uiState.value.coreVersion?.versionName)
        assertEquals("0.1.0", viewModel.uiState.value.androidVersion?.versionName)
        assertTrue(viewModel.uiState.value.recordPeriodInput.matches(Regex("""\d{4}-\d{2}""")))
        assertNotNull(viewModel.uiState.value.statusMessage)
    }

    @Test
    fun importBundledSampleStoresResult() = runTest {
        val viewModel = BillsViewModel(FakeBillsRepository())
        advanceUntilIdle()

        viewModel.importBundledSample()
        advanceUntilIdle()

        assertEquals(1, viewModel.uiState.value.importResult?.imported)
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
        val viewModel = BillsViewModel(FakeBillsRepository())
        advanceUntilIdle()

        viewModel.runBundledMonthQuery()
        advanceUntilIdle()

        assertEquals(QueryType.MONTH, viewModel.uiState.value.queryResult?.type)
    }

    @Test
    fun openRecordPeriodLoadsGeneratedTemplate() = runTest {
        val viewModel = BillsViewModel(FakeBillsRepository())
        advanceUntilIdle()

        viewModel.updateRecordPeriodInput("2026-03")
        viewModel.openRecordPeriod()
        advanceUntilIdle()

        assertEquals("2026-03", viewModel.uiState.value.activeRecordDocument?.period)
        assertEquals(false, viewModel.uiState.value.activeRecordDocument?.persisted)
        assertTrue(viewModel.uiState.value.recordDraftText.contains("date:2026-03"))
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
        viewModel.updateThemePaletteDraft(ThemePalette.HARBOR)

        assertEquals(ThemeMode.SYSTEM, repository.savedTheme.mode)
        assertEquals(ThemePalette.EMBER, repository.savedTheme.palette)
        assertEquals(ThemeMode.DARK, viewModel.uiState.value.themeDraft.mode)
        assertEquals(ThemePalette.HARBOR, viewModel.uiState.value.themeDraft.palette)
    }

    @Test
    fun applyThemeDraftPersistsThemeSelection() = runTest {
        val repository = FakeBillsRepository()
        val viewModel = BillsViewModel(repository)
        advanceUntilIdle()

        viewModel.updateThemeModeDraft(ThemeMode.DARK)
        viewModel.updateThemePaletteDraft(ThemePalette.HARBOR)
        viewModel.applyThemeDraft()
        advanceUntilIdle()

        assertEquals("Applied theme Harbor.", viewModel.uiState.value.statusMessage)
        assertEquals(ThemeMode.DARK, repository.savedTheme.mode)
        assertEquals(ThemePalette.HARBOR, repository.savedTheme.palette)
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

    override suspend fun initialize(): AppEnvironment = AppEnvironment(
        bundledSampleFile = File("samples/2025-01.txt"),
        bundledSampleLabel = "2025-01",
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
            versionName = "0.1.0",
            versionCode = 1,
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
        message = "2025-01",
        processed = 1,
        success = 1,
        failure = 0,
        imported = 1,
        rawJson = """{"ok":true}""",
    )

    override suspend fun queryBundledYear(): QueryResult = QueryResult(
        ok = true,
        message = "2025",
        type = QueryType.YEAR,
        year = 2025,
        month = null,
        matchedBills = 1,
        totalIncome = 10.0,
        totalExpense = -5.0,
        balance = 5.0,
        monthlySummary = emptyList(),
        standardReportMarkdown = "# 2025",
        standardReportJson = "{}",
        rawJson = """{"ok":true}""",
    )

    override suspend fun queryBundledMonth(): QueryResult = QueryResult(
        ok = true,
        message = "2025-01",
        type = QueryType.MONTH,
        year = 2025,
        month = 1,
        matchedBills = 1,
        totalIncome = 10.0,
        totalExpense = -5.0,
        balance = 5.0,
        monthlySummary = emptyList(),
        standardReportMarkdown = "# 2025-01",
        standardReportJson = "{}",
        rawJson = """{"ok":true}""",
    )

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
}
