package com.billstracer.android

import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.features.query.QueryViewMode
import com.billstracer.android.features.query.QueryViewModel
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
        val viewModel = QueryViewModel(
            workspaceService = FakeWorkspaceService(),
            queryService = FakeQueryService(),
            sessionBus = AppSessionBus(),
        )
        advanceUntilIdle()

        assertEquals("2026", viewModel.state.value.queryYearInput)
        assertEquals("2026", viewModel.state.value.queryPeriodYearInput)
        assertEquals("03", viewModel.state.value.queryPeriodMonthInput)
    }

    @Test
    fun runYearQueryUsesSelectedDatabaseYear() = runTest {
        val queryService = FakeQueryService()
        val viewModel = QueryViewModel(
            workspaceService = FakeWorkspaceService(),
            queryService = queryService,
            sessionBus = AppSessionBus(),
        )
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
        val viewModel = QueryViewModel(
            workspaceService = FakeWorkspaceService(),
            queryService = queryService,
            sessionBus = AppSessionBus(),
        )
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
        val viewModel = QueryViewModel(
            workspaceService = FakeWorkspaceService(),
            queryService = FakeQueryService(),
            sessionBus = AppSessionBus(),
        )
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
        val viewModel = QueryViewModel(
            workspaceService = FakeWorkspaceService(),
            queryService = queryService,
            sessionBus = AppSessionBus(),
        )
        advanceUntilIdle()

        viewModel.runMonthQuery()
        advanceUntilIdle()

        assertEquals(QueryViewMode.STRUCTURED, viewModel.state.value.selectedQueryViewMode)
    }
}
