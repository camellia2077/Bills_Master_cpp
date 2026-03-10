package com.billstracer.android.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.ColorScheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.graphics.Color
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePalette
import com.billstracer.android.model.ThemePreferences

private data class PaletteSchemes(
    val light: ColorScheme,
    val dark: ColorScheme,
)

private val emberSchemes = PaletteSchemes(
    light = lightColorScheme(
        primary = Ember,
        onPrimary = Cream,
        secondary = Moss,
        tertiary = Slate,
        surface = Clay,
        onSurface = ForestInk,
        background = Sand,
        onBackground = ForestInk,
        primaryContainer = Ember.copy(alpha = 0.18f),
        onPrimaryContainer = ForestInk,
        secondaryContainer = Moss.copy(alpha = 0.18f),
        onSecondaryContainer = ForestInk,
        tertiaryContainer = Slate.copy(alpha = 0.14f),
        onTertiaryContainer = ForestInk,
    ),
    dark = darkColorScheme(
        primary = Dune,
        onPrimary = Ash,
        secondary = Pine,
        tertiary = Tide,
        surface = Ash,
        onSurface = Sand,
        background = Color(0xFF110F0E),
        onBackground = Sand,
        primaryContainer = Color(0xFF5E2A15),
        onPrimaryContainer = Dune,
        secondaryContainer = Color(0xFF263724),
        onSecondaryContainer = Color(0xFFD9E8CF),
        tertiaryContainer = Color(0xFF21313D),
        onTertiaryContainer = Color(0xFFD7E6F0),
    ),
)

private val harborSchemes = PaletteSchemes(
    light = lightColorScheme(
        primary = Harbor,
        onPrimary = Sand,
        secondary = SeaGlass,
        tertiary = HarborDeep,
        surface = HarborMist,
        onSurface = HarborDeep,
        background = Sand,
        onBackground = HarborDeep,
        primaryContainer = Tide.copy(alpha = 0.28f),
        onPrimaryContainer = HarborDeep,
        secondaryContainer = SeaGlass.copy(alpha = 0.18f),
        onSecondaryContainer = HarborDeep,
        tertiaryContainer = Harbor.copy(alpha = 0.14f),
        onTertiaryContainer = HarborDeep,
    ),
    dark = darkColorScheme(
        primary = Tide,
        onPrimary = Ash,
        secondary = Color(0xFF7FD2C7),
        tertiary = HarborMist,
        surface = Color(0xFF0D1821),
        onSurface = HarborMist,
        background = Color(0xFF091219),
        onBackground = HarborMist,
        primaryContainer = Color(0xFF114568),
        onPrimaryContainer = HarborMist,
        secondaryContainer = Color(0xFF123B37),
        onSecondaryContainer = Color(0xFFD1F2EC),
        tertiaryContainer = Color(0xFF182A3C),
        onTertiaryContainer = Color(0xFFD8ECF8),
    ),
)

private val groveSchemes = PaletteSchemes(
    light = lightColorScheme(
        primary = Grove,
        onPrimary = Sand,
        secondary = Fern,
        tertiary = Slate,
        surface = GroveMist,
        onSurface = GroveDeep,
        background = Sand,
        onBackground = GroveDeep,
        primaryContainer = Grove.copy(alpha = 0.18f),
        onPrimaryContainer = GroveDeep,
        secondaryContainer = Fern.copy(alpha = 0.18f),
        onSecondaryContainer = GroveDeep,
        tertiaryContainer = Slate.copy(alpha = 0.12f),
        onTertiaryContainer = GroveDeep,
    ),
    dark = darkColorScheme(
        primary = Pine,
        onPrimary = GroveDeep,
        secondary = Color(0xFFB7D47C),
        tertiary = Tide,
        surface = Color(0xFF0F1712),
        onSurface = Color(0xFFDFEBDD),
        background = Color(0xFF0A120D),
        onBackground = Color(0xFFDFEBDD),
        primaryContainer = Color(0xFF1D4A35),
        onPrimaryContainer = Color(0xFFD8ECD8),
        secondaryContainer = Color(0xFF324416),
        onSecondaryContainer = Color(0xFFE4F0C4),
        tertiaryContainer = Color(0xFF1C2D39),
        onTertiaryContainer = Color(0xFFD6E6F2),
    ),
)

private val canyonSchemes = PaletteSchemes(
    light = lightColorScheme(
        primary = Canyon,
        onPrimary = Sand,
        secondary = Brass,
        tertiary = Slate,
        surface = CanyonMist,
        onSurface = CanyonDeep,
        background = Sand,
        onBackground = CanyonDeep,
        primaryContainer = Canyon.copy(alpha = 0.16f),
        onPrimaryContainer = CanyonDeep,
        secondaryContainer = Sandstone.copy(alpha = 0.24f),
        onSecondaryContainer = CanyonDeep,
        tertiaryContainer = Slate.copy(alpha = 0.12f),
        onTertiaryContainer = CanyonDeep,
    ),
    dark = darkColorScheme(
        primary = Sandstone,
        onPrimary = CanyonDeep,
        secondary = Color(0xFFE3C77A),
        tertiary = HarborMist,
        surface = Color(0xFF18110D),
        onSurface = Color(0xFFF2E2D2),
        background = Color(0xFF120B08),
        onBackground = Color(0xFFF2E2D2),
        primaryContainer = Color(0xFF6C391B),
        onPrimaryContainer = Color(0xFFF3DEC8),
        secondaryContainer = Color(0xFF4A3814),
        onSecondaryContainer = Color(0xFFF1E0B3),
        tertiaryContainer = Color(0xFF1F2E39),
        onTertiaryContainer = Color(0xFFD6E6F2),
    ),
)

private val paletteSchemes = mapOf(
    ThemePalette.EMBER to emberSchemes,
    ThemePalette.HARBOR to harborSchemes,
    ThemePalette.GROVE to groveSchemes,
    ThemePalette.CANYON to canyonSchemes,
)

fun resolveDarkTheme(
    mode: ThemeMode,
    systemDarkTheme: Boolean,
): Boolean = when (mode) {
    ThemeMode.SYSTEM -> systemDarkTheme
    ThemeMode.LIGHT -> false
    ThemeMode.DARK -> true
}

fun billsColorScheme(
    palette: ThemePalette,
    darkTheme: Boolean,
): ColorScheme {
    val schemes = paletteSchemes.getValue(palette)
    return if (darkTheme) schemes.dark else schemes.light
}

@Composable
fun BillsAndroidTheme(
    themePreferences: ThemePreferences = ThemePreferences(),
    content: @Composable () -> Unit,
) {
    val darkTheme = resolveDarkTheme(
        mode = themePreferences.mode,
        systemDarkTheme = isSystemInDarkTheme(),
    )
    val colorScheme = remember(themePreferences.palette, darkTheme) {
        billsColorScheme(
            palette = themePreferences.palette,
            darkTheme = darkTheme,
        )
    }

    MaterialTheme(
        colorScheme = colorScheme,
        typography = BillsTypography,
        content = content,
    )
}
