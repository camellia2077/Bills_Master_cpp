package com.billstracer.android.features.query

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Test

class YearlyStandardReportParserTest {
    @Test
    fun parseYearlyStandardReportReadsSummaryAndMonthlyRows() {
        val report = parseYearlyStandardReport(
            """
            {
              "meta": {
                "report_type": "yearly"
              },
              "scope": {
                "period_start": "2025-01",
                "period_end": "2025-12",
                "remark": "",
                "data_found": true
              },
              "summary": {
                "total_income": 100.5,
                "total_expense": -20.5,
                "balance": 80.0
              },
              "items": {
                "monthly_summary": [
                  {
                    "month": 1,
                    "income": 10.0,
                    "expense": -1.5,
                    "balance": 11.5
                  },
                  {
                    "month": 2,
                    "income": 20.0,
                    "expense": -2.5,
                    "balance": 22.5
                  }
                ]
              }
            }
            """.trimIndent(),
        )

        assertNotNull(report)
        assertEquals("2025-01", report?.periodStart)
        assertEquals("2025-12", report?.periodEnd)
        assertEquals(100.5, report?.totalIncome ?: 0.0, 0.0)
        assertEquals(-20.5, report?.totalExpense ?: 0.0, 0.0)
        assertEquals(80.0, report?.balance ?: 0.0, 0.0)
        assertEquals(2, report?.monthlySummary?.size)
        assertEquals(1, report?.monthlySummary?.first()?.month)
    }

    @Test
    fun parseYearlyStandardReportRejectsNonYearlyPayload() {
        val report = parseYearlyStandardReport(
            """
            {
              "meta": {
                "report_type": "monthly"
              }
            }
            """.trimIndent(),
        )

        assertEquals(null, report)
    }
}
