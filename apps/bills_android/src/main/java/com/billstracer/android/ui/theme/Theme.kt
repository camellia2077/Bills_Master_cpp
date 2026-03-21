package com.billstracer.android.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.ColorScheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.lerp
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePreferences

private const val SurfaceContainerTintRatio = 0.10f

private data class PrimaryPalette(
    val p100: Color,
    val p400: Color,
    val p600: Color,
    val p900: Color,
)

private data class NeutralPalette(
    val n50: Color,
    val n100: Color,
    val n200: Color,
    val n300: Color,
    val n400: Color,
    val n500: Color,
    val n600: Color,
    val n700: Color,
    val n800: Color,
    val n900: Color,
    val n950: Color,
)

fun themeColorDisplayOrder(): List<ThemeColor> {
    val rainbow = listOf(
        ThemeColor.CRIMSON,
        ThemeColor.BURGUNDY,
        ThemeColor.ROSE,
        ThemeColor.SAKURA,
        ThemeColor.PEACH,
        ThemeColor.ORANGE,
        ThemeColor.AMBER,
        ThemeColor.GOLD,
        ThemeColor.LIME,
        ThemeColor.MINT,
        ThemeColor.EMERALD,
        ThemeColor.TEAL,
        ThemeColor.TURQUOISE,
        ThemeColor.CYAN,
        ThemeColor.SKY,
        ThemeColor.COBALT,
        ThemeColor.PERIWINKLE,
        ThemeColor.NAVY,
        ThemeColor.SLATE,
        ThemeColor.LAVENDER,
        ThemeColor.VIOLET,
        ThemeColor.MAGENTA,
        ThemeColor.PINK,
    )
    val neutrals = listOf(
        ThemeColor.COCOA,
        ThemeColor.GRAPHITE,
    )
    return rainbow + neutrals + ThemeColor.entries.filter { it !in rainbow && it !in neutrals }
}

private fun getPrimaryPalette(themeColor: ThemeColor): PrimaryPalette =
    when (themeColor) {
        ThemeColor.ROSE -> PrimaryPalette(Rose100, Rose400, Rose600, Rose900)
        ThemeColor.ORANGE -> PrimaryPalette(Orange100, Orange400, Orange600, Orange900)
        ThemeColor.PEACH -> PrimaryPalette(Peach100, Peach400, Peach600, Peach900)
        ThemeColor.AMBER -> PrimaryPalette(Amber100, Amber400, Amber600, Amber900)
        ThemeColor.GOLD -> PrimaryPalette(Gold100, Gold400, Gold600, Gold900)
        ThemeColor.MINT -> PrimaryPalette(Mint100, Mint400, Mint600, Mint900)
        ThemeColor.EMERALD -> PrimaryPalette(Emerald100, Emerald400, Emerald600, Emerald900)
        ThemeColor.TEAL -> PrimaryPalette(Teal100, Teal400, Teal600, Teal900)
        ThemeColor.TURQUOISE -> PrimaryPalette(Turquoise100, Turquoise400, Turquoise600, Turquoise900)
        ThemeColor.CYAN -> PrimaryPalette(Cyan100, Cyan400, Cyan600, Cyan900)
        ThemeColor.SKY -> PrimaryPalette(Sky100, Sky400, Sky600, Sky900)
        ThemeColor.PERIWINKLE -> PrimaryPalette(Periwinkle100, Periwinkle400, Periwinkle600, Periwinkle900)
        ThemeColor.LAVENDER -> PrimaryPalette(Lavender100, Lavender400, Lavender600, Lavender900)
        ThemeColor.VIOLET -> PrimaryPalette(Violet100, Violet400, Violet600, Violet900)
        ThemeColor.PINK -> PrimaryPalette(Pink100, Pink400, Pink600, Pink900)
        ThemeColor.SAKURA -> PrimaryPalette(Sakura100, Sakura400, Sakura600, Sakura900)
        ThemeColor.MAGENTA -> PrimaryPalette(Magenta100, Magenta400, Magenta600, Magenta900)
        ThemeColor.COBALT -> PrimaryPalette(Cobalt100, Cobalt400, Cobalt600, Cobalt900)
        ThemeColor.NAVY -> PrimaryPalette(Navy100, Navy400, Navy600, Navy900)
        ThemeColor.CRIMSON -> PrimaryPalette(Crimson100, Crimson400, Crimson600, Crimson900)
        ThemeColor.BURGUNDY -> PrimaryPalette(Burgundy100, Burgundy400, Burgundy600, Burgundy900)
        ThemeColor.LIME -> PrimaryPalette(Lime100, Lime400, Lime600, Lime900)
        ThemeColor.COCOA -> PrimaryPalette(Cocoa100, Cocoa400, Cocoa600, Cocoa900)
        ThemeColor.GRAPHITE -> PrimaryPalette(Graphite100, Graphite400, Graphite600, Graphite900)
        ThemeColor.SLATE -> PrimaryPalette(Indigo100, Indigo400, Indigo600, Indigo900)
    }

private fun getNeutralPalette(themeColor: ThemeColor): NeutralPalette =
    when (themeColor) {
        ThemeColor.ROSE -> NeutralPalette(
            n50 = Stone50, n100 = Stone100, n200 = Stone200, n300 = Stone300,
            n400 = Stone400, n500 = Stone500, n600 = Stone600, n700 = Stone700,
            n800 = Stone800, n900 = Stone900, n950 = Stone950,
        )

        ThemeColor.PEACH,
        ThemeColor.AMBER,
        ThemeColor.GOLD,
        ThemeColor.ORANGE,
        ThemeColor.SAKURA,
        ThemeColor.CRIMSON,
        ThemeColor.BURGUNDY,
        ThemeColor.COCOA,
        -> NeutralPalette(
            n50 = Stone50, n100 = Stone100, n200 = Stone200, n300 = Stone300,
            n400 = Stone400, n500 = Stone500, n600 = Stone600, n700 = Stone700,
            n800 = Stone800, n900 = Stone900, n950 = Stone950,
        )

        ThemeColor.EMERALD,
        ThemeColor.MINT,
        ThemeColor.TEAL,
        ThemeColor.TURQUOISE,
        ThemeColor.CYAN,
        ThemeColor.LIME,
        -> NeutralPalette(
            n50 = Neutral50, n100 = Neutral100, n200 = Neutral200, n300 = Neutral300,
            n400 = Neutral400, n500 = Neutral500, n600 = Neutral600, n700 = Neutral700,
            n800 = Neutral800, n900 = Neutral900, n950 = Neutral950,
        )

        ThemeColor.SKY -> NeutralPalette(
            n50 = SkyNeutral50, n100 = SkyNeutral100, n200 = SkyNeutral200, n300 = SkyNeutral300,
            n400 = SkyNeutral400, n500 = SkyNeutral500, n600 = SkyNeutral600, n700 = SkyNeutral700,
            n800 = SkyNeutral800, n900 = SkyNeutral900, n950 = SkyNeutral950,
        )

        ThemeColor.VIOLET,
        ThemeColor.PERIWINKLE,
        ThemeColor.LAVENDER,
        ThemeColor.SLATE,
        ThemeColor.COBALT,
        ThemeColor.NAVY,
        ThemeColor.GRAPHITE,
        -> NeutralPalette(
            n50 = Slate50, n100 = Slate100, n200 = Slate200, n300 = Slate300,
            n400 = Slate400, n500 = Slate500, n600 = Slate600, n700 = Slate700,
            n800 = Slate800, n900 = Slate900, n950 = Slate950,
        )

        else -> NeutralPalette(
            n50 = Slate50, n100 = Slate100, n200 = Slate200, n300 = Slate300,
            n400 = Slate400, n500 = Slate500, n600 = Slate600, n700 = Slate700,
            n800 = Slate800, n900 = Slate900, n950 = Slate950,
        )
    }

private fun tintSurface(base: Color, themePrimary: Color): Color =
    lerp(base, themePrimary, SurfaceContainerTintRatio)

fun themeColorPreview(themeColor: ThemeColor): Color =
    when (themeColor) {
        ThemeColor.ROSE -> Rose500
        ThemeColor.ORANGE -> Orange500
        ThemeColor.PEACH -> Peach500
        ThemeColor.AMBER -> Amber500
        ThemeColor.GOLD -> Gold500
        ThemeColor.MINT -> Mint500
        ThemeColor.EMERALD -> Emerald500
        ThemeColor.TEAL -> Teal500
        ThemeColor.TURQUOISE -> Turquoise500
        ThemeColor.CYAN -> Cyan500
        ThemeColor.SKY -> Sky500
        ThemeColor.PERIWINKLE -> Periwinkle500
        ThemeColor.LAVENDER -> Lavender500
        ThemeColor.VIOLET -> Violet500
        ThemeColor.PINK -> Pink500
        ThemeColor.SAKURA -> Sakura500
        ThemeColor.MAGENTA -> Magenta500
        ThemeColor.COBALT -> Cobalt500
        ThemeColor.NAVY -> Navy500
        ThemeColor.CRIMSON -> Crimson500
        ThemeColor.BURGUNDY -> Burgundy500
        ThemeColor.LIME -> Lime500
        ThemeColor.COCOA -> Cocoa500
        ThemeColor.GRAPHITE -> Graphite500
        ThemeColor.SLATE -> Indigo500
    }

fun resolveDarkTheme(
    mode: ThemeMode,
    systemDarkTheme: Boolean,
): Boolean = when (mode) {
    ThemeMode.SYSTEM -> systemDarkTheme
    ThemeMode.LIGHT -> false
    ThemeMode.DARK -> true
}

fun billsColorScheme(
    themeColor: ThemeColor,
    darkTheme: Boolean,
): ColorScheme {
    val primary = getPrimaryPalette(themeColor)
    val neutrals = getNeutralPalette(themeColor)

    return if (darkTheme) {
        darkColorScheme(
            primary = primary.p400,
            onPrimary = neutrals.n900,
            primaryContainer = primary.p900,
            onPrimaryContainer = primary.p100,
            inversePrimary = primary.p600,
            secondary = neutrals.n400,
            onSecondary = neutrals.n950,
            secondaryContainer = neutrals.n700,
            onSecondaryContainer = neutrals.n100,
            tertiary = neutrals.n500,
            onTertiary = neutrals.n950,
            tertiaryContainer = neutrals.n700,
            onTertiaryContainer = neutrals.n100,
            background = neutrals.n950,
            onBackground = neutrals.n100,
            surface = neutrals.n900,
            onSurface = neutrals.n100,
            surfaceVariant = neutrals.n800,
            onSurfaceVariant = neutrals.n300,
            surfaceTint = primary.p400,
            inverseSurface = neutrals.n100,
            inverseOnSurface = neutrals.n900,
            outline = neutrals.n500,
            outlineVariant = neutrals.n700,
            scrim = Color.Black,
            surfaceBright = neutrals.n800,
            surfaceContainerLowest = tintSurface(neutrals.n950, primary.p400),
            surfaceContainerLow = tintSurface(neutrals.n900, primary.p400),
            surfaceContainer = tintSurface(neutrals.n800, primary.p400),
            surfaceContainerHigh = tintSurface(neutrals.n700, primary.p400),
            surfaceContainerHighest = tintSurface(neutrals.n600, primary.p400),
            surfaceDim = neutrals.n950,
        )
    } else {
        lightColorScheme(
            primary = primary.p600,
            onPrimary = LightSurface,
            primaryContainer = primary.p100,
            onPrimaryContainer = primary.p900,
            inversePrimary = primary.p400,
            secondary = neutrals.n600,
            onSecondary = neutrals.n50,
            secondaryContainer = neutrals.n200,
            onSecondaryContainer = neutrals.n900,
            tertiary = neutrals.n500,
            onTertiary = neutrals.n50,
            tertiaryContainer = neutrals.n100,
            onTertiaryContainer = neutrals.n900,
            background = neutrals.n100,
            onBackground = neutrals.n900,
            surface = neutrals.n50,
            onSurface = neutrals.n900,
            surfaceVariant = neutrals.n200,
            onSurfaceVariant = neutrals.n600,
            surfaceTint = primary.p600,
            inverseSurface = neutrals.n800,
            inverseOnSurface = neutrals.n100,
            outline = neutrals.n400,
            outlineVariant = neutrals.n300,
            scrim = Color.Black,
            surfaceBright = LightSurface,
            surfaceContainerLowest = tintSurface(LightSurface, primary.p600),
            surfaceContainerLow = tintSurface(neutrals.n50, primary.p600),
            surfaceContainer = tintSurface(neutrals.n100, primary.p600),
            surfaceContainerHigh = tintSurface(neutrals.n200, primary.p600),
            surfaceContainerHighest = tintSurface(neutrals.n300, primary.p600),
            surfaceDim = neutrals.n200,
        )
    }
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

    MaterialTheme(
        colorScheme = billsColorScheme(
            themeColor = themePreferences.color,
            darkTheme = darkTheme,
        ),
        typography = billsTypography,
        content = content,
    )
}
