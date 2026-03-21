package com.billstracer.android.ui.data

import android.net.Uri
import androidx.lifecycle.viewModelScope
import com.billstracer.android.BuildConfig
import com.billstracer.android.ui.BillsViewModel
import com.billstracer.android.ui.currentState
import com.billstracer.android.ui.updateUiState
import kotlinx.coroutines.launch

internal fun BillsViewModel.importBundledSampleAction() {
    if (!BuildConfig.DEBUG) {
        return
    }
    val label = currentState.bundledSampleLabel
    viewModelScope.launch {
        updateUiState {
            it.copy(
                isWorking = true,
                errorMessage = null,
                statusMessage = "Importing $label into SQLite...",
            )
        }
        runCatching { repository.importBundledSample() }
            .onSuccess { result ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        importResult = result,
                        errorMessage = if (result.ok) null else result.message,
                        statusMessage = if (result.ok) {
                            "Imported ${result.imported} bundled bill file(s)."
                        } else {
                            result.message
                        },
                    )
                }
            }
            .onFailure { error ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        errorMessage = error.message ?: "Import failed.",
                        statusMessage = "Import failed.",
                    )
                }
            }
    }
}

internal fun BillsViewModel.exportRecordFilesAction(targetDirectoryUri: Uri) {
    if (!BuildConfig.DEBUG) {
        return
    }
    viewModelScope.launch {
        updateUiState {
            it.copy(
                isWorking = true,
                errorMessage = null,
                statusMessage = "Exporting saved TXT record files and configs to the selected folder...",
            )
        }
        runCatching { repository.exportRecordFiles(targetDirectoryUri) }
            .onSuccess { result ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        statusMessage = when {
                            result.exportedRecordFiles > 0 && result.exportedConfigFiles > 0 ->
                                "Exported ${result.exportedRecordFiles} TXT record file(s) and ${result.exportedConfigFiles} TOML config file(s) to ${result.destinationDisplayPath}."

                            result.exportedRecordFiles > 0 ->
                                "Exported ${result.exportedRecordFiles} TXT record file(s) to ${result.destinationDisplayPath}."

                            result.exportedConfigFiles > 0 ->
                                "Exported ${result.exportedConfigFiles} TOML config file(s) to ${result.destinationDisplayPath}."

                            else ->
                                "No TXT record files or config files were found to export."
                        },
                    )
                }
            }
            .onFailure { error ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        errorMessage = error.message ?: "Failed to export TXT record files and config files.",
                        statusMessage = "Failed to export TXT record files and config files.",
                    )
                }
            }
    }
}

internal fun BillsViewModel.clearDatabaseAction() {
    viewModelScope.launch {
        updateUiState {
            it.copy(
                isWorking = true,
                errorMessage = null,
                statusMessage = "Clearing the private SQLite database...",
            )
        }
        runCatching { repository.clearDatabase() }
            .onSuccess { removed ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        importResult = null,
                        queryResult = null,
                        statusMessage = if (removed) {
                            "Database file cleared."
                        } else {
                            "Database file was already empty."
                        },
                    )
                }
            }
            .onFailure { error ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        errorMessage = error.message ?: "Failed to clear the database.",
                        statusMessage = "Failed to clear the database.",
                    )
                }
            }
    }
}
