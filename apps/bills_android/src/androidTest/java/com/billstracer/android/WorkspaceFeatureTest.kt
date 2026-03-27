package com.billstracer.android

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onAllNodesWithTag
import androidx.compose.ui.test.onNodeWithTag
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class WorkspaceFeatureTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<MainActivity>()

    @Test
    fun workspacePageShowsImportAndClearActions() {
        composeRule.onNodeWithTag("workspace_import_txt_directory_button").assertIsDisplayed()
        composeRule.onNodeWithTag("workspace_export_button").assertIsDisplayed()
        composeRule.onNodeWithTag("workspace_import_bundle_button").assertIsDisplayed()
        assertTrue(composeRule.onAllNodesWithTag("workspace_clear_txt_button").fetchSemanticsNodes().isNotEmpty())
        assertTrue(composeRule.onAllNodesWithTag("workspace_clear_database_button").fetchSemanticsNodes().isNotEmpty())
    }
}
