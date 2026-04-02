package com.billstracer.android.features.workspace

import android.net.Uri
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.BuildConfig
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.app.navigation.WorkspaceDataChangeBus
import com.billstracer.android.data.services.WorkspaceService
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.ExportedParseBundleResult
import com.billstracer.android.model.ImportedParseBundleResult
import com.billstracer.android.model.RecordDirectoryImportResult
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
    val recordDirectoryImportResult: RecordDirectoryImportResult? = null,
    val lastExportResult: ExportedParseBundleResult? = null,
    val lastImportedBundleResult: ImportedParseBundleResult? = null,
)

class WorkspaceViewModel(
    private val workspaceService: WorkspaceService,
    private val sessionBus: AppSessionBus,
    private val workspaceDataChangeBus: WorkspaceDataChangeBus,
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
                    val readyMessage = "Workspace ready."
                    mutableState.update { current ->
                        current.copy(
                            isInitializing = false,
                            environment = environment,
                            statusMessage = readyMessage,
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

    fun importTxtDirectoryAndSyncDatabase(sourceDirectoryUri: Uri) {
        viewModelScope.launch {
            val pendingMessage =
                "Importing TXT files from the selected directory into records/ and SQLite..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { workspaceService.importTxtDirectoryAndSyncDatabase(sourceDirectoryUri) }
                .onSuccess { result ->
                    val message = buildRecordDirectoryImportMessage(result)
                    val failureMessage = result.firstFailureMessage?.takeIf { result.failure > 0 }
                    if (result.failure == 0) {
                        sessionBus.publishStatus(message)
                    } else {
                        sessionBus.publishError(failureMessage ?: message, message)
                    }
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            recordDirectoryImportResult = result,
                            errorMessage = failureMessage,
                            statusMessage = message,
                        )
                    }
                    notifyWorkspaceDataChangedIf(result.imported > 0)
                }
                .onFailure { error ->
                    val message = error.message ?: "TXT directory import and SQLite sync failed."
                    sessionBus.publishError(message, "TXT directory import and SQLite sync failed.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "TXT directory import and SQLite sync failed.",
                        )
                    }
                }
        }
    }

    fun exportParseBundle(targetDocumentUri: Uri) {
        viewModelScope.launch {
            val pendingMessage =
                "Exporting a parse bundle ZIP from saved TXT record files and configs..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { workspaceService.exportParseBundle(targetDocumentUri) }
                .onSuccess { result ->
                    val message = when {
                        result.exportedRecordFiles > 0 && result.exportedConfigFiles > 0 ->
                            "Exported a parse bundle with ${result.exportedRecordFiles} TXT record file(s) and ${result.exportedConfigFiles} TOML config file(s) to ${result.destinationDisplayPath}."
                        result.exportedRecordFiles > 0 ->
                            "Exported a parse bundle with ${result.exportedRecordFiles} TXT record file(s) to ${result.destinationDisplayPath}."
                        result.exportedConfigFiles > 0 ->
                            "Exported a parse bundle with ${result.exportedConfigFiles} TOML config file(s) to ${result.destinationDisplayPath}."
                        else ->
                            "No TXT record files or config files were found for parse bundle export."
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
                        error.message ?: "Failed to export the parse bundle ZIP."
                    sessionBus.publishError(message, "Failed to export the parse bundle ZIP.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to export the parse bundle ZIP.",
                        )
                    }
                }
        }
    }

    fun importParseBundle(sourceDocumentUri: Uri) {
        if (!BuildConfig.DEBUG) {
            return
        }
        viewModelScope.launch {
            val pendingMessage =
                "Importing a parse bundle ZIP into the private workspace..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { workspaceService.importParseBundle(sourceDocumentUri) }
                .onSuccess { result ->
                    val message = if (result.ok) {
                        when {
                            result.importedBills > 0 &&
                                result.importedRecordFiles > 0 &&
                                result.importedConfigFiles > 0 ->
                                "Imported a parse bundle from ${result.sourceDisplayPath} with ${result.importedRecordFiles} TXT record file(s), ${result.importedConfigFiles} TOML config file(s), and synced ${result.importedBills} bill file(s) into SQLite."
                            result.importedBills > 0 && result.importedRecordFiles > 0 ->
                                "Imported a parse bundle from ${result.sourceDisplayPath} with ${result.importedRecordFiles} TXT record file(s) and synced ${result.importedBills} bill file(s) into SQLite."
                            result.importedRecordFiles > 0 && result.importedConfigFiles > 0 ->
                                "Imported a parse bundle from ${result.sourceDisplayPath} with ${result.importedRecordFiles} TXT record file(s) and ${result.importedConfigFiles} TOML config file(s)."
                            result.importedRecordFiles > 0 ->
                                "Imported a parse bundle from ${result.sourceDisplayPath} with ${result.importedRecordFiles} TXT record file(s)."
                            result.importedConfigFiles > 0 ->
                                "Imported a parse bundle from ${result.sourceDisplayPath} with ${result.importedConfigFiles} TOML config file(s)."
                            else ->
                                "Imported a parse bundle from ${result.sourceDisplayPath}."
                        }
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
                            lastImportedBundleResult = result,
                            errorMessage = if (result.ok) null else result.message,
                            statusMessage = message,
                        )
                    }
                    notifyWorkspaceDataChangedIf(result.importedBills > 0)
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to import the parse bundle ZIP."
                    sessionBus.publishError(message, "Failed to import the parse bundle ZIP.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to import the parse bundle ZIP.",
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
                    notifyWorkspaceDataChangedIf(removed)
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

    private fun buildRecordDirectoryImportMessage(result: RecordDirectoryImportResult): String {
        if (result.processed == 0) {
            return if (result.failure == 0) {
                "No TXT files were found in the selected directory."
            } else {
                result.firstFailureMessage ?: "TXT directory import and SQLite sync failed."
            }
        }
        val overwriteSuffix = if (result.overwritten > 0) {
            " Overwrote ${result.overwritten} existing file(s)."
        } else {
            ""
        }
        return when {
            result.failure == 0 ->
                "Imported and synced ${result.imported} TXT record file(s).$overwriteSuffix"
            result.imported > 0 ->
                "Imported and synced ${result.imported} TXT record file(s) with ${result.failure} failure(s).$overwriteSuffix"
            else ->
                "No TXT record files were imported and synced. ${result.failure} file(s) failed."
        }
    }

    private fun notifyWorkspaceDataChangedIf(changed: Boolean) {
        if (changed) {
            workspaceDataChangeBus.notifyChanged()
        }
    }
}

class WorkspaceViewModelFactory(
    private val workspaceService: WorkspaceService,
    private val sessionBus: AppSessionBus,
    private val workspaceDataChangeBus: WorkspaceDataChangeBus,
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return WorkspaceViewModel(workspaceService, sessionBus, workspaceDataChangeBus) as T
    }
}
