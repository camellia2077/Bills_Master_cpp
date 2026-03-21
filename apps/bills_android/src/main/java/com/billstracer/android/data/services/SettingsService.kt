package com.billstracer.android.data.services

import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ThemePreferences
import com.billstracer.android.model.VersionInfo

interface SettingsService {
    suspend fun loadBundledConfigs(): List<BundledConfigFile>

    suspend fun updateBundledConfig(fileName: String, rawText: String): List<BundledConfigFile>

    suspend fun loadThemePreferences(): ThemePreferences

    suspend fun updateThemePreferences(preferences: ThemePreferences): ThemePreferences

    suspend fun loadBundledNotices(): BundledNotices

    suspend fun loadVersionInfo(): Pair<VersionInfo, VersionInfo>
}
