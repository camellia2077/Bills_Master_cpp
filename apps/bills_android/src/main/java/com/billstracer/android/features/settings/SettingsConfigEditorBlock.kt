package com.billstracer.android.features.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp

@Composable
internal fun ConfigEditorBlock(
    state: SettingsUiState,
    onSelectConfig: (String) -> Unit,
    onConfigDraftChange: (String) -> Unit,
    onModifyConfig: () -> Unit,
    onResetConfigDraft: () -> Unit,
) {
    var selectorExpanded by rememberSaveable { mutableStateOf(false) }
    val selectedConfig = state.bundledConfigs.firstOrNull { config ->
        config.fileName == state.selectedConfigFileName
    } ?: state.bundledConfigs.firstOrNull()
    val selectedFileName = selectedConfig?.fileName.orEmpty()
    val draftText = state.configDrafts[selectedFileName] ?: selectedConfig?.rawText.orEmpty()
    val hasUnsavedChanges = selectedConfig != null && draftText != selectedConfig.rawText

    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Box(modifier = Modifier.fillMaxWidth()) {
            OutlinedButton(
                onClick = { selectorExpanded = true },
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("settings_config_selector_button"),
            ) {
                Text(
                    text = selectedFileName.ifBlank { "Select TOML file" },
                    fontFamily = FontFamily.Monospace,
                )
            }
            DropdownMenu(
                expanded = selectorExpanded,
                onDismissRequest = { selectorExpanded = false },
                modifier = Modifier.fillMaxWidth(0.92f),
            ) {
                state.bundledConfigs.forEach { config ->
                    DropdownMenuItem(
                        text = { Text(text = config.fileName, fontFamily = FontFamily.Monospace) },
                        onClick = {
                            selectorExpanded = false
                            onSelectConfig(config.fileName)
                        },
                    )
                }
            }
        }
        Text(
            text = if (hasUnsavedChanges) "Unsaved changes" else "Editing persisted runtime_config",
            style = MaterialTheme.typography.labelMedium,
            fontFamily = FontFamily.Monospace,
        )
        Text(
            text = "Draft changes stay local until you press Modify.",
            style = MaterialTheme.typography.bodySmall,
            fontFamily = FontFamily.Monospace,
        )
        OutlinedTextField(
            value = draftText,
            onValueChange = onConfigDraftChange,
            enabled = !state.isInitializing && !state.isWorking && selectedConfig != null,
            modifier = Modifier
                .fillMaxWidth()
                .heightIn(min = 280.dp, max = 420.dp)
                .testTag("settings_config_editor_field"),
            textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
            label = { Text(text = selectedFileName.ifBlank { "TOML" }, fontFamily = FontFamily.Monospace) },
            minLines = 14,
        )
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            Button(
                onClick = onModifyConfig,
                enabled = !state.isInitializing && !state.isWorking && hasUnsavedChanges,
                modifier = Modifier
                    .weight(1f)
                    .testTag("settings_config_modify_button"),
            ) {
                Text("Modify")
            }
            OutlinedButton(
                onClick = onResetConfigDraft,
                enabled = !state.isInitializing && !state.isWorking && hasUnsavedChanges,
                modifier = Modifier
                    .weight(1f)
                    .testTag("settings_config_reset_button"),
            ) {
                Text("Reset Draft")
            }
        }
    }
}
