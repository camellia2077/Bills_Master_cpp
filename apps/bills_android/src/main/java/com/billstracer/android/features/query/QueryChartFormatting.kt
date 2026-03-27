package com.billstracer.android.features.query

import androidx.compose.material3.ColorScheme
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.lerp
import kotlin.math.absoluteValue
import kotlin.math.max
import kotlin.math.min

internal data class ChartAxisLayout(
    val minValue: Double,
    val maxValue: Double,
    val ticks: List<Double>,
) {
    val range: Double = (maxValue - minValue).takeIf { it > 0.0 } ?: 1.0

    fun valueToRatio(value: Double): Float =
        ((maxValue - value) / range).toFloat().coerceIn(0f, 1f)
}

internal data class GroupedBarChartColors(
    val fallbackIncome: Color,
    val fallbackExpense: Color,
    val fallbackBalance: Color,
    val axis: Color,
    val grid: Color,
    val zeroLine: Color,
    val legendFillAlpha: Float = 0.14f,
)

internal fun buildGroupedBarChartAxisLayout(
    values: List<Double>,
    tickCount: Int = 4,
): ChartAxisLayout {
    val sanitizedTickCount = tickCount.coerceAtLeast(2)
    val maxValue = max(values.maxOrNull() ?: 0.0, 0.0)
    val minValue = min(values.minOrNull() ?: 0.0, 0.0)
    if (maxValue == 0.0 && minValue == 0.0) {
        return ChartAxisLayout(
            minValue = 0.0,
            maxValue = 0.0,
            ticks = List(sanitizedTickCount) { 0.0 },
        )
    }
    val range = (maxValue - minValue).takeIf { it > 0.0 } ?: 1.0
    val step = range / (sanitizedTickCount - 1)
    val ticks = (0 until sanitizedTickCount).map { index ->
        maxValue - (step * index)
    }
    return ChartAxisLayout(
        minValue = minValue,
        maxValue = maxValue,
        ticks = ticks,
    )
}

internal fun formatChartAxisAmount(amount: Double): String {
    val absolute = amount.absoluteValue
    if (absolute < 1000.0) {
        return formatAmount(amount)
    }
    val sign = if (amount < 0) "-" else ""
    return when {
        absolute >= 1_000_000.0 -> sign + String.format(java.util.Locale.US, "%.1fm", absolute / 1_000_000.0)
        else -> sign + String.format(java.util.Locale.US, "%.1fk", absolute / 1_000.0)
    }
}

internal fun groupedBarChartColors(colorScheme: ColorScheme): GroupedBarChartColors =
    GroupedBarChartColors(
        fallbackIncome = Color(0xFF2563EB),
        fallbackExpense = Color(0xFFDC2626),
        fallbackBalance = Color(0xFF7C3AED),
        axis = colorScheme.outline.copy(alpha = 0.62f),
        grid = colorScheme.outlineVariant.copy(alpha = 0.42f),
        zeroLine = colorScheme.onSurface.copy(alpha = 0.72f),
    )

internal fun groupedBarSeriesColor(
    series: GroupedBarChartSeriesUiModel,
    colors: GroupedBarChartColors,
): Color = parseHexColorOrFallback(
    colorHex = series.colorHex,
    fallback = when (series.id) {
        "income" -> colors.fallbackIncome
        "expense" -> colors.fallbackExpense
        "balance" -> colors.fallbackBalance
        else -> colors.fallbackBalance
    },
)

internal fun pieChartFallbackPalette(colorScheme: ColorScheme): List<Color> = listOf(
    Color(0xFF2563EB),
    Color(0xFFDC2626),
    Color(0xFF059669),
    Color(0xFFD97706),
    Color(0xFF7C3AED),
    Color(0xFFDB2777),
    Color(0xFF0891B2),
    Color(0xFF65A30D),
    colorScheme.surfaceVariant.copy(alpha = 0.95f),
    lerp(colorScheme.surfaceVariant, colorScheme.outlineVariant, 0.30f),
    colorScheme.outlineVariant.copy(alpha = 0.92f),
    lerp(colorScheme.outlineVariant, colorScheme.surface, 0.22f),
)

internal fun pieChartSegmentColor(
    segment: PieChartSegmentUiModel,
    fallback: Color,
): Color = parseHexColorOrFallback(segment.colorHex, fallback)

internal fun parseHexColorOrFallback(
    colorHex: String?,
    fallback: Color,
): Color {
    val normalized = colorHex
        ?.trim()
        ?.takeIf { it.startsWith("#") && (it.length == 7 || it.length == 9) }
        ?: return fallback

    return runCatching {
        val hex = normalized.removePrefix("#")
        val raw = hex.toLong(16)
        if (hex.length == 6) {
            Color(
                red = ((raw shr 16) and 0xFF).toInt(),
                green = ((raw shr 8) and 0xFF).toInt(),
                blue = (raw and 0xFF).toInt(),
                alpha = 0xFF,
            )
        } else {
            Color(
                red = ((raw shr 16) and 0xFF).toInt(),
                green = ((raw shr 8) and 0xFF).toInt(),
                blue = (raw and 0xFF).toInt(),
                alpha = ((raw shr 24) and 0xFF).toInt(),
            )
        }
    }.getOrElse { fallback }
}

internal fun yearlyChartSubtitle(unit: String): String = "Amounts in $unit"

internal fun monthlyChartSubtitle(): String = "Expense categories sorted by amount"

internal fun yearlyChartEmptyMessage(): String =
    "No yearly chart data is available for this result."

internal fun monthlyChartEmptyMessage(): String =
    "No expense category data is available for this month."

internal fun yearlyStructuredEmptyMessage(): String =
    "No monthly summary is available for this year."

internal fun monthlyStructuredEmptyMessage(): String =
    "No categorized transactions are available for this month."

internal fun queryTextEmptyMessage(): String =
    "No text report is available for this query result."
