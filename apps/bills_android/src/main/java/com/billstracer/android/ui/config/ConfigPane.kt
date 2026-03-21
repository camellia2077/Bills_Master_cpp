package com.billstracer.android.ui.config

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import com.billstracer.android.model.BillsUiState
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.ui.common.PaneContent

private enum class ConfigSubview(
    val label: String,
) {
    TOML("TOML"),
    THEME("Theme"),
    ABOUT("About"),
}

@Composable
internal fun ConfigPane(
    state: BillsUiState,
    onSelectConfig: (String) -> Unit,
    onConfigDraftChange: (String) -> Unit,
    onModifyConfig: () -> Unit,
    onResetConfigDraft: () -> Unit,
    onSelectThemeMode: (ThemeMode) -> Unit,
    onSelectThemeColor: (ThemeColor) -> Unit,
    onApplyTheme: () -> Unit,
    onResetThemeDraft: () -> Unit,
    modifier: Modifier = Modifier,
) {
    var selectedSubview by rememberSaveable { mutableStateOf(ConfigSubview.TOML) }

    PaneContent(modifier = modifier) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState()),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            ConfigSubview.entries.forEach { subview ->
                ConfigModeChip(
                    label = subview.label,
                    selected = selectedSubview == subview,
                    onClick = { selectedSubview = subview },
                    testTag = when (subview) {
                        ConfigSubview.TOML -> "config_toml_button"
                        ConfigSubview.THEME -> "config_theme_button"
                        ConfigSubview.ABOUT -> "config_about_button"
                    },
                )
            }
        }
        when (selectedSubview) {
            ConfigSubview.TOML -> {
                if (state.bundledConfigs.isEmpty()) {
                    Text(
                        text = if (state.isInitializing) {
                            "Loading bundled config..."
                        } else {
                            "No bundled config loaded."
                        },
                        style = MaterialTheme.typography.bodyMedium,
                        fontFamily = FontFamily.Monospace,
                    )
                } else {
                    ConfigEditorBlock(
                        state = state,
                        onSelectConfig = onSelectConfig,
                        onConfigDraftChange = onConfigDraftChange,
                        onModifyConfig = onModifyConfig,
                        onResetConfigDraft = onResetConfigDraft,
                    )
                }
            }

            ConfigSubview.THEME -> {
                ThemeSettingsBlock(
                    state = state,
                    onSelectThemeMode = onSelectThemeMode,
                    onSelectThemeColor = onSelectThemeColor,
                    onApplyTheme = onApplyTheme,
                    onResetThemeDraft = onResetThemeDraft,
                )
            }

            ConfigSubview.ABOUT -> {
                val bundledNotices = state.bundledNotices
                if (bundledNotices == null) {
                    Text(
                        text = if (state.isInitializing) {
                            "Loading bundled notices..."
                        } else {
                            "No bundled notices loaded."
                        },
                        style = MaterialTheme.typography.bodyMedium,
                        fontFamily = FontFamily.Monospace,
                    )
                } else {
                    AboutBlock(notices = bundledNotices)
                }
            }
        }
        VersionInfoBlock(
            coreVersion = state.coreVersion,
            androidVersion = state.androidVersion,
        )
    }
}
