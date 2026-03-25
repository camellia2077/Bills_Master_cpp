package com.billstracer.android.features.query

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.data.services.QueryService
import com.billstracer.android.data.services.WorkspaceService
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
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
    val availablePeriods: List<String> = emptyList(),
    val queryYearInput: String = "",
    val queryPeriodYearInput: String = "",
    val queryPeriodMonthInput: String = "",
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
        refreshAvailablePeriods(initialLoad = true)
    }

    fun refreshAvailablePeriods() {
        refreshAvailablePeriods(initialLoad = false)
    }

    private fun refreshAvailablePeriods(initialLoad: Boolean) {
        viewModelScope.launch {
            if (!initialLoad) {
                mutableState.update { current ->
                    current.copy(
                        isWorking = true,
                        errorMessage = null,
                        statusMessage = "Loading queryable months from database...",
                    )
                }
            }
            runCatching {
                workspaceService.initializeEnvironment()
                queryService.listAvailablePeriods()
            }.onSuccess { periods ->
                val currentState = state.value
                val selectedYear = resolveSelectedYear(
                    currentYear = currentState.queryYearInput,
                    periods = periods,
                    preferredYear = currentState.queryResult?.year?.toString(),
                )
                val selectedMonth = resolveSelectedYearMonth(
                    currentYear = currentState.queryPeriodYearInput,
                    currentMonth = currentState.queryPeriodMonthInput,
                    periods = periods,
                    preferredPeriod = currentState.queryResult?.takeIf { it.type == QueryType.MONTH }
                        ?.let { query ->
                            val year = query.year ?: return@let null
                            val month = query.month ?: return@let null
                            "$year-${month.toString().padStart(2, '0')}"
                        },
                )
                val message = if (periods.isEmpty()) {
                    "No imported months found in database."
                } else {
                    "Loaded ${periods.size} queryable month(s) from database."
                }
                mutableState.update { current ->
                    current.copy(
                        isInitializing = false,
                        isWorking = false,
                        statusMessage = message,
                        errorMessage = null,
                        availablePeriods = periods,
                        queryYearInput = selectedYear,
                        queryPeriodYearInput = selectedMonth.year,
                        queryPeriodMonthInput = selectedMonth.month,
                    )
                }
            }.onFailure { error ->
                val message = error.message ?: "Failed to load query periods."
                sessionBus.publishError(message, "Query setup failed.")
                mutableState.update { current ->
                    current.copy(
                        isInitializing = false,
                        isWorking = false,
                        errorMessage = message,
                        statusMessage = "Query setup failed.",
                    )
                }
            }
        }
    }

    fun selectQueryYear(year: String) {
        mutableState.update { current ->
            current.copy(queryYearInput = year)
        }
    }

    fun selectQueryPeriodYear(year: String) {
        mutableState.update { current ->
            val availableMonths = monthsForYear(current.availablePeriods, year)
            val selectedMonth = if (availableMonths.contains(current.queryPeriodMonthInput)) {
                current.queryPeriodMonthInput
            } else {
                availableMonths.firstOrNull().orEmpty()
            }
            current.copy(
                queryPeriodYearInput = year,
                queryPeriodMonthInput = selectedMonth,
            )
        }
    }

    fun selectQueryPeriodMonth(month: String) {
        mutableState.update { current ->
            current.copy(queryPeriodMonthInput = month)
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
                    errorMessage = "Select an imported year before running the query.",
                    statusMessage = "Year query selection is missing.",
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
                            selectedQueryViewMode = if (query.type == QueryType.YEAR) {
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

    fun runMonthQuery() {
        val queryMonth = yearMonthOrNull(
            yearInput = state.value.queryPeriodYearInput,
            monthInput = state.value.queryPeriodMonthInput,
        )
        if (queryMonth == null) {
            mutableState.update { current ->
                current.copy(
                    errorMessage = "Select an imported year/month before running the query.",
                    statusMessage = "Month query selection is missing.",
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

private data class SelectedYearMonth(
    val year: String = "",
    val month: String = "",
)

private fun resolveSelectedYear(
    currentYear: String,
    periods: List<String>,
    preferredYear: String? = null,
): String {
    val years = yearsFromPeriods(periods)
    if (years.isEmpty()) {
        return ""
    }
    return when {
        !preferredYear.isNullOrBlank() && years.contains(preferredYear) -> preferredYear
        currentYear.isNotBlank() && years.contains(currentYear) -> currentYear
        else -> years.first()
    }
}

private fun resolveSelectedYearMonth(
    currentYear: String,
    currentMonth: String,
    periods: List<String>,
    preferredPeriod: String? = null,
): SelectedYearMonth {
    val years = yearsFromPeriods(periods)
    if (years.isEmpty()) {
        return SelectedYearMonth()
    }

    val preferredYear = preferredPeriod?.substringBefore('-', "")
    val preferredMonth = preferredPeriod?.substringAfter('-', "")
    val selectedYear = when {
        !preferredYear.isNullOrBlank() && years.contains(preferredYear) -> preferredYear
        currentYear.isNotBlank() && years.contains(currentYear) -> currentYear
        else -> years.first()
    }
    val months = monthsForYear(periods, selectedYear)
    val selectedMonth = when {
        !preferredMonth.isNullOrBlank() &&
            preferredYear == selectedYear &&
            months.contains(preferredMonth) -> preferredMonth
        currentMonth.isNotBlank() && months.contains(currentMonth) -> currentMonth
        else -> months.firstOrNull().orEmpty()
    }

    return SelectedYearMonth(
        year = selectedYear,
        month = selectedMonth,
    )
}

private fun yearsFromPeriods(periods: List<String>): List<String> =
    periods.mapNotNull { period -> period.substringBefore('-').takeIf { it.length == 4 } }
        .distinct()

private fun monthsForYear(
    periods: List<String>,
    year: String,
): List<String> = periods
    .filter { period -> period.startsWith("$year-") && period.length == 7 }
    .map { period -> period.substringAfter('-') }
    .distinct()
