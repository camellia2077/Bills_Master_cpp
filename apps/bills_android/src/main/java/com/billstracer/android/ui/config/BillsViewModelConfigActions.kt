package com.billstracer.android.ui.config

import androidx.lifecycle.viewModelScope
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.ui.BillsViewModel
import com.billstracer.android.ui.currentState
import com.billstracer.android.ui.updateUiState
import kotlinx.coroutines.launch

internal fun BillsViewModel.updateThemeModeDraftAction(mode: ThemeMode) {
    updateUiState { state ->
        state.copy(themeDraft = state.themeDraft.copy(mode = mode))
    }
}

internal fun BillsViewModel.updateThemeColorDraftAction(color: ThemeColor) {
    updateUiState { state ->
        state.copy(themeDraft = state.themeDraft.copy(color = color))
    }
}

internal fun BillsViewModel.resetThemeDraftAction() {
    updateUiState { state ->
        state.copy(
            themeDraft = state.themePreferences,
            statusMessage = "Restored the theme draft from persisted settings.",
        )
    }
}

internal fun BillsViewModel.applyThemeDraftAction() {
    val draft = currentState.themeDraft
    viewModelScope.launch {
        updateUiState {
            it.copy(
                isWorking = true,
                errorMessage = null,
                statusMessage = "Applying ${draft.color.displayName} in ${draft.mode.displayName} mode...",
            )
        }
        runCatching { repository.updateThemePreferences(draft) }
            .onSuccess { persistedTheme ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        themePreferences = persistedTheme,
                        themeDraft = persistedTheme,
                        statusMessage = "Applied ${persistedTheme.color.displayName} in ${persistedTheme.mode.displayName} mode.",
                    )
                }
            }
            .onFailure { error ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        errorMessage = error.message ?: "Failed to persist theme settings.",
                        statusMessage = "Failed to persist theme settings.",
                    )
                }
            }
    }
}

internal fun BillsViewModel.selectBundledConfigAction(fileName: String) {
    updateUiState { state ->
        if (state.bundledConfigs.none { config -> config.fileName == fileName }) {
            state
        } else {
            state.copy(selectedConfigFileName = fileName)
        }
    }
}

internal fun BillsViewModel.updateConfigDraftAction(rawText: String) {
    updateUiState { state ->
        val fileName = state.selectedConfigFileName
        if (fileName.isBlank()) {
            state
        } else {
            state.copy(configDrafts = state.configDrafts + (fileName to rawText))
        }
    }
}

internal fun BillsViewModel.resetSelectedConfigDraftAction() {
    updateUiState { state ->
        val fileName = state.selectedConfigFileName
        val persistedText = state.bundledConfigs.firstOrNull { config ->
            config.fileName == fileName
        }?.rawText
        if (fileName.isBlank() || persistedText == null) {
            state
        } else {
            state.copy(
                configDrafts = state.configDrafts + (fileName to persistedText),
                statusMessage = "Restored the draft for $fileName from persisted TOML.",
            )
        }
    }
}

internal fun BillsViewModel.saveSelectedConfigAction() {
    val state = currentState
    val fileName = state.selectedConfigFileName
    val rawText = state.configDrafts[fileName]
    if (fileName.isBlank() || rawText == null) {
        return
    }
    viewModelScope.launch {
        updateUiState {
            it.copy(
                isWorking = true,
                errorMessage = null,
                statusMessage = "Persisting $fileName into runtime_config...",
            )
        }
        runCatching { repository.updateBundledConfig(fileName, rawText) }
            .onSuccess { updatedConfigs ->
                updateUiState { current ->
                    current.copy(
                        isWorking = false,
                        bundledConfigs = updatedConfigs,
                        configDrafts = current.configDrafts + (
                            fileName to (
                                updatedConfigs.firstOrNull { config -> config.fileName == fileName }?.rawText
                                    ?: rawText
                                )
                            ),
                        statusMessage = "Modified and persisted $fileName.",
                    )
                }
            }
            .onFailure { error ->
                updateUiState {
                    it.copy(
                        isWorking = false,
                        errorMessage = error.message ?: "Failed to persist bundled config.",
                        statusMessage = "Failed to persist bundled config.",
                    )
                }
            }
    }
}
