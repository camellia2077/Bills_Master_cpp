package com.billstracer.android.features.query

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.app.navigation.WorkspaceDataChangeBus
import com.billstracer.android.data.prefs.QueryInputModePreferenceStore
import com.billstracer.android.data.services.QueryService
import com.billstracer.android.data.services.WorkspaceService
import com.billstracer.android.features.common.monthsForYear
import com.billstracer.android.features.common.resolveRangePeriodSelection
import com.billstracer.android.features.common.resolveYearMonthSelection
import com.billstracer.android.features.common.resolveYearSelection
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
    TEXT,
    CHART,
}

enum class QueryInputMode(val label: String) {
    MONTH("Month"),
    YEAR("Year"),
    RANGE("Range"),
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
    val queryRangeStartInput: String = "",
    val queryRangeEndInput: String = "",
    val queryInputMode: QueryInputMode = QueryInputMode.MONTH,
    val queryResult: QueryResult? = null,
    val selectedQueryViewMode: QueryViewMode = QueryViewMode.TEXT,
)

class QueryViewModel(
    private val workspaceService: WorkspaceService,
    private val queryService: QueryService,
    private val sessionBus: AppSessionBus,
    private val workspaceDataChangeBus: WorkspaceDataChangeBus,
    private val queryInputModePreferenceStore: QueryInputModePreferenceStore? = null,
) : ViewModel() {
    private val mutableState = MutableStateFlow(QueryUiState())
    val state: StateFlow<QueryUiState> = mutableState.asStateFlow()
    private var observedWorkspaceDataVersion = workspaceDataChangeBus.version.value

    init {
        loadQueryInputMode()
        observeWorkspaceDataChanges()
        refreshAvailablePeriods(initialLoad = true)
    }

    private fun loadQueryInputMode() {
        val preferenceStore = queryInputModePreferenceStore ?: return
        viewModelScope.launch {
            preferenceStore.load()
                ?.let { rawMode -> rawMode.toQueryInputModeOrNull() }
                ?.let { inputMode ->
                    mutableState.update { current -> current.copy(queryInputMode = inputMode) }
                }
        }
    }

    fun selectQueryInputMode(inputMode: QueryInputMode) {
        mutableState.update { current -> current.copy(queryInputMode = inputMode) }
        val preferenceStore = queryInputModePreferenceStore ?: return
        viewModelScope.launch {
            preferenceStore.save(inputMode.name)
        }
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
                val selectedYear = resolveYearSelection(
                    currentYear = currentState.queryYearInput,
                    periods = periods,
                    preferredYear = currentState.queryResult?.periodStart
                        ?.substringBefore('-', missingDelimiterValue = "")
                        ?.takeIf { it.length == 4 },
                )
                val selectedMonth = resolveYearMonthSelection(
                    currentYear = currentState.queryPeriodYearInput,
                    currentMonth = currentState.queryPeriodMonthInput,
                    periods = periods,
                    preferredPeriod = currentState.queryResult?.takeIf { it.type == QueryType.MONTH }
                        ?.periodStart
                        ?.takeIf { it.length == 7 },
                )
                val selectedRange = resolveRangePeriodSelection(
                    currentStart = currentState.queryRangeStartInput,
                    currentEnd = currentState.queryRangeEndInput,
                    periods = periods,
                    preferredStart = currentState.queryResult?.takeIf { it.type == QueryType.RANGE }
                        ?.periodStart
                        ?.takeIf { it.length == 7 },
                    preferredEnd = currentState.queryResult?.takeIf { it.type == QueryType.RANGE }
                        ?.periodEnd
                        ?.takeIf { it.length == 7 },
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
                        queryRangeStartInput = selectedRange.start,
                        queryRangeEndInput = selectedRange.end,
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

    fun selectQueryRangeStart(period: String) {
        mutableState.update { current ->
            current.copy(
                queryRangeStartInput = period,
                queryRangeEndInput = if (current.queryRangeEndInput.isBlank() || period > current.queryRangeEndInput) {
                    period
                } else {
                    current.queryRangeEndInput
                },
            )
        }
    }

    fun selectQueryRangeEnd(period: String) {
        mutableState.update { current ->
            current.copy(
                queryRangeStartInput = if (current.queryRangeStartInput.isBlank() || period < current.queryRangeStartInput) {
                    period
                } else {
                    current.queryRangeStartInput
                },
                queryRangeEndInput = period,
            )
        }
    }

    fun selectQueryViewMode(viewMode: QueryViewMode) {
        mutableState.update { current ->
            val queryResult = current.queryResult
            current.copy(
                selectedQueryViewMode = if (queryResult != null) {
                    resolvePreferredQueryViewMode(queryResult, viewMode)
                } else {
                    viewMode
                },
            )
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
                            selectedQueryViewMode = resolvePreferredQueryViewMode(query),
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
                            selectedQueryViewMode = resolvePreferredQueryViewMode(query),
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

    fun runRangeQuery() {
        val start = state.value.queryRangeStartInput
        val end = state.value.queryRangeEndInput
        if (start.isBlank() || end.isBlank()) {
            mutableState.update { current ->
                current.copy(
                    errorMessage = "Select imported start/end months before running the range query.",
                    statusMessage = "Range query selection is missing.",
                )
            }
            return
        }
        if (start > end) {
            mutableState.update { current ->
                current.copy(
                    errorMessage = "Range start must be earlier than or equal to range end.",
                    statusMessage = "Range query selection is invalid.",
                )
            }
            return
        }
        viewModelScope.launch {
            val pendingMessage = "Running range query for $start to $end..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { queryService.queryRange(start, end) }
                .onSuccess { query ->
                    val message = if (query.ok) {
                        "Range query returned ${query.matchedBills} matching bill(s)."
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
                            selectedQueryViewMode = resolvePreferredQueryViewMode(query),
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

    private fun observeWorkspaceDataChanges() {
        viewModelScope.launch {
            workspaceDataChangeBus.version.collect { version ->
                if (version == observedWorkspaceDataVersion) {
                    return@collect
                }
                observedWorkspaceDataVersion = version
                refreshAvailablePeriods(initialLoad = false)
            }
        }
    }
}

class QueryViewModelFactory(
    private val workspaceService: WorkspaceService,
    private val queryService: QueryService,
    private val sessionBus: AppSessionBus,
    private val workspaceDataChangeBus: WorkspaceDataChangeBus,
    private val queryInputModePreferenceStore: QueryInputModePreferenceStore? = null,
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return QueryViewModel(
            workspaceService,
            queryService,
            sessionBus,
            workspaceDataChangeBus,
            queryInputModePreferenceStore,
        ) as T
    }
}

private fun String.toQueryInputModeOrNull(): QueryInputMode? =
    runCatching { enumValueOf<QueryInputMode>(this) }.getOrNull()
