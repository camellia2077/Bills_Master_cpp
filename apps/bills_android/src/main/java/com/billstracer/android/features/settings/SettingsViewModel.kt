package com.billstracer.android.features.settings

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.data.services.SettingsService
import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePreferences
import com.billstracer.android.model.VersionInfo
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
)

class SettingsViewModel(
    private val settingsService: SettingsService,
    private val sessionBus: AppSessionBus,
) : ViewModel() {
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
        val fileName = state.value.selectedConfigFileName
        val rawText = state.value.configDrafts[fileName]
        if (fileName.isBlank() || rawText == null) {
            return
        }
        viewModelScope.launch {
            val pendingMessage = "Persisting $fileName into runtime_config..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
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
}

class SettingsViewModelFactory(
    private val settingsService: SettingsService,
    private val sessionBus: AppSessionBus,
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return SettingsViewModel(settingsService, sessionBus) as T
    }
}

private data class LoadedSettings(
    val bundledConfigs: List<BundledConfigFile>,
    val themePreferences: ThemePreferences,
    val bundledNotices: BundledNotices,
    val coreVersion: VersionInfo,
    val androidVersion: VersionInfo,
)
