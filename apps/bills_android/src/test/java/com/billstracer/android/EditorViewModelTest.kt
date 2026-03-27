package com.billstracer.android

import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.app.navigation.WorkspaceDataChangeBus
import com.billstracer.android.features.editor.EditorViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.advanceUntilIdle
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test

@OptIn(ExperimentalCoroutinesApi::class)
class EditorViewModelTest {
    private val dispatcher = StandardTestDispatcher()
    private fun createViewModel(editorService: FakeEditorService = FakeEditorService()): EditorViewModel =
        EditorViewModel(editorService, AppSessionBus(), WorkspaceDataChangeBus())

    @Before
    fun setUp() {
        Dispatchers.setMain(dispatcher)
    }

    @After
    fun tearDown() {
        Dispatchers.resetMain()
    }

    @Test
    fun initializationSelectsLatestDatabaseMonth() = runTest {
        val editorService = FakeEditorService().apply {
            persistedPeriods.clear()
            persistedPeriods += listOf("2025-01", "2026-03", "2026-02")
        }
        val viewModel = createViewModel(editorService)

        advanceUntilIdle()

        assertEquals(false, viewModel.state.value.isInitializing)
        assertEquals("2026", viewModel.state.value.selectedExistingRecordYear)
        assertEquals("03", viewModel.state.value.selectedExistingRecordMonth)
        assertEquals(listOf("2026-03", "2026-02", "2025-01"), viewModel.state.value.persistedRecordPeriods)
    }

    @Test
    fun openingSelectedRecordLoadsPersistedTxt() = runTest {
        val editorService = FakeEditorService()
        val viewModel = createViewModel(editorService)

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()

        assertEquals("2026-03", viewModel.state.value.activeRecordDocument?.period)
        assertTrue(viewModel.state.value.activeRecordDocument?.persisted == true)
    }

    @Test
    fun selectingYearDefaultsMonthWithinThatYear() = runTest {
        val editorService = FakeEditorService().apply {
            persistedPeriods.clear()
            persistedPeriods += listOf("2026-03", "2026-02", "2025-12", "2025-01")
        }
        val viewModel = createViewModel(editorService)

        advanceUntilIdle()
        viewModel.selectExistingRecordYear("2025")

        assertEquals("2025", viewModel.state.value.selectedExistingRecordYear)
        assertEquals("12", viewModel.state.value.selectedExistingRecordMonth)
    }

    @Test
    fun saveRecordDraftPersistsTxtAndSyncsDatabase() = runTest {
        val editorService = FakeEditorService()
        val viewModel = createViewModel(editorService)

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()
        viewModel.updateRecordDraft("date:2026-03\nremark:test\n\nmeal\nmeal_low 12 lunch\n")
        viewModel.saveRecordDraft()
        advanceUntilIdle()

        assertEquals(
            "date:2026-03\nremark:test\n\nmeal\nmeal_low 12 lunch\n",
            editorService.savedRecords.getValue("2026-03"),
        )
        assertEquals(listOf("2026-03"), editorService.committedPeriods)
        assertTrue(viewModel.state.value.activeRecordDocument?.persisted == true)
        assertEquals(null, viewModel.state.value.errorMessage)
    }

    @Test
    fun saveRecordDraftKeepsPersistedTxtWhenCommitFails() = runTest {
        val editorService = FakeEditorService().apply {
            commitFailures["2026-03"] = "validation: The first line must be 'date:YYYY-MM'."
        }
        val viewModel = createViewModel(editorService)

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()
        val originalText = viewModel.state.value.activeRecordDocument?.rawText
        viewModel.updateRecordDraft("date:2026-03\nremark:broken\n\nmeal\nmeal_low 12 lunch\n")
        viewModel.saveRecordDraft()
        advanceUntilIdle()

        assertEquals(
            originalText,
            editorService.savedRecords.getValue("2026-03"),
        )
        assertEquals(
            "validation: The first line must be 'date:YYYY-MM'.",
            viewModel.state.value.errorMessage,
        )
        assertEquals(
            originalText,
            viewModel.state.value.activeRecordDocument?.rawText,
        )
        assertEquals(
            "date:2026-03\nremark:broken\n\nmeal\nmeal_low 12 lunch\n",
            viewModel.state.value.recordDraftText,
        )
    }

    @Test
    fun missingPersistedTxtFailsToOpen() = runTest {
        val editorService = FakeEditorService().apply {
            missingPersistedPeriods += "2026-03"
        }
        val viewModel = createViewModel(editorService)

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()

        assertEquals(null, viewModel.state.value.activeRecordDocument)
        assertTrue(viewModel.state.value.errorMessage?.contains("out of sync") == true)
    }

    @Test
    fun workspaceDataChangeRefreshesPersistedPeriods() = runTest {
        val editorService = FakeEditorService()
        val workspaceDataChangeBus = WorkspaceDataChangeBus()
        val viewModel = EditorViewModel(editorService, AppSessionBus(), workspaceDataChangeBus)
        advanceUntilIdle()

        editorService.persistedPeriods += "2027-01"
        editorService.savedRecords["2027-01"] = "date:2027-01\nremark:\n\nmeal\nmeal_low\n"
        workspaceDataChangeBus.notifyChanged()
        advanceUntilIdle()

        assertTrue(viewModel.state.value.persistedRecordPeriods.contains("2027-01"))
    }
}
