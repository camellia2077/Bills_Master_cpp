package com.billstracer.android.data.services

import com.billstracer.android.data.prefs.ThemePreferenceStore
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ThemePreferences
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File

internal class SettingsDataSource(
    private val runtime: AndroidWorkspaceRuntime,
    private val themePreferenceStore: ThemePreferenceStore,
) {
    private companion object {
        val bundledConfigOrder = listOf(
            "validator_config.toml",
            "modifier_config.toml",
            "export_formats.toml",
        )
    }

    suspend fun loadBundledConfigs(): List<BundledConfigFile> = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        bundledConfigOrder.map { fileName ->
            BundledConfigFile(
                fileName = fileName,
                rawText = File(workspace.configRoot, fileName).readText(Charsets.UTF_8),
            )
        }
    }

    suspend fun updateBundledConfig(
        fileName: String,
        rawText: String,
    ): List<BundledConfigFile> = withContext(Dispatchers.IO) {
        require(fileName in bundledConfigOrder) {
            "Unsupported bundled config file: $fileName"
        }
        val workspace = runtime.initializeWorkspace()
        File(workspace.configRoot, fileName).writeText(rawText, Charsets.UTF_8)
        loadBundledConfigs()
    }

    suspend fun loadThemePreferences(): ThemePreferences = themePreferenceStore.load()

    suspend fun updateThemePreferences(preferences: ThemePreferences): ThemePreferences =
        themePreferenceStore.save(preferences)

    suspend fun loadBundledNotices(): BundledNotices = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        BundledNotices(
            markdownText = File(workspace.noticesRoot, "NOTICE.md").readText(Charsets.UTF_8),
            rawJson = File(workspace.noticesRoot, "notices.json").readText(Charsets.UTF_8),
        )
    }
}
