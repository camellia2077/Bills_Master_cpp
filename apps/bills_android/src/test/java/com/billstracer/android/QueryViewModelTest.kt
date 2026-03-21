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
    fun runYearQueryUsesEditableInput() = runTest {
        val queryService = FakeQueryService()
        val viewModel = QueryViewModel(
            workspaceService = FakeWorkspaceService(),
            queryService = queryService,
            sessionBus = AppSessionBus(),
        )
        advanceUntilIdle()

        viewModel.updateQueryYearInput("2024")
        viewModel.runYearQuery()
        advanceUntilIdle()

        assertEquals("2024", queryService.lastQueriedYear)
        assertEquals(QueryViewMode.MARKDOWN, viewModel.state.value.selectedQueryViewMode)
    }

    @Test
    fun invalidQueryYearDoesNotRun() = runTest {
        val queryService = FakeQueryService()
        val viewModel = QueryViewModel(
            workspaceService = FakeWorkspaceService(),
            queryService = queryService,
            sessionBus = AppSessionBus(),
        )
        advanceUntilIdle()

        viewModel.updateQueryYearInput("24")
        viewModel.runYearQuery()
        advanceUntilIdle()

        assertEquals(null, queryService.lastQueriedYear)
        assertEquals("Year query must use 4 digits.", viewModel.state.value.errorMessage)
    }

    @Test
    fun monthQueryDefaultsToStructuredView() = runTest {
        val viewModel = QueryViewModel(
            workspaceService = FakeWorkspaceService(),
            queryService = FakeQueryService(),
            sessionBus = AppSessionBus(),
        )
        advanceUntilIdle()

        viewModel.updateQueryPeriodYearInput("2025")
        viewModel.updateQueryPeriodMonthInput("03")
        viewModel.runMonthQuery()
        advanceUntilIdle()

        assertEquals(QueryViewMode.STRUCTURED, viewModel.state.value.selectedQueryViewMode)
    }
}
