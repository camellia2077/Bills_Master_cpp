package com.billstracer.android.features.settings

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.rememberScrollState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.platform.PaneContent

private enum class SettingsSubview(
    val label: String,
) {
    TOML("TOML"),
    BACKUP("Backup"),
    THEME("Theme"),
    ABOUT("About"),
}

@Composable
internal fun SettingsScreen(
    state: SettingsUiState,
    onSelectConfig: (String) -> Unit,
    onConfigDraftChange: (String) -> Unit,
    onModifyConfig: () -> Unit,
    onResetConfigDraft: () -> Unit,
    onRequestExportBackup: () -> Unit,
    onRequestImportBackup: () -> Unit,
    onSelectThemeMode: (ThemeMode) -> Unit,
    onSelectThemeColor: (ThemeColor) -> Unit,
    onApplyTheme: () -> Unit,
    onResetThemeDraft: () -> Unit,
    modifier: Modifier = Modifier,
) {
    var selectedSubview by rememberSaveable { mutableStateOf(SettingsSubview.TOML) }

    PaneContent(modifier = modifier) {
        Row(
            modifier = Modifier
                .horizontalScroll(rememberScrollState()),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            SettingsSubview.entries.forEach { subview ->
                ConfigModeChip(
                    label = subview.label,
                    selected = selectedSubview == subview,
                    onClick = { selectedSubview = subview },
                    testTag = when (subview) {
                        SettingsSubview.TOML -> "settings_toml_button"
                        SettingsSubview.BACKUP -> "settings_backup_button"
                        SettingsSubview.THEME -> "settings_theme_button"
                        SettingsSubview.ABOUT -> "settings_about_button"
                    },
                )
            }
        }

        when (selectedSubview) {
            SettingsSubview.TOML -> {
                ConfigEditorBlock(
                    state = state,
                    onSelectConfig = onSelectConfig,
                    onConfigDraftChange = onConfigDraftChange,
                    onModifyConfig = onModifyConfig,
                    onResetConfigDraft = onResetConfigDraft,
                )
            }
            SettingsSubview.BACKUP -> {
                BackupSettingsBlock(
                    state = state,
                    onRequestExportBackup = onRequestExportBackup,
                    onRequestImportBackup = onRequestImportBackup,
                )
            }
            SettingsSubview.THEME -> {
                ThemeSettingsBlock(
                    state = state,
                    onSelectThemeMode = onSelectThemeMode,
                    onSelectThemeColor = onSelectThemeColor,
                    onApplyTheme = onApplyTheme,
                    onResetThemeDraft = onResetThemeDraft,
                )
            }
            SettingsSubview.ABOUT -> {
                state.bundledNotices?.let { notices ->
                    AboutBlock(notices = notices)
                }
            }
        }

        VersionInfoBlock(
            coreVersion = state.coreVersion,
            androidVersion = state.androidVersion,
        )
    }
}
