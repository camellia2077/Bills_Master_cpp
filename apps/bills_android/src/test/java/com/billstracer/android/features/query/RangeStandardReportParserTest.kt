package com.billstracer.android.features.query

import com.billstracer.android.fakeRangeStandardReportJson
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Test

class RangeStandardReportParserTest {
    @Test
    fun parseRangeStandardReportReadsSummaryAndMonthlyRows() {
        val report = parseRangeStandardReport(
            fakeRangeStandardReportJson(
                startIsoMonth = "2026-02",
                endIsoMonth = "2026-03",
                includeChartData = true,
            ),
        )

        assertNotNull(report)
        assertEquals("2026-02", report?.periodStart)
        assertEquals("2026-03", report?.periodEnd)
        assertEquals(20.0, report?.totalIncome ?: 0.0, 0.0)
        assertEquals(-10.0, report?.totalExpense ?: 0.0, 0.0)
        assertEquals(10.0, report?.balance ?: 0.0, 0.0)
        assertEquals(2, report?.monthlySummary?.size)
        assertEquals("2026-02", report?.monthlySummary?.first()?.period)
    }

    @Test
    fun parseRangeStandardReportRejectsNonRangePayload() {
        val report = parseRangeStandardReport(
            """
            {
              "meta": {
                "report_type": "yearly"
              }
            }
            """.trimIndent(),
        )

        assertEquals(null, report)
    }
}
