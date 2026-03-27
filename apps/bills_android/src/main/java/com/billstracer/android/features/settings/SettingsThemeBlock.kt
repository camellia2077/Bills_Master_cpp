package com.billstracer.android.features.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
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
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePreferences
import kotlin.math.max

private val themeColorCellSize = 44.dp
private val themeColorPreviewSize = 34.dp
private val themeColorGridSpacing = 8.dp

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
        systemDarkTheme = isSystemInDarkTheme(),
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
