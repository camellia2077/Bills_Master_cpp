package com.billstracer.android.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.Download
import androidx.compose.material.icons.outlined.Edit
import androidx.compose.material.icons.outlined.Search
import androidx.compose.material.icons.outlined.Settings
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.BillsUiState
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePalette
import com.billstracer.android.model.ThemePreferences
import com.billstracer.android.model.VersionInfo
import com.billstracer.android.ui.theme.billsColorScheme
import com.billstracer.android.ui.theme.resolveDarkTheme
import java.time.YearMonth

private enum class BillsSection(
    val label: String,
    val icon: ImageVector,
) {
    DATA("Data", Icons.Outlined.Download),
    RECORD("Record", Icons.Outlined.Edit),
    REPORT("Report", Icons.Outlined.Search),
    CONFIG("Config", Icons.Outlined.Settings),
}

private enum class ConfigSubview(
    val label: String,
) {
    TOML("TOML"),
    THEME("Theme"),
    ABOUT("About"),
}

@Composable
fun BillsApp(
    viewModel: BillsViewModel,
) {
    val state by viewModel.uiState.collectAsStateWithLifecycle()
    val sections = BillsSection.entries
    var selectedIndex by rememberSaveable { mutableIntStateOf(0) }
    val selectedSection = sections[selectedIndex]

    Scaffold(
        contentWindowInsets = WindowInsets.safeDrawing,
        bottomBar = {
            BottomSectionBar(
                sections = sections,
                selectedIndex = selectedIndex,
                onSelectedIndexChange = { selectedIndex = it },
            )
        },
    ) { innerPadding ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(
                    Brush.verticalGradient(
                        colors = listOf(
                            MaterialTheme.colorScheme.surface,
                            MaterialTheme.colorScheme.secondaryContainer,
                            MaterialTheme.colorScheme.tertiaryContainer,
                        ),
                    ),
                )
                .padding(innerPadding),
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(20.dp),
            ) {
                HeroHeader(
                    state = state,
                    sectionLabel = selectedSection.label,
                )
                Spacer(modifier = Modifier.height(18.dp))

                when (selectedSection) {
                    BillsSection.DATA -> {
                        ImportPane(
                            state = state,
                            onImport = viewModel::importBundledSample,
                            onClearDatabase = viewModel::clearDatabase,
                            modifier = Modifier
                                .fillMaxWidth()
                                .weight(1f),
                        )
                    }

                    BillsSection.RECORD -> {
                        RecordPane(
                            state = state,
                            onSelectExistingRecordYear = viewModel::selectExistingRecordYear,
                            onSelectExistingRecordMonth = viewModel::selectExistingRecordMonth,
                            onOpenSelectedExistingRecord = viewModel::openSelectedExistingRecordPeriod,
                            onOpenCurrentMonthRecord = viewModel::openCurrentMonthRecord,
                            onRecordPeriodChange = viewModel::updateRecordPeriodInput,
                            onOpenManualRecord = viewModel::openRecordPeriod,
                            onPreviewRecord = viewModel::previewRecordDraft,
                            onSaveRecord = viewModel::saveRecordDraft,
                            onRefreshRecordPeriods = viewModel::refreshRecordPeriods,
                            onRecordDraftChange = viewModel::updateRecordDraft,
                            onResetRecordDraft = viewModel::resetRecordDraft,
                            modifier = Modifier
                                .fillMaxWidth()
                                .weight(1f),
                        )
                    }

                    BillsSection.REPORT -> {
                        QueryPane(
                            state = state,
                            onRunYearQuery = viewModel::runBundledYearQuery,
                            onRunMonthQuery = viewModel::runBundledMonthQuery,
                            modifier = Modifier
                                .fillMaxWidth()
                                .weight(1f),
                        )
                    }

                    BillsSection.CONFIG -> {
                        ConfigPane(
                            state = state,
                            onSelectConfig = viewModel::selectBundledConfig,
                            onConfigDraftChange = viewModel::updateConfigDraft,
                            onModifyConfig = viewModel::saveSelectedConfig,
                            onResetConfigDraft = viewModel::resetSelectedConfigDraft,
                            onSelectThemeMode = viewModel::updateThemeModeDraft,
                            onSelectThemePalette = viewModel::updateThemePaletteDraft,
                            onApplyTheme = viewModel::applyThemeDraft,
                            onResetThemeDraft = viewModel::resetThemeDraft,
                            modifier = Modifier
                                .fillMaxWidth()
                                .weight(1f),
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun BottomSectionBar(
    sections: List<BillsSection>,
    selectedIndex: Int,
    onSelectedIndexChange: (Int) -> Unit,
) {
    NavigationBar(
        containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.97f),
        tonalElevation = 8.dp,
    ) {
        sections.forEachIndexed { index, section ->
            NavigationBarItem(
                selected = selectedIndex == index,
                onClick = { onSelectedIndexChange(index) },
                icon = {
                    Icon(
                        imageVector = section.icon,
                        contentDescription = section.label,
                    )
                },
                label = { Text(section.label) },
                colors = NavigationBarItemDefaults.colors(
                    selectedIconColor = MaterialTheme.colorScheme.onSecondaryContainer,
                    selectedTextColor = MaterialTheme.colorScheme.onSurface,
                    indicatorColor = MaterialTheme.colorScheme.secondaryContainer,
                ),
            )
        }
    }
}

@Composable
private fun HeroHeader(
    state: BillsUiState,
    sectionLabel: String,
) {
    Column(
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        Surface(
            shape = RoundedCornerShape(999.dp),
            color = MaterialTheme.colorScheme.surface.copy(alpha = 0.72f),
        ) {
            Row(
                modifier = Modifier.padding(horizontal = 12.dp, vertical = 6.dp),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                Box(
                    modifier = Modifier
                        .size(8.dp)
                        .background(
                            color = MaterialTheme.colorScheme.primary,
                            shape = RoundedCornerShape(999.dp),
                        ),
                )
                Text(
                    text = sectionLabel,
                    style = MaterialTheme.typography.labelLarge,
                )
            }
        }
        Text(
            text = state.statusMessage,
            style = MaterialTheme.typography.bodyLarge,
            fontFamily = FontFamily.Monospace,
        )
        if (state.isInitializing || state.isWorking) {
            LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
        }
        if (!state.errorMessage.isNullOrBlank()) {
            Surface(
                shape = RoundedCornerShape(18.dp),
                color = MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.92f),
            ) {
                Text(
                    text = state.errorMessage ?: "",
                    color = MaterialTheme.colorScheme.onErrorContainer,
                    style = MaterialTheme.typography.bodyMedium,
                    modifier = Modifier.padding(horizontal = 14.dp, vertical = 12.dp),
                )
            }
        }
    }
}

@Composable
private fun ImportPane(
    state: BillsUiState,
    onImport: () -> Unit,
    onClearDatabase: () -> Unit,
    modifier: Modifier = Modifier,
) {
    PaneContent(modifier = modifier) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState()),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            StatPill(
                label = "sample",
                value = state.bundledSampleLabel.ifBlank { "2025-01" },
            )
            StatPill(
                label = "db",
                value = "sqlite",
            )
        }
        Button(
            onClick = onImport,
            enabled = !state.isInitializing && !state.isWorking,
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text("Import bundled sample")
        }
        Button(
            onClick = onClearDatabase,
            enabled = !state.isInitializing && !state.isWorking,
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text("Clear database")
        }
        state.importResult?.let { result ->
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .horizontalScroll(rememberScrollState()),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                StatPill(label = "processed", value = result.processed.toString())
                StatPill(label = "imported", value = result.imported.toString())
                StatPill(label = "failure", value = result.failure.toString())
            }
            if (!result.ok && result.message.isNotBlank()) {
                Text(
                    text = result.message,
                    color = MaterialTheme.colorScheme.error,
                    style = MaterialTheme.typography.bodyMedium,
                )
            }
        }
    }
}

@Composable
private fun QueryPane(
    state: BillsUiState,
    onRunYearQuery: () -> Unit,
    onRunMonthQuery: () -> Unit,
    modifier: Modifier = Modifier,
) {
    PaneContent(modifier = modifier) {
        Button(
            onClick = onRunYearQuery,
            enabled = !state.isInitializing && !state.isWorking,
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text("Query fixed year ${state.bundledSampleYear}")
        }
        Button(
            onClick = onRunMonthQuery,
            enabled = !state.isInitializing && !state.isWorking,
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text("Query fixed month ${state.bundledSampleMonth}")
        }
        state.queryResult?.let { result ->
            QueryResultDisplay(result = result)
        }
    }
}

@Composable
private fun RecordPane(
    state: BillsUiState,
    onSelectExistingRecordYear: (String) -> Unit,
    onSelectExistingRecordMonth: (String) -> Unit,
    onOpenSelectedExistingRecord: () -> Unit,
    onOpenCurrentMonthRecord: () -> Unit,
    onRecordPeriodChange: (String) -> Unit,
    onOpenManualRecord: () -> Unit,
    onPreviewRecord: () -> Unit,
    onSaveRecord: () -> Unit,
    onRefreshRecordPeriods: () -> Unit,
    onRecordDraftChange: (String) -> Unit,
    onResetRecordDraft: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val activeRecord = state.activeRecordDocument
    val listedPeriods = state.listedRecordPeriods
    val hasUnsavedChanges = activeRecord != null && state.recordDraftText != activeRecord.rawText
    val existingYears = listedPeriods?.periods.orEmpty()
        .mapNotNull { period -> period.substringBefore('-').takeIf { it.length == 4 } }
        .distinct()
    val existingMonths = listedPeriods?.periods.orEmpty()
        .filter { period ->
            state.selectedExistingRecordYear.isNotBlank() &&
                period.startsWith("${state.selectedExistingRecordYear}-") &&
                period.length == 7
        }
        .map { period -> period.substringAfter('-') }
        .distinct()
    val currentMonth = YearMonth.now().toString()
    var yearSelectorExpanded by rememberSaveable { mutableStateOf(false) }
    var monthSelectorExpanded by rememberSaveable { mutableStateOf(false) }

    PaneContent(modifier = modifier) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState()),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            StatPill(
                label = "saved",
                value = listedPeriods?.periods?.size?.toString() ?: "0",
            )
            StatPill(
                label = "years",
                value = existingYears.size.toString(),
            )
            StatPill(
                label = "invalid",
                value = listedPeriods?.invalid?.toString() ?: "0",
            )
        }
        Text(
            text = "Saved TXT periods",
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
        )
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            Box(modifier = Modifier.weight(1f)) {
                OutlinedButton(
                    onClick = { yearSelectorExpanded = true },
                    enabled = !state.isInitializing && !state.isWorking && existingYears.isNotEmpty(),
                    modifier = Modifier
                        .fillMaxWidth()
                        .testTag("record_year_selector_button"),
                ) {
                    Text(
                        text = state.selectedExistingRecordYear.ifBlank { "Select year" },
                        fontFamily = FontFamily.Monospace,
                    )
                }
                DropdownMenu(
                    expanded = yearSelectorExpanded,
                    onDismissRequest = { yearSelectorExpanded = false },
                    modifier = Modifier.fillMaxWidth(0.9f),
                ) {
                    existingYears.forEach { year ->
                        DropdownMenuItem(
                            text = {
                                Text(
                                    text = year,
                                    fontFamily = FontFamily.Monospace,
                                )
                            },
                            onClick = {
                                yearSelectorExpanded = false
                                onSelectExistingRecordYear(year)
                            },
                        )
                    }
                }
            }
            Box(modifier = Modifier.weight(1f)) {
                OutlinedButton(
                    onClick = { monthSelectorExpanded = true },
                    enabled = !state.isInitializing && !state.isWorking && existingMonths.isNotEmpty(),
                    modifier = Modifier
                        .fillMaxWidth()
                        .testTag("record_month_selector_button"),
                ) {
                    Text(
                        text = state.selectedExistingRecordMonth.ifBlank { "Select month" },
                        fontFamily = FontFamily.Monospace,
                    )
                }
                DropdownMenu(
                    expanded = monthSelectorExpanded,
                    onDismissRequest = { monthSelectorExpanded = false },
                    modifier = Modifier.fillMaxWidth(0.9f),
                ) {
                    existingMonths.forEach { month ->
                        DropdownMenuItem(
                            text = {
                                Text(
                                    text = month,
                                    fontFamily = FontFamily.Monospace,
                                )
                            },
                            onClick = {
                                monthSelectorExpanded = false
                                onSelectExistingRecordMonth(month)
                            },
                        )
                    }
                }
            }
        }
        if (listedPeriods != null) {
            Text(
                text = listedPeriods.message,
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
        }
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            Button(
                onClick = onOpenSelectedExistingRecord,
                enabled = !state.isInitializing &&
                    !state.isWorking &&
                    state.selectedExistingRecordYear.isNotBlank() &&
                    state.selectedExistingRecordMonth.isNotBlank(),
                modifier = Modifier
                    .weight(1f)
                    .testTag("record_open_existing_button"),
            ) {
                Text("Open Existing")
            }
            OutlinedButton(
                onClick = onRefreshRecordPeriods,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .weight(1f)
                    .testTag("record_refresh_button"),
            ) {
                Text("Refresh Periods")
            }
        }
        if (listedPeriods == null || listedPeriods.periods.isEmpty()) {
            Text(
                text = "No saved record periods yet. Existing year/month choices only come from list_periods over records/*.txt content.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
        }
        if (listedPeriods != null) {
            if (listedPeriods.invalidFiles.isNotEmpty()) {
                listedPeriods.invalidFiles.forEach { invalidFile ->
                    Text(
                        text = "[INVALID] ${invalidFile.path} | ${invalidFile.error}",
                        style = MaterialTheme.typography.bodySmall,
                        fontFamily = FontFamily.Monospace,
                        color = MaterialTheme.colorScheme.error,
                    )
                }
            }
        }
        Text(
            text = "Create or open by period",
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
        )
        Button(
            onClick = onOpenCurrentMonthRecord,
            enabled = !state.isInitializing && !state.isWorking,
            modifier = Modifier
                .fillMaxWidth()
                .testTag("record_new_current_button"),
        ) {
            Text("New Current Month $currentMonth")
        }
        OutlinedTextField(
            value = state.recordPeriodInput,
            onValueChange = onRecordPeriodChange,
            enabled = !state.isInitializing && !state.isWorking,
            modifier = Modifier
                .fillMaxWidth()
                .testTag("record_manual_period_field"),
            label = {
                Text(
                    text = "Manual period (YYYY-MM)",
                    fontFamily = FontFamily.Monospace,
                )
            },
            textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
            singleLine = true,
        )
        OutlinedButton(
            onClick = onOpenManualRecord,
            enabled = !state.isInitializing && !state.isWorking && state.recordPeriodInput.isNotBlank(),
            modifier = Modifier
                .fillMaxWidth()
                .testTag("record_open_manual_button"),
        ) {
            Text("Open Manual Period")
        }
        if (activeRecord == null) {
            Text(
                text = "Open an existing TXT period from the selectors above, or create a new template from the current month / manual YYYY-MM input.",
                style = MaterialTheme.typography.bodyMedium,
                fontFamily = FontFamily.Monospace,
            )
        } else {
            Text(
                text = if (activeRecord.persisted) {
                    "Editing persisted TXT source: ${activeRecord.relativePath}"
                } else {
                    "Editing generated template: ${activeRecord.relativePath}"
                },
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            Text(
                text = if (hasUnsavedChanges) {
                    "Draft changes are local until you press Save Record."
                } else {
                    "Editor is synced with the currently opened TXT source."
                },
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            OutlinedTextField(
                value = state.recordDraftText,
                onValueChange = onRecordDraftChange,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .heightIn(min = 280.dp, max = 460.dp)
                    .testTag("record_editor_field"),
                textStyle = MaterialTheme.typography.bodyMedium.copy(fontFamily = FontFamily.Monospace),
                label = {
                    Text(
                        text = activeRecord.relativePath,
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
                    onClick = onPreviewRecord,
                    enabled = !state.isInitializing && !state.isWorking && state.recordDraftText.isNotBlank(),
                    modifier = Modifier
                        .weight(1f)
                        .testTag("record_preview_button"),
                ) {
                    Text("Preview")
                }
                Button(
                    onClick = onSaveRecord,
                    enabled = !state.isInitializing && !state.isWorking && hasUnsavedChanges,
                    modifier = Modifier
                        .weight(1f)
                        .testTag("record_save_button"),
                ) {
                    Text("Save Record")
                }
                OutlinedButton(
                    onClick = onResetRecordDraft,
                    enabled = !state.isInitializing && !state.isWorking && hasUnsavedChanges,
                    modifier = Modifier
                        .weight(1f)
                        .testTag("record_reset_button"),
                ) {
                    Text("Reset Draft")
                }
            }
        }
        state.recordPreviewResult?.let { preview ->
            RecordPreviewBlock(preview = preview)
        }
    }
}

@Composable
private fun RecordPreviewBlock(preview: com.billstracer.android.model.RecordPreviewResult) {
    Column(
        verticalArrangement = Arrangement.spacedBy(8.dp),
    ) {
        Text(
            text = "Preview",
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
        )
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState()),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            StatPill(label = "processed", value = preview.processed.toString())
            StatPill(label = "success", value = preview.success.toString())
            StatPill(label = "failure", value = preview.failure.toString())
        }
        if (preview.periods.isNotEmpty()) {
            Text(
                text = "Periods: ${preview.periods.joinToString(", ")}",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
        }
        preview.files.forEach { file ->
            Surface(
                modifier = Modifier.fillMaxWidth(),
                shape = RoundedCornerShape(18.dp),
                color = if (file.ok) {
                    MaterialTheme.colorScheme.secondaryContainer.copy(alpha = 0.78f)
                } else {
                    MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.9f)
                },
            ) {
                Column(
                    modifier = Modifier.padding(12.dp),
                    verticalArrangement = Arrangement.spacedBy(6.dp),
                ) {
                    Text(
                        text = file.path,
                        style = MaterialTheme.typography.bodySmall,
                        fontFamily = FontFamily.Monospace,
                        color = if (file.ok) {
                            MaterialTheme.colorScheme.onSecondaryContainer
                        } else {
                            MaterialTheme.colorScheme.onErrorContainer
                        },
                    )
                    Text(
                        text = if (file.ok) {
                            "period=${file.period} txns=${file.transactionCount} income=${file.totalIncome} expense=${file.totalExpense} balance=${file.balance}"
                        } else {
                            file.error ?: "Preview failed."
                        },
                        style = MaterialTheme.typography.bodyMedium,
                        fontFamily = FontFamily.Monospace,
                        color = if (file.ok) {
                            MaterialTheme.colorScheme.onSecondaryContainer
                        } else {
                            MaterialTheme.colorScheme.onErrorContainer
                        },
                    )
                }
            }
        }
    }
}

@Composable
private fun ConfigPane(
    state: BillsUiState,
    onSelectConfig: (String) -> Unit,
    onConfigDraftChange: (String) -> Unit,
    onModifyConfig: () -> Unit,
    onResetConfigDraft: () -> Unit,
    onSelectThemeMode: (ThemeMode) -> Unit,
    onSelectThemePalette: (ThemePalette) -> Unit,
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
                    onSelectThemePalette = onSelectThemePalette,
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

@Composable
private fun ThemeSettingsBlock(
    state: BillsUiState,
    onSelectThemeMode: (ThemeMode) -> Unit,
    onSelectThemePalette: (ThemePalette) -> Unit,
    onApplyTheme: () -> Unit,
    onResetThemeDraft: () -> Unit,
) {
    val hasUnsavedChanges = state.themeDraft != state.themePreferences

    Column(
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
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
                    testTag = "theme_mode_${mode.name.lowercase()}",
                )
            }
        }
        Text(
            text = "Palette",
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
        )
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState()),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            ThemePalette.entries.forEach { palette ->
                ConfigModeChip(
                    label = palette.displayName,
                    selected = state.themeDraft.palette == palette,
                    onClick = { onSelectThemePalette(palette) },
                    testTag = "theme_palette_${palette.name.lowercase()}",
                )
            }
        }
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
                    .testTag("theme_apply_button"),
            ) {
                Text("Apply Theme")
            }
            OutlinedButton(
                onClick = onResetThemeDraft,
                enabled = !state.isInitializing && !state.isWorking && hasUnsavedChanges,
                modifier = Modifier
                    .weight(1f)
                    .testTag("theme_reset_button"),
            ) {
                Text("Reset Theme")
            }
        }
    }
}

@Composable
private fun ThemePreviewCard(
    themePreferences: ThemePreferences,
) {
    val previewDarkTheme = resolveDarkTheme(
        mode = themePreferences.mode,
        systemDarkTheme = isSystemInDarkTheme(),
    )
    val previewColors = billsColorScheme(
        palette = themePreferences.palette,
        darkTheme = previewDarkTheme,
    )

    Surface(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("theme_preview_card"),
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
                text = "${themePreferences.palette.displayName} / ${themePreferences.mode.displayName}",
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
                        .background(previewColors.secondary, RoundedCornerShape(999.dp)),
                )
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .height(18.dp)
                        .background(previewColors.tertiary, RoundedCornerShape(999.dp)),
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
private fun AboutBlock(notices: BundledNotices) {
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
private fun VersionInfoBlock(
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
private fun PaneContent(
    modifier: Modifier = Modifier,
    content: @Composable ColumnScope.() -> Unit,
) {
    Column(
        modifier = modifier.verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(12.dp),
        content = content,
    )
}

@Composable
private fun StatPill(label: String, value: String) {
    Surface(
        shape = RoundedCornerShape(999.dp),
        color = MaterialTheme.colorScheme.primary.copy(alpha = 0.12f),
    ) {
        Column(
            modifier = Modifier
                .widthIn(min = 88.dp)
                .padding(horizontal = 12.dp, vertical = 8.dp),
        ) {
            Text(label, style = MaterialTheme.typography.labelSmall)
            Text(
                text = value,
                style = MaterialTheme.typography.labelLarge,
                fontFamily = FontFamily.Monospace,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
        }
    }
}

@Composable
private fun ConfigModeChip(
    label: String,
    selected: Boolean,
    onClick: () -> Unit,
    testTag: String,
) {
    if (selected) {
        Button(
            onClick = onClick,
            modifier = Modifier
                .testTag(testTag),
        ) {
            Text(
                text = label,
                fontFamily = FontFamily.Monospace,
            )
        }
    } else {
        OutlinedButton(
            onClick = onClick,
            modifier = Modifier
                .testTag(testTag),
        ) {
            Text(
                text = label,
                fontFamily = FontFamily.Monospace,
            )
        }
    }
}

@Composable
private fun ConfigEditorBlock(
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
