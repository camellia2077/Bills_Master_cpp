package com.billstracer.android

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertIsEnabled
import androidx.compose.ui.test.assertCountEquals
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onAllNodesWithText
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class QueryFeatureTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<MainActivity>()

    @Test
    fun queryPageUsesDatabaseBackedDateSelectors() {
        composeRule.onNodeWithTag("tab_query").performClick()

        composeRule.onNodeWithTag("query_year_selector_button").assertIsDisplayed()
        composeRule.onNodeWithTag("query_month_year_selector_button").assertIsDisplayed()
        composeRule.onNodeWithTag("query_month_month_selector_button").assertIsDisplayed()
    }

    @Test
    fun yearQueryShowsModeSwitchAndChartView() {
        composeRule.waitUntil(timeoutMillis = 10_000) {
            runCatching {
                composeRule.onNodeWithText("Import bundled sample").assertIsDisplayed()
                true
            }.getOrDefault(false)
        }
        composeRule.onNodeWithText("Import bundled sample").performClick()

        composeRule.onNodeWithTag("tab_query").performClick()
        composeRule.waitUntil(timeoutMillis = 10_000) {
            runCatching {
                composeRule.onNodeWithTag("query_run_year_button").assertIsEnabled()
                true
            }.getOrDefault(false)
        }

        composeRule.onNodeWithTag("query_run_year_button").performClick()
        composeRule.waitUntil(timeoutMillis = 10_000) {
            runCatching {
                composeRule.onNodeWithTag("query_result_mode_switch").assertIsDisplayed()
                true
            }.getOrDefault(false)
        }

        composeRule.onNodeWithTag("query_mode_structured_button").assertIsDisplayed()
        composeRule.onNodeWithTag("query_mode_text_button").assertIsDisplayed()
        composeRule.onNodeWithTag("query_mode_chart_button").assertIsDisplayed()
        composeRule.onNodeWithTag("query_mode_chart_button").performClick()
        composeRule.onNodeWithTag("query_chart_subtitle").assertIsDisplayed()
        composeRule.onNodeWithTag("query_chart_y_axis").assertIsDisplayed()
        composeRule.onNodeWithTag("query_chart_y_axis_label_0").assertIsDisplayed()
        composeRule.onNodeWithTag("query_grouped_bar_chart").assertIsDisplayed()
        composeRule.onAllNodesWithText("Raw JSON").assertCountEquals(0)
    }

    @Test
    fun monthQueryChartModeShowsPieChart() {
        composeRule.waitUntil(timeoutMillis = 10_000) {
            runCatching {
                composeRule.onNodeWithText("Import bundled sample").assertIsDisplayed()
                true
            }.getOrDefault(false)
        }
        composeRule.onNodeWithText("Import bundled sample").performClick()

        composeRule.onNodeWithTag("tab_query").performClick()
        composeRule.waitUntil(timeoutMillis = 10_000) {
            runCatching {
                composeRule.onNodeWithTag("query_run_month_button").assertIsEnabled()
                true
            }.getOrDefault(false)
        }

        composeRule.onNodeWithTag("query_run_month_button").performClick()
        composeRule.waitUntil(timeoutMillis = 10_000) {
            runCatching {
                composeRule.onNodeWithTag("query_mode_chart_button").assertIsDisplayed()
                true
            }.getOrDefault(false)
        }

        composeRule.onNodeWithTag("query_mode_chart_button").performClick()
        composeRule.onNodeWithTag("query_pie_chart").assertIsDisplayed()
        composeRule.onNodeWithText("meal").performClick()
        composeRule.onNodeWithTag("query_pie_chart_center").assertIsDisplayed()
        composeRule.onNodeWithTag("query_pie_chart_legend").assertIsDisplayed()
        composeRule.onNodeWithTag("query_pie_chart_selection").assertIsDisplayed()
    }
}
