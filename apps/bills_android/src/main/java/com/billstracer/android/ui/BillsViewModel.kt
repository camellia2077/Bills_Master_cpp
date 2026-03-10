package com.billstracer.android.ui

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.data.BillsRepository
import com.billstracer.android.model.BillsUiState
import com.billstracer.android.model.ListedRecordPeriodsResult
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePalette
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import java.time.YearMonth

class BillsViewModel(
    private val repository: BillsRepository,
) : ViewModel() {
    private data class ExistingRecordSelection(
        val year: String = "",
        val month: String = "",
    )

    private val _uiState = MutableStateFlow(BillsUiState())
    val uiState: StateFlow<BillsUiState> = _uiState.asStateFlow()

    init {
        initialize()
    }

    fun initialize() {
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isInitializing = true,
                    errorMessage = null,
                    statusMessage = "Preparing bundled samples and private workspace...",
                )
            }
            runCatching { repository.initialize() }
                .onSuccess { environment ->
                    val bundledConfigs = environment.bundledConfigs
                    val currentPeriod = YearMonth.now().toString()
                    val listedRecordPeriods = runCatching {
                        repository.listRecordPeriods()
                    }.getOrNull()
                    _uiState.update {
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
                            recordPeriodInput = currentPeriod,
                            listedRecordPeriods = listedRecordPeriods,
                            themePreferences = environment.themePreferences,
                            themeDraft = environment.themePreferences,
                            bundledNotices = environment.bundledNotices,
                            statusMessage = "Ready to import ${environment.bundledSampleLabel} into ${environment.dbFile.name}.",
                            ),
                            listedPeriods = listedRecordPeriods,
                            preferredPeriod = currentPeriod,
                        )
                    }
                }
                .onFailure { error ->
                    _uiState.update {
                        it.copy(
                            isInitializing = false,
                            errorMessage = error.message ?: "Failed to prepare workspace.",
                            statusMessage = "Workspace setup failed.",
                        )
                    }
                }
        }
    }

    fun updateRecordPeriodInput(period: String) {
        _uiState.update { state ->
            state.copy(recordPeriodInput = period)
        }
    }

    fun selectExistingRecordYear(year: String) {
        _uiState.update { state ->
            val months = monthsForYear(state.listedRecordPeriods, year)
            val selectedMonth = if (months.contains(state.selectedExistingRecordMonth)) {
                state.selectedExistingRecordMonth
            } else {
                months.firstOrNull().orEmpty()
            }
            state.copy(
                selectedExistingRecordYear = year,
                selectedExistingRecordMonth = selectedMonth,
            )
        }
    }

    fun selectExistingRecordMonth(month: String) {
        _uiState.update { state ->
            state.copy(selectedExistingRecordMonth = month)
        }
    }

    fun openRecordPeriod() {
        val period = _uiState.value.recordPeriodInput.trim()
        if (period.isBlank()) {
            return
        }
        openRecordPeriodInternal(period)
    }

    fun openCurrentMonthRecord() {
        val currentPeriod = YearMonth.now().toString()
        _uiState.update { state -> state.copy(recordPeriodInput = currentPeriod) }
        openRecordPeriodInternal(currentPeriod)
    }

    fun openSelectedExistingRecordPeriod() {
        val state = _uiState.value
        val period = composePeriod(
            year = state.selectedExistingRecordYear,
            month = state.selectedExistingRecordMonth,
        ) ?: return
        openRecordPeriodInternal(period)
    }

    private fun openRecordPeriodInternal(period: String) {
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Opening record period $period...",
                )
            }
            runCatching { repository.openRecordPeriod(period) }
                .onSuccess { document ->
                    val listedRecordPeriods = runCatching {
                        repository.listRecordPeriods()
                    }.getOrNull() ?: _uiState.value.listedRecordPeriods
                    _uiState.update {
                        applyExistingRecordSelection(
                            it.copy(
                            isWorking = false,
                            recordPeriodInput = document.period,
                            activeRecordDocument = document,
                            recordDraftText = document.rawText,
                            recordPreviewResult = null,
                            statusMessage = if (document.persisted) {
                                "Loaded persisted record ${document.period}."
                            } else {
                                "Generated a fresh record template for ${document.period}."
                            },
                            ),
                            listedPeriods = listedRecordPeriods,
                            preferredPeriod = document.period,
                        )
                    }
                }
                .onFailure { error ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            errorMessage = error.message ?: "Failed to open record period.",
                            statusMessage = "Failed to open record period.",
                        )
                    }
                }
        }
    }

    fun updateRecordDraft(rawText: String) {
        _uiState.update { state ->
            state.copy(recordDraftText = rawText)
        }
    }

    fun resetRecordDraft() {
        _uiState.update { state ->
            val activeRecord = state.activeRecordDocument ?: return@update state
            state.copy(
                recordDraftText = activeRecord.rawText,
                statusMessage = "Restored the draft for ${activeRecord.period}.",
            )
        }
    }

    fun previewRecordDraft() {
        val state = _uiState.value
        val activeRecord = state.activeRecordDocument ?: return
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Previewing ${activeRecord.period} through core record_preview...",
                )
            }
            runCatching { repository.previewRecordDocument(activeRecord.period, state.recordDraftText) }
                .onSuccess { preview ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            recordPreviewResult = preview,
                            errorMessage = if (preview.ok) null else preview.message,
                            statusMessage = if (preview.ok) {
                                "Previewed ${preview.success} record file(s) successfully."
                            } else {
                                preview.message
                            },
                        )
                    }
                }
                .onFailure { error ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            errorMessage = error.message ?: "Failed to preview record draft.",
                            statusMessage = "Failed to preview record draft.",
                        )
                    }
                }
        }
    }

    fun saveRecordDraft() {
        val state = _uiState.value
        val activeRecord = state.activeRecordDocument ?: return
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Saving ${activeRecord.period} into private records...",
                )
            }
            runCatching {
                val savedDocument = repository.saveRecordDocument(activeRecord.period, state.recordDraftText)
                val listedPeriods = repository.listRecordPeriods()
                savedDocument to listedPeriods
            }.onSuccess { (savedDocument, listedPeriods) ->
                _uiState.update {
                    applyExistingRecordSelection(
                        it.copy(
                        isWorking = false,
                        activeRecordDocument = savedDocument,
                        recordDraftText = savedDocument.rawText,
                        statusMessage = "Saved ${savedDocument.relativePath}.",
                        ),
                        listedPeriods = listedPeriods,
                        preferredPeriod = savedDocument.period,
                    )
                }
            }.onFailure { error ->
                _uiState.update {
                    it.copy(
                        isWorking = false,
                        errorMessage = error.message ?: "Failed to save record draft.",
                        statusMessage = "Failed to save record draft.",
                    )
                }
            }
        }
    }

    fun refreshRecordPeriods() {
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Scanning private record periods...",
                )
            }
            runCatching { repository.listRecordPeriods() }
                .onSuccess { listedPeriods ->
                    _uiState.update {
                        applyExistingRecordSelection(
                            it.copy(
                            isWorking = false,
                            errorMessage = if (listedPeriods.ok) null else listedPeriods.message,
                            statusMessage = listedPeriods.message,
                            ),
                            listedPeriods = listedPeriods,
                            preferredPeriod = it.activeRecordDocument?.period,
                        )
                    }
                }
                .onFailure { error ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            errorMessage = error.message ?: "Failed to scan record periods.",
                            statusMessage = "Failed to scan record periods.",
                        )
                    }
                }
        }
    }

    private fun applyExistingRecordSelection(
        state: BillsUiState,
        listedPeriods: ListedRecordPeriodsResult?,
        preferredPeriod: String? = state.activeRecordDocument?.period,
    ): BillsUiState {
        val selection = resolveExistingRecordSelection(
            currentYear = state.selectedExistingRecordYear,
            currentMonth = state.selectedExistingRecordMonth,
            listedPeriods = listedPeriods,
            preferredPeriod = preferredPeriod,
        )
        return state.copy(
            listedRecordPeriods = listedPeriods,
            selectedExistingRecordYear = selection.year,
            selectedExistingRecordMonth = selection.month,
        )
    }

    private fun resolveExistingRecordSelection(
        currentYear: String,
        currentMonth: String,
        listedPeriods: ListedRecordPeriodsResult?,
        preferredPeriod: String?,
    ): ExistingRecordSelection {
        val years = yearsFromPeriods(listedPeriods)
        if (years.isEmpty()) {
            return ExistingRecordSelection()
        }

        val preferredYear = preferredPeriod?.substringBefore('-', "")
        val preferredMonth = preferredPeriod?.substringAfter('-', "")
        val selectedYear = when {
            !preferredYear.isNullOrBlank() && years.contains(preferredYear) -> preferredYear
            currentYear.isNotBlank() && years.contains(currentYear) -> currentYear
            else -> years.first()
        }

        val months = monthsForYear(listedPeriods, selectedYear)
        val selectedMonth = when {
            !preferredMonth.isNullOrBlank() &&
                preferredYear == selectedYear &&
                months.contains(preferredMonth) -> preferredMonth

            currentMonth.isNotBlank() && months.contains(currentMonth) -> currentMonth
            else -> months.firstOrNull().orEmpty()
        }

        return ExistingRecordSelection(
            year = selectedYear,
            month = selectedMonth,
        )
    }

    private fun yearsFromPeriods(listedPeriods: ListedRecordPeriodsResult?): List<String> =
        listedPeriods?.periods.orEmpty()
            .mapNotNull { period -> period.substringBefore('-').takeIf { it.length == 4 } }
            .distinct()

    private fun monthsForYear(
        listedPeriods: ListedRecordPeriodsResult?,
        year: String,
    ): List<String> = listedPeriods?.periods.orEmpty()
        .filter { period -> period.startsWith("$year-") && period.length == 7 }
        .map { period -> period.substringAfter('-') }
        .distinct()

    private fun composePeriod(year: String, month: String): String? =
        if (year.length == 4 && month.length == 2) {
            "$year-$month"
        } else {
            null
        }

    fun updateThemeModeDraft(mode: ThemeMode) {
        _uiState.update { state ->
            state.copy(themeDraft = state.themeDraft.copy(mode = mode))
        }
    }

    fun updateThemePaletteDraft(palette: ThemePalette) {
        _uiState.update { state ->
            state.copy(themeDraft = state.themeDraft.copy(palette = palette))
        }
    }

    fun resetThemeDraft() {
        _uiState.update { state ->
            state.copy(
                themeDraft = state.themePreferences,
                statusMessage = "Restored the theme draft from persisted settings.",
            )
        }
    }

    fun applyThemeDraft() {
        val draft = _uiState.value.themeDraft
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Applying theme ${draft.palette.displayName} in ${draft.mode.displayName} mode...",
                )
            }
            runCatching { repository.updateThemePreferences(draft) }
                .onSuccess { persistedTheme ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            themePreferences = persistedTheme,
                            themeDraft = persistedTheme,
                            statusMessage = "Applied theme ${persistedTheme.palette.displayName}.",
                        )
                    }
                }
                .onFailure { error ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            errorMessage = error.message ?: "Failed to persist theme settings.",
                            statusMessage = "Failed to persist theme settings.",
                        )
                    }
                }
        }
    }

    fun selectBundledConfig(fileName: String) {
        _uiState.update { state ->
            if (state.bundledConfigs.none { config -> config.fileName == fileName }) {
                state
            } else {
                state.copy(selectedConfigFileName = fileName)
            }
        }
    }

    fun updateConfigDraft(rawText: String) {
        _uiState.update { state ->
            val fileName = state.selectedConfigFileName
            if (fileName.isBlank()) {
                state
            } else {
                state.copy(configDrafts = state.configDrafts + (fileName to rawText))
            }
        }
    }

    fun resetSelectedConfigDraft() {
        _uiState.update { state ->
            val fileName = state.selectedConfigFileName
            val persistedText = state.bundledConfigs.firstOrNull { config ->
                config.fileName == fileName
            }?.rawText
            if (fileName.isBlank() || persistedText == null) {
                state
            } else {
                state.copy(
                    configDrafts = state.configDrafts + (fileName to persistedText),
                    statusMessage = "Restored the draft for $fileName from persisted TOML.",
                )
            }
        }
    }

    fun saveSelectedConfig() {
        val state = _uiState.value
        val fileName = state.selectedConfigFileName
        val rawText = state.configDrafts[fileName]
        if (fileName.isBlank() || rawText == null) {
            return
        }
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Persisting $fileName into runtime_config...",
                )
            }
            runCatching { repository.updateBundledConfig(fileName, rawText) }
                .onSuccess { updatedConfigs ->
                    _uiState.update { current ->
                        current.copy(
                            isWorking = false,
                            bundledConfigs = updatedConfigs,
                            configDrafts = current.configDrafts + (
                                fileName to (
                                    updatedConfigs.firstOrNull { config -> config.fileName == fileName }?.rawText
                                        ?: rawText
                                    )
                                ),
                            statusMessage = "Modified and persisted $fileName.",
                        )
                    }
                }
                .onFailure { error ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            errorMessage = error.message ?: "Failed to persist bundled config.",
                            statusMessage = "Failed to persist bundled config.",
                        )
                    }
                }
        }
    }

    fun importBundledSample() {
        val label = _uiState.value.bundledSampleLabel
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Importing $label into SQLite...",
                )
            }
            runCatching { repository.importBundledSample() }
                .onSuccess { result ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            importResult = result,
                            errorMessage = if (result.ok) null else result.message,
                            statusMessage = if (result.ok) {
                                "Imported ${result.imported} bundled bill file(s)."
                            } else {
                                result.message
                            },
                        )
                    }
                }
                .onFailure { error ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            errorMessage = error.message ?: "Import failed.",
                            statusMessage = "Import failed.",
                        )
                    }
                }
        }
    }

    fun runBundledYearQuery() {
        val state = _uiState.value
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Running fixed year query for ${state.bundledSampleYear}...",
                )
            }
            runCatching { repository.queryBundledYear() }
                .onSuccess { query ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            queryResult = query,
                            statusMessage = if (query.ok) {
                                "Year query returned ${query.matchedBills} matching bill(s)."
                            } else {
                                query.message
                            },
                            errorMessage = if (query.ok) null else query.message,
                        )
                    }
                }.onFailure { error ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            errorMessage = error.message ?: "Query failed.",
                            statusMessage = "Query failed.",
                        )
                    }
                }
        }
    }

    fun runBundledMonthQuery() {
        val state = _uiState.value
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Running fixed month query for ${state.bundledSampleMonth}...",
                )
            }
            runCatching { repository.queryBundledMonth() }
                .onSuccess { query ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            queryResult = query,
                            statusMessage = if (query.ok) {
                                "Month query returned ${query.matchedBills} matching bill(s)."
                            } else {
                                query.message
                            },
                            errorMessage = if (query.ok) null else query.message,
                        )
                    }
                }.onFailure { error ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            errorMessage = error.message ?: "Query failed.",
                            statusMessage = "Query failed.",
                        )
                    }
                }
        }
    }

    fun clearDatabase() {
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Clearing the private SQLite database...",
                )
            }
            runCatching { repository.clearDatabase() }
                .onSuccess { removed ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            importResult = null,
                            queryResult = null,
                            statusMessage = if (removed) {
                                "Database file cleared."
                            } else {
                                "Database file was already empty."
                            },
                        )
                    }
                }.onFailure { error ->
                    _uiState.update {
                        it.copy(
                            isWorking = false,
                            errorMessage = error.message ?: "Failed to clear the database.",
                            statusMessage = "Failed to clear the database.",
                        )
                    }
                }
        }
    }
}

class BillsViewModelFactory(
    private val repository: BillsRepository,
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return BillsViewModel(repository) as T
    }
}
