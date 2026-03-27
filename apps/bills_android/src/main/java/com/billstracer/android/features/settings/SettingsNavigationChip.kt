package com.billstracer.android.features.settings

import androidx.compose.material3.Button
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily

@Composable
internal fun ConfigModeChip(
    label: String,
    selected: Boolean,
    onClick: () -> Unit,
    testTag: String,
) {
    if (selected) {
        Button(onClick = onClick, modifier = Modifier.testTag(testTag)) {
            Text(text = label, fontFamily = FontFamily.Monospace)
        }
    } else {
        OutlinedButton(onClick = onClick, modifier = Modifier.testTag(testTag)) {
            Text(text = label, fontFamily = FontFamily.Monospace)
        }
    }
}
