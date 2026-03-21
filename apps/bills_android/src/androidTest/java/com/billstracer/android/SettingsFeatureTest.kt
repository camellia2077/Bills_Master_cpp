package com.billstracer.android

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertIsEnabled
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performTextReplacement
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class SettingsFeatureTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<MainActivity>()

    @Test
    fun settingsPageShowsConfigThemeAndAboutFlows() {
        composeRule.onNodeWithTag("tab_settings").performClick()

        composeRule.onNodeWithTag("settings_config_selector_button").assertIsDisplayed()
        composeRule.onNodeWithTag("settings_config_editor_field").performTextReplacement("enabled_formats = [\"json\"]\n")
        composeRule.onNodeWithTag("settings_config_modify_button").assertIsEnabled()

        composeRule.onNodeWithTag("settings_theme_button").performClick()
        composeRule.onNodeWithTag("settings_theme_preview_card").assertIsDisplayed()
        composeRule.onNodeWithTag("settings_theme_color_selector").assertIsDisplayed()

        composeRule.onNodeWithTag("settings_about_button").performClick()
        composeRule.onNodeWithTag("settings_notices_markdown_card").assertIsDisplayed()
    }
}
