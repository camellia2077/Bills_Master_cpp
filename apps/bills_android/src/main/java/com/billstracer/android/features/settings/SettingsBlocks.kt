package com.billstracer.android.features.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.foundation.verticalScroll
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
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import com.billstracer.android.app.theme.billsColorScheme
import com.billstracer.android.app.theme.resolveDarkTheme
import com.billstracer.android.app.theme.themeColorDisplayOrder
import com.billstracer.android.app.theme.themeColorPreview
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePreferences
import com.billstracer.android.model.VersionInfo
import kotlin.math.max

private val themeColorCellSize = 44.dp
private val themeColorPreviewSize = 34.dp
private val themeColorGridSpacing = 8.dp

@Composable
internal fun ConfigModeChip(
    label: String,
    selected: Boolean,
    onClick: () -> Unit,
    testTag: String,
) {
    if (selected) {
        Button(onClick = onClick, modifier = Modifier.testTag(testTag)) {
            Text(text = label, fontFamily = FontFamily.Monospace)
        }
    } else {
        OutlinedButton(onClick = onClick, modifier = Modifier.testTag(testTag)) {
            Text(text = label, fontFamily = FontFamily.Monospace)
        }
    }
}

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

@Composable
internal fun ThemeSettingsBlock(
    state: SettingsUiState,
    onSelectThemeMode: (ThemeMode) -> Unit,
    onSelectThemeColor: (ThemeColor) -> Unit,
    onApplyTheme: () -> Unit,
    onResetThemeDraft: () -> Unit,
) {
    val hasUnsavedChanges = state.themeDraft != state.themePreferences

    Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
        Text(
            text = "Theme mode",
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
        )
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState()),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            ThemeMode.entries.forEach { mode ->
                ConfigModeChip(
                    label = mode.displayName,
                    selected = state.themeDraft.mode == mode,
                    onClick = { onSelectThemeMode(mode) },
                    testTag = "settings_theme_mode_${mode.name.lowercase()}",
                )
            }
        }
        Text(
            text = "Brand color",
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
        )
        Text(
            text = "Rainbow-sorted custom brand colors from time_tracer.",
            style = MaterialTheme.typography.bodySmall,
            fontFamily = FontFamily.Monospace,
        )
        ThemeColorSelector(
            selectedColor = state.themeDraft.color,
            onSelectThemeColor = onSelectThemeColor,
        )
        ThemePreviewCard(themePreferences = state.themeDraft)
        Text(
            text = if (hasUnsavedChanges) {
                "Theme changes are pending. Press Apply Theme to persist them with DataStore."
            } else {
                "Theme is already synced with persisted DataStore settings."
            },
            style = MaterialTheme.typography.bodySmall,
            fontFamily = FontFamily.Monospace,
        )
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            Button(
                onClick = onApplyTheme,
                enabled = !state.isInitializing && !state.isWorking && hasUnsavedChanges,
                modifier = Modifier
                    .weight(1f)
                    .testTag("settings_theme_apply_button"),
            ) {
                Text("Apply Theme")
            }
            OutlinedButton(
                onClick = onResetThemeDraft,
                enabled = !state.isInitializing && !state.isWorking && hasUnsavedChanges,
                modifier = Modifier
                    .weight(1f)
                    .testTag("settings_theme_reset_button"),
            ) {
                Text("Reset Theme")
            }
        }
    }
}

@Composable
private fun ThemeColorSelector(
    selectedColor: ThemeColor,
    onSelectThemeColor: (ThemeColor) -> Unit,
) {
    val colors = themeColorDisplayOrder()

    BoxWithConstraints(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("settings_theme_color_selector"),
    ) {
        val columns = max(
            1,
            ((maxWidth + themeColorGridSpacing) / (themeColorCellSize + themeColorGridSpacing)).toInt(),
        )
        val rowCount = (colors.size + columns - 1) / columns
        val gridHeight = (themeColorCellSize * rowCount) +
            (themeColorGridSpacing * (rowCount - 1).coerceAtLeast(0))

        LazyVerticalGrid(
            columns = GridCells.Fixed(columns),
            modifier = Modifier
                .fillMaxWidth()
                .height(gridHeight),
            horizontalArrangement = Arrangement.spacedBy(themeColorGridSpacing),
            verticalArrangement = Arrangement.spacedBy(themeColorGridSpacing),
            userScrollEnabled = false,
        ) {
            items(items = colors, key = { color -> color.name }) { color ->
                Box(modifier = Modifier.fillMaxWidth(), contentAlignment = Alignment.Center) {
                    ThemeColorOption(
                        color = color,
                        isSelected = color == selectedColor,
                        onSelectThemeColor = onSelectThemeColor,
                    )
                }
            }
        }
    }
    Text(
        text = "Selected: ${selectedColor.displayName}",
        style = MaterialTheme.typography.bodySmall,
        fontFamily = FontFamily.Monospace,
    )
}

@Composable
private fun ThemeColorOption(
    color: ThemeColor,
    isSelected: Boolean,
    onSelectThemeColor: (ThemeColor) -> Unit,
) {
    val preview = themeColorPreview(color)

    Box(
        modifier = Modifier
            .size(themeColorCellSize)
            .testTag("settings_theme_color_${color.name.lowercase()}")
            .selectable(
                selected = isSelected,
                onClick = { onSelectThemeColor(color) },
                role = Role.RadioButton,
            ),
        contentAlignment = Alignment.Center,
    ) {
        Box(
            modifier = Modifier
                .size(themeColorPreviewSize)
                .background(preview, CircleShape),
        )
        if (isSelected) {
            Box(
                modifier = Modifier
                    .size(themeColorCellSize)
                    .background(Color.Transparent, CircleShape)
                    .border(
                        width = 2.dp,
                        color = MaterialTheme.colorScheme.primary,
                        shape = CircleShape,
                    ),
            )
        }
    }
}

@Composable
private fun ThemePreviewCard(themePreferences: ThemePreferences) {
    val previewDarkTheme = resolveDarkTheme(
        mode = themePreferences.mode,
        systemDarkTheme = androidx.compose.foundation.isSystemInDarkTheme(),
    )
    val previewColors = billsColorScheme(
        themeColor = themePreferences.color,
        darkTheme = previewDarkTheme,
    )

    Surface(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("settings_theme_preview_card"),
        shape = RoundedCornerShape(22.dp),
        color = previewColors.surface,
        contentColor = previewColors.onSurface,
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
            ) {
                Text(
                    text = "Preview",
                    style = MaterialTheme.typography.titleMedium,
                    fontFamily = FontFamily.Monospace,
                    color = previewColors.onSurface,
                )
                Text(
                    text = if (previewDarkTheme) "Dark" else "Light",
                    style = MaterialTheme.typography.labelLarge,
                    fontFamily = FontFamily.Monospace,
                    color = previewColors.onSurface,
                )
            }
            Text(
                text = "${themePreferences.color.displayName} / ${themePreferences.mode.displayName}",
                style = MaterialTheme.typography.bodyMedium,
                fontFamily = FontFamily.Monospace,
                color = previewColors.onSurface,
            )
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .height(18.dp)
                        .background(previewColors.primary, RoundedCornerShape(999.dp)),
                )
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .height(18.dp)
                        .background(previewColors.primaryContainer, RoundedCornerShape(999.dp)),
                )
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .height(18.dp)
                        .background(previewColors.surfaceContainer, RoundedCornerShape(999.dp)),
                )
            }
            Surface(
                shape = RoundedCornerShape(18.dp),
                color = previewColors.secondaryContainer,
            ) {
                Text(
                    text = "Material3 preview card",
                    modifier = Modifier.padding(horizontal = 14.dp, vertical = 12.dp),
                    style = MaterialTheme.typography.bodyMedium,
                    fontFamily = FontFamily.Monospace,
                    color = previewColors.onSecondaryContainer,
                )
            }
        }
    }
}

@Composable
internal fun AboutBlock(notices: BundledNotices) {
    var showRawJson by rememberSaveable(notices.markdownText, notices.rawJson) {
        mutableStateOf(false)
    }
    val scrollState = rememberScrollState()
    val title = if (showRawJson) "notices.json" else "NOTICE.md"
    val content = if (showRawJson) notices.rawJson else notices.markdownText
    val toggleLabel = if (showRawJson) "Show NOTICE.md" else "Show Raw JSON"
    val cardTag = if (showRawJson) "settings_notices_json_card" else "settings_notices_markdown_card"
    val textTag = if (showRawJson) "settings_notices_json_text" else "settings_notices_markdown_text"

    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Text(
            text = "About",
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
        )
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

@Composable
internal fun VersionInfoBlock(
    coreVersion: VersionInfo?,
    androidVersion: VersionInfo?,
) {
    Surface(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("settings_versions_block"),
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
            VersionLine(label = "core", version = coreVersion, fallback = "Loading core version...")
            VersionLine(label = "android", version = androidVersion, fallback = "Loading android version...")
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
