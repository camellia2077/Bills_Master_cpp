package com.billstracer.android

import com.billstracer.android.app.navigation.AppSessionBus
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
            databasePeriods.clear()
            databasePeriods += listOf("2025-01", "2026-03", "2026-02")
        }
        val viewModel = EditorViewModel(editorService, AppSessionBus())

        advanceUntilIdle()

        assertEquals(false, viewModel.state.value.isInitializing)
        assertEquals("2026", viewModel.state.value.selectedExistingRecordYear)
        assertEquals("03", viewModel.state.value.selectedExistingRecordMonth)
        assertEquals(listOf("2026-03", "2026-02", "2025-01"), viewModel.state.value.databaseRecordPeriods)
    }

    @Test
    fun openingSelectedRecordLoadsPersistedTxt() = runTest {
        val editorService = FakeEditorService()
        val viewModel = EditorViewModel(editorService, AppSessionBus())

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()

        assertEquals("2026-03", viewModel.state.value.activeRecordDocument?.period)
        assertTrue(viewModel.state.value.activeRecordDocument?.persisted == true)
    }

    @Test
    fun selectingYearDefaultsMonthWithinThatYear() = runTest {
        val editorService = FakeEditorService().apply {
            databasePeriods.clear()
            databasePeriods += listOf("2026-03", "2026-02", "2025-12", "2025-01")
        }
        val viewModel = EditorViewModel(editorService, AppSessionBus())

        advanceUntilIdle()
        viewModel.selectExistingRecordYear("2025")

        assertEquals("2025", viewModel.state.value.selectedExistingRecordYear)
        assertEquals("12", viewModel.state.value.selectedExistingRecordMonth)
    }

    @Test
    fun saveRecordDraftPersistsTxtAndSyncsDatabase() = runTest {
        val editorService = FakeEditorService()
        val viewModel = EditorViewModel(editorService, AppSessionBus())

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
        assertEquals(listOf("2026-03"), editorService.syncedPeriods)
        assertTrue(viewModel.state.value.activeRecordDocument?.persisted == true)
        assertEquals(null, viewModel.state.value.errorMessage)
    }

    @Test
    fun saveRecordDraftKeepsTxtWhenSyncFails() = runTest {
        val editorService = FakeEditorService().apply {
            syncFailures["2026-03"] = "validation: The first line must be 'date:YYYY-MM'."
        }
        val viewModel = EditorViewModel(editorService, AppSessionBus())

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()
        viewModel.updateRecordDraft("date:2026-03\nremark:broken\n\nmeal\nmeal_low 12 lunch\n")
        viewModel.saveRecordDraft()
        advanceUntilIdle()

        assertEquals(
            "date:2026-03\nremark:broken\n\nmeal\nmeal_low 12 lunch\n",
            editorService.savedRecords.getValue("2026-03"),
        )
        assertEquals(
            "validation: The first line must be 'date:YYYY-MM'.",
            viewModel.state.value.errorMessage,
        )
        assertEquals(
            "date:2026-03\nremark:broken\n\nmeal\nmeal_low 12 lunch\n",
            viewModel.state.value.activeRecordDocument?.rawText,
        )
    }

    @Test
    fun missingPersistedTxtFailsToOpen() = runTest {
        val editorService = FakeEditorService().apply {
            missingPersistedPeriods += "2026-03"
        }
        val viewModel = EditorViewModel(editorService, AppSessionBus())

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()

        assertEquals(null, viewModel.state.value.activeRecordDocument)
        assertTrue(viewModel.state.value.errorMessage?.contains("No persisted TXT file exists") == true)
    }
}
