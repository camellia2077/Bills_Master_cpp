package com.billstracer.android

import com.billstracer.android.data.services.SettingsService
import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ConfigFileValidationResult
import com.billstracer.android.model.ConfigTextsValidationResult
import com.billstracer.android.model.ConfigValidationReport
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePreferences
import com.billstracer.android.model.VersionInfo

internal class FakeSettingsService : SettingsService {
    val savedConfigs = linkedMapOf(
        "validator_config.toml" to "[[categories]]\n",
        "modifier_config.toml" to "metadata_prefixes = [\"date:\"]\n",
        "export_formats.toml" to "enabled_formats = [\"json\", \"md\"]\n",
    )
    var savedTheme = ThemePreferences()
    var nextConfigValidationResult = ConfigTextsValidationResult(
        ok = true,
        code = "ok",
        message = "Config texts are valid.",
        configValidation = ConfigValidationReport(
            processed = 3,
            success = 3,
            failure = 0,
            allValid = true,
            files = listOf(
                ConfigFileValidationResult(
                    sourceKind = "config_text",
                    fileName = "validator_config.toml",
                    path = "config/validator_config.toml",
                    ok = true,
                    issues = emptyList(),
                ),
                ConfigFileValidationResult(
                    sourceKind = "config_text",
                    fileName = "modifier_config.toml",
                    path = "config/modifier_config.toml",
                    ok = true,
                    issues = emptyList(),
                ),
                ConfigFileValidationResult(
                    sourceKind = "config_text",
                    fileName = "export_formats.toml",
                    path = "config/export_formats.toml",
                    ok = true,
                    issues = emptyList(),
                ),
            ),
            enabledExportFormats = listOf("json", "md"),
            availableExportFormats = listOf("json", "md"),
        ),
        enabledExportFormats = listOf("json", "md"),
        availableExportFormats = listOf("json", "md"),
        rawJson = """{"ok":true}""",
    )
    var lastValidatedConfigTexts: Triple<String, String, String>? = null

    override suspend fun loadBundledConfigs(): List<BundledConfigFile> =
        savedConfigs.map { (fileName, rawText) -> BundledConfigFile(fileName, rawText) }

    override suspend fun validateBundledConfigs(
        validatorText: String,
        modifierText: String,
        exportFormatsText: String,
    ): ConfigTextsValidationResult {
        lastValidatedConfigTexts = Triple(validatorText, modifierText, exportFormatsText)
        return nextConfigValidationResult
    }

    override suspend fun updateBundledConfig(
        fileName: String,
        rawText: String,
    ): List<BundledConfigFile> {
        savedConfigs[fileName] = rawText
        return loadBundledConfigs()
    }

    override suspend fun loadThemePreferences(): ThemePreferences = savedTheme

    override suspend fun updateThemePreferences(preferences: ThemePreferences): ThemePreferences {
        savedTheme = preferences
        return savedTheme
    }

    override suspend fun loadBundledNotices(): BundledNotices =
        BundledNotices(
            markdownText = "# Open Source Notices\n",
            rawJson = """{"schema_version":"1"}""",
        )

    override suspend fun loadVersionInfo(): Pair<VersionInfo, VersionInfo> =
        VersionInfo(
            versionName = "0.4.2",
            lastUpdated = "2026-03-10",
        ) to VersionInfo(
            versionName = "0.2.0",
            versionCode = 3,
        )
}

internal class FailingSettingsService : SettingsService by FakeSettingsService() {
    override suspend fun loadVersionInfo(): Pair<VersionInfo, VersionInfo> {
        error("version load failed")
    }
}

internal val emeraldDarkTheme = ThemePreferences(
    color = ThemeColor.EMERALD,
    mode = ThemeMode.DARK,
)
