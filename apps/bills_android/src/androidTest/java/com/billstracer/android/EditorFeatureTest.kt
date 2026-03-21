package com.billstracer.android

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertIsEnabled
import androidx.compose.ui.test.assertIsNotEnabled
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onAllNodesWithTag
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performTextReplacement
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class EditorFeatureTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<MainActivity>()

    @Test
    fun editorPageOpensTemplateAndEnablesSaveFlow() {
        composeRule.onNodeWithTag("tab_editor").performClick()

        composeRule.onNodeWithTag("editor_manual_period_year_field").performTextReplacement("2026")
        composeRule.onNodeWithTag("editor_manual_period_month_field").performTextReplacement("13")
        composeRule.onNodeWithTag("editor_open_manual_button").assertIsNotEnabled()
        composeRule.onNodeWithTag("editor_manual_period_month_field").performTextReplacement("03")
        composeRule.onNodeWithTag("editor_open_manual_button").performClick()

        composeRule.waitUntil(timeoutMillis = 10_000) {
            composeRule.onAllNodesWithTag("editor_record_field").fetchSemanticsNodes().isNotEmpty()
        }

        composeRule.onNodeWithTag("editor_record_field")
            .performTextReplacement("date:2026-03\nremark:test\n\nmeal\nmeal_low\n")
        composeRule.onNodeWithTag("editor_preview_button").assertIsEnabled()
        composeRule.onNodeWithTag("editor_save_button").assertIsEnabled()
    }
}
