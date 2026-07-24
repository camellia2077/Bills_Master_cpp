package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.parseRoot
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test
import kotlinx.serialization.json.jsonObject

class DefaultQueryServiceParsingTest {
    @Test
    fun parseQueryResultPayloadReadsRangeAwareFields() {
        val data = parseRoot(
            """
            {
              "ok": true,
              "message": "Year query completed successfully.",
              "data": {
                "query_type": "year",
                "query_value": "2025",
                "period_start": "2025-01",
                "period_end": "2025-12",
                "matched_bills": 4,
                "transaction_count": 9,
                "total_income": 100.5,
                "total_expense": -20.5,
                "balance": 80.0,
                "remark": "annual summary",
                "monthly_summary": [
                  {
                    "period": "2025-01",
                    "income": 10.0,
                    "expense": -1.5,
                    "balance": 8.5
                  }
                ],
                "report_markdown": "# report",
                "standard_report": {
                  "meta": {
                    "report_type": "yearly"
                  }
                }
              }
            }
            """.trimIndent(),
        )["data"]!!.jsonObject

        val payload = parseQueryResultPayload(data)

        assertEquals("2025-01", payload.periodStart)
        assertEquals("2025-12", payload.periodEnd)
        assertEquals(9, payload.transactionCount)
        assertEquals("annual summary", payload.remark)
        assertEquals(2025, payload.year)
        assertEquals(1, payload.month)
        assertEquals(4, payload.matchedBills)
        assertEquals(100.5, payload.totalIncome, 0.0)
        assertEquals(-20.5, payload.totalExpense, 0.0)
        assertEquals(80.0, payload.balance, 0.0)
        assertEquals(1, payload.monthlySummary.size)
        assertEquals("2025-01", payload.monthlySummary.first().period)
    }

    @Test
    fun yearQueryResultKeepsDerivedYearButDropsMonthCompatibilityField() {
        val serviceParserInput = """
            {
              "ok": true,
              "message": "ok",
              "data": {
                "period_start": "2025-01",
                "period_end": "2025-12",
                "matched_bills": 1,
                "transaction_count": 2,
                "total_income": 1.0,
                "total_expense": -1.0,
                "balance": 0.0,
                "remark": "",
                "monthly_summary": []
              }
            }
        """.trimIndent()

        val root = parseRoot(serviceParserInput)
        val payload = parseQueryResultPayload(root["data"]!!.jsonObject)

        assertEquals(2025, payload.year)
        assertEquals(1, payload.month)

        val queryResult = com.billstracer.android.model.QueryResult(
            ok = true,
            message = "ok",
            type = com.billstracer.android.model.QueryType.YEAR,
            periodStart = payload.periodStart,
            periodEnd = payload.periodEnd,
            transactionCount = payload.transactionCount,
            remark = payload.remark,
            year = payload.year,
            month = null,
            matchedBills = payload.matchedBills,
            totalIncome = payload.totalIncome,
            totalExpense = payload.totalExpense,
            balance = payload.balance,
            monthlySummary = payload.monthlySummary,
            standardReportMarkdown = payload.standardReportMarkdown,
            standardReportJson = payload.standardReportJson,
            rawJson = serviceParserInput,
        )

        assertEquals(2025, queryResult.year)
        assertNull(queryResult.month)
    }
}
