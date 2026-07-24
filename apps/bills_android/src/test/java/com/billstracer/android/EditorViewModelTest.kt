package com.billstracer.android

import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.app.navigation.WorkspaceDataChangeBus
import com.billstracer.android.features.editor.EditorMode
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
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test

@OptIn(ExperimentalCoroutinesApi::class)
class EditorViewModelTest {
    private val dispatcher = StandardTestDispatcher()

    private fun createViewModel(
        editorService: FakeEditorService = FakeEditorService(),
        workspaceDataChangeBus: WorkspaceDataChangeBus = WorkspaceDataChangeBus(),
        currentPeriod: String = "2026-03",
    ): EditorViewModel = EditorViewModel(
        editorService,
        AppSessionBus(),
        workspaceDataChangeBus,
        currentPeriodProvider = { currentPeriod },
    )

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

        assertFalse(viewModel.state.value.isInitializing)
        assertEquals("2026", viewModel.state.value.selectedExistingRecordYear)
        assertEquals("03", viewModel.state.value.selectedExistingRecordMonth)
        assertEquals(listOf("2026-03", "2026-02", "2025-01"), viewModel.state.value.persistedRecordPeriods)
    }

    @Test
    fun openingSelectedRecordLoadsPersistedTxtIntoStructuredMode() = runTest {
        val viewModel = createViewModel()

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()

        assertEquals("2026-03", viewModel.state.value.activeRecordDocument?.period)
        assertEquals(EditorMode.Structured, viewModel.state.value.editorMode)
        assertNotNull(viewModel.state.value.structuredDraft)
        assertTrue(viewModel.state.value.activeRecordDocument?.persisted == true)
    }

    @Test
    fun showingEditorCreatesAndOpensCurrentMonthWhenTxtIsMissing() = runTest {
        val editorService = FakeEditorService().apply {
            persistedPeriods.clear()
            persistedPeriods += listOf("2026-03", "2026-02")
            savedRecords.keys.retainAll(persistedPeriods)
        }
        val workspaceDataChangeBus = WorkspaceDataChangeBus()
        val viewModel = createViewModel(
            editorService = editorService,
            workspaceDataChangeBus = workspaceDataChangeBus,
            currentPeriod = "2026-04",
        )

        advanceUntilIdle()
        viewModel.onEditorScreenShown()
        advanceUntilIdle()

        assertEquals("2026-04", viewModel.state.value.activeRecordDocument?.period)
        assertTrue(viewModel.state.value.activeRecordDocument?.persisted == true)
        assertEquals(EditorMode.Structured, viewModel.state.value.editorMode)
        assertEquals(listOf("2026-04"), editorService.committedPeriods)
        assertTrue(viewModel.state.value.persistedRecordPeriods.contains("2026-04"))
    }

    @Test
    fun addingBlankEntryMarksDraftIncomplete() = runTest {
        val viewModel = createViewModel()

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()
        viewModel.addStructuredEntry("meal", "meal_low")

        assertTrue(viewModel.state.value.hasIncompleteEntries)
    }

    @Test
    fun saveStructuredRecordSerializesAndCommitsTxt() = runTest {
        val editorService = FakeEditorService()
        val viewModel = createViewModel(editorService)

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()
        viewModel.addStructuredEntry("meal", "meal_low")
        val entryId = viewModel.state.value.structuredDraft!!
            .sections.first().subSections.first().entries.first().id
        viewModel.updateStructuredEntryAmount("meal", "meal_low", entryId, "12")
        viewModel.updateStructuredEntryDescription("meal", "meal_low", entryId, "lunch")
        viewModel.saveRecordDraft()
        advanceUntilIdle()

        assertEquals(listOf("2026-03"), editorService.committedPeriods)
        assertEquals(
            "date:2026-03\nremark:\n\nmeal\n\nmeal_low\n12 lunch",
            editorService.savedRecords.getValue("2026-03"),
        )
        assertEquals(EditorMode.Structured, viewModel.state.value.editorMode)
        assertFalse(viewModel.state.value.hasIncompleteEntries)
    }

    @Test
    fun enterRawExpertModeSerializesStructuredDraft() = runTest {
        val viewModel = createViewModel()

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()
        viewModel.addStructuredEntry("meal", "meal_low")
        val entryId = viewModel.state.value.structuredDraft!!
            .sections.first().subSections.first().entries.first().id
        viewModel.updateStructuredEntryAmount("meal", "meal_low", entryId, "12")
        viewModel.updateStructuredEntryDescription("meal", "meal_low", entryId, "lunch")

        viewModel.enterRawExpertMode()
        advanceUntilIdle()

        assertEquals(EditorMode.RawExpert, viewModel.state.value.editorMode)
        assertTrue(viewModel.state.value.recordDraftText.contains("12 lunch"))
    }

    @Test
    fun saveRawRecordTextReturnsToStructuredModeWhenParseSucceeds() = runTest {
        val viewModel = createViewModel()

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()

        val updatedRawText = "date:2026-03\nremark:raw\n\nmeal\n\nmeal_low\n12 lunch"
        viewModel.saveRawRecordText(updatedRawText)
        advanceUntilIdle()

        assertEquals(EditorMode.Structured, viewModel.state.value.editorMode)
        assertEquals(updatedRawText, viewModel.state.value.activeRecordDocument?.rawText)
        assertNotNull(viewModel.state.value.structuredDraft)
    }

    @Test
    fun saveRawRecordTextStaysInRawModeWhenStructuredParseFails() = runTest {
        val viewModel = createViewModel()

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()

        val unsupportedRawText = "date:2026-03\nremark:raw\n\nmeal\n\nmeal_low\ntext only"
        viewModel.saveRawRecordText(unsupportedRawText)
        advanceUntilIdle()

        assertEquals(EditorMode.RawExpert, viewModel.state.value.editorMode)
        assertNull(viewModel.state.value.structuredDraft)
        assertTrue(viewModel.state.value.activeRecordDocument?.rawFallbackReason?.contains("not supported") == true)
    }

    @Test
    fun saveRawRecordTextKeepsExpressionAndCommentInStructuredDraft() = runTest {
        val viewModel = createViewModel()

        advanceUntilIdle()
        viewModel.openSelectedExistingRecord()
        advanceUntilIdle()

        val updatedRawText = """
            date:2026-03
            remark:raw

            meal

            meal_low
            103.60*5+6.03 饭 // 有优惠买的
        """.trimIndent()
        viewModel.saveRawRecordText(updatedRawText)
        advanceUntilIdle()

        val entry = viewModel.state.value.structuredDraft
            ?.sections?.first()?.subSections?.first()?.entries?.first()
        assertEquals(EditorMode.Structured, viewModel.state.value.editorMode)
        assertNotNull(entry)
        assertEquals("103.60*5+6.03", entry?.amountExpression)
        assertEquals("饭", entry?.description)
        assertEquals("有优惠买的", entry?.comment)
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

        assertNull(viewModel.state.value.activeRecordDocument)
        assertTrue(viewModel.state.value.errorMessage?.contains("out of sync") == true)
    }

    @Test
    fun workspaceDataChangeRefreshesPersistedPeriods() = runTest {
        val editorService = FakeEditorService()
        val workspaceDataChangeBus = WorkspaceDataChangeBus()
        val viewModel = createViewModel(
            editorService = editorService,
            workspaceDataChangeBus = workspaceDataChangeBus,
        )
        advanceUntilIdle()

        editorService.persistedPeriods += "2027-01"
        editorService.savedRecords["2027-01"] = "date:2027-01\nremark:\n\nmeal\n\nmeal_low\n"
        workspaceDataChangeBus.notifyChanged()
        advanceUntilIdle()

        assertTrue(viewModel.state.value.persistedRecordPeriods.contains("2027-01"))
        assertFalse(viewModel.state.value.isInitializing)
    }

    @Test
    fun workspaceDataChangeReloadsOpenCurrentMonthAfterImportReplacesTemplate() = runTest {
        val editorService = FakeEditorService().apply {
            persistedPeriods.clear()
            persistedPeriods += listOf("2026-03", "2026-02")
            savedRecords.keys.retainAll(persistedPeriods)
        }
        val workspaceDataChangeBus = WorkspaceDataChangeBus()
        val viewModel = createViewModel(
            editorService = editorService,
            workspaceDataChangeBus = workspaceDataChangeBus,
            currentPeriod = "2026-04",
        )

        advanceUntilIdle()
        viewModel.onEditorScreenShown()
        advanceUntilIdle()

        assertEquals("date:2026-04\nremark:\n\nmeal\n\nmeal_low\n", viewModel.state.value.recordDraftText)

        val importedText = "date:2026-04\nremark:\n\nmeal\n\nmeal_low\n12 lunch"
        editorService.savedRecords["2026-04"] = importedText
        editorService.persistedPeriods += "2026-04"
        workspaceDataChangeBus.notifyChanged()
        advanceUntilIdle()

        assertEquals("2026-04", viewModel.state.value.activeRecordDocument?.period)
        assertEquals(importedText, viewModel.state.value.recordDraftText)
        assertTrue(viewModel.state.value.activeRecordDocument?.persisted == true)
    }
}
