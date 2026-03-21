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
    fun openManualRecordLoadsTemplate() = runTest {
        val viewModel = EditorViewModel(FakeEditorService(), AppSessionBus())
        advanceUntilIdle()

        viewModel.updateRecordPeriodYearInput("2026")
        viewModel.updateRecordPeriodMonthInput("03")
        viewModel.openManualRecord()
        advanceUntilIdle()

        assertEquals("2026-03", viewModel.state.value.activeRecordDocument?.period)
        assertEquals(false, viewModel.state.value.activeRecordDocument?.persisted)
    }

    @Test
    fun invalidManualMonthDoesNotOpenRecord() = runTest {
        val viewModel = EditorViewModel(FakeEditorService(), AppSessionBus())
        advanceUntilIdle()

        viewModel.updateRecordPeriodYearInput("2026")
        viewModel.updateRecordPeriodMonthInput("13")
        viewModel.openManualRecord()
        advanceUntilIdle()

        assertEquals(null, viewModel.state.value.activeRecordDocument)
        assertEquals(
            "Manual period must use YYYY-MM, and month must be between 01 and 12.",
            viewModel.state.value.errorMessage,
        )
    }

    @Test
    fun saveRecordDraftPersistsTxtSource() = runTest {
        val editorService = FakeEditorService()
        val viewModel = EditorViewModel(editorService, AppSessionBus())
        advanceUntilIdle()

        viewModel.updateRecordPeriodYearInput("2026")
        viewModel.updateRecordPeriodMonthInput("03")
        viewModel.openManualRecord()
        advanceUntilIdle()
        viewModel.updateRecordDraft("date:2026-03\nremark:test\n\nmeal\nmeal_low 12 lunch\n")
        viewModel.saveRecordDraft()
        advanceUntilIdle()

        assertEquals(
            "date:2026-03\nremark:test\n\nmeal\nmeal_low 12 lunch\n",
            editorService.savedRecords.getValue("2026-03"),
        )
        assertTrue(viewModel.state.value.activeRecordDocument?.persisted == true)
    }
}
