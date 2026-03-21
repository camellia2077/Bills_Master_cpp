package com.billstracer.android.ui

import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.billstracer.android.R
import com.billstracer.android.ui.app.AppSectionItem
import com.billstracer.android.ui.app.BottomSectionBar
import com.billstracer.android.ui.app.HeroHeader
import com.billstracer.android.ui.config.ConfigPane
import com.billstracer.android.ui.data.DataPane
import com.billstracer.android.ui.record.RecordPane
import com.billstracer.android.ui.report.QueryPane

private enum class BillsSection(
    val label: String,
    val iconResId: Int,
) {
    DATA("Data", R.drawable.ic_section_data),
    RECORD("Record", R.drawable.ic_section_record),
    REPORT("Report", R.drawable.ic_section_report),
    CONFIG("Config", R.drawable.ic_section_config),
}

@Composable
fun BillsApp(
    viewModel: BillsViewModel,
) {
    val state by viewModel.uiState.collectAsStateWithLifecycle()
    val exportDirectoryLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocumentTree(),
    ) { targetDirectoryUri ->
        if (targetDirectoryUri != null) {
            viewModel.exportRecordFiles(targetDirectoryUri)
        }
    }
    val sections = BillsSection.entries.map { section ->
        AppSectionItem(
            label = section.label,
            iconResId = section.iconResId,
        )
    }
    var selectedIndex by rememberSaveable { mutableIntStateOf(0) }
    val selectedSection = BillsSection.entries[selectedIndex]

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
                .background(MaterialTheme.colorScheme.background)
                .padding(innerPadding),
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(20.dp),
            ) {
                HeroHeader(state = state)
                Spacer(modifier = Modifier.height(18.dp))

                when (selectedSection) {
                    BillsSection.DATA -> DataPane(
                        state = state,
                        onImport = viewModel::importBundledSample,
                        onExportRecordFiles = { exportDirectoryLauncher.launch(null) },
                        onClearRecordFiles = viewModel::clearRecordFiles,
                        onClearDatabase = viewModel::clearDatabase,
                        modifier = Modifier
                            .fillMaxWidth()
                            .weight(1f),
                    )

                    BillsSection.RECORD -> RecordPane(
                        state = state,
                        onSelectExistingRecordYear = viewModel::selectExistingRecordYear,
                        onSelectExistingRecordMonth = viewModel::selectExistingRecordMonth,
                        onOpenSelectedExistingRecord = viewModel::openSelectedExistingRecordPeriod,
                        onRecordPeriodYearChange = viewModel::updateRecordPeriodYearInput,
                        onRecordPeriodMonthChange = viewModel::updateRecordPeriodMonthInput,
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

                    BillsSection.REPORT -> QueryPane(
                        state = state,
                        onQueryYearChange = viewModel::updateQueryYearInput,
                        onQueryPeriodYearChange = viewModel::updateQueryPeriodYearInput,
                        onQueryPeriodMonthChange = viewModel::updateQueryPeriodMonthInput,
                        onRunYearQuery = viewModel::runBundledYearQuery,
                        onRunMonthQuery = viewModel::runBundledMonthQuery,
                        modifier = Modifier
                            .fillMaxWidth()
                            .weight(1f),
                    )

                    BillsSection.CONFIG -> ConfigPane(
                        state = state,
                        onSelectConfig = viewModel::selectBundledConfig,
                        onConfigDraftChange = viewModel::updateConfigDraft,
                        onModifyConfig = viewModel::saveSelectedConfig,
                        onResetConfigDraft = viewModel::resetSelectedConfigDraft,
                        onSelectThemeMode = viewModel::updateThemeModeDraft,
                        onSelectThemeColor = viewModel::updateThemeColorDraft,
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
