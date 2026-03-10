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
                    QueryResultDisplay(
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
    fun monthQueryShowsMarkdownBeforeStructuredSummary() {
        val markdown = """
            # DATE:202501
            ## income_salary
            - CNY9586.68 工资
        """.trimIndent()

        composeRule.setContent {
            BillsAndroidTheme {
                Column(modifier = Modifier.verticalScroll(rememberScrollState())) {
                    QueryResultDisplay(
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
                            standardReportJson = "{}",
                            rawJson = """{"ok":true}""",
                        ),
                    )
                }
            }
        }

        composeRule.onNodeWithText("Markdown Report").assertIsDisplayed()
        composeRule.onNodeWithTag("query_markdown_text").assertIsDisplayed()
        composeRule.onNodeWithText(markdown).assertIsDisplayed()
        composeRule.onNodeWithText("Structured Summary").assertIsDisplayed()
        composeRule.onNodeWithText("Show Raw JSON").assertIsDisplayed()

        val markdownTop = composeRule.onNodeWithTag("query_markdown_card").fetchSemanticsNode().boundsInRoot.top
        val summaryTop = composeRule.onNodeWithTag("query_summary_card").fetchSemanticsNode().boundsInRoot.top

        assertTrue(markdownTop < summaryTop)
    }
}
