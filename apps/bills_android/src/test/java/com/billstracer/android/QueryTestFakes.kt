package com.billstracer.android

import com.billstracer.android.data.services.QueryService
import com.billstracer.android.model.MonthlySummaryItem
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType

internal class FakeQueryService : QueryService {
    var availablePeriods: List<String> = listOf("2026-03", "2026-02", "2025-12")
    var lastQueriedYear: String? = null
    var lastQueriedMonth: String? = null
    var yearQueryResultOverride: QueryResult? = null
    var monthQueryResultOverride: QueryResult? = null

    override suspend fun listAvailablePeriods(): List<String> = availablePeriods

    override suspend fun queryYear(isoYear: String): QueryResult {
        lastQueriedYear = isoYear
        return yearQueryResultOverride ?: QueryResult(
            ok = true,
            message = isoYear,
            type = QueryType.YEAR,
            year = isoYear.toIntOrNull(),
            month = null,
            matchedBills = 1,
            totalIncome = 10.0,
            totalExpense = -5.0,
            balance = 5.0,
            monthlySummary = listOf(
                MonthlySummaryItem(month = 1, income = 10.0, expense = -5.0, balance = 5.0),
            ),
            standardReportMarkdown = "# $isoYear",
            standardReportJson = fakeYearStandardReportJson(isoYear.toIntOrNull() ?: 2026),
            rawJson = """{"ok":true}""",
        )
    }

    override suspend fun queryMonth(isoMonth: String): QueryResult {
        lastQueriedMonth = isoMonth
        return monthQueryResultOverride ?: QueryResult(
            ok = true,
            message = isoMonth,
            type = QueryType.MONTH,
            year = isoMonth.substringBefore('-').toIntOrNull(),
            month = isoMonth.substringAfter('-').toIntOrNull(),
            matchedBills = 1,
            totalIncome = 10.0,
            totalExpense = -5.0,
            balance = 5.0,
            monthlySummary = emptyList(),
            standardReportMarkdown = "# $isoMonth",
            standardReportJson = fakeMonthStandardReportJson(isoMonth),
            rawJson = """{"ok":true}""",
        )
    }
}

internal fun fakeYearStandardReportJson(
    year: Int = 2026,
    includeChartData: Boolean = true,
): String {
    val chartData = if (includeChartData) {
        """
        "chart_data": {
          "schema_version": "1.0.0",
          "views": [
            {
              "id": "yearly_monthly_overview",
              "title": "Monthly Income, Expense, and Balance",
              "chart_type": "grouped_bar",
              "x_labels": ["01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12"],
              "series": [
                { "id": "income", "label": "Income", "unit": "CNY", "color": "#2563EB", "values": [10.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0] },
                { "id": "expense", "label": "Expense", "unit": "CNY", "color": "#DC2626", "values": [5.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0] },
                { "id": "balance", "label": "Balance", "unit": "CNY", "color": "#7C3AED", "values": [5.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0] }
              ]
            }
          ]
        }
        """.trimIndent()
    } else {
        """
        "chart_data": {
          "schema_version": "1.0.0",
          "views": []
        }
        """.trimIndent()
    }
    return """
        {
          "meta": {
            "report_type": "yearly"
          },
          "scope": {
            "period_start": "$year-01",
            "period_end": "$year-12",
            "remark": "",
            "data_found": true
          },
          "summary": {
            "total_income": 10.0,
            "total_expense": -5.0,
            "balance": 5.0
          },
          "items": {
            "monthly_summary": [
              {
                "month": 1,
                "income": 10.0,
                "expense": -5.0,
                "balance": 5.0
              }
            ]
          },
          "extensions": {
            $chartData
          }
        }
    """.trimIndent()
}

internal fun fakeMonthStandardReportJson(
    isoMonth: String = "2026-03",
    includeChartData: Boolean = true,
): String {
    val chartData = if (includeChartData) {
        """
        "chart_data": {
          "schema_version": "1.0.0",
          "views": [
            {
              "id": "monthly_expense_by_category",
              "title": "Expense by Category",
              "chart_type": "pie",
              "unit": "CNY",
              "segments": [
                { "id": "meal", "label": "meal", "value": 5.0, "color": "#2563EB" }
              ]
            }
          ]
        }
        """.trimIndent()
    } else {
        """
        "chart_data": {
          "schema_version": "1.0.0",
          "views": []
        }
        """.trimIndent()
    }
    return """
        {
          "meta": {
            "report_type": "monthly"
          },
          "scope": {
            "period_start": "$isoMonth",
            "period_end": "$isoMonth",
            "remark": "",
            "data_found": true
          },
          "summary": {
            "total_income": 10.0,
            "total_expense": -5.0,
            "balance": 5.0
          },
          "items": {
            "categories": [
              {
                "name": "meal",
                "total": -5.0,
                "sub_categories": [
                  {
                    "name": "meal_low",
                    "subtotal": -5.0,
                    "transactions": [
                      {
                        "description": "lunch",
                        "source": "",
                        "comment": "",
                        "transaction_type": "",
                        "amount": -5.0
                      }
                    ]
                  }
                ]
              }
            ],
            "monthly_summary": []
          },
          "extensions": {
            $chartData
          }
        }
    """.trimIndent()
}
