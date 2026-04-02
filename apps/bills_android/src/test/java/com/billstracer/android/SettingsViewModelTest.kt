package com.billstracer.android

import android.net.Uri
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.app.navigation.WorkspaceDataChangeBus
import com.billstracer.android.features.settings.SettingsViewModel
import com.billstracer.android.model.ConfigFileValidationResult
import com.billstracer.android.model.ConfigTextsValidationResult
import com.billstracer.android.model.ConfigValidationIssue
import com.billstracer.android.model.ConfigValidationReport
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
import org.mockito.Mockito.mock

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
        val viewModel = SettingsViewModel(
            settingsService = settingsService,
            backupService = FakeBackupService(),
            sessionBus = AppSessionBus(),
            workspaceDataChangeBus = WorkspaceDataChangeBus(),
        )
        advanceUntilIdle()

        viewModel.selectBundledConfig("export_formats.toml")
        viewModel.updateConfigDraft("enabled_formats = [\"json\"]\n")
        viewModel.saveSelectedConfig()
        advanceUntilIdle()

        assertEquals(
            "enabled_formats = [\"json\"]\n",
            settingsService.savedConfigs.getValue("export_formats.toml"),
        )
        assertEquals(
            Triple(
                settingsService.savedConfigs.getValue("validator_config.toml"),
                settingsService.savedConfigs.getValue("modifier_config.toml"),
                "enabled_formats = [\"json\"]\n",
            ),
            settingsService.lastValidatedConfigTexts,
        )
        assertEquals("Modified and persisted export_formats.toml.", viewModel.state.value.statusMessage)
    }

    @Test
    fun saveSelectedConfigBlocksInvalidTomlBeforePersist() = runTest {
        val settingsService = FakeSettingsService().apply {
            nextConfigValidationResult = ConfigTextsValidationResult(
                ok = false,
                code = "business.validation_failed",
                message = "Unknown export format [config/export_formats.toml] field=enabled_formats[0]",
                configValidation = ConfigValidationReport(
                    processed = 3,
                    success = 2,
                    failure = 1,
                    allValid = false,
                    files = listOf(
                        ConfigFileValidationResult(
                            sourceKind = "config_text",
                            fileName = "export_formats.toml",
                            path = "config/export_formats.toml",
                            ok = false,
                            issues = listOf(
                                ConfigValidationIssue(
                                    sourceKind = "config_text",
                                    stage = "validate",
                                    code = "config.export_format.unknown",
                                    message = "Unknown export format",
                                    path = "config/export_formats.toml",
                                    line = 1,
                                    column = 1,
                                    fieldPath = "enabled_formats[0]",
                                    severity = "error",
                                ),
                            ),
                        ),
                    ),
                    enabledExportFormats = emptyList(),
                    availableExportFormats = listOf("json", "md"),
                ),
                enabledExportFormats = emptyList(),
                availableExportFormats = listOf("json", "md"),
                rawJson = """{"ok":false}""",
            )
        }
        val viewModel = SettingsViewModel(
            settingsService = settingsService,
            backupService = FakeBackupService(),
            sessionBus = AppSessionBus(),
            workspaceDataChangeBus = WorkspaceDataChangeBus(),
        )
        advanceUntilIdle()

        viewModel.selectBundledConfig("export_formats.toml")
        viewModel.updateConfigDraft("enabled_formats = [\"bad\"]\n")
        viewModel.saveSelectedConfig()
        advanceUntilIdle()

        assertEquals(
            "enabled_formats = [\"json\", \"md\"]\n",
            settingsService.savedConfigs.getValue("export_formats.toml"),
        )
        assertEquals(
            "Unknown export format [config/export_formats.toml] field=enabled_formats[0]",
            viewModel.state.value.errorMessage,
        )
        assertEquals(
            "Unknown export format [config/export_formats.toml] field=enabled_formats[0]",
            viewModel.state.value.statusMessage,
        )
    }

    @Test
    fun applyThemeDraftPersistsThemeSelection() = runTest {
        val settingsService = FakeSettingsService()
        val sessionBus = AppSessionBus()
        val viewModel = SettingsViewModel(
            settingsService = settingsService,
            backupService = FakeBackupService(),
            sessionBus = sessionBus,
            workspaceDataChangeBus = WorkspaceDataChangeBus(),
        )
        advanceUntilIdle()

        viewModel.updateThemeModeDraft(ThemeMode.DARK)
        viewModel.updateThemeColorDraft(ThemeColor.EMERALD)
        viewModel.applyThemeDraft()
        advanceUntilIdle()

        assertEquals(emeraldDarkTheme, settingsService.savedTheme)
        assertEquals(emeraldDarkTheme, viewModel.state.value.themePreferences)
        assertEquals(emeraldDarkTheme, sessionBus.state.value.themePreferences)
    }

    @Test
    fun exportBackupBundleUpdatesStatus() = runTest {
        val viewModel = SettingsViewModel(
            settingsService = FakeSettingsService(),
            backupService = FakeBackupService(),
            sessionBus = AppSessionBus(),
            workspaceDataChangeBus = WorkspaceDataChangeBus(),
        )
        advanceUntilIdle()

        viewModel.exportBackupBundle(mock(Uri::class.java))
        advanceUntilIdle()

        assertEquals(
            "Exported a backup bundle with 1 TXT record file(s) and 2 config file(s) to bills_backup.zip.",
            viewModel.state.value.statusMessage,
        )
        assertEquals("bills_backup.zip", viewModel.state.value.lastExportedBackupResult?.destinationDisplayPath)
    }

    @Test
    fun importBackupBundleRefreshesConfigsAndNotifiesWorkspace() = runTest {
        val settingsService = FakeSettingsService()
        val workspaceDataChangeBus = WorkspaceDataChangeBus()
        val backupService = FakeBackupService().apply {
            importedResult = importedResult.copy(
                restoredRecordFiles = 2,
                restoredConfigFiles = 2,
                sourceDisplayPath = "phone_backup.zip",
            )
        }
        val viewModel = SettingsViewModel(
            settingsService = settingsService,
            backupService = backupService,
            sessionBus = AppSessionBus(),
            workspaceDataChangeBus = workspaceDataChangeBus,
        )
        advanceUntilIdle()

        settingsService.savedConfigs["validator_config.toml"] = "validator = true\n"
        viewModel.importBackupBundle(mock(Uri::class.java))
        advanceUntilIdle()

        assertEquals(
            "Restored 2 TXT record file(s) and 2 config file(s) from phone_backup.zip, and rebuilt SQLite.",
            viewModel.state.value.statusMessage,
        )
        assertEquals("validator = true\n", viewModel.state.value.configDrafts["validator_config.toml"])
        assertEquals(1, workspaceDataChangeBus.version.value)
    }
}
