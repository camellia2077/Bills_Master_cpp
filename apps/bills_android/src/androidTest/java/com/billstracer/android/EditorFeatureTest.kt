package com.billstracer.android

import android.content.Context
import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertIsEnabled
import androidx.compose.ui.test.assertTextContains
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onAllNodesWithTag
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performTextReplacement
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import kotlinx.coroutines.runBlocking
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class EditorFeatureTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<MainActivity>()

    @Test
    fun editorPageOpensPersistedRecordAndEnablesSaveFlow() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val services = createAndroidServiceBundle(context)
        services.workspaceService.clearRecordFiles()
        services.workspaceService.clearDatabase()
        services.editorService.saveRecordDocument(
            period = "2026-03",
            rawText = "date:2026-03\nremark:test\n\nmeal\nmeal_low 12 lunch\n",
        )
        services.editorService.syncSavedRecordToDatabase("2026-03")

        composeRule.onNodeWithTag("tab_editor").performClick()

        composeRule.waitUntil(timeoutMillis = 10_000) {
            runCatching {
                composeRule.onNodeWithTag("editor_year_selector_button").assertTextContains("2026")
                true
            }.getOrDefault(false)
        }

        composeRule.onNodeWithTag("editor_open_existing_button").assertIsEnabled()
        composeRule.onNodeWithTag("editor_open_existing_button").performClick()

        composeRule.waitUntil(timeoutMillis = 10_000) {
            composeRule.onAllNodesWithTag("editor_record_field").fetchSemanticsNodes().isNotEmpty()
        }

        composeRule.onNodeWithTag("editor_record_field")
            .performTextReplacement("date:2026-03\nremark:updated\n\nmeal\nmeal_low 12 lunch\n")
        composeRule.onNodeWithTag("editor_record_field").assertIsDisplayed()
        composeRule.onNodeWithTag("editor_preview_button").assertIsEnabled()
        composeRule.onNodeWithTag("editor_save_button").assertIsEnabled()
    }
}
