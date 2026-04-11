package com.billstracer.android

import android.net.Uri
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.app.navigation.WorkspaceDataChangeBus
import com.billstracer.android.features.workspace.WorkspaceViewModel
import com.billstracer.android.model.ImportedBackupBundleResult
import com.billstracer.android.model.RecordDirectoryImportResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.advanceUntilIdle
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.mockito.Mockito.mock

@OptIn(ExperimentalCoroutinesApi::class)
class WorkspaceViewModelTest {
    private val dispatcher = StandardTestDispatcher()
    private fun testUri(): Uri = mock(Uri::class.java)
    private fun createViewModel(
        workspaceService: FakeWorkspaceService = FakeWorkspaceService(),
        backupService: FakeBackupService = FakeBackupService(),
    ): WorkspaceViewModel =
        WorkspaceViewModel(workspaceService, backupService, AppSessionBus(), WorkspaceDataChangeBus())

    @Before
    fun setUp() {
        Dispatchers.setMain(dispatcher)
    }

    @After
    fun tearDown() {
        Dispatchers.resetMain()
    }

    @Test
    fun importTxtDirectoryAndSyncDatabaseStoresResult() = runTest {
        val viewModel = createViewModel()
        advanceUntilIdle()

        viewModel.importTxtDirectoryAndSyncDatabase(testUri())
        advanceUntilIdle()

        assertEquals(2, viewModel.state.value.recordDirectoryImportResult?.imported)
        assertEquals("Imported and synced 2 TXT record file(s).", viewModel.state.value.statusMessage)
    }

    @Test
    fun importTxtDirectoryAndSyncDatabasePublishesWorkspaceDataChange() = runTest {
        val workspaceDataChangeBus = WorkspaceDataChangeBus()
        val viewModel = WorkspaceViewModel(
            FakeWorkspaceService(),
            FakeBackupService(),
            AppSessionBus(),
            workspaceDataChangeBus,
        )
        advanceUntilIdle()

        viewModel.importTxtDirectoryAndSyncDatabase(testUri())
        advanceUntilIdle()

        assertEquals(1, workspaceDataChangeBus.version.value)
    }

    @Test
    fun importTxtDirectoryAndSyncDatabaseStoresPartialFailure() = runTest {
        val workspaceService = FakeWorkspaceService().apply {
            recordDirectoryImportResult = RecordDirectoryImportResult(
                processed = 3,
                imported = 1,
                overwritten = 1,
                failure = 2,
                invalid = 1,
                duplicatePeriodConflicts = 1,
                firstFailureMessage = "TXT validation failed for broken.txt.",
            )
        }
        val viewModel = createViewModel(workspaceService)
        advanceUntilIdle()

        viewModel.importTxtDirectoryAndSyncDatabase(testUri())
        advanceUntilIdle()

        assertEquals("TXT validation failed for broken.txt.", viewModel.state.value.errorMessage)
        assertEquals(
            "Imported and synced 1 TXT record file(s) with 2 failure(s). Overwrote 1 existing file(s).",
            viewModel.state.value.statusMessage,
        )
    }

    @Test
    fun clearRecordFilesUpdatesStatus() = runTest {
        val viewModel = createViewModel()
        advanceUntilIdle()

        viewModel.clearRecordFiles()
        advanceUntilIdle()

        assertEquals("Cleared 1 TXT record file(s).", viewModel.state.value.statusMessage)
    }

    @Test
    fun clearDatabaseResetsStatus() = runTest {
        val viewModel = createViewModel()
        advanceUntilIdle()

        viewModel.clearDatabase()
        advanceUntilIdle()

        assertEquals("Database file cleared.", viewModel.state.value.statusMessage)
    }

    @Test
    fun exportWorkspaceUpdatesStatus() = runTest {
        val viewModel = createViewModel()
        advanceUntilIdle()

        viewModel.exportWorkspace(testUri())
        advanceUntilIdle()

        assertEquals(
            "Exported a workspace bundle with 1 TXT record file(s) and 3 config file(s) to bills_backup.zip.",
            viewModel.state.value.statusMessage,
        )
    }

    @Test
    fun importParseBundleUpdatesStatus() = runTest {
        val viewModel = createViewModel()
        advanceUntilIdle()

        viewModel.importParseBundle(testUri())
        advanceUntilIdle()

        assertEquals(
            "Imported a parse bundle from parse_bundle.zip with 1 TXT record file(s), 3 TOML config file(s), and synced 1 bill file(s) into SQLite.",
            viewModel.state.value.statusMessage,
        )
    }

    @Test
    fun importParseBundleFailurePublishesError() = runTest {
        val workspaceService = FakeWorkspaceService().apply {
            importedBundleResult = importedBundleResult.copy(
                ok = false,
                code = "business.import_parse_bundle_failed",
                message = "TXT validation failed for parse bundle.",
                failedPhase = "validate_records",
            )
        }
        val viewModel = createViewModel(workspaceService)
        advanceUntilIdle()

        viewModel.importParseBundle(testUri())
        advanceUntilIdle()

        assertEquals("TXT validation failed for parse bundle.", viewModel.state.value.errorMessage)
        assertEquals("TXT validation failed for parse bundle.", viewModel.state.value.statusMessage)
    }

    @Test
    fun importBackupBundleUpdatesStatusAndPublishesWorkspaceDataChange() = runTest {
        val workspaceDataChangeBus = WorkspaceDataChangeBus()
        val backupService = FakeBackupService().apply {
            importedResult = importedResult.copy(
                restoredRecordFiles = 2,
                restoredConfigFiles = 3,
                sourceDisplayPath = "phone_backup.zip",
            )
        }
        val viewModel = WorkspaceViewModel(
            FakeWorkspaceService(),
            backupService,
            AppSessionBus(),
            workspaceDataChangeBus,
        )
        advanceUntilIdle()

        viewModel.importBackupBundle(testUri())
        advanceUntilIdle()

        assertEquals(
            "Restored 2 TXT record file(s) and 3 config file(s) from phone_backup.zip, and rebuilt SQLite.",
            viewModel.state.value.statusMessage,
        )
        assertEquals(1, workspaceDataChangeBus.version.value)
    }

    @Test
    fun importBackupBundleFailureStoresPhaseAndFirstError() = runTest {
        val backupService = FakeBackupService().apply {
            importedResult = ImportedBackupBundleResult(
                ok = false,
                code = "business.import_backup_failed",
                message = "Backup bundle restore failed.",
                restoredRecordFiles = 0,
                restoredConfigFiles = 0,
                restoredBills = 0,
                failedPhase = "validate_records",
                firstErrorMessage = "TXT validation failed for 2026-03.txt.",
                sourceDisplayPath = "phone_backup.zip",
                rawJson = """{"ok":false}""",
            )
        }
        val viewModel = createViewModel(backupService = backupService)
        advanceUntilIdle()

        viewModel.importBackupBundle(testUri())
        advanceUntilIdle()

        assertEquals("Backup bundle restore failed.", viewModel.state.value.errorMessage)
        assertEquals("validate_records", viewModel.state.value.backupRestoreFailedPhase)
        assertEquals(
            "TXT validation failed for 2026-03.txt.",
            viewModel.state.value.backupRestoreFirstErrorMessage,
        )
    }
}
