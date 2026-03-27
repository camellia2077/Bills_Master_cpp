package com.billstracer.android.features.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import com.billstracer.android.model.VersionInfo

@Composable
internal fun VersionInfoBlock(
    coreVersion: VersionInfo?,
    androidVersion: VersionInfo?,
) {
    Surface(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("settings_versions_block"),
        shape = RoundedCornerShape(18.dp),
        color = MaterialTheme.colorScheme.surface.copy(alpha = 0.92f),
    ) {
        Column(
            modifier = Modifier.padding(14.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            Text(
                text = "Versions",
                style = MaterialTheme.typography.titleMedium,
                fontFamily = FontFamily.Monospace,
            )
            VersionLine(label = "core", version = coreVersion, fallback = "Loading core version...")
            VersionLine(label = "android", version = androidVersion, fallback = "Loading android version...")
        }
    }
}

@Composable
private fun VersionLine(
    label: String,
    version: VersionInfo?,
    fallback: String,
) {
    val details = if (version == null) {
        fallback
    } else {
        buildString {
            append(version.versionName)
            version.versionCode?.let { code ->
                append(" (code ")
                append(code)
                append(')')
            }
            version.lastUpdated?.takeIf { it.isNotBlank() }?.let { lastUpdated ->
                append("  updated ")
                append(lastUpdated)
            }
        }
    }
    Text(
        text = "$label: $details",
        style = MaterialTheme.typography.bodyMedium,
        fontFamily = FontFamily.Monospace,
    )
}
