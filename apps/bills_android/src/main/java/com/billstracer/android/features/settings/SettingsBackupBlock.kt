package com.billstracer.android.features.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import com.billstracer.android.platform.SectionGroupCard

@Composable
internal fun BackupSettingsBlock(
    state: SettingsUiState,
    onRequestExportBackup: () -> Unit,
    onRequestImportBackup: () -> Unit,
) {
    SectionGroupCard(title = "Backup Bundle") {
        Text(
            text = "Backup bundles are meant for device migration. They include records/*.txt plus validator_config.toml and modifier_config.toml.",
            style = MaterialTheme.typography.bodySmall,
            fontFamily = FontFamily.Monospace,
        )
        Text(
            text = "Restore replaces the current TXT workspace with the bundle contents and rebuilds SQLite. export_formats.toml stays on the target device.",
            style = MaterialTheme.typography.bodySmall,
            fontFamily = FontFamily.Monospace,
        )
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            Button(
                onClick = onRequestExportBackup,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .weight(1f)
                    .testTag("settings_export_backup_button"),
            ) {
                Text("Export Backup")
            }
            OutlinedButton(
                onClick = onRequestImportBackup,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .weight(1f)
                    .testTag("settings_import_backup_button"),
            ) {
                Text("Import Backup")
            }
        }
        if (state.statusMessage.isNotBlank()) {
            Text(
                text = state.statusMessage,
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier.testTag("settings_backup_status_message"),
            )
        }
        state.errorMessage?.takeIf { it.isNotBlank() }?.let { errorMessage ->
            Surface(
                color = MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.92f),
                shape = MaterialTheme.shapes.medium,
            ) {
                Text(
                    text = errorMessage,
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(12.dp)
                        .testTag("settings_backup_error_message"),
                    color = MaterialTheme.colorScheme.onErrorContainer,
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
        }
        state.lastExportedBackupResult?.let { result ->
            Text(
                text = "Last export: ${result.exportedRecordFiles} TXT file(s), ${result.exportedConfigFiles} config file(s) to ${result.destinationDisplayPath}.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier.testTag("settings_backup_last_export"),
            )
        }
        state.lastImportedBackupResult?.let { result ->
            val details = buildString {
                append("Last restore: ")
                append(result.restoredRecordFiles)
                append(" TXT file(s), ")
                append(result.restoredConfigFiles)
                append(" config file(s) from ")
                append(result.sourceDisplayPath)
                if (result.ok) {
                    append(", SQLite rebuilt.")
                } else if (!result.failedPhase.isNullOrBlank()) {
                    append(", failed at ")
                    append(result.failedPhase)
                    append('.')
                }
            }
            Text(
                text = details,
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier.testTag("settings_backup_last_import"),
            )
        }
    }
}
