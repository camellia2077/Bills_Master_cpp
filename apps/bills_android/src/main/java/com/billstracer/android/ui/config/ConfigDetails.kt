package com.billstracer.android.ui.config

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
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
import com.billstracer.android.model.BillsUiState
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.VersionInfo

@Composable
internal fun ConfigModeChip(
    label: String,
    selected: Boolean,
    onClick: () -> Unit,
    testTag: String,
) {
    if (selected) {
        Button(
            onClick = onClick,
            modifier = Modifier.testTag(testTag),
        ) {
            Text(
                text = label,
                fontFamily = FontFamily.Monospace,
            )
        }
    } else {
        OutlinedButton(
            onClick = onClick,
            modifier = Modifier.testTag(testTag),
        ) {
            Text(
                text = label,
                fontFamily = FontFamily.Monospace,
            )
        }
    }
}

@Composable
internal fun ConfigEditorBlock(
    state: BillsUiState,
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

    Column(
        verticalArrangement = Arrangement.spacedBy(8.dp),
    ) {
        Box(modifier = Modifier.fillMaxWidth()) {
            OutlinedButton(
                onClick = { selectorExpanded = true },
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("config_selector_button"),
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
                        text = {
                            Text(
                                text = config.fileName,
                                fontFamily = FontFamily.Monospace,
                            )
                        },
                        onClick = {
                            selectorExpanded = false
                            onSelectConfig(config.fileName)
                        },
                    )
                }
            }
        }
        Text(
            text = if (hasUnsavedChanges) {
                "Unsaved changes"
            } else {
                "Editing persisted runtime_config"
            },
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
                .testTag("config_editor_field"),
            textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
            label = {
                Text(
                    text = selectedFileName.ifBlank { "TOML" },
                    fontFamily = FontFamily.Monospace,
                )
            },
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
                    .testTag("config_modify_button"),
            ) {
                Text("Modify")
            }
            OutlinedButton(
                onClick = onResetConfigDraft,
                enabled = !state.isInitializing && !state.isWorking && hasUnsavedChanges,
                modifier = Modifier
                    .weight(1f)
                    .testTag("config_reset_button"),
            ) {
                Text("Reset Draft")
            }
        }
    }
}

@Composable
internal fun AboutBlock(notices: BundledNotices) {
    Column(
        verticalArrangement = Arrangement.spacedBy(8.dp),
    ) {
        Text(
            text = "About",
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
        )
        Text(
            text = "Open-source licenses are bundled below.",
            style = MaterialTheme.typography.bodyMedium,
            fontFamily = FontFamily.Monospace,
        )
        NoticesSourceBlock(notices)
    }
}

@Composable
internal fun VersionInfoBlock(
    coreVersion: VersionInfo?,
    androidVersion: VersionInfo?,
) {
    Surface(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("config_versions_block"),
        shape = RoundedCornerShape(18.dp),
        color = MaterialTheme.colorScheme.surface.copy(alpha = 0.92f),
    ) {
        Column(
            modifier = Modifier.padding(14.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            Text(
                text = "Versions",
                style = MaterialTheme.typography.titleMedium,
                fontFamily = FontFamily.Monospace,
            )
            VersionLine(
                label = "core",
                version = coreVersion,
                fallback = "Loading core version...",
            )
            VersionLine(
                label = "android",
                version = androidVersion,
                fallback = "Loading android version...",
            )
        }
    }
}

@Composable
private fun VersionLine(
    label: String,
    version: VersionInfo?,
    fallback: String,
) {
    val details = if (version == null) {
        fallback
    } else {
        buildString {
            append(version.versionName)
            version.versionCode?.let { code ->
                append(" (code ")
                append(code)
                append(')')
            }
            version.lastUpdated?.takeIf { it.isNotBlank() }?.let { lastUpdated ->
                append("  updated ")
                append(lastUpdated)
            }
        }
    }
    Text(
        text = "$label: $details",
        style = MaterialTheme.typography.bodyMedium,
        fontFamily = FontFamily.Monospace,
    )
}

@Composable
private fun NoticesSourceBlock(notices: BundledNotices) {
    var showRawJson by rememberSaveable(notices.markdownText, notices.rawJson) {
        mutableStateOf(false)
    }
    val scrollState = rememberScrollState()
    val title = if (showRawJson) "notices.json" else "NOTICE.md"
    val content = if (showRawJson) notices.rawJson else notices.markdownText
    val toggleLabel = if (showRawJson) "Show NOTICE.md" else "Show Raw JSON"
    val cardTag = if (showRawJson) "notices_json_card" else "notices_markdown_card"
    val textTag = if (showRawJson) "notices_json_text" else "notices_markdown_text"

    Column(
        verticalArrangement = Arrangement.spacedBy(8.dp),
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
        ) {
            Text(
                text = title,
                style = MaterialTheme.typography.titleMedium,
                fontFamily = FontFamily.Monospace,
            )
            Button(
                onClick = { showRawJson = !showRawJson },
                modifier = Modifier.widthIn(min = 132.dp),
            ) {
                Text(toggleLabel)
            }
        }
        Surface(
            modifier = Modifier
                .fillMaxWidth()
                .heightIn(max = 420.dp)
                .testTag(cardTag),
            shape = RoundedCornerShape(18.dp),
            color = MaterialTheme.colorScheme.surface.copy(alpha = 0.92f),
        ) {
            SelectionContainer {
                Text(
                    text = content,
                    modifier = Modifier
                        .fillMaxWidth()
                        .verticalScroll(scrollState)
                        .padding(14.dp)
                        .testTag(textTag),
                    style = MaterialTheme.typography.bodyMedium,
                    fontFamily = FontFamily.Monospace,
                )
            }
        }
    }
}
