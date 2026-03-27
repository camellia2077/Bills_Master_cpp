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
class MainActivityNavigationTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<MainActivity>()

    @Test
    fun appShowsBottomTabsAndDefaultsToWorkspace() {
        composeRule.onNodeWithTag("tab_workspace").assertIsDisplayed()
        composeRule.onNodeWithTag("tab_editor").assertIsDisplayed()
        composeRule.onNodeWithTag("tab_query").assertIsDisplayed()
        composeRule.onNodeWithTag("tab_settings").assertIsDisplayed()
        composeRule.onNodeWithTag("workspace_import_txt_directory_button").assertIsDisplayed()
    }
}
