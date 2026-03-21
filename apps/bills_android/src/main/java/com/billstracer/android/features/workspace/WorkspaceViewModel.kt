package com.billstracer.android.features.workspace

import android.net.Uri
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.BuildConfig
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.data.services.WorkspaceService
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.ExportedRecordFilesResult
import com.billstracer.android.model.ImportResult
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

data class WorkspaceUiState(
    val isInitializing: Boolean = true,
    val isWorking: Boolean = false,
    val statusMessage: String = "",
    val errorMessage: String? = null,
    val environment: AppEnvironment? = null,
    val importResult: ImportResult? = null,
    val lastExportResult: ExportedRecordFilesResult? = null,
)

class WorkspaceViewModel(
    private val workspaceService: WorkspaceService,
    private val sessionBus: AppSessionBus,
) : ViewModel() {
    private val mutableState = MutableStateFlow(WorkspaceUiState())
    val state: StateFlow<WorkspaceUiState> = mutableState.asStateFlow()

    init {
        loadEnvironment()
    }

    private fun loadEnvironment() {
        viewModelScope.launch {
            runCatching { workspaceService.initializeEnvironment() }
                .onSuccess { environment ->
                    mutableState.update { current ->
                        current.copy(
                            isInitializing = false,
                            environment = environment,
                            statusMessage = if (BuildConfig.DEBUG) {
                                "Workspace ready for ${environment.bundledSampleLabel}."
                            } else {
                                "Workspace ready."
                            },
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to prepare workspace."
                    sessionBus.publishError(message, "Workspace setup failed.")
                    mutableState.update { current ->
                        current.copy(
                            isInitializing = false,
                            errorMessage = message,
                            statusMessage = "Workspace setup failed.",
                        )
                    }
                }
        }
    }

    fun importBundledSample() {
        if (!BuildConfig.DEBUG) {
            return
        }
        val label = state.value.environment?.bundledSampleLabel.orEmpty()
        viewModelScope.launch {
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Importing $label into SQLite...",
                )
            }
            sessionBus.publishStatus("Importing $label into SQLite...")
            runCatching { workspaceService.importBundledSample() }
                .onSuccess { result ->
                    val message = if (result.ok) {
                        "Imported ${result.imported} bundled bill file(s)."
                    } else {
                        result.message
                    }
                    if (result.ok) {
                        sessionBus.publishStatus(message)
                    } else {
                        sessionBus.publishError(result.message, message)
                    }
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            importResult = result,
                            errorMessage = if (result.ok) null else result.message,
                            statusMessage = message,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Import failed."
                    sessionBus.publishError(message, "Import failed.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Import failed.",
                        )
                    }
                }
        }
    }

    fun exportWorkspaceFiles(targetDirectoryUri: Uri) {
        if (!BuildConfig.DEBUG) {
            return
        }
        viewModelScope.launch {
            val pendingMessage =
                "Exporting saved TXT record files and configs to the selected folder..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { workspaceService.exportWorkspaceFiles(targetDirectoryUri) }
                .onSuccess { result ->
                    val message = when {
                        result.exportedRecordFiles > 0 && result.exportedConfigFiles > 0 ->
                            "Exported ${result.exportedRecordFiles} TXT record file(s) and ${result.exportedConfigFiles} TOML config file(s) to ${result.destinationDisplayPath}."
                        result.exportedRecordFiles > 0 ->
                            "Exported ${result.exportedRecordFiles} TXT record file(s) to ${result.destinationDisplayPath}."
                        result.exportedConfigFiles > 0 ->
                            "Exported ${result.exportedConfigFiles} TOML config file(s) to ${result.destinationDisplayPath}."
                        else ->
                            "No TXT record files or config files were found to export."
                    }
                    sessionBus.publishStatus(message)
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            lastExportResult = result,
                            statusMessage = message,
                        )
                    }
                }
                .onFailure { error ->
                    val message =
                        error.message ?: "Failed to export TXT record files and config files."
                    sessionBus.publishError(message, "Failed to export TXT record files and config files.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to export TXT record files and config files.",
                        )
                    }
                }
        }
    }

    fun clearDatabase() {
        viewModelScope.launch {
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Clearing the private SQLite database...",
                    importResult = null,
                )
            }
            sessionBus.publishStatus("Clearing the private SQLite database...")
            runCatching { workspaceService.clearDatabase() }
                .onSuccess { removed ->
                    val message = if (removed) {
                        "Database file cleared."
                    } else {
                        "Database file was already empty."
                    }
                    sessionBus.publishStatus(message)
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            statusMessage = message,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to clear the database."
                    sessionBus.publishError(message, "Failed to clear the database.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to clear the database.",
                        )
                    }
                }
        }
    }

    fun clearRecordFiles() {
        viewModelScope.launch {
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = "Clearing saved TXT record files...",
                )
            }
            sessionBus.publishStatus("Clearing saved TXT record files...")
            runCatching { workspaceService.clearRecordFiles() }
                .onSuccess { removed ->
                    val message = if (removed > 0) {
                        "Cleared $removed TXT record file(s)."
                    } else {
                        "No TXT record files were found."
                    }
                    sessionBus.publishStatus(message)
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            statusMessage = message,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to clear TXT record files."
                    sessionBus.publishError(message, "Failed to clear TXT record files.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to clear TXT record files.",
                        )
                    }
                }
        }
    }
}

class WorkspaceViewModelFactory(
    private val workspaceService: WorkspaceService,
    private val sessionBus: AppSessionBus,
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return WorkspaceViewModel(workspaceService, sessionBus) as T
    }
}
