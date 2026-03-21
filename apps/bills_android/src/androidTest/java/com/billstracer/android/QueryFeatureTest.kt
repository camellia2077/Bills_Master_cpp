package com.billstracer.android

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertIsEnabled
import androidx.compose.ui.test.assertTextEquals
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performTextReplacement
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class QueryFeatureTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<MainActivity>()

    @Test
    fun queryPageUsesEditableDateInputs() {
        composeRule.onNodeWithTag("tab_query").performClick()

        composeRule.onNodeWithTag("query_year_field").assertIsDisplayed()
        composeRule.onNodeWithTag("query_month_period_field").assertIsDisplayed()
        composeRule.onNodeWithTag("query_year_field").performTextReplacement("2024")
        composeRule.onNodeWithTag("query_run_year_button").assertIsEnabled()
        composeRule.onNodeWithTag("query_month_year_field").performTextReplacement("2025")
        composeRule.onNodeWithTag("query_month_month_field").performTextReplacement("3")
        composeRule.onNodeWithTag("query_month_month_field").assertTextEquals("03")
        composeRule.onNodeWithTag("query_run_month_button").assertIsEnabled()
    }
}
