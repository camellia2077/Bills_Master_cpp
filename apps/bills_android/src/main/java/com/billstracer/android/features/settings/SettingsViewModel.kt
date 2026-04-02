package com.billstracer.android.features.settings

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.app.navigation.WorkspaceDataChangeBus
import com.billstracer.android.data.services.BackupService
import com.billstracer.android.data.services.SettingsService
import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ExportedBackupBundleResult
import com.billstracer.android.model.ImportedBackupBundleResult
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePreferences
import com.billstracer.android.model.VersionInfo
import android.net.Uri
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

data class SettingsUiState(
    val isInitializing: Boolean = true,
    val isWorking: Boolean = false,
    val statusMessage: String = "",
    val errorMessage: String? = null,
    val bundledConfigs: List<BundledConfigFile> = emptyList(),
    val selectedConfigFileName: String = "",
    val configDrafts: Map<String, String> = emptyMap(),
    val themePreferences: ThemePreferences = ThemePreferences(),
    val themeDraft: ThemePreferences = ThemePreferences(),
    val bundledNotices: BundledNotices? = null,
    val coreVersion: VersionInfo? = null,
    val androidVersion: VersionInfo? = null,
    val lastExportedBackupResult: ExportedBackupBundleResult? = null,
    val lastImportedBackupResult: ImportedBackupBundleResult? = null,
)

class SettingsViewModel(
    private val settingsService: SettingsService,
    private val backupService: BackupService,
    private val sessionBus: AppSessionBus,
    private val workspaceDataChangeBus: WorkspaceDataChangeBus,
) : ViewModel() {
    private companion object {
        const val validatorConfigFileName = "validator_config.toml"
        const val modifierConfigFileName = "modifier_config.toml"
        const val exportFormatsConfigFileName = "export_formats.toml"
    }

    private val mutableState = MutableStateFlow(SettingsUiState())
    val state: StateFlow<SettingsUiState> = mutableState.asStateFlow()

    init {
        initialize()
    }

    private fun initialize() {
        viewModelScope.launch {
            runCatching {
                val bundledConfigs = settingsService.loadBundledConfigs()
                val themePreferences = settingsService.loadThemePreferences()
                val bundledNotices = settingsService.loadBundledNotices()
                val (coreVersion, androidVersion) = settingsService.loadVersionInfo()
                LoadedSettings(
                    bundledConfigs = bundledConfigs,
                    themePreferences = themePreferences,
                    bundledNotices = bundledNotices,
                    coreVersion = coreVersion,
                    androidVersion = androidVersion,
                )
            }.onSuccess { loaded ->
                sessionBus.updateThemePreferences(loaded.themePreferences)
                mutableState.update { current ->
                    current.copy(
                        isInitializing = false,
                        bundledConfigs = loaded.bundledConfigs,
                        selectedConfigFileName = loaded.bundledConfigs.firstOrNull()?.fileName.orEmpty(),
                        configDrafts = loaded.bundledConfigs.associate { config ->
                            config.fileName to config.rawText
                        },
                        themePreferences = loaded.themePreferences,
                        themeDraft = loaded.themePreferences,
                        bundledNotices = loaded.bundledNotices,
                        coreVersion = loaded.coreVersion,
                        androidVersion = loaded.androidVersion,
                        statusMessage = "Settings loaded.",
                    )
                }
            }.onFailure { error ->
                val message = error.message ?: "Failed to load settings."
                sessionBus.publishError(message, "Settings setup failed.")
                mutableState.update { current ->
                    current.copy(
                        isInitializing = false,
                        errorMessage = message,
                        statusMessage = "Settings setup failed.",
                    )
                }
            }
        }
    }

    fun selectBundledConfig(fileName: String) {
        mutableState.update { current ->
            if (current.bundledConfigs.none { config -> config.fileName == fileName }) {
                current
            } else {
                current.copy(selectedConfigFileName = fileName)
            }
        }
    }

    fun updateConfigDraft(rawText: String) {
        mutableState.update { current ->
            val fileName = current.selectedConfigFileName
            if (fileName.isBlank()) {
                current
            } else {
                current.copy(configDrafts = current.configDrafts + (fileName to rawText))
            }
        }
    }

    fun resetSelectedConfigDraft() {
        mutableState.update { current ->
            val fileName = current.selectedConfigFileName
            val persistedText = current.bundledConfigs.firstOrNull { config ->
                config.fileName == fileName
            }?.rawText
            if (fileName.isBlank() || persistedText == null) {
                current
            } else {
                current.copy(
                    configDrafts = current.configDrafts + (fileName to persistedText),
                    statusMessage = "Restored the draft for $fileName from persisted TOML.",
                )
            }
        }
    }

    fun saveSelectedConfig() {
        val currentState = state.value
        val fileName = currentState.selectedConfigFileName
        val rawText = currentState.configDrafts[fileName]
        if (fileName.isBlank() || rawText == null) {
            return
        }
        val validatorText = resolveConfigDraftText(currentState, validatorConfigFileName) ?: return
        val modifierText = resolveConfigDraftText(currentState, modifierConfigFileName) ?: return
        val exportFormatsText =
            resolveConfigDraftText(currentState, exportFormatsConfigFileName) ?: return
        viewModelScope.launch {
            val pendingMessage = "Validating and persisting $fileName into runtime_config..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            val validation = runCatching {
                settingsService.validateBundledConfigs(
                    validatorText = validatorText,
                    modifierText = modifierText,
                    exportFormatsText = exportFormatsText,
                )
            }
            if (validation.isFailure) {
                val message = validation.exceptionOrNull()?.message
                    ?: "Failed to validate runtime_config TOML."
                sessionBus.publishError(message, "Failed to validate runtime_config TOML.")
                mutableState.update { current ->
                    current.copy(
                        isWorking = false,
                        errorMessage = message,
                        statusMessage = "Failed to validate runtime_config TOML.",
                    )
                }
                return@launch
            }

            val validationResult = validation.getOrThrow()
            if (!validationResult.ok) {
                sessionBus.publishError(validationResult.message, "Config validation failed.")
                mutableState.update { current ->
                    current.copy(
                        isWorking = false,
                        errorMessage = validationResult.message,
                        statusMessage = validationResult.message,
                    )
                }
                return@launch
            }

            runCatching { settingsService.updateBundledConfig(fileName, rawText) }
                .onSuccess { updatedConfigs ->
                    val message = "Modified and persisted $fileName."
                    sessionBus.publishStatus(message)
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            bundledConfigs = updatedConfigs,
                            configDrafts = current.configDrafts + (
                                fileName to (
                                    updatedConfigs.firstOrNull { config -> config.fileName == fileName }?.rawText
                                        ?: rawText
                                    )
                                ),
                            statusMessage = message,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to persist bundled config."
                    sessionBus.publishError(message, "Failed to persist bundled config.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to persist bundled config.",
                        )
                    }
                }
        }
    }

    private fun resolveConfigDraftText(state: SettingsUiState, fileName: String): String? =
        state.configDrafts[fileName]
            ?: state.bundledConfigs.firstOrNull { config -> config.fileName == fileName }?.rawText

    fun updateThemeModeDraft(mode: ThemeMode) {
        mutableState.update { current ->
            current.copy(themeDraft = current.themeDraft.copy(mode = mode))
        }
    }

    fun updateThemeColorDraft(color: ThemeColor) {
        mutableState.update { current ->
            current.copy(themeDraft = current.themeDraft.copy(color = color))
        }
    }

    fun resetThemeDraft() {
        mutableState.update { current ->
            current.copy(
                themeDraft = current.themePreferences,
                statusMessage = "Restored the theme draft from persisted settings.",
            )
        }
    }

    fun applyThemeDraft() {
        val draft = state.value.themeDraft
        viewModelScope.launch {
            val pendingMessage =
                "Applying ${draft.color.displayName} in ${draft.mode.displayName} mode..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { settingsService.updateThemePreferences(draft) }
                .onSuccess { persistedTheme ->
                    val message =
                        "Applied ${persistedTheme.color.displayName} in ${persistedTheme.mode.displayName} mode."
                    sessionBus.updateThemePreferences(persistedTheme)
                    sessionBus.publishStatus(message)
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            themePreferences = persistedTheme,
                            themeDraft = persistedTheme,
                            statusMessage = message,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to persist theme settings."
                    sessionBus.publishError(message, "Failed to persist theme settings.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to persist theme settings.",
                        )
                    }
            }
        }
    }

    fun exportBackupBundle(targetDocumentUri: Uri) {
        viewModelScope.launch {
            val pendingMessage =
                "Exporting a backup bundle from saved TXT records and migration configs..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { backupService.exportBackupBundle(targetDocumentUri) }
                .onSuccess { result ->
                    val message = when {
                        result.exportedRecordFiles > 0 && result.exportedConfigFiles > 0 ->
                            "Exported a backup bundle with ${result.exportedRecordFiles} TXT record file(s) and ${result.exportedConfigFiles} config file(s) to ${result.destinationDisplayPath}."
                        result.exportedRecordFiles > 0 ->
                            "Exported a backup bundle with ${result.exportedRecordFiles} TXT record file(s) to ${result.destinationDisplayPath}."
                        result.exportedConfigFiles > 0 ->
                            "Exported a backup bundle with ${result.exportedConfigFiles} config file(s) to ${result.destinationDisplayPath}."
                        else ->
                            "No TXT record files or config files were found for backup export."
                    }
                    sessionBus.publishStatus(message)
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            lastExportedBackupResult = result,
                            statusMessage = message,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to export the backup bundle ZIP."
                    sessionBus.publishError(message, "Failed to export the backup bundle ZIP.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to export the backup bundle ZIP.",
                        )
                    }
                }
        }
    }

    fun importBackupBundle(sourceDocumentUri: Uri) {
        viewModelScope.launch {
            val pendingMessage =
                "Restoring a backup bundle into the private workspace and SQLite..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { backupService.importBackupBundle(sourceDocumentUri) }
                .onSuccess { result ->
                    val message = if (result.ok) {
                        when {
                            result.restoredRecordFiles > 0 && result.restoredConfigFiles > 0 ->
                                "Restored ${result.restoredRecordFiles} TXT record file(s) and ${result.restoredConfigFiles} config file(s) from ${result.sourceDisplayPath}, and rebuilt SQLite."
                            result.restoredRecordFiles > 0 ->
                                "Restored ${result.restoredRecordFiles} TXT record file(s) from ${result.sourceDisplayPath}, and rebuilt SQLite."
                            result.restoredConfigFiles > 0 ->
                                "Restored ${result.restoredConfigFiles} config file(s) from ${result.sourceDisplayPath}, and rebuilt SQLite."
                            else ->
                                "Restored backup bundle from ${result.sourceDisplayPath}, and rebuilt SQLite."
                        }
                    } else {
                        result.message
                    }

                    val updatedConfigs = if (result.ok) {
                        val loadedConfigs = runCatching { settingsService.loadBundledConfigs() }
                        if (loadedConfigs.isFailure) {
                            val refreshError = loadedConfigs.exceptionOrNull()
                            val refreshMessage =
                                "Restored the backup bundle, but failed to refresh runtime_config."
                            sessionBus.publishError(
                                refreshError?.message ?: refreshMessage,
                                refreshMessage,
                            )
                            mutableState.update { current ->
                                current.copy(
                                    isWorking = false,
                                    lastImportedBackupResult = result,
                                    errorMessage = refreshError?.message ?: refreshMessage,
                                    statusMessage = refreshMessage,
                                )
                            }
                            workspaceDataChangeBus.notifyChanged()
                            return@onSuccess
                        }
                        loadedConfigs.getOrNull()
                    } else {
                        null
                    }
                    if (result.ok) {
                        sessionBus.publishStatus(message)
                    } else {
                        sessionBus.publishError(result.message, message)
                    }
                    mutableState.update { current ->
                        val selectedFileName = current.selectedConfigFileName
                        val nextConfigs = updatedConfigs ?: current.bundledConfigs
                        current.copy(
                            isWorking = false,
                            bundledConfigs = nextConfigs,
                            selectedConfigFileName = selectedFileName
                                .takeIf { fileName -> nextConfigs.any { it.fileName == fileName } }
                                ?: nextConfigs.firstOrNull()?.fileName.orEmpty(),
                            configDrafts = if (updatedConfigs != null) {
                                updatedConfigs.associate { config -> config.fileName to config.rawText }
                            } else {
                                current.configDrafts
                            },
                            lastImportedBackupResult = result,
                            errorMessage = if (result.ok) null else result.message,
                            statusMessage = message,
                        )
                    }
                    if (result.ok) {
                        workspaceDataChangeBus.notifyChanged()
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to import the backup bundle ZIP."
                    sessionBus.publishError(message, "Failed to import the backup bundle ZIP.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to import the backup bundle ZIP.",
                        )
                    }
                }
        }
    }
}

class SettingsViewModelFactory(
    private val settingsService: SettingsService,
    private val backupService: BackupService,
    private val sessionBus: AppSessionBus,
    private val workspaceDataChangeBus: WorkspaceDataChangeBus,
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return SettingsViewModel(
            settingsService = settingsService,
            backupService = backupService,
            sessionBus = sessionBus,
            workspaceDataChangeBus = workspaceDataChangeBus,
        ) as T
    }
}

private data class LoadedSettings(
    val bundledConfigs: List<BundledConfigFile>,
    val themePreferences: ThemePreferences,
    val bundledNotices: BundledNotices,
    val coreVersion: VersionInfo,
    val androidVersion: VersionInfo,
)
