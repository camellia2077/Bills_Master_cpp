package com.billstracer.android.app.navigation

import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Icon
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.saveable.rememberSaveableStateHolder
import androidx.compose.runtime.setValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.billstracer.android.R
import com.billstracer.android.features.editor.EditorScreen
import com.billstracer.android.features.editor.EditorViewModel
import com.billstracer.android.features.query.QueryScreen
import com.billstracer.android.features.query.QueryViewModel
import com.billstracer.android.features.settings.SettingsScreen
import com.billstracer.android.features.settings.SettingsViewModel
import com.billstracer.android.features.workspace.WorkspaceScreen
import com.billstracer.android.features.workspace.WorkspaceViewModel
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter

private enum class AppTab(
    val label: String,
    val testTag: String,
    val iconResId: Int,
) {
    WORKSPACE(
        label = "Workspace",
        testTag = "tab_workspace",
        iconResId = R.drawable.ic_section_data,
    ),
    EDITOR(
        label = "Editor",
        testTag = "tab_editor",
        iconResId = R.drawable.ic_section_record,
    ),
    QUERY(
        label = "Query",
        testTag = "tab_query",
        iconResId = R.drawable.ic_section_report,
    ),
    SETTINGS(
        label = "Settings",
        testTag = "tab_settings",
        iconResId = R.drawable.ic_section_config,
    ),
}

private fun titleForTab(tab: AppTab): String = when (tab) {
    AppTab.WORKSPACE -> "Workspace"
    AppTab.EDITOR -> "Editor"
    AppTab.QUERY -> "Query"
    AppTab.SETTINGS -> "Settings"
}

@Composable
internal fun BillsAndroidApp(
    sessionViewModel: AppSessionViewModel,
    workspaceViewModel: WorkspaceViewModel,
    queryViewModel: QueryViewModel,
    editorViewModel: EditorViewModel,
    settingsViewModel: SettingsViewModel,
) {
    // Start in Editor because the app is centered on the structure-locked
    // monthly ledger workflow rather than a general dashboard landing page.
    val sessionState = sessionViewModel.state.collectAsStateWithLifecycle()
    var selectedTab by rememberSaveable { mutableStateOf(AppTab.EDITOR) }
    val importTxtDirectoryLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocumentTree(),
    ) { sourceDirectoryUri ->
        if (sourceDirectoryUri != null) {
            workspaceViewModel.importTxtDirectoryAndSyncDatabase(sourceDirectoryUri)
        }
    }
    val exportWorkspaceLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.CreateDocument("application/zip"),
    ) { targetDocumentUri ->
        if (targetDocumentUri != null) {
            workspaceViewModel.exportWorkspace(targetDocumentUri)
        }
    }
    val exportBackupBundleLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.CreateDocument("application/zip"),
    ) { targetDocumentUri ->
        if (targetDocumentUri != null) {
            settingsViewModel.exportBackupBundle(targetDocumentUri)
        }
    }
    val importBackupBundleLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocument(),
    ) { sourceDocumentUri ->
        if (sourceDocumentUri != null) {
            // Workspace owns restore so import/export/clear actions stay grouped
            // around the private workspace lifecycle instead of Settings.
            workspaceViewModel.importBackupBundle(sourceDocumentUri)
        }
    }
    val tabStateHolder = rememberSaveableStateHolder()

    Scaffold(
        modifier = Modifier.fillMaxSize(),
        topBar = {
            AppTopBar(title = titleForTab(selectedTab))
        },
        bottomBar = {
            AppBottomBar(
                selectedTab = selectedTab,
                onSelectTab = { selectedTab = it },
            )
        },
    ) { innerPadding ->
        val contentModifier = Modifier
            .fillMaxSize()
            .padding(innerPadding)
            .padding(horizontal = 20.dp, vertical = 16.dp)

        tabStateHolder.SaveableStateProvider(selectedTab) {
            when (selectedTab) {
                AppTab.WORKSPACE -> {
                    val state = workspaceViewModel.state.collectAsStateWithLifecycle()
                    WorkspaceScreen(
                        sessionState = sessionState.value,
                        state = state.value,
                        onRequestImportTxtDirectory = {
                            importTxtDirectoryLauncher.launch(null)
                        },
                        onRequestExportWorkspace = {
                            exportWorkspaceLauncher.launch(
                                "workspace_${DateTimeFormatter.ofPattern("yyyyMMdd_HHmmss").format(LocalDateTime.now())}.zip",
                            )
                        },
                        onRequestRestoreWorkspace = {
                            importBackupBundleLauncher.launch(
                                arrayOf("application/zip", "application/octet-stream"),
                            )
                        },
                        onClearRecordFiles = workspaceViewModel::clearRecordFiles,
                        onClearDatabase = workspaceViewModel::clearDatabase,
                        modifier = contentModifier,
                    )
                }
                AppTab.QUERY -> {
                    val state = queryViewModel.state.collectAsStateWithLifecycle()
                    QueryScreen(
                        state = state.value,
                        onSelectQueryYear = queryViewModel::selectQueryYear,
                        onSelectQueryPeriodYear = queryViewModel::selectQueryPeriodYear,
                        onSelectQueryPeriodMonth = queryViewModel::selectQueryPeriodMonth,
                        onRunYearQuery = queryViewModel::runYearQuery,
                        onRunMonthQuery = queryViewModel::runMonthQuery,
                        onSelectQueryViewMode = queryViewModel::selectQueryViewMode,
                        modifier = contentModifier,
                    )
                }
                AppTab.EDITOR -> {
                    val state = editorViewModel.state.collectAsStateWithLifecycle()
                    EditorScreen(
                        state = state.value,
                        onScreenShown = editorViewModel::onEditorScreenShown,
                        onSelectExistingRecordYear = editorViewModel::selectExistingRecordYear,
                        onSelectExistingRecordMonth = editorViewModel::selectExistingRecordMonth,
                        onSaveStructuredRecord = editorViewModel::saveRecordDraft,
                        onSaveRawRecordText = editorViewModel::saveRawRecordText,
                        onRecordDraftChange = editorViewModel::updateRecordDraft,
                        onStructuredRemarkChange = editorViewModel::updateStructuredRemark,
                        onAddStructuredEntry = editorViewModel::addStructuredEntry,
                        onRemoveStructuredEntry = editorViewModel::removeStructuredEntry,
                        onStructuredEntryAmountChange = editorViewModel::updateStructuredEntryAmount,
                        onStructuredEntryDescriptionChange = editorViewModel::updateStructuredEntryDescription,
                        onStructuredEntryCommentChange = editorViewModel::updateStructuredEntryComment,
                        onEnterRawExpertMode = editorViewModel::enterRawExpertMode,
                        onReturnToStructuredMode = editorViewModel::returnToStructuredMode,
                        modifier = contentModifier,
                    )
                }
                AppTab.SETTINGS -> {
                    val state = settingsViewModel.state.collectAsStateWithLifecycle()
                    SettingsScreen(
                        state = state.value,
                        onSelectConfig = settingsViewModel::selectBundledConfig,
                        onConfigDraftChange = settingsViewModel::updateConfigDraft,
                        onModifyConfig = settingsViewModel::saveSelectedConfig,
                        onResetConfigDraft = settingsViewModel::resetSelectedConfigDraft,
                        onRequestExportBackup = {
                            exportBackupBundleLauncher.launch(
                                "bills_backup_${DateTimeFormatter.ofPattern("yyyyMMdd_HHmmss").format(LocalDateTime.now())}.zip",
                            )
                        },
                        onSelectThemeMode = settingsViewModel::updateThemeModeDraft,
                        onSelectThemeColor = settingsViewModel::updateThemeColorDraft,
                        onApplyTheme = settingsViewModel::applyThemeDraft,
                        onResetThemeDraft = settingsViewModel::resetThemeDraft,
                        modifier = contentModifier,
                    )
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun AppTopBar(title: String) {
    TopAppBar(
        title = { Text(title) },
    )
}

@Composable
private fun AppBottomBar(
    selectedTab: AppTab,
    onSelectTab: (AppTab) -> Unit,
) {
    NavigationBar {
        AppTab.entries.forEach { tab ->
            NavigationBarItem(
                modifier = Modifier.testTag(tab.testTag),
                selected = selectedTab == tab,
                onClick = { onSelectTab(tab) },
                icon = {
                    Icon(
                        painter = painterResource(id = tab.iconResId),
                        contentDescription = tab.label,
                    )
                },
                label = { Text(tab.label) },
                colors = NavigationBarItemDefaults.colors(
                    selectedIconColor = Color.White,
                    unselectedIconColor = Color.Black,
                    selectedTextColor = androidx.compose.material3.MaterialTheme.colorScheme.onSurface,
                    unselectedTextColor = androidx.compose.material3.MaterialTheme.colorScheme.onSurface,
                    indicatorColor = androidx.compose.material3.MaterialTheme.colorScheme.primary,
                ),
            )
        }
    }
}
