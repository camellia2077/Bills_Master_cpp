package com.billstracer.android.app.navigation

import com.billstracer.android.model.ThemePreferences
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update

data class AppSessionSharedState(
    val themePreferences: ThemePreferences = ThemePreferences(),
    val globalStatusMessage: String = "Preparing private workspace...",
    val globalErrorMessage: String? = null,
)

class AppSessionBus {
    private val mutableState = MutableStateFlow(AppSessionSharedState())
    val state: StateFlow<AppSessionSharedState> = mutableState.asStateFlow()

    fun publishStatus(message: String, errorMessage: String? = null) {
        mutableState.update { current ->
            current.copy(
                globalStatusMessage = message,
                globalErrorMessage = errorMessage,
            )
        }
    }

    fun publishError(errorMessage: String, fallbackStatusMessage: String) {
        mutableState.update { current ->
            current.copy(
                globalStatusMessage = fallbackStatusMessage,
                globalErrorMessage = errorMessage,
            )
        }
    }

    fun updateThemePreferences(preferences: ThemePreferences) {
        mutableState.update { current ->
            current.copy(themePreferences = preferences)
        }
    }
}
