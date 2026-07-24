package com.billstracer.android.data.prefs

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.first

private val Context.queryDataStore: DataStore<Preferences> by preferencesDataStore(
    name = "bills_android_query_preferences",
)

class QueryInputModePreferenceStore(
    context: Context,
) {
    private val dataStore = context.applicationContext.queryDataStore

    suspend fun load(): String? = dataStore.data.first()[queryInputModeKey]

    suspend fun save(mode: String) {
        dataStore.edit { preferences ->
            preferences[queryInputModeKey] = mode
        }
    }

    private companion object {
        val queryInputModeKey = stringPreferencesKey("query_input_mode")
    }
}
