package com.billstracer.android.features.workspace

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.TextButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
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
    onRequestExportWorkspace: () -> Unit,
    onRequestRestoreWorkspace: () -> Unit,
    onClearRecordFiles: () -> Unit,
    onClearDatabase: () -> Unit,
    modifier: Modifier = Modifier,
) {
    var showClearTxtConfirmDialog by remember { mutableStateOf(false) }
    var showClearDatabaseConfirmDialog by remember { mutableStateOf(false) }
    var showRestoreWorkspaceConfirmDialog by remember { mutableStateOf(false) }

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
        SectionGroupCard(title = "Import TXT") {
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
        }
        SectionGroupCard(title = "Export Workspace") {
            Text(
                text = "Export the current private workspace into a reusable ZIP snapshot, including TXT records and config files.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            Button(
                onClick = onRequestExportWorkspace,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("workspace_export_workspace_button"),
            ) {
                Text("Export Workspace")
            }
            state.lastExportedWorkspaceResult?.destinationDisplayPath?.let { destination ->
                Text(
                    text = "Last workspace export: $destination",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
        }
        SectionGroupCard(title = "Restore Workspace") {
            Text(
                text = "Restore the whole private workspace from a workspace ZIP, including TXT records, config, and SQLite rebuild.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            Button(
                onClick = { showRestoreWorkspaceConfirmDialog = true },
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("workspace_restore_backup_button"),
            ) {
                Text("Restore Workspace")
            }
            state.lastImportedBackupResult?.let { result ->
                Text(
                    text = if (result.ok) {
                        "Last restore: ${result.restoredRecordFiles} TXT file(s), ${result.restoredConfigFiles} config file(s) from ${result.sourceDisplayPath}, SQLite rebuilt."
                    } else {
                        "Last restore failed from ${result.sourceDisplayPath}."
                    },
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                    modifier = Modifier.testTag("workspace_last_restore_message"),
                )
            }
            if (!state.backupRestoreFailedPhase.isNullOrBlank() || !state.backupRestoreFirstErrorMessage.isNullOrBlank()) {
                Surface(
                    color = MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.92f),
                    shape = MaterialTheme.shapes.medium,
                    modifier = Modifier
                        .fillMaxWidth()
                        .testTag("workspace_restore_failure_details"),
                ) {
                    Column(
                        modifier = Modifier.padding(12.dp),
                        verticalArrangement = Arrangement.spacedBy(4.dp),
                    ) {
                        state.backupRestoreFailedPhase?.takeIf { it.isNotBlank() }?.let { phase ->
                            Text(
                                text = "Failed phase: $phase",
                                color = MaterialTheme.colorScheme.onErrorContainer,
                                style = MaterialTheme.typography.bodySmall,
                                fontFamily = FontFamily.Monospace,
                            )
                        }
                        state.backupRestoreFirstErrorMessage?.takeIf { it.isNotBlank() }?.let { firstError ->
                            Text(
                                text = "First error: $firstError",
                                color = MaterialTheme.colorScheme.onErrorContainer,
                                style = MaterialTheme.typography.bodySmall,
                                fontFamily = FontFamily.Monospace,
                            )
                        }
                    }
                }
            }
        }
        SectionGroupCard(title = "Workspace") {
            Button(
                onClick = { showClearTxtConfirmDialog = true },
                enabled = !state.isInitializing && !state.isWorking,
                colors = ButtonDefaults.buttonColors(
                    containerColor = MaterialTheme.colorScheme.error,
                    contentColor = MaterialTheme.colorScheme.onError,
                ),
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("workspace_clear_txt_button"),
            ) {
                Icon(
                    imageVector = Icons.Filled.Delete,
                    contentDescription = null,
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text("Clear all TXT files")
            }
            Button(
                onClick = { showClearDatabaseConfirmDialog = true },
                enabled = !state.isInitializing && !state.isWorking,
                colors = ButtonDefaults.buttonColors(
                    containerColor = MaterialTheme.colorScheme.error,
                    contentColor = MaterialTheme.colorScheme.onError,
                ),
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("workspace_clear_database_button"),
            ) {
                Icon(
                    imageVector = Icons.Filled.Delete,
                    contentDescription = null,
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text("Clear database")
            }
        }
    }

    if (showClearTxtConfirmDialog) {
        AlertDialog(
            onDismissRequest = { showClearTxtConfirmDialog = false },
            title = {
                Text(
                    text = "Delete All TXT Files?",
                    fontFamily = FontFamily.Monospace,
                )
            },
            text = {
                Text(
                    text = "This will delete all saved TXT record files in the private workspace. Continue?",
                    fontFamily = FontFamily.Monospace,
                )
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        showClearTxtConfirmDialog = false
                        onClearRecordFiles()
                    },
                    colors = ButtonDefaults.textButtonColors(
                        contentColor = MaterialTheme.colorScheme.error,
                    ),
                    modifier = Modifier.testTag("workspace_clear_txt_confirm_button"),
                ) {
                    Text("Confirm")
                }
            },
            dismissButton = {
                TextButton(
                    onClick = { showClearTxtConfirmDialog = false },
                    colors = ButtonDefaults.textButtonColors(
                        contentColor = MaterialTheme.colorScheme.primary,
                    ),
                    modifier = Modifier.testTag("workspace_clear_txt_cancel_button"),
                ) {
                    Text("Cancel")
                }
            },
        )
    }

    if (showRestoreWorkspaceConfirmDialog) {
        AlertDialog(
            onDismissRequest = { showRestoreWorkspaceConfirmDialog = false },
            title = {
                Text(
                    text = "Restore Workspace?",
                    fontFamily = FontFamily.Monospace,
                )
            },
            text = {
                Text(
                    text = "This restores the whole private workspace from a backup ZIP, including TXT records, config, and SQLite. Continue?",
                    fontFamily = FontFamily.Monospace,
                )
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        showRestoreWorkspaceConfirmDialog = false
                        onRequestRestoreWorkspace()
                    },
                    colors = ButtonDefaults.textButtonColors(
                        contentColor = MaterialTheme.colorScheme.error,
                    ),
                    modifier = Modifier.testTag("workspace_restore_confirm_button"),
                ) {
                    Text("Confirm")
                }
            },
            dismissButton = {
                TextButton(
                    onClick = { showRestoreWorkspaceConfirmDialog = false },
                    colors = ButtonDefaults.textButtonColors(
                        contentColor = MaterialTheme.colorScheme.primary,
                    ),
                    modifier = Modifier.testTag("workspace_restore_cancel_button"),
                ) {
                    Text("Cancel")
                }
            },
        )
    }

    if (showClearDatabaseConfirmDialog) {
        AlertDialog(
            onDismissRequest = { showClearDatabaseConfirmDialog = false },
            title = {
                Text(
                    text = "Delete Database?",
                    fontFamily = FontFamily.Monospace,
                )
            },
            text = {
                Text(
                    text = "This will delete the private workspace database. Continue?",
                    fontFamily = FontFamily.Monospace,
                )
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        showClearDatabaseConfirmDialog = false
                        onClearDatabase()
                    },
                    colors = ButtonDefaults.textButtonColors(
                        contentColor = MaterialTheme.colorScheme.error,
                    ),
                    modifier = Modifier.testTag("workspace_clear_database_confirm_button"),
                ) {
                    Text("Confirm")
                }
            },
            dismissButton = {
                TextButton(
                    onClick = { showClearDatabaseConfirmDialog = false },
                    colors = ButtonDefaults.textButtonColors(
                        contentColor = MaterialTheme.colorScheme.primary,
                    ),
                    modifier = Modifier.testTag("workspace_clear_database_cancel_button"),
                ) {
                    Text("Cancel")
                }
            },
        )
    }
}
