package com.billstracer.android.features.query

import androidx.compose.material3.lightColorScheme
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class QueryChartFormattingTest {
    @Test
    fun formatChartAxisAmountSupportsCompactThresholds() {
        assertEquals("12.34", formatChartAxisAmount(12.34))
        assertEquals("999.99", formatChartAxisAmount(999.99))
        assertEquals("1.2k", formatChartAxisAmount(1200.0))
        assertEquals("12.5k", formatChartAxisAmount(12500.0))
    }

    @Test
    fun buildGroupedBarChartAxisLayoutStartsFromZeroForPositiveValues() {
        val layout = buildGroupedBarChartAxisLayout(
            values = listOf(10.0, 25.0, 40.0),
            tickCount = 4,
        )

        assertEquals(0.0, layout.minValue, 0.0)
        assertEquals(40.0, layout.maxValue, 0.0)
        assertEquals(4, layout.ticks.size)
        assertEquals(40.0, layout.ticks.first(), 0.0)
        assertEquals(0.0, layout.ticks.last(), 0.0)
    }

    @Test
    fun buildGroupedBarChartAxisLayoutKeepsZeroStableForAllZeroValues() {
        val layout = buildGroupedBarChartAxisLayout(
            values = listOf(0.0, 0.0, 0.0),
            tickCount = 4,
        )

        assertEquals(0.0, layout.minValue, 0.0)
        assertEquals(0.0, layout.maxValue, 0.0)
        assertEquals(4, layout.ticks.size)
        assertTrue(layout.ticks.all { it == 0.0 })
    }

    @Test
    fun buildGroupedBarChartAxisLayoutHandlesNegativeValuesWithoutBreakingZeroLine() {
        val layout = buildGroupedBarChartAxisLayout(
            values = listOf(-50.0, 10.0, 40.0),
            tickCount = 4,
        )

        assertEquals(-50.0, layout.minValue, 0.0)
        assertEquals(40.0, layout.maxValue, 0.0)
        assertTrue(layout.valueToRatio(0.0) in 0f..1f)
    }

    @Test
    fun parseHexColorOrFallbackUsesProvidedHexWhenValid() {
        val parsed = parseHexColorOrFallback(
            colorHex = "#2563EB",
            fallback = Color.Gray,
        )

        assertEquals(0xFF2563EB.toInt(), parsed.toArgb())
    }

    @Test
    fun parseHexColorOrFallbackFallsBackForInvalidOrMissingHex() {
        val fallback = Color(red = 0x12, green = 0x34, blue = 0x56)

        val invalid = parseHexColorOrFallback(
            colorHex = "not-a-color",
            fallback = fallback,
        )
        val missing = parseHexColorOrFallback(
            colorHex = null,
            fallback = fallback,
        )

        assertEquals(fallback.toArgb(), invalid.toArgb())
        assertEquals(fallback.toArgb(), missing.toArgb())
    }

    @Test
    fun groupedBarSeriesColorPrefersLibsColorWhenPresent() {
        val color = groupedBarSeriesColor(
            series = GroupedBarChartSeriesUiModel(
                id = "income",
                label = "Income",
                unit = "CNY",
                colorHex = "#0F766E",
                values = listOf(10.0),
            ),
            colors = groupedBarChartColors(lightColorScheme()),
        )

        assertEquals(0xFF0F766E.toInt(), color.toArgb())
    }
}
