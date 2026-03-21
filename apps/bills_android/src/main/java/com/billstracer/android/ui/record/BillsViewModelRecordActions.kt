package com.billstracer.android.ui.record

import androidx.lifecycle.viewModelScope
import com.billstracer.android.model.BillsUiState
import com.billstracer.android.model.ListedRecordPeriodsResult
import com.billstracer.android.ui.BillsViewModel
import com.billstracer.android.ui.currentState
import com.billstracer.android.ui.updateUiState
import com.billstracer.android.ui.common.manualRecordPeriodOrNull
import com.billstracer.android.ui.common.withManualRecordPeriod
import com.billstracer.android.ui.common.withManualRecordPeriodInputs
import kotlinx.coroutines.launch
import java.time.YearMonth

private data class ExistingRecordSelection(
    val year: String = "",
    val month: String = "",
)

internal fun BillsViewModel.applyRecordPeriodInput(period: String) {
    updateUiState { state ->
        state.withManualRecordPeriod(period)
    }
}

internal fun BillsViewModel.applyRecordPeriodYearInput(year: String) {
    updateUiState { state ->
        state.withManualRecordPeriodInputs(
            yearInput = year,
            monthInput = state.recordPeriodMonthInput,
        )
    }
}

internal fun BillsViewModel.applyRecordPeriodMonthInput(month: String) {
    updateUiState { state ->
        state.withManualRecordPeriodInputs(
            yearInput = state.recordPeriodYearInput,
            monthInput = month,
        )
    }
}

internal fun BillsViewModel.selectExistingRecordYearAction(year: String) {
    updateUiState { state ->
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

internal fun BillsViewModel.selectExistingRecordMonthAction(month: String) {
    updateUiState { state ->
        state.copy(selectedExistingRecordMonth = month)
    }
}

internal fun BillsViewModel.openRecordPeriodAction() {
    val state = currentState
    val period = manualRecordPeriodOrNull(
        yearInput = state.recordPeriodYearInput,
        monthInput = state.recordPeriodMonthInput,
    )
    if (period == null) {
        updateUiState {
            it.copy(
                errorMessage = "Manual period must use YYYY-MM, and month must be between 01 and 12.",
                statusMessage = "Manual period input is invalid.",
            )
        }
        return
    }
    openRecordPeriodInternal(period)
}

internal fun BillsViewModel.openCurrentMonthRecordAction() {
    val currentPeriod = YearMonth.now().toString()
    updateUiState { state -> state.withManualRecordPeriod(currentPeriod) }
    openRecordPeriodInternal(currentPeriod)
}

internal fun BillsViewModel.openSelectedExistingRecordPeriodAction() {
    val state = currentState
    val period = composePeriod(
        year = state.selectedExistingRecordYear,
        month = state.selectedExistingRecordMonth,
    ) ?: return
    openRecordPeriodInternal(period)
}

private fun BillsViewModel.openRecordPeriodInternal(period: String) {
    viewModelScope.launch {
        updateUiState {
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
                }.getOrNull() ?: currentState.listedRecordPeriods
                updateUiState {
                    applyExistingRecordSelection(
                        it.copy(
                            isWorking = false,
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
                    ).withManualRecordPeriod(document.period)
                }
            }
            .onFailure { error ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        errorMessage = error.message ?: "Failed to open record period.",
                        statusMessage = "Failed to open record period.",
                    )
                }
            }
    }
}

internal fun BillsViewModel.updateRecordDraftAction(rawText: String) {
    updateUiState { state ->
        state.copy(recordDraftText = rawText)
    }
}

internal fun BillsViewModel.resetRecordDraftAction() {
    updateUiState { state ->
        val activeRecord = state.activeRecordDocument ?: return@updateUiState state
        state.copy(
            recordDraftText = activeRecord.rawText,
            statusMessage = "Restored the draft for ${activeRecord.period}.",
        )
    }
}

internal fun BillsViewModel.previewRecordDraftAction() {
    val state = currentState
    val activeRecord = state.activeRecordDocument ?: return
    viewModelScope.launch {
        updateUiState {
            it.copy(
                isWorking = true,
                errorMessage = null,
                statusMessage = "Previewing ${activeRecord.period} through core record_preview...",
            )
        }
        runCatching { repository.previewRecordDocument(activeRecord.period, state.recordDraftText) }
            .onSuccess { preview ->
                updateUiState {
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
                updateUiState {
                    it.copy(
                        isWorking = false,
                        errorMessage = error.message ?: "Failed to preview record draft.",
                        statusMessage = "Failed to preview record draft.",
                    )
                }
            }
    }
}

internal fun BillsViewModel.saveRecordDraftAction() {
    val state = currentState
    val activeRecord = state.activeRecordDocument ?: return
    viewModelScope.launch {
        updateUiState {
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
            updateUiState {
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
            updateUiState {
                it.copy(
                    isWorking = false,
                    errorMessage = error.message ?: "Failed to save record draft.",
                    statusMessage = "Failed to save record draft.",
                )
            }
        }
    }
}

internal fun BillsViewModel.refreshRecordPeriodsAction() {
    viewModelScope.launch {
        updateUiState {
            it.copy(
                isWorking = true,
                errorMessage = null,
                statusMessage = "Scanning private record periods...",
            )
        }
        runCatching { repository.listRecordPeriods() }
            .onSuccess { listedPeriods ->
                updateUiState {
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
                updateUiState {
                    it.copy(
                        isWorking = false,
                        errorMessage = error.message ?: "Failed to scan record periods.",
                        statusMessage = "Failed to scan record periods.",
                    )
                }
            }
    }
}

internal fun BillsViewModel.clearRecordFilesAction() {
    viewModelScope.launch {
        updateUiState {
            it.copy(
                isWorking = true,
                errorMessage = null,
                statusMessage = "Clearing saved TXT record files...",
            )
        }
        runCatching {
            val removed = repository.clearRecordFiles()
            removed to repository.listRecordPeriods()
        }.onSuccess { (removed, listedPeriods) ->
            updateUiState {
                applyExistingRecordSelection(
                    it.copy(
                        isWorking = false,
                        activeRecordDocument = null,
                        recordDraftText = "",
                        recordPreviewResult = null,
                        listedRecordPeriods = listedPeriods,
                        statusMessage = if (removed > 0) {
                            "Cleared $removed TXT record file(s)."
                        } else {
                            "No TXT record files were found."
                        },
                    ),
                    listedPeriods = listedPeriods,
                    preferredPeriod = null,
                )
            }
        }.onFailure { error ->
            updateUiState {
                it.copy(
                    isWorking = false,
                    errorMessage = error.message ?: "Failed to clear TXT record files.",
                    statusMessage = "Failed to clear TXT record files.",
                )
            }
        }
    }
}

internal fun applyExistingRecordSelection(
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
    manualRecordPeriodOrNull(yearInput = year, monthInput = month)
