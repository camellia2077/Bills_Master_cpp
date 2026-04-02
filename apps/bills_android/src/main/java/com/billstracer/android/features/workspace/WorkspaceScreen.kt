package com.billstracer.android.features.workspace

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import com.billstracer.android.platform.PaneContent
import com.billstracer.android.platform.SectionGroupCard
import com.billstracer.android.app.navigation.AppSessionState

@Composable
internal fun WorkspaceScreen(
    sessionState: AppSessionState,
    state: WorkspaceUiState,
    onRequestImportTxtDirectory: () -> Unit,
    onRequestExportTextAndConfigZip: () -> Unit,
    onClearRecordFiles: () -> Unit,
    onClearDatabase: () -> Unit,
    modifier: Modifier = Modifier,
) {
    PaneContent(modifier = modifier) {
        SectionGroupCard(title = "Overview") {
            val environment = sessionState.environment ?: state.environment
            Text(
                text = sessionState.globalStatusMessage,
                style = MaterialTheme.typography.bodyLarge,
                fontFamily = FontFamily.Monospace,
            )
            sessionState.globalErrorMessage?.takeIf { it.isNotBlank() }?.let { errorMessage ->
                Surface(
                    color = MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.92f),
                    shape = MaterialTheme.shapes.medium,
                ) {
                    Text(
                        text = errorMessage,
                        modifier = Modifier.fillMaxWidth(),
                        color = MaterialTheme.colorScheme.onErrorContainer,
                        style = MaterialTheme.typography.bodyMedium,
                        fontFamily = FontFamily.Monospace,
                    )
                }
            }
            Text(
                text = if (environment == null) {
                    if (state.isInitializing) {
                        "Preparing private workspace..."
                    } else {
                        "Workspace is unavailable."
                    }
                } else {
                    "db=${environment.dbFile.name}"
                },
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                Text(
                    text = "core=${sessionState.coreVersion?.versionName ?: "loading"}",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
                Text(
                    text = "android=${sessionState.androidVersion?.versionName ?: "loading"}",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
        }
        SectionGroupCard(title = "Data") {
            Text(
                text = "Select a directory and import valid TXT files into the private workspace. Successful periods are copied into records/ and synced to SQLite immediately.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            Button(
                onClick = onRequestImportTxtDirectory,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("workspace_import_txt_directory_button"),
            ) {
                Text("Import TXT from directory")
            }
            Button(
                onClick = onRequestExportTextAndConfigZip,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("workspace_export_text_and_config_zip_button"),
            ) {
                Text("Export TXT + Config ZIP")
            }
            state.recordDirectoryImportResult?.let { result ->
                Text(
                    text = "processed ${result.processed}  imported ${result.imported}  overwritten ${result.overwritten}  failure ${result.failure}  invalid ${result.invalid}  duplicate ${result.duplicatePeriodConflicts}",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
                result.firstFailureMessage?.takeIf { it.isNotBlank() }?.let { failureMessage ->
                    Text(
                        text = failureMessage,
                        color = if (result.failure > 0) {
                            MaterialTheme.colorScheme.error
                        } else {
                            MaterialTheme.colorScheme.onSurface
                        },
                        style = MaterialTheme.typography.bodyMedium,
                    )
                }
            }
            state.lastExportResult?.destinationDisplayPath?.let { destination ->
                Text(
                    text = "Last ZIP export: $destination",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
        }
        SectionGroupCard(title = "Workspace") {
            Button(
                onClick = onClearRecordFiles,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("workspace_clear_txt_button"),
            ) {
                Text("Clear all TXT files")
            }
            Button(
                onClick = onClearDatabase,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("workspace_clear_database_button"),
            ) {
                Text("Clear database")
            }
        }
    }
}
