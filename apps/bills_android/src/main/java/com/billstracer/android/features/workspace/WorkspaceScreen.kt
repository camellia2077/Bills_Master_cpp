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
import com.billstracer.android.app.navigation.AppSessionState
import com.billstracer.android.BuildConfig
import com.billstracer.android.platform.PaneContent
import com.billstracer.android.platform.SectionGroupCard

@Composable
internal fun WorkspaceScreen(
    sessionState: AppSessionState,
    state: WorkspaceUiState,
    onImportBundledSample: () -> Unit,
    onRequestExportDirectory: () -> Unit,
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
                    "db=${environment.dbFile.name}  sample=${environment.bundledSampleLabel}  period=${environment.bundledSampleMonth}"
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
        if (BuildConfig.DEBUG) {
            SectionGroupCard(title = "Debug") {
                Text(
                    text = "Debug build only. Sample import and export tools are excluded from release builds.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
                Button(
                    onClick = onImportBundledSample,
                    enabled = !state.isInitializing && !state.isWorking,
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Text("Import bundled sample")
                }
                Button(
                    onClick = onRequestExportDirectory,
                    enabled = !state.isInitializing && !state.isWorking,
                    modifier = Modifier
                        .fillMaxWidth()
                        .testTag("workspace_export_button"),
                ) {
                    Text("Export TXT and configs")
                }
                state.importResult?.let { result ->
                    Text(
                        text = "processed ${result.processed}  imported ${result.imported}  failure ${result.failure}",
                        style = MaterialTheme.typography.bodySmall,
                        fontFamily = FontFamily.Monospace,
                    )
                    if (!result.ok && result.message.isNotBlank()) {
                        Text(
                            text = result.message,
                            color = MaterialTheme.colorScheme.error,
                            style = MaterialTheme.typography.bodyMedium,
                        )
                    }
                }
                state.lastExportResult?.destinationDisplayPath?.let { destination ->
                    Text(
                        text = "Last export: $destination",
                        style = MaterialTheme.typography.bodySmall,
                        fontFamily = FontFamily.Monospace,
                    )
                }
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
