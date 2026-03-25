package com.billstracer.android.model

import java.io.File

data class AppEnvironment(
    val bundledSampleInputPath: File? = null,
    val bundledSampleLabel: String? = null,
    val bundledSampleYear: String? = null,
    val bundledSampleMonth: String? = null,
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
