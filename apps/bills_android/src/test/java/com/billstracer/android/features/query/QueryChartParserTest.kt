package com.billstracer.android.features.query

import com.billstracer.android.fakeMonthStandardReportJson
import com.billstracer.android.fakeYearStandardReportJson
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Test

class QueryChartParserTest {
    @Test
    fun parseQueryChartDataReadsGroupedBarView() {
        val chart = parseQueryChartData(fakeYearStandardReportJson(year = 2026))

        assertNotNull(chart)
        val view = chart?.views?.singleOrNull() as? GroupedBarChartViewUiModel
        assertNotNull(view)
        assertEquals("yearly_monthly_overview", view?.id)
        assertEquals(12, view?.xLabels?.size)
        assertEquals(3, view?.series?.size)
        assertEquals("#2563EB", view?.series?.first()?.colorHex)
        assertEquals(5.0, view?.series?.get(1)?.values?.first() ?: 0.0, 0.0)
    }

    @Test
    fun parseQueryChartDataReadsPieView() {
        val chart = parseQueryChartData(fakeMonthStandardReportJson("2026-03"))

        assertNotNull(chart)
        val view = chart?.views?.singleOrNull() as? PieChartViewUiModel
        assertNotNull(view)
        assertEquals("monthly_expense_by_category", view?.id)
        assertEquals("CNY", view?.unit)
        assertEquals(1, view?.segments?.size)
        assertEquals("#2563EB", view?.segments?.first()?.colorHex)
        assertEquals(5.0, view?.segments?.first()?.value ?: 0.0, 0.0)
    }

    @Test
    fun parseQueryChartDataKeepsWorkingWhenColorFieldIsMissing() {
        val chart = parseQueryChartData(
            """
            {
              "extensions": {
                "chart_data": {
                  "schema_version": "1.0.0",
                  "views": [
                    {
                      "id": "yearly_monthly_overview",
                      "title": "Monthly Income, Expense, and Balance",
                      "chart_type": "grouped_bar",
                      "x_labels": ["01"],
                      "series": [
                        { "id": "income", "label": "Income", "unit": "CNY", "values": [10.0] }
                      ]
                    },
                    {
                      "id": "monthly_expense_by_category",
                      "title": "Expense by Category",
                      "chart_type": "pie",
                      "unit": "CNY",
                      "segments": [
                        { "id": "meal", "label": "meal", "value": 5.0 }
                      ]
                    }
                  ]
                }
              }
            }
            """.trimIndent(),
        )

        assertNotNull(chart)
        val groupedBar = chart?.views?.get(0) as? GroupedBarChartViewUiModel
        val pie = chart?.views?.get(1) as? PieChartViewUiModel
        assertNull(groupedBar?.series?.first()?.colorHex)
        assertNull(pie?.segments?.first()?.colorHex)
    }

    @Test
    fun parseQueryChartDataReturnsNullForMissingOrInvalidChartData() {
        assertNull(parseQueryChartData("""{"meta":{"report_type":"yearly"}}"""))
        assertNull(
            parseQueryChartData(
                """
                {
                  "extensions": {
                    "chart_data": {
                      "schema_version": "1.0.0",
                      "views": [
                        {
                          "id": "broken",
                          "title": "Broken",
                          "chart_type": "grouped_bar",
                          "x_labels": ["01"],
                          "series": [
                            { "id": "income", "label": "Income", "unit": "CNY", "values": [1.0, 2.0] }
                          ]
                        }
                      ]
                    }
                  }
                }
                """.trimIndent(),
            ),
        )
    }
}
