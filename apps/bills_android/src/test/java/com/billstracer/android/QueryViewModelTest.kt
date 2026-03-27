package com.billstracer.android

import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.app.navigation.WorkspaceDataChangeBus
import com.billstracer.android.features.query.QueryViewMode
import com.billstracer.android.features.query.QueryViewModel
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.advanceUntilIdle
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test

@OptIn(ExperimentalCoroutinesApi::class)
class QueryViewModelTest {
    private val dispatcher = StandardTestDispatcher()
    private fun createViewModel(
        workspaceService: FakeWorkspaceService = FakeWorkspaceService(),
        queryService: FakeQueryService = FakeQueryService(),
    ): QueryViewModel = QueryViewModel(
        workspaceService = workspaceService,
        queryService = queryService,
        sessionBus = AppSessionBus(),
        workspaceDataChangeBus = WorkspaceDataChangeBus(),
    )

    @Before
    fun setUp() {
        Dispatchers.setMain(dispatcher)
    }

    @After
    fun tearDown() {
        Dispatchers.resetMain()
    }

    @Test
    fun loadAvailablePeriodsDefaultsToMostRecentDatabasePeriod() = runTest {
        val viewModel = createViewModel()
        advanceUntilIdle()

        assertEquals("2026", viewModel.state.value.queryYearInput)
        assertEquals("2026", viewModel.state.value.queryPeriodYearInput)
        assertEquals("03", viewModel.state.value.queryPeriodMonthInput)
    }

    @Test
    fun runYearQueryUsesSelectedDatabaseYear() = runTest {
        val queryService = FakeQueryService()
        val viewModel = createViewModel(queryService = queryService)
        advanceUntilIdle()

        viewModel.selectQueryYear("2025")
        viewModel.runYearQuery()
        advanceUntilIdle()

        assertEquals("2025", queryService.lastQueriedYear)
        assertEquals(QueryViewMode.STRUCTURED, viewModel.state.value.selectedQueryViewMode)
    }

    @Test
    fun emptyAvailablePeriodsLeaveSelectionsBlankAndBlockQueries() = runTest {
        val queryService = FakeQueryService().apply {
            availablePeriods = emptyList()
        }
        val viewModel = createViewModel(queryService = queryService)
        advanceUntilIdle()

        assertEquals("", viewModel.state.value.queryYearInput)
        assertEquals("", viewModel.state.value.queryPeriodYearInput)
        assertEquals("", viewModel.state.value.queryPeriodMonthInput)

        viewModel.runYearQuery()
        advanceUntilIdle()

        assertEquals(null, queryService.lastQueriedYear)
        assertEquals(
            "Select an imported year before running the query.",
            viewModel.state.value.errorMessage,
        )
    }

    @Test
    fun selectingMonthQueryYearChoosesFirstAvailableMonthForThatYear() = runTest {
        val viewModel = createViewModel()
        advanceUntilIdle()

        viewModel.selectQueryPeriodYear("2025")

        assertEquals("2025", viewModel.state.value.queryPeriodYearInput)
        assertEquals("12", viewModel.state.value.queryPeriodMonthInput)
    }

    @Test
    fun monthQueryDefaultsToStructuredView() = runTest {
        val queryService = FakeQueryService().apply {
            availablePeriods = listOf("2025-03")
        }
        val viewModel = createViewModel(queryService = queryService)
        advanceUntilIdle()

        viewModel.runMonthQuery()
        advanceUntilIdle()

        assertEquals(QueryViewMode.STRUCTURED, viewModel.state.value.selectedQueryViewMode)
    }

    @Test
    fun queryDefaultsToChartWhenStructuredViewIsUnavailable() = runTest {
        val queryService = FakeQueryService().apply {
            yearQueryResultOverride = QueryResult(
                ok = true,
                message = "2026",
                type = QueryType.YEAR,
                year = 2026,
                month = null,
                matchedBills = 1,
                totalIncome = 10.0,
                totalExpense = -5.0,
                balance = 5.0,
                monthlySummary = emptyList(),
                standardReportMarkdown = "# 2026",
                standardReportJson = """
                    {
                      "extensions": {
                        "chart_data": {
                          "schema_version": "1.0.0",
                          "views": [
                            {
                              "id": "yearly_monthly_overview",
                              "title": "Monthly Income, Expense, and Balance",
                              "chart_type": "grouped_bar",
                              "x_labels": ["01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12"],
                              "series": [
                                { "id": "income", "label": "Income", "unit": "CNY", "values": [10.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0] }
                              ]
                            }
                          ]
                        }
                      }
                    }
                """.trimIndent(),
                rawJson = """{"ok":true}""",
            )
        }
        val viewModel = createViewModel(queryService = queryService)
        advanceUntilIdle()

        viewModel.runYearQuery()
        advanceUntilIdle()

        assertEquals(QueryViewMode.CHART, viewModel.state.value.selectedQueryViewMode)
    }

    @Test
    fun chartSelectionFallsBackToStructuredWhenNextResultHasNoChart() = runTest {
        val queryService = FakeQueryService()
        val viewModel = createViewModel(queryService = queryService)
        advanceUntilIdle()

        viewModel.runYearQuery()
        advanceUntilIdle()
        viewModel.selectQueryViewMode(QueryViewMode.CHART)
        assertEquals(QueryViewMode.CHART, viewModel.state.value.selectedQueryViewMode)

        queryService.yearQueryResultOverride = QueryResult(
            ok = true,
            message = "2026",
            type = QueryType.YEAR,
            year = 2026,
            month = null,
            matchedBills = 1,
            totalIncome = 10.0,
            totalExpense = -5.0,
            balance = 5.0,
            monthlySummary = listOf(),
            standardReportMarkdown = "# 2026",
            standardReportJson = fakeYearStandardReportJson(
                year = 2026,
                includeChartData = false,
            ),
            rawJson = """{"ok":true}""",
        )

        viewModel.runYearQuery()
        advanceUntilIdle()

        assertEquals(QueryViewMode.STRUCTURED, viewModel.state.value.selectedQueryViewMode)
    }

    @Test
    fun workspaceDataChangeRefreshesAvailablePeriods() = runTest {
        val queryService = FakeQueryService()
        val workspaceDataChangeBus = WorkspaceDataChangeBus()
        val viewModel = QueryViewModel(
            workspaceService = FakeWorkspaceService(),
            queryService = queryService,
            sessionBus = AppSessionBus(),
            workspaceDataChangeBus = workspaceDataChangeBus,
        )
        advanceUntilIdle()

        queryService.availablePeriods = listOf("2027-01", "2026-03", "2026-02", "2025-12")
        workspaceDataChangeBus.notifyChanged()
        advanceUntilIdle()

        assertEquals(listOf("2027-01", "2026-03", "2026-02", "2025-12"), viewModel.state.value.availablePeriods)
    }
}
