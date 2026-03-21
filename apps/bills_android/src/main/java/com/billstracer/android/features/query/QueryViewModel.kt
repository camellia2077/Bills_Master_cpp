package com.billstracer.android.features.query

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.data.services.QueryService
import com.billstracer.android.data.services.WorkspaceService
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import com.billstracer.android.platform.sanitizeMonthInput
import com.billstracer.android.platform.sanitizeYearInput
import com.billstracer.android.platform.yearInputOrNull
import com.billstracer.android.platform.yearMonthOrNull
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

enum class QueryViewMode {
    STRUCTURED,
    MARKDOWN,
    RAW_JSON,
}

data class QueryUiState(
    val isInitializing: Boolean = true,
    val isWorking: Boolean = false,
    val statusMessage: String = "",
    val errorMessage: String? = null,
    val queryYearInput: String = "",
    val queryPeriodYearInput: String = "",
    val queryPeriodMonthInput: String = "",
    val bundledSampleYear: String = "",
    val queryResult: QueryResult? = null,
    val selectedQueryViewMode: QueryViewMode = QueryViewMode.MARKDOWN,
)

class QueryViewModel(
    private val workspaceService: WorkspaceService,
    private val queryService: QueryService,
    private val sessionBus: AppSessionBus,
) : ViewModel() {
    private val mutableState = MutableStateFlow(QueryUiState())
    val state: StateFlow<QueryUiState> = mutableState.asStateFlow()

    init {
        loadEnvironment()
    }

    private fun loadEnvironment() {
        viewModelScope.launch {
            runCatching { workspaceService.initializeEnvironment() }
                .onSuccess { environment ->
                    val month = environment.bundledSampleMonth.substringAfter('-', "")
                    mutableState.update { current ->
                        current.copy(
                            isInitializing = false,
                            bundledSampleYear = environment.bundledSampleYear,
                            queryYearInput = environment.bundledSampleYear,
                            queryPeriodYearInput = environment.bundledSampleYear,
                            queryPeriodMonthInput = month,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to prepare query inputs."
                    sessionBus.publishError(message, "Query setup failed.")
                    mutableState.update { current ->
                        current.copy(
                            isInitializing = false,
                            errorMessage = message,
                            statusMessage = "Query setup failed.",
                        )
                    }
                }
        }
    }

    fun updateQueryYearInput(year: String) {
        mutableState.update { current ->
            current.copy(queryYearInput = sanitizeYearInput(year))
        }
    }

    fun updateQueryPeriodYearInput(year: String) {
        mutableState.update { current ->
            current.copy(queryPeriodYearInput = sanitizeYearInput(year))
        }
    }

    fun updateQueryPeriodMonthInput(month: String) {
        mutableState.update { current ->
            current.copy(queryPeriodMonthInput = sanitizeMonthInput(month))
        }
    }

    fun selectQueryViewMode(viewMode: QueryViewMode) {
        mutableState.update { current ->
            current.copy(selectedQueryViewMode = viewMode)
        }
    }

    fun runYearQuery() {
        val queryYear = yearInputOrNull(state.value.queryYearInput)
        if (queryYear == null) {
            mutableState.update { current ->
                current.copy(
                    errorMessage = "Year query must use 4 digits.",
                    statusMessage = "Year query input is invalid.",
                )
            }
            return
        }
        viewModelScope.launch {
            val pendingMessage = "Running year query for $queryYear..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { queryService.queryYear(queryYear) }
                .onSuccess { query ->
                    val message = if (query.ok) {
                        "Year query returned ${query.matchedBills} matching bill(s)."
                    } else {
                        query.message
                    }
                    if (query.ok) {
                        sessionBus.publishStatus(message)
                    } else {
                        sessionBus.publishError(query.message, message)
                    }
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            queryResult = query,
                            selectedQueryViewMode = QueryViewMode.MARKDOWN,
                            statusMessage = message,
                            errorMessage = if (query.ok) null else query.message,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Query failed."
                    sessionBus.publishError(message, "Query failed.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Query failed.",
                        )
                    }
                }
        }
    }

    fun runMonthQuery() {
        val queryMonth = yearMonthOrNull(
            yearInput = state.value.queryPeriodYearInput,
            monthInput = state.value.queryPeriodMonthInput,
        )
        if (queryMonth == null) {
            mutableState.update { current ->
                current.copy(
                    errorMessage = "Month query must use YYYY-MM, and month must be between 01 and 12.",
                    statusMessage = "Month query input is invalid.",
                )
            }
            return
        }
        viewModelScope.launch {
            val pendingMessage = "Running month query for $queryMonth..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { queryService.queryMonth(queryMonth) }
                .onSuccess { query ->
                    val message = if (query.ok) {
                        "Month query returned ${query.matchedBills} matching bill(s)."
                    } else {
                        query.message
                    }
                    if (query.ok) {
                        sessionBus.publishStatus(message)
                    } else {
                        sessionBus.publishError(query.message, message)
                    }
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            queryResult = query,
                            selectedQueryViewMode = if (query.type == QueryType.MONTH) {
                                QueryViewMode.STRUCTURED
                            } else {
                                QueryViewMode.MARKDOWN
                            },
                            statusMessage = message,
                            errorMessage = if (query.ok) null else query.message,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Query failed."
                    sessionBus.publishError(message, "Query failed.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Query failed.",
                        )
                    }
                }
        }
    }
}

class QueryViewModelFactory(
    private val workspaceService: WorkspaceService,
    private val queryService: QueryService,
    private val sessionBus: AppSessionBus,
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return QueryViewModel(workspaceService, queryService, sessionBus) as T
    }
}
