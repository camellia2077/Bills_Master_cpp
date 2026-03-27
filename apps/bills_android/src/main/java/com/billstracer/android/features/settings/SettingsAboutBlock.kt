package com.billstracer.android.features.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import com.billstracer.android.model.BundledNotices

@Composable
internal fun AboutBlock(notices: BundledNotices) {
    var showRawJson by rememberSaveable(notices.markdownText, notices.rawJson) {
        mutableStateOf(false)
    }
    val scrollState = rememberScrollState()
    val title = if (showRawJson) "notices.json" else "NOTICE.md"
    val content = if (showRawJson) notices.rawJson else notices.markdownText
    val toggleLabel = if (showRawJson) "Show NOTICE.md" else "Show Raw JSON"
    val cardTag = if (showRawJson) "settings_notices_json_card" else "settings_notices_markdown_card"
    val textTag = if (showRawJson) "settings_notices_json_text" else "settings_notices_markdown_text"

    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Text(
            text = "About",
            style = MaterialTheme.typography.titleMedium,
            fontFamily = FontFamily.Monospace,
        )
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
        ) {
            Text(
                text = title,
                style = MaterialTheme.typography.titleMedium,
                fontFamily = FontFamily.Monospace,
            )
            Button(
                onClick = { showRawJson = !showRawJson },
                modifier = Modifier.widthIn(min = 132.dp),
            ) {
                Text(toggleLabel)
            }
        }
        Surface(
            modifier = Modifier
                .fillMaxWidth()
                .heightIn(max = 420.dp)
                .testTag(cardTag),
            shape = RoundedCornerShape(18.dp),
            color = MaterialTheme.colorScheme.surface.copy(alpha = 0.92f),
        ) {
            SelectionContainer {
                Text(
                    text = content,
                    modifier = Modifier
                        .fillMaxWidth()
                        .verticalScroll(scrollState)
                        .padding(14.dp)
                        .testTag(textTag),
                    style = MaterialTheme.typography.bodyMedium,
                    fontFamily = FontFamily.Monospace,
                )
            }
        }
    }
}
