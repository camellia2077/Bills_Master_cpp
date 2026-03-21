package com.billstracer.android

import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.features.workspace.WorkspaceViewModel
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

@OptIn(ExperimentalCoroutinesApi::class)
class WorkspaceViewModelTest {
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
    fun importBundledSampleStoresResult() = runTest {
        val viewModel = WorkspaceViewModel(FakeWorkspaceService(), AppSessionBus())
        advanceUntilIdle()

        viewModel.importBundledSample()
        advanceUntilIdle()

        assertEquals(12, viewModel.state.value.importResult?.imported)
        assertEquals("Imported 12 bundled bill file(s).", viewModel.state.value.statusMessage)
    }

    @Test
    fun clearRecordFilesUpdatesStatus() = runTest {
        val viewModel = WorkspaceViewModel(FakeWorkspaceService(), AppSessionBus())
        advanceUntilIdle()

        viewModel.clearRecordFiles()
        advanceUntilIdle()

        assertEquals("Cleared 1 TXT record file(s).", viewModel.state.value.statusMessage)
    }

    @Test
    fun clearDatabaseResetsStatus() = runTest {
        val viewModel = WorkspaceViewModel(FakeWorkspaceService(), AppSessionBus())
        advanceUntilIdle()

        viewModel.clearDatabase()
        advanceUntilIdle()

        assertEquals("Database file cleared.", viewModel.state.value.statusMessage)
    }
}
