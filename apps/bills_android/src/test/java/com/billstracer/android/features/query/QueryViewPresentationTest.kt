package com.billstracer.android.features.query

import com.billstracer.android.fakeYearStandardReportJson
import com.billstracer.android.fakeRangeStandardReportJson
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class QueryViewPresentationTest {
    @Test
    fun resolveQueryModeAvailabilityOmitsChartModeWhenChartViewsAreUnavailable() {
        val result = QueryResult(
            ok = true,
            message = "2026",
            type = QueryType.YEAR,
            periodStart = "2026-01",
            periodEnd = "2026-12",
            transactionCount = 1,
            remark = "",
            year = 2026,
            month = null,
            matchedBills = 1,
            totalIncome = 10.0,
            totalExpense = -5.0,
            balance = 5.0,
            monthlySummary = emptyList(),
            standardReportMarkdown = "# 2026",
            standardReportJson = fakeYearStandardReportJson(
                year = 2026,
                includeChartData = false,
            ),
            rawJson = """{"ok":true}""",
        )

        val availability = resolveQueryModeAvailability(result)

        assertFalse(availability.hasChartView)
        assertEquals(
            listOf(QueryViewMode.STRUCTURED, QueryViewMode.TEXT),
            availability.availableModes(),
        )
    }

    @Test
    fun resolveQueryModeAvailabilitySupportsRangeStructuredView() {
        val result = QueryResult(
            ok = true,
            message = "2026-02 to 2026-03",
            type = QueryType.RANGE,
            periodStart = "2026-02",
            periodEnd = "2026-03",
            transactionCount = 2,
            remark = "",
            year = null,
            month = null,
            matchedBills = 2,
            totalIncome = 20.0,
            totalExpense = -10.0,
            balance = 10.0,
            monthlySummary = emptyList(),
            standardReportMarkdown = "# 2026-02 to 2026-03",
            standardReportJson = fakeRangeStandardReportJson(
                startIsoMonth = "2026-02",
                endIsoMonth = "2026-03",
                includeChartData = false,
            ),
            rawJson = """{"ok":true}""",
        )

        val availability = resolveQueryModeAvailability(result)

        assertTrue(availability.hasStructuredView)
        assertFalse(availability.hasChartView)
    }
}
