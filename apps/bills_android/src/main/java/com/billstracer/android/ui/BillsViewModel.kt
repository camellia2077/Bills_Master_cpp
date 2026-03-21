package com.billstracer.android.ui

import android.net.Uri
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.BuildConfig
import com.billstracer.android.data.BillsRepository
import com.billstracer.android.model.BillsUiState
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.ui.common.withManualRecordPeriod
import com.billstracer.android.ui.common.withQueryPeriod
import com.billstracer.android.ui.common.withQueryYearInput
import com.billstracer.android.ui.config.applyThemeDraftAction
import com.billstracer.android.ui.config.resetSelectedConfigDraftAction
import com.billstracer.android.ui.config.resetThemeDraftAction
import com.billstracer.android.ui.config.saveSelectedConfigAction
import com.billstracer.android.ui.config.selectBundledConfigAction
import com.billstracer.android.ui.config.updateConfigDraftAction
import com.billstracer.android.ui.config.updateThemeColorDraftAction
import com.billstracer.android.ui.config.updateThemeModeDraftAction
import com.billstracer.android.ui.data.clearDatabaseAction
import com.billstracer.android.ui.data.exportRecordFilesAction
import com.billstracer.android.ui.data.importBundledSampleAction
import com.billstracer.android.ui.record.applyExistingRecordSelection
import com.billstracer.android.ui.record.applyRecordPeriodInput
import com.billstracer.android.ui.record.applyRecordPeriodMonthInput
import com.billstracer.android.ui.record.applyRecordPeriodYearInput
import com.billstracer.android.ui.record.clearRecordFilesAction
import com.billstracer.android.ui.record.openCurrentMonthRecordAction
import com.billstracer.android.ui.record.openRecordPeriodAction
import com.billstracer.android.ui.record.openSelectedExistingRecordPeriodAction
import com.billstracer.android.ui.record.previewRecordDraftAction
import com.billstracer.android.ui.record.refreshRecordPeriodsAction
import com.billstracer.android.ui.record.resetRecordDraftAction
import com.billstracer.android.ui.record.saveRecordDraftAction
import com.billstracer.android.ui.record.selectExistingRecordMonthAction
import com.billstracer.android.ui.record.selectExistingRecordYearAction
import com.billstracer.android.ui.record.updateRecordDraftAction
import com.billstracer.android.ui.report.applyQueryPeriodMonthInput
import com.billstracer.android.ui.report.applyQueryPeriodYearInput
import com.billstracer.android.ui.report.applyQueryYearInput
import com.billstracer.android.ui.report.runBundledMonthQueryAction
import com.billstracer.android.ui.report.runBundledYearQueryAction
import java.time.YearMonth
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

class BillsViewModel(
    internal val repository: BillsRepository,
) : ViewModel() {
    internal val mutableUiState = MutableStateFlow(BillsUiState())
    val uiState: StateFlow<BillsUiState> = mutableUiState.asStateFlow()

    init {
        initialize()
    }

    fun initialize() {
        viewModelScope.launch {
            updateUiState {
                it.copy(
                    isInitializing = true,
                    errorMessage = null,
                    statusMessage = if (BuildConfig.DEBUG) {
                        "Preparing bundled samples and private workspace..."
                    } else {
                        "Preparing private workspace..."
                    },
                )
            }
            runCatching { repository.initialize() }
                .onSuccess { environment ->
                    val bundledConfigs = environment.bundledConfigs
                    val currentPeriod = YearMonth.now().toString()
                    val listedRecordPeriods = runCatching {
                        repository.listRecordPeriods()
                    }.getOrNull()
                    updateUiState {
                        applyExistingRecordSelection(
                            it.copy(
                                isInitializing = false,
                                bundledSampleLabel = environment.bundledSampleLabel,
                                bundledSampleYear = environment.bundledSampleYear,
                                bundledSampleMonth = environment.bundledSampleMonth,
                                coreVersion = environment.coreVersion,
                                androidVersion = environment.androidVersion,
                                bundledConfigs = bundledConfigs,
                                selectedConfigFileName = bundledConfigs.firstOrNull()?.fileName.orEmpty(),
                                configDrafts = bundledConfigs.associate { config ->
                                    config.fileName to config.rawText
                                },
                                listedRecordPeriods = listedRecordPeriods,
                                themePreferences = environment.themePreferences,
                                themeDraft = environment.themePreferences,
                                bundledNotices = environment.bundledNotices,
                                statusMessage = if (BuildConfig.DEBUG) {
                                    "Ready to import ${environment.bundledSampleLabel} into ${environment.dbFile.name}."
                                } else {
                                    "Private workspace ready: ${environment.dbFile.name}."
                                },
                            ),
                            listedPeriods = listedRecordPeriods,
                            preferredPeriod = currentPeriod,
                        ).withManualRecordPeriod(currentPeriod)
                            .withQueryYearInput(environment.bundledSampleYear)
                            .withQueryPeriod(environment.bundledSampleMonth)
                    }
                }
                .onFailure { error ->
                    updateUiState {
                        it.copy(
                            isInitializing = false,
                            errorMessage = error.message ?: "Failed to prepare workspace.",
                            statusMessage = "Workspace setup failed.",
                        )
                    }
                }
        }
    }

    fun updateRecordPeriodInput(period: String) = applyRecordPeriodInput(period)

    fun updateRecordPeriodYearInput(year: String) = applyRecordPeriodYearInput(year)

    fun updateRecordPeriodMonthInput(month: String) = applyRecordPeriodMonthInput(month)

    fun updateQueryYearInput(year: String) = applyQueryYearInput(year)

    fun updateQueryPeriodYearInput(year: String) = applyQueryPeriodYearInput(year)

    fun updateQueryPeriodMonthInput(month: String) = applyQueryPeriodMonthInput(month)

    fun selectExistingRecordYear(year: String) = selectExistingRecordYearAction(year)

    fun selectExistingRecordMonth(month: String) = selectExistingRecordMonthAction(month)

    fun openRecordPeriod() = openRecordPeriodAction()

    fun openCurrentMonthRecord() = openCurrentMonthRecordAction()

    fun openSelectedExistingRecordPeriod() = openSelectedExistingRecordPeriodAction()

    fun updateRecordDraft(rawText: String) = updateRecordDraftAction(rawText)

    fun resetRecordDraft() = resetRecordDraftAction()

    fun previewRecordDraft() = previewRecordDraftAction()

    fun saveRecordDraft() = saveRecordDraftAction()

    fun refreshRecordPeriods() = refreshRecordPeriodsAction()

    fun updateThemeModeDraft(mode: ThemeMode) = updateThemeModeDraftAction(mode)

    fun updateThemeColorDraft(color: ThemeColor) = updateThemeColorDraftAction(color)

    fun resetThemeDraft() = resetThemeDraftAction()

    fun applyThemeDraft() = applyThemeDraftAction()

    fun selectBundledConfig(fileName: String) = selectBundledConfigAction(fileName)

    fun updateConfigDraft(rawText: String) = updateConfigDraftAction(rawText)

    fun resetSelectedConfigDraft() = resetSelectedConfigDraftAction()

    fun saveSelectedConfig() = saveSelectedConfigAction()

    fun importBundledSample() = importBundledSampleAction()

    fun exportRecordFiles(targetDirectoryUri: Uri) = exportRecordFilesAction(targetDirectoryUri)

    fun runBundledYearQuery() = runBundledYearQueryAction()

    fun runBundledMonthQuery() = runBundledMonthQueryAction()

    fun clearDatabase() = clearDatabaseAction()

    fun clearRecordFiles() = clearRecordFilesAction()
}

internal val BillsViewModel.currentState: BillsUiState
    get() = mutableUiState.value

internal fun BillsViewModel.updateUiState(transform: (BillsUiState) -> BillsUiState) {
    mutableUiState.update(transform)
}

class BillsViewModelFactory(
    private val repository: BillsRepository,
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return BillsViewModel(repository) as T
    }
}
