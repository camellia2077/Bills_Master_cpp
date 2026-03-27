package com.billstracer.android.features.query

import com.billstracer.android.fakeYearStandardReportJson
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Test

class QueryViewPresentationTest {
    @Test
    fun resolveQueryModeAvailabilityOmitsChartModeWhenChartViewsAreUnavailable() {
        val result = QueryResult(
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
}
