package com.billstracer.android.ui

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.ui.Modifier
import androidx.compose.ui.test.assertCountEquals
import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.junit4.createComposeRule
import androidx.compose.ui.test.onAllNodesWithTag
import androidx.compose.ui.test.onAllNodesWithText
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.billstracer.android.model.MonthlySummaryItem
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import com.billstracer.android.ui.theme.BillsAndroidTheme
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class QueryResultDisplayTest {
    @get:Rule
    val composeRule = createComposeRule()

    @Test
    fun yearQueryDefaultsToMarkdownAndTogglesToRawJsonWithoutSummary() {
        val markdown = """
            # YEAR:2025
            - total income: CNY9586.68
        """.trimIndent()
        val rawJson = """{"ok":true,"query_type":"year"}"""

        composeRule.setContent {
            BillsAndroidTheme {
                Column(modifier = Modifier.verticalScroll(rememberScrollState())) {
                    queryResultDisplay(
                        result = QueryResult(
                            ok = true,
                            message = "ok",
                            type = QueryType.YEAR,
                            year = 2025,
                            month = null,
                            matchedBills = 1,
                            totalIncome = 9586.68,
                            totalExpense = -1511.57,
                            balance = 8075.11,
                            monthlySummary = listOf(
                                MonthlySummaryItem(
                                    month = 1,
                                    income = 9586.68,
                                    expense = -1511.57,
                                    balance = 8075.11,
                                ),
                            ),
                            standardReportMarkdown = markdown,
                            standardReportJson = "{}",
                            rawJson = rawJson,
                        ),
                    )
                }
            }
        }

        composeRule.onNodeWithTag("query_markdown_card").assertIsDisplayed()
        composeRule.onNodeWithTag("query_markdown_text").assertIsDisplayed()
        composeRule.onNodeWithText(markdown).assertIsDisplayed()
        composeRule.onAllNodesWithText("Structured Summary").assertCountEquals(0)
        composeRule.onAllNodesWithText("Monthly Summary").assertCountEquals(0)
        composeRule.onAllNodesWithTag("query_summary_card").assertCountEquals(0)
        composeRule.onAllNodesWithText(rawJson).assertCountEquals(0)

        composeRule.onNodeWithTag("query_toggle_button").performClick()

        composeRule.onNodeWithTag("query_json_card").assertIsDisplayed()
        composeRule.onNodeWithTag("query_json_text").assertIsDisplayed()
        composeRule.onNodeWithText(rawJson).assertIsDisplayed()
        composeRule.onAllNodesWithText(markdown).assertCountEquals(0)
    }

    @Test
    fun monthQueryDefaultsToNativeJsonViewAndTogglesToMarkdown() {
        val markdown = """
            # DATE:202501
            ## income_salary
            - CNY9586.68 工资
        """.trimIndent()
        val standardReportJson = """
            {
              "meta": {
                "schema_version": "1.0",
                "report_type": "monthly",
                "generated_at_utc": "2026-03-10T00:00:00Z",
                "source": "android-test"
              },
              "scope": {
                "period_start": "2025-01-01",
                "period_end": "2025-01-31",
                "remark": "sample month",
                "data_found": true
              },
              "summary": {
                "total_income": 9586.68,
                "total_expense": -1511.57,
                "balance": 8075.11
              },
              "items": {
                "categories": [
                  {
                    "name": "income_salary",
                    "total": 9586.68,
                    "sub_categories": [
                      {
                        "name": "salary",
                        "subtotal": 9586.68,
                        "transactions": [
                          {
                            "parent_category": "income_salary",
                            "sub_category": "salary",
                            "transaction_type": "income",
                            "description": "工资",
                            "source": "txt",
                            "comment": "",
                            "amount": 9586.68
                          }
                        ]
                      }
                    ]
                  }
                ],
                "monthly_summary": []
              },
              "extensions": {}
            }
        """.trimIndent()

        composeRule.setContent {
            BillsAndroidTheme {
                Column(modifier = Modifier.verticalScroll(rememberScrollState())) {
                    queryResultDisplay(
                        result = QueryResult(
                            ok = true,
                            message = "ok",
                            type = QueryType.MONTH,
                            year = 2025,
                            month = 1,
                            matchedBills = 1,
                            totalIncome = 9586.68,
                            totalExpense = -1511.57,
                            balance = 8075.11,
                            monthlySummary = emptyList(),
                            standardReportMarkdown = markdown,
                            standardReportJson = standardReportJson,
                            rawJson = """{"ok":true}""",
                        ),
                    )
                }
            }
        }

        composeRule.onNodeWithTag("query_monthly_standard_card").assertIsDisplayed()
        composeRule.onNodeWithText("Monthly Report").assertIsDisplayed()
        composeRule.onNodeWithText("2025-01-01 to 2025-01-31").assertIsDisplayed()
        composeRule.onNodeWithText("sample month").assertIsDisplayed()
        composeRule.onNodeWithText("income_salary").assertIsDisplayed()
        composeRule.onNodeWithText("工资").assertIsDisplayed()
        composeRule.onNodeWithText("Show Markdown").assertIsDisplayed()
        composeRule.onNodeWithText("Structured Summary").assertIsDisplayed()
        composeRule.onAllNodesWithTag("query_markdown_card").assertCountEquals(0)

        val nativeTop = composeRule.onNodeWithTag("query_monthly_standard_card").fetchSemanticsNode().boundsInRoot.top
        val summaryTop = composeRule.onNodeWithTag("query_summary_card").fetchSemanticsNode().boundsInRoot.top

        assertTrue(nativeTop < summaryTop)

        composeRule.onNodeWithTag("query_toggle_button").performClick()

        composeRule.onNodeWithTag("query_markdown_card").assertIsDisplayed()
        composeRule.onNodeWithTag("query_markdown_text").assertIsDisplayed()
        composeRule.onNodeWithText("DATE:202501").assertIsDisplayed()
        composeRule.onNodeWithText("CNY9586.68 工资").assertIsDisplayed()
        composeRule.onNodeWithText("Show Native View").assertIsDisplayed()
        composeRule.onAllNodesWithTag("query_monthly_standard_card").assertCountEquals(0)
    }
}
