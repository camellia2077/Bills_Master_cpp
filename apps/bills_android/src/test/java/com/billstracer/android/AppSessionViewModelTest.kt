package com.billstracer.android

import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.app.navigation.AppSessionViewModel
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
class AppSessionViewModelTest {
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
    fun initializeLoadsEnvironmentThemeAndVersions() = runTest {
        val viewModel = AppSessionViewModel(
            workspaceService = FakeWorkspaceService(),
            settingsService = FakeSettingsService(),
            sessionBus = AppSessionBus(),
        )

        advanceUntilIdle()

        assertEquals(false, viewModel.state.value.isInitializing)
        assertEquals("2025 full-year sample", viewModel.state.value.environment?.bundledSampleLabel)
        assertEquals("0.4.2", viewModel.state.value.coreVersion?.versionName)
        assertEquals("0.1.3", viewModel.state.value.androidVersion?.versionName)
        assertEquals("2025", viewModel.state.value.environment?.bundledSampleYear)
        assertEquals("2025-01", viewModel.state.value.environment?.bundledSampleMonth)
        assertTrue(viewModel.state.value.globalStatusMessage.isNotBlank())
    }

    @Test
    fun initializeFailurePublishesGlobalError() = runTest {
        val viewModel = AppSessionViewModel(
            workspaceService = FakeWorkspaceService(),
            settingsService = FailingSettingsService(),
            sessionBus = AppSessionBus(),
        )

        advanceUntilIdle()

        assertEquals(false, viewModel.state.value.isInitializing)
        assertEquals("version load failed", viewModel.state.value.globalErrorMessage)
        assertEquals("Workspace setup failed.", viewModel.state.value.globalStatusMessage)
    }
}
