package com.billstracer.android

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertIsEnabled
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onAllNodesWithText
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performTextReplacement
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class MainActivityTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<MainActivity>()

    @Test
    fun dataReportAndConfigTabsRender() {
        composeRule.onNodeWithText("Data").fetchSemanticsNode()
        composeRule.onNodeWithText("Record").fetchSemanticsNode()
        composeRule.onNodeWithText("Report").fetchSemanticsNode()
        composeRule.onNodeWithText("Config").fetchSemanticsNode()
        composeRule.onNodeWithText("Import bundled sample").fetchSemanticsNode()
        composeRule.onNodeWithText("Clear database").fetchSemanticsNode()
        composeRule.onNodeWithText("Record").performClick()
        composeRule.waitUntil(timeoutMillis = 5_000) {
            composeRule.onAllNodesWithText("Open Existing").fetchSemanticsNodes().isNotEmpty()
        }
        composeRule.onNodeWithTag("record_year_selector_button").assertIsDisplayed()
        composeRule.onNodeWithTag("record_month_selector_button").assertIsDisplayed()
        composeRule.onNodeWithTag("record_open_existing_button").assertIsDisplayed()
        composeRule.onNodeWithTag("record_new_current_button").assertIsDisplayed()
        composeRule.onNodeWithTag("record_manual_period_field").assertIsDisplayed()
        composeRule.onNodeWithTag("record_open_manual_button").assertIsDisplayed()
        composeRule.onNodeWithTag("record_refresh_button").assertIsDisplayed()
        composeRule.onNodeWithText("Report").performClick()
        composeRule.waitUntil(timeoutMillis = 5_000) {
            composeRule.onAllNodesWithText("Query fixed year 2025").fetchSemanticsNodes().isNotEmpty()
        }
        composeRule.onNodeWithText("Query fixed year 2025").fetchSemanticsNode()
        composeRule.onNodeWithText("Query fixed month 2025-01").fetchSemanticsNode()
        composeRule.onNodeWithText("Config").performClick()
        composeRule.waitUntil(timeoutMillis = 5_000) {
            composeRule.onAllNodesWithText("validator_config.toml").fetchSemanticsNodes().isNotEmpty()
        }
        composeRule.onNodeWithText("validator_config.toml").assertIsDisplayed()
        composeRule.onNodeWithTag("config_selector_button").assertIsDisplayed()
        composeRule.onNodeWithTag("config_editor_field").assertIsDisplayed()
        composeRule.onNodeWithTag("config_modify_button").assertIsDisplayed()
        composeRule.onNodeWithTag("config_toml_button").assertIsDisplayed()
        composeRule.onNodeWithTag("config_theme_button").assertIsDisplayed()
        composeRule.onNodeWithTag("config_about_button").assertIsDisplayed()
        composeRule.onNodeWithTag("config_theme_button").performClick()
        composeRule.onNodeWithTag("theme_palette_ember").assertIsDisplayed()
        composeRule.onNodeWithTag("config_about_button").performClick()
        composeRule.waitUntil(timeoutMillis = 5_000) {
            composeRule.onAllNodesWithText("NOTICE.md").fetchSemanticsNodes().isNotEmpty()
        }
        composeRule.onNodeWithText("About").assertIsDisplayed()
        composeRule.onNodeWithText("NOTICE.md").assertIsDisplayed()
        composeRule.onNodeWithText("Show Raw JSON").assertIsDisplayed()
    }

    @Test
    fun recordTabOpensTemplateAndEnablesSaveFlow() {
        composeRule.onNodeWithText("Record").performClick()
        composeRule.onNodeWithTag("record_manual_period_field").performTextReplacement("2026-03")
        composeRule.onNodeWithTag("record_open_manual_button").performClick()

        composeRule.waitUntil(timeoutMillis = 10_000) {
            composeRule.onAllNodesWithText("Save Record").fetchSemanticsNodes().isNotEmpty()
        }

        composeRule.onNodeWithTag("record_editor_field")
            .performTextReplacement("date:2026-03\nremark:test\n\nmeal\nmeal_low\n")
        composeRule.onNodeWithTag("record_preview_button").assertIsEnabled()
        composeRule.onNodeWithTag("record_save_button").assertIsEnabled()
    }

    @Test
    fun configEditorKeepsDraftLocalUntilModify() {
        composeRule.onNodeWithText("Config").performClick()

        composeRule.waitUntil(timeoutMillis = 5_000) {
            composeRule.onAllNodesWithText("validator_config.toml").fetchSemanticsNodes().isNotEmpty()
        }

        composeRule.onNodeWithTag("config_editor_field")
            .performTextReplacement("enabled_formats = [\"json\"]\n")
        composeRule.onNodeWithTag("config_modify_button").assertIsEnabled()
        composeRule.onNodeWithText("Draft changes stay local until you press Modify.").assertIsDisplayed()
    }

    @Test
    fun themeSettingsAllowDraftAndApplyFlow() {
        composeRule.onNodeWithText("Config").performClick()
        composeRule.onNodeWithTag("config_theme_button").performClick()

        composeRule.onNodeWithTag("theme_mode_dark").performClick()
        composeRule.onNodeWithTag("theme_palette_harbor").performClick()
        composeRule.onNodeWithTag("theme_apply_button").assertIsEnabled()
        composeRule.onNodeWithText("Theme changes are pending. Press Apply Theme to persist them with DataStore.").assertIsDisplayed()
    }

    @Test
    fun importBundledSampleShowsResultStats() {
        composeRule.onNodeWithText("Import bundled sample").performClick()

        composeRule.waitUntil(timeoutMillis = 15_000) {
            composeRule.onAllNodesWithText("processed").fetchSemanticsNodes().isNotEmpty()
        }

        composeRule.onNodeWithText("processed").assertIsDisplayed()
        composeRule.onNodeWithText("imported").assertIsDisplayed()
    }
}
