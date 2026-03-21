package com.billstracer.android.ui.report

import androidx.lifecycle.viewModelScope
import com.billstracer.android.ui.BillsViewModel
import com.billstracer.android.ui.currentState
import com.billstracer.android.ui.updateUiState
import com.billstracer.android.ui.common.manualRecordPeriodOrNull
import com.billstracer.android.ui.common.withQueryPeriodInputs
import com.billstracer.android.ui.common.withQueryYearInput
import com.billstracer.android.ui.common.yearInputOrNull
import kotlinx.coroutines.launch

internal fun BillsViewModel.applyQueryYearInput(year: String) {
    updateUiState { state ->
        state.withQueryYearInput(year)
    }
}

internal fun BillsViewModel.applyQueryPeriodYearInput(year: String) {
    updateUiState { state ->
        state.withQueryPeriodInputs(
            yearInput = year,
            monthInput = state.queryPeriodMonthInput,
        )
    }
}

internal fun BillsViewModel.applyQueryPeriodMonthInput(month: String) {
    updateUiState { state ->
        state.withQueryPeriodInputs(
            yearInput = state.queryPeriodYearInput,
            monthInput = month,
        )
    }
}

internal fun BillsViewModel.runBundledYearQueryAction() {
    val state = currentState
    val queryYear = yearInputOrNull(state.queryYearInput)
    if (queryYear == null) {
        updateUiState {
            it.copy(
                errorMessage = "Year query must use 4 digits.",
                statusMessage = "Year query input is invalid.",
            )
        }
        return
    }
    viewModelScope.launch {
        updateUiState {
            it.copy(
                isWorking = true,
                errorMessage = null,
                statusMessage = "Running year query for $queryYear...",
            )
        }
        runCatching { repository.queryYear(queryYear) }
            .onSuccess { query ->
                updateUiState {
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
            }
            .onFailure { error ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        errorMessage = error.message ?: "Query failed.",
                        statusMessage = "Query failed.",
                    )
                }
            }
    }
}

internal fun BillsViewModel.runBundledMonthQueryAction() {
    val state = currentState
    val queryMonth = manualRecordPeriodOrNull(
        yearInput = state.queryPeriodYearInput,
        monthInput = state.queryPeriodMonthInput,
    )
    if (queryMonth == null) {
        updateUiState {
            it.copy(
                errorMessage = "Month query must use YYYY-MM, and month must be between 01 and 12.",
                statusMessage = "Month query input is invalid.",
            )
        }
        return
    }
    viewModelScope.launch {
        updateUiState {
            it.copy(
                isWorking = true,
                errorMessage = null,
                statusMessage = "Running month query for $queryMonth...",
            )
        }
        runCatching { repository.queryMonth(queryMonth) }
            .onSuccess { query ->
                updateUiState {
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
            }
            .onFailure { error ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        errorMessage = error.message ?: "Query failed.",
                        statusMessage = "Query failed.",
                    )
                }
            }
    }
}
