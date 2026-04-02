package com.billstracer.android.model

import java.io.File

data class AppEnvironment(
    val configRoot: File,
    val recordsRoot: File,
    val dbFile: File,
    val coreVersion: VersionInfo? = null,
    val androidVersion: VersionInfo? = null,
    val bundledConfigs: List<BundledConfigFile> = emptyList(),
    val themePreferences: ThemePreferences = ThemePreferences(),
    val bundledNotices: BundledNotices? = null,
)

data class BundledConfigFile(
    val fileName: String,
    val rawText: String,
)

data class ConfigValidationIssue(
    val sourceKind: String,
    val stage: String,
    val code: String,
    val message: String,
    val path: String,
    val line: Int,
    val column: Int,
    val fieldPath: String,
    val severity: String,
)

data class ConfigFileValidationResult(
    val sourceKind: String,
    val fileName: String,
    val path: String,
    val ok: Boolean,
    val issues: List<ConfigValidationIssue>,
)

data class ConfigValidationReport(
    val processed: Int,
    val success: Int,
    val failure: Int,
    val allValid: Boolean,
    val files: List<ConfigFileValidationResult>,
    val enabledExportFormats: List<String>,
    val availableExportFormats: List<String>,
)

data class ConfigTextsValidationResult(
    val ok: Boolean,
    val code: String,
    val message: String,
    val configValidation: ConfigValidationReport,
    val enabledExportFormats: List<String>,
    val availableExportFormats: List<String>,
    val rawJson: String,
)

data class BundledNotices(
    val markdownText: String,
    val rawJson: String,
)

data class VersionInfo(
    val versionName: String,
    val versionCode: Int? = null,
    val lastUpdated: String? = null,
)

enum class ThemeMode(
    val displayName: String,
) {
    SYSTEM("Follow system"),
    LIGHT("Light"),
    DARK("Dark"),
}

enum class ThemeColor(
    val displayName: String,
) {
    ROSE("Rose"),
    ORANGE("Orange"),
    PEACH("Peach"),
    AMBER("Amber"),
    GOLD("Gold"),
    MINT("Mint"),
    EMERALD("Emerald"),
    TEAL("Teal"),
    TURQUOISE("Turquoise"),
    CYAN("Cyan"),
    SKY("Sky"),
    PERIWINKLE("Periwinkle"),
    LAVENDER("Lavender"),
    VIOLET("Violet"),
    PINK("Pink"),
    SAKURA("Sakura"),
    MAGENTA("Magenta"),
    COBALT("Cobalt"),
    NAVY("Navy"),
    CRIMSON("Crimson"),
    BURGUNDY("Burgundy"),
    LIME("Lime"),
    COCOA("Cocoa"),
    GRAPHITE("Graphite"),
    SLATE("Slate"),
}

data class ThemePreferences(
    val mode: ThemeMode = ThemeMode.SYSTEM,
    val color: ThemeColor = ThemeColor.SLATE,
)
