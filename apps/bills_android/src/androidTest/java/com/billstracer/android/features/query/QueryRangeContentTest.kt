package com.billstracer.android.features.query

import androidx.activity.ComponentActivity
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertIsEnabled
import androidx.compose.ui.test.assertTextContains
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import com.billstracer.android.app.theme.BillsAndroidTheme
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import org.junit.Rule
import org.junit.Test

class QueryRangeContentTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<ComponentActivity>()

    @Test
    fun rangeQuerySectionAllowsSelectingStartEndAndRunningQuery() {
        composeRule.setContent {
            var state by mutableStateOf(
                QueryUiState(
                    isInitializing = false,
                    availablePeriods = listOf("2026-03", "2026-02", "2025-12"),
                    queryRangeStartInput = "2026-03",
                    queryRangeEndInput = "2026-03",
                    queryInputMode = QueryInputMode.RANGE,
                ),
            )
            BillsAndroidTheme {
                QueryScreen(
                    state = state,
                    onSelectQueryYear = {},
                    onSelectQueryPeriodYear = {},
                    onSelectQueryPeriodMonth = {},
                    onSelectQueryRangeStart = { period ->
                        state = state.copy(
                            queryRangeStartInput = period,
                            queryRangeEndInput = if (state.queryRangeEndInput < period) period else state.queryRangeEndInput,
                        )
                    },
                    onSelectQueryRangeEnd = { period ->
                        state = state.copy(
                            queryRangeStartInput = if (state.queryRangeStartInput > period) period else state.queryRangeStartInput,
                            queryRangeEndInput = period,
                        )
                    },
                    onRunYearQuery = {},
                    onRunMonthQuery = {},
                    onRunRangeQuery = {
                        state = state.copy(statusMessage = "Running range query for ${state.queryRangeStartInput} to ${state.queryRangeEndInput}...")
                    },
                    onSelectQueryViewMode = {},
                )
            }
        }

        composeRule.onNodeWithTag("query_range_start_selector_button").assertIsDisplayed().performClick()
        composeRule.onNodeWithText("2025-12").performClick()
        composeRule.onNodeWithTag("query_range_start_selector_button").assertTextContains("2025-12")

        composeRule.onNodeWithTag("query_range_end_selector_button").assertIsDisplayed().performClick()
        composeRule.onNodeWithText("2026-03").performClick()
        composeRule.onNodeWithTag("query_range_end_selector_button").assertTextContains("2026-03")

        composeRule.onNodeWithTag("query_run_range_button").assertIsEnabled().performClick()
        composeRule.onNodeWithTag("query_status_message")
            .assertIsDisplayed()
            .assertTextContains("2025-12 to 2026-03")
    }

    @Test
    fun rangeQueryStructuredViewShowsDedicatedRangeCard() {
        composeRule.setContent {
            BillsAndroidTheme {
                QueryResultDisplayContent(
                    result = QueryResult(
                        ok = true,
                        message = "2026-02 to 2026-03",
                        type = QueryType.RANGE,
                        periodStart = "2026-02",
                        periodEnd = "2026-03",
                        transactionCount = 2,
                        remark = "range remark",
                        year = null,
                        month = null,
                        matchedBills = 2,
                        totalIncome = 20.0,
                        totalExpense = -10.0,
                        balance = 10.0,
                        monthlySummary = emptyList(),
                        standardReportMarkdown = "# range",
                        standardReportJson = fakeRangeStandardReportJson(),
                        rawJson = """{"ok":true}""",
                    ),
                    selectedViewMode = QueryViewMode.STRUCTURED,
                    onSelectViewMode = {},
                )
            }
        }

        composeRule.onNodeWithTag("query_range_standard_card").assertIsDisplayed()
        composeRule.onNodeWithText("Range Report").assertIsDisplayed()
        composeRule.onNodeWithText("2026-02 to 2026-03").assertIsDisplayed()
        composeRule.onNodeWithText("range remark").assertIsDisplayed()
        composeRule.onNodeWithText("Monthly Summary").assertIsDisplayed()
        composeRule.onNodeWithText("2026-02").assertIsDisplayed()
        composeRule.onNodeWithText("2026-03").assertIsDisplayed()
    }

    private fun fakeRangeStandardReportJson(): String = """
        {
          "meta": {
            "report_type": "range"
          },
          "scope": {
            "period_start": "2026-02",
            "period_end": "2026-03",
            "remark": "range remark",
            "data_found": true
          },
          "summary": {
            "total_income": 20.0,
            "total_expense": -10.0,
            "balance": 10.0
          },
          "items": {
            "monthly_summary": [
              {
                "period": "2026-02",
                "income": 10.0,
                "expense": -5.0,
                "balance": 5.0
              },
              {
                "period": "2026-03",
                "income": 10.0,
                "expense": -5.0,
                "balance": 5.0
              }
            ]
          },
          "extensions": {
            "chart_data": {
              "schema_version": "1.0.0",
              "views": []
            }
          }
        }
    """.trimIndent()
}
