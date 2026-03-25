package com.billstracer.android

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
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
}
