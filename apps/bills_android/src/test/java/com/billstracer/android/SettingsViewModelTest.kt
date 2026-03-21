package com.billstracer.android

import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.features.settings.SettingsViewModel
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
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
class SettingsViewModelTest {
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
    fun saveSelectedConfigPersistsEditedToml() = runTest {
        val settingsService = FakeSettingsService()
        val viewModel = SettingsViewModel(settingsService, AppSessionBus())
        advanceUntilIdle()

        viewModel.selectBundledConfig("export_formats.toml")
        viewModel.updateConfigDraft("enabled_formats = [\"json\"]\n")
        viewModel.saveSelectedConfig()
        advanceUntilIdle()

        assertEquals(
            "enabled_formats = [\"json\"]\n",
            settingsService.savedConfigs.getValue("export_formats.toml"),
        )
        assertEquals("Modified and persisted export_formats.toml.", viewModel.state.value.statusMessage)
    }

    @Test
    fun applyThemeDraftPersistsThemeSelection() = runTest {
        val settingsService = FakeSettingsService()
        val sessionBus = AppSessionBus()
        val viewModel = SettingsViewModel(settingsService, sessionBus)
        advanceUntilIdle()

        viewModel.updateThemeModeDraft(ThemeMode.DARK)
        viewModel.updateThemeColorDraft(ThemeColor.EMERALD)
        viewModel.applyThemeDraft()
        advanceUntilIdle()

        assertEquals(emeraldDarkTheme, settingsService.savedTheme)
        assertEquals(emeraldDarkTheme, viewModel.state.value.themePreferences)
        assertEquals(emeraldDarkTheme, sessionBus.state.value.themePreferences)
    }
}
