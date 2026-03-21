package com.billstracer.android.data.services

import com.billstracer.android.BuildConfig
import com.billstracer.android.data.nativebridge.SettingsNativeBindings
import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ThemePreferences
import com.billstracer.android.model.VersionInfo
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

internal class DefaultSettingsService(
    private val settingsDataSource: SettingsDataSource,
) : SettingsService {
    override suspend fun loadBundledConfigs(): List<BundledConfigFile> =
        settingsDataSource.loadBundledConfigs()

    override suspend fun updateBundledConfig(
        fileName: String,
        rawText: String,
    ): List<BundledConfigFile> = settingsDataSource.updateBundledConfig(fileName, rawText)

    override suspend fun loadThemePreferences(): ThemePreferences =
        settingsDataSource.loadThemePreferences()

    override suspend fun updateThemePreferences(preferences: ThemePreferences): ThemePreferences =
        settingsDataSource.updateThemePreferences(preferences)

    override suspend fun loadBundledNotices(): BundledNotices =
        settingsDataSource.loadBundledNotices()

    override suspend fun loadVersionInfo(): Pair<VersionInfo, VersionInfo> =
        withContext(Dispatchers.IO) {
            val root = parseRoot(SettingsNativeBindings.coreVersionNative())
            val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
            if (!root.boolean("ok")) {
                error(root.string("message").ifBlank { "Failed to load core version." })
            }

            val coreVersion = VersionInfo(
                versionName = data.string("version_name"),
                lastUpdated = data["last_updated"]?.jsonPrimitive?.contentOrNull,
            )
            val androidVersion = VersionInfo(
                versionName = BuildConfig.PRESENTATION_VERSION_NAME,
                versionCode = BuildConfig.PRESENTATION_VERSION_CODE,
            )
            coreVersion to androidVersion
        }
}
