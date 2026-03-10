package com.billstracer.android.data

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePalette
import com.billstracer.android.model.ThemePreferences
import kotlinx.coroutines.flow.first

private val Context.themeDataStore: DataStore<Preferences> by preferencesDataStore(
    name = "bills_android_theme_preferences",
)

internal class ThemePreferenceStore(
    context: Context,
) {
    private val dataStore = context.themeDataStore

    suspend fun load(): ThemePreferences {
        val preferences = dataStore.data.first()
        return ThemePreferences(
            mode = preferences[themeModeKey]
                ?.let { rawValue -> rawValue.valueOfOrNull<ThemeMode>() }
                ?: ThemeMode.SYSTEM,
            palette = preferences[themePaletteKey]
                ?.let { rawValue -> rawValue.valueOfOrNull<ThemePalette>() }
                ?: ThemePalette.EMBER,
        )
    }

    suspend fun save(themePreferences: ThemePreferences): ThemePreferences {
        dataStore.edit { preferences ->
            preferences[themeModeKey] = themePreferences.mode.name
            preferences[themePaletteKey] = themePreferences.palette.name
        }
        return themePreferences
    }

    private companion object {
        val themeModeKey = stringPreferencesKey("theme_mode")
        val themePaletteKey = stringPreferencesKey("theme_palette")

        private inline fun <reified T : Enum<T>> String.valueOfOrNull(): T? =
            runCatching { enumValueOf<T>(this) }.getOrNull()
    }
}
