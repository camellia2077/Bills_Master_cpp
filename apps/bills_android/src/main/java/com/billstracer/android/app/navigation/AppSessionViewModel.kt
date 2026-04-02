package com.billstracer.android.app.navigation

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.data.services.SettingsService
import com.billstracer.android.data.services.WorkspaceService
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.ThemePreferences
import com.billstracer.android.model.VersionInfo
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

data class AppSessionState(
    val isInitializing: Boolean = true,
    val environment: AppEnvironment? = null,
    val coreVersion: VersionInfo? = null,
    val androidVersion: VersionInfo? = null,
    val themePreferences: ThemePreferences = ThemePreferences(),
    val globalStatusMessage: String = "Preparing private workspace...",
    val globalErrorMessage: String? = null,
)

class AppSessionViewModel(
    private val workspaceService: WorkspaceService,
    private val settingsService: SettingsService,
    private val sessionBus: AppSessionBus,
) : ViewModel() {
    private val mutableState = MutableStateFlow(AppSessionState())
    val state: StateFlow<AppSessionState> = mutableState.asStateFlow()

    init {
        observeSessionBus()
        initialize()
    }

    private fun observeSessionBus() {
        viewModelScope.launch {
            sessionBus.state.collect { sharedState ->
                mutableState.update { current ->
                    current.copy(
                        themePreferences = sharedState.themePreferences,
                        globalStatusMessage = sharedState.globalStatusMessage,
                        globalErrorMessage = sharedState.globalErrorMessage,
                    )
                }
            }
        }
    }

    fun initialize() {
        viewModelScope.launch {
            mutableState.update { current ->
                current.copy(
                    isInitializing = true,
                    globalErrorMessage = null,
                    globalStatusMessage = "Preparing private workspace...",
                )
            }
            runCatching {
                val environment = workspaceService.initializeEnvironment()
                val (coreVersion, androidVersion) = settingsService.loadVersionInfo()
                val themePreferences = settingsService.loadThemePreferences()
                Triple(environment, coreVersion, androidVersion) to themePreferences
            }.onSuccess { (versions, themePreferences) ->
                val (environment, coreVersion, androidVersion) = versions
                sessionBus.updateThemePreferences(themePreferences)
                val readyMessage = "Private workspace ready: ${environment.dbFile.name}."
                sessionBus.publishStatus(
                    readyMessage,
                )
                mutableState.update { current ->
                    current.copy(
                        isInitializing = false,
                        environment = environment,
                        coreVersion = coreVersion,
                        androidVersion = androidVersion,
                    )
                }
            }.onFailure { error ->
                sessionBus.publishError(
                    errorMessage = error.message ?: "Failed to prepare workspace.",
                    fallbackStatusMessage = "Workspace setup failed.",
                )
                mutableState.update { current ->
                    current.copy(isInitializing = false)
                }
            }
        }
    }
}

class AppSessionViewModelFactory(
    private val workspaceService: WorkspaceService,
    private val settingsService: SettingsService,
    private val sessionBus: AppSessionBus,
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return AppSessionViewModel(workspaceService, settingsService, sessionBus) as T
    }
}
