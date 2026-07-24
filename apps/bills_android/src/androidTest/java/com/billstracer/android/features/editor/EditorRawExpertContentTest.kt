package com.billstracer.android.features.editor

import androidx.activity.ComponentActivity
import androidx.compose.foundation.layout.Column
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertIsEnabled
import androidx.compose.ui.test.assertTextContains
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.performClick
import androidx.test.espresso.Espresso.closeSoftKeyboard
import androidx.test.espresso.Espresso.onView
import androidx.test.espresso.action.ViewActions.replaceText
import androidx.test.espresso.assertion.ViewAssertions.matches
import androidx.test.espresso.matcher.ViewMatchers.isDisplayed
import androidx.test.espresso.matcher.ViewMatchers.withText
import com.billstracer.android.app.theme.BillsAndroidTheme
import com.billstracer.android.model.RecordEditorDocument
import org.hamcrest.CoreMatchers.`is`
import org.hamcrest.CoreMatchers.instanceOf
import org.hamcrest.CoreMatchers.allOf
import org.hamcrest.Matcher
import org.junit.Rule
import org.junit.Test

class EditorRawExpertContentTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<ComponentActivity>()

    @Test
    fun rawExpertInputUpdatesDraftAndEnablesSave() {
        val initialText = "date:2026-03\nremark:\n\nmeal\n\nmeal_low\n"
        composeRule.setContent {
            var state by mutableStateOf(
                EditorUiState(
                    isInitializing = false,
                    activeRecordDocument = RecordEditorDocument(
                        period = "2026-03",
                        relativePath = "2026/2026-03.txt",
                        rawText = initialText,
                        persisted = true,
                    ),
                    recordDraftText = initialText,
                    editorMode = EditorMode.RawExpert,
                ),
            )
            BillsAndroidTheme {
                EditorScreen(
                    state = state,
                    onScreenShown = {},
                    onSelectExistingRecordYear = {},
                    onSelectExistingRecordMonth = {},
                    onSaveStructuredRecord = {},
                    onSaveRawRecordText = {},
                    onRecordDraftChange = { updated ->
                        state = state.copy(recordDraftText = updated)
                    },
                    onStructuredRemarkChange = {},
                    onAddStructuredEntry = { _, _ -> },
                    onRemoveStructuredEntry = { _, _, _ -> },
                    onStructuredEntryAmountChange = { _, _, _, _ -> },
                    onStructuredEntryDescriptionChange = { _, _, _, _ -> },
                    onStructuredEntryCommentChange = { _, _, _, _ -> },
                    onEnterRawExpertMode = {},
                    onReturnToStructuredMode = {},
                )
            }
        }

        composeRule.onNodeWithTag("editor_record_field").assertIsDisplayed()
        onView(withNativeEditorTag()).check(matches(isDisplayed()))
        onView(withNativeEditorTag()).perform(replaceText("date:2026-03\nremark:raw\n\nmeal\n\nmeal_low\n12 lunch"))
        closeSoftKeyboard()

        composeRule.onNodeWithTag("editor_save_button").assertIsEnabled()
    }

    @Test
    fun rawTextEditTextReflectsExternalTextReplacement() {
        composeRule.setContent {
            var rawText by mutableStateOf("date:2026-03\nremark:\n")
            BillsAndroidTheme {
                Column {
                    Button(
                        onClick = { rawText = "date:2026-04\nremark:external\n" },
                        modifier = androidx.compose.ui.Modifier.testTag("replace_text_button"),
                    ) {
                        Text("Replace")
                    }
                    RawTextEditText(
                        value = rawText,
                        onValueChange = { updated -> rawText = updated },
                    )
                    Text(
                        text = rawText,
                        modifier = androidx.compose.ui.Modifier.testTag("raw_text_mirror"),
                    )
                }
            }
        }

        onView(withNativeEditorTag()).perform(replaceText("date:2026-03\nremark:edited\n"))
        closeSoftKeyboard()
        composeRule.onNodeWithTag("raw_text_mirror").assertTextContains("remark:edited")

        composeRule.onNodeWithTag("replace_text_button").performClick()

        composeRule.onNodeWithTag("raw_text_mirror").assertTextContains("date:2026-04")
        onView(withNativeEditorTag()).check(matches(withText("date:2026-04\nremark:external\n")))
    }

    private fun withNativeEditorTag(): Matcher<android.view.View> =
        allOf(
            isDisplayed(),
            androidx.test.espresso.matcher.ViewMatchers.withTagValue(
                `is`("editor_record_field_native" as Any),
            ),
            instanceOf(android.widget.EditText::class.java),
        )
}
