package com.billstracer.android.features.query

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Fill
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import kotlin.math.abs
import kotlin.math.atan2
import kotlin.math.hypot
import kotlin.math.min

private val GroupedBarChartHeight = 220.dp
private val ChartAxisLabelWidth = 58.dp

@Composable
internal fun QueryChartContent(
    chartData: QueryChartUiModel,
    modifier: Modifier = Modifier,
) {
    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(14.dp),
    ) {
        chartData.views.forEach { view ->
            when (view) {
                is GroupedBarChartViewUiModel -> GroupedBarChartCard(view = view)
                is PieChartViewUiModel -> PieChartCard(view = view)
            }
        }
    }
}

@Composable
private fun GroupedBarChartCard(view: GroupedBarChartViewUiModel) {
    val colors = groupedBarChartColors(MaterialTheme.colorScheme)
    val unit = view.series.firstOrNull()?.unit ?: "CNY"
    val layout = buildGroupedBarChartAxisLayout(view.series.flatMap { it.values })

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("query_chart_card"),
        shape = RoundedCornerShape(22.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.secondaryContainer.copy(alpha = 0.50f),
        ),
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(14.dp),
        ) {
            Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                Text(
                    text = view.title,
                    style = MaterialTheme.typography.titleMedium,
                )
                Text(
                    text = yearlyChartSubtitle(unit),
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.testTag("query_chart_subtitle"),
                )
            }
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .horizontalScroll(rememberScrollState()),
                horizontalArrangement = Arrangement.spacedBy(12.dp),
            ) {
                view.series.forEach { series ->
                    ChartLegendPill(
                        label = series.label,
                        color = groupedBarSeriesColor(series, colors),
                        backgroundAlpha = colors.legendFillAlpha,
                    )
                }
            }
            Surface(
                shape = RoundedCornerShape(18.dp),
                color = MaterialTheme.colorScheme.surface.copy(alpha = 0.92f),
            ) {
                Column(
                    modifier = Modifier.padding(horizontal = 12.dp, vertical = 14.dp),
                    verticalArrangement = Arrangement.spacedBy(12.dp),
                ) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(10.dp),
                    ) {
                        Column(
                            modifier = Modifier
                                .width(ChartAxisLabelWidth)
                                .height(GroupedBarChartHeight)
                                .testTag("query_chart_y_axis"),
                            verticalArrangement = Arrangement.SpaceBetween,
                            horizontalAlignment = Alignment.End,
                        ) {
                            layout.ticks.forEachIndexed { index, tick ->
                                Text(
                                    text = formatChartAxisAmount(tick),
                                    style = MaterialTheme.typography.labelSmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                    fontFamily = FontFamily.Monospace,
                                    modifier = Modifier.testTag("query_chart_y_axis_label_$index"),
                                )
                            }
                        }
                        Canvas(
                            modifier = Modifier
                                .fillMaxWidth()
                                .height(GroupedBarChartHeight)
                                .testTag("query_grouped_bar_chart"),
                        ) {
                            val plotTop = 4.dp.toPx()
                            val plotBottom = size.height - 4.dp.toPx()
                            val plotHeight = plotBottom - plotTop
                            val slotWidth = size.width / view.xLabels.size.toFloat()
                            val groupWidth = slotWidth * 0.72f
                            val barWidth = groupWidth / view.series.size.toFloat()
                            val zeroY = plotTop + (layout.valueToRatio(0.0) * plotHeight)

                            layout.ticks.forEach { tick ->
                                val y = plotTop + (layout.valueToRatio(tick) * plotHeight)
                                drawLine(
                                    color = colors.grid,
                                    start = Offset(0f, y),
                                    end = Offset(size.width, y),
                                    strokeWidth = 1.dp.toPx(),
                                )
                            }

                            if (layout.minValue <= 0.0 && layout.maxValue >= 0.0) {
                                drawLine(
                                    color = colors.zeroLine,
                                    start = Offset(0f, zeroY),
                                    end = Offset(size.width, zeroY),
                                    strokeWidth = 1.8.dp.toPx(),
                                )
                            }

                            drawLine(
                                color = colors.axis,
                                start = Offset(0f, plotTop),
                                end = Offset(0f, plotBottom),
                                strokeWidth = 1.dp.toPx(),
                            )

                            view.series.forEachIndexed { seriesIndex, series ->
                                val seriesColor = groupedBarSeriesColor(series, colors)
                                series.values.forEachIndexed { labelIndex, value ->
                                    val groupLeft = labelIndex * slotWidth + ((slotWidth - groupWidth) / 2f)
                                    val left = groupLeft + (seriesIndex * barWidth)
                                    val valueY = plotTop + (layout.valueToRatio(value) * plotHeight)
                                    val top = min(valueY, zeroY)
                                    val height = abs(zeroY - valueY).coerceAtLeast(2.dp.toPx())

                                    drawRoundRect(
                                        color = seriesColor,
                                        topLeft = Offset(left, top),
                                        size = Size(width = barWidth * 0.82f, height = height),
                                        cornerRadius = CornerRadius(8.dp.toPx()),
                                    )
                                }
                            }
                        }
                    }
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(start = ChartAxisLabelWidth + 10.dp),
                        horizontalArrangement = Arrangement.SpaceBetween,
                    ) {
                        view.xLabels.forEach { label ->
                            Text(
                                text = label,
                                style = MaterialTheme.typography.labelSmall,
                                color = MaterialTheme.colorScheme.onSurfaceVariant,
                                textAlign = TextAlign.Center,
                                fontFamily = FontFamily.Monospace,
                            )
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun PieChartCard(view: PieChartViewUiModel) {
    var selectedSegmentIndex by rememberSaveable(view.id) { mutableStateOf<Int?>(null) }
    val fallbackPalette = pieChartFallbackPalette(MaterialTheme.colorScheme)
    val palette = view.segments.mapIndexed { index, segment ->
        pieChartSegmentColor(
            segment = segment,
            fallback = fallbackPalette[index % fallbackPalette.size],
        )
    }
    val total = view.segments.sumOf { it.value }.takeIf { it > 0.0 } ?: 1.0
    val centerFillColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.92f)
    val outlineColor = MaterialTheme.colorScheme.outline.copy(alpha = 0.20f)
    val selectedSegment = selectedSegmentIndex?.let(view.segments::getOrNull)

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("query_chart_card"),
        shape = RoundedCornerShape(22.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.secondaryContainer.copy(alpha = 0.50f),
        ),
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(14.dp),
        ) {
            Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                Text(
                    text = view.title,
                    style = MaterialTheme.typography.titleMedium,
                )
                Text(
                    text = monthlyChartSubtitle(),
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.testTag("query_chart_subtitle"),
                )
            }
            Surface(
                shape = RoundedCornerShape(18.dp),
                color = MaterialTheme.colorScheme.surface.copy(alpha = 0.92f),
            ) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(18.dp),
                    contentAlignment = Alignment.Center,
                ) {
                    Canvas(
                        modifier = Modifier
                            .fillMaxWidth()
                            .height(240.dp)
                            .pointerInput(view.id, view.segments, total) {
                                detectTapGestures { tapOffset ->
                                    val tappedIndex = detectPieChartSegmentIndex(
                                        tapOffset = tapOffset,
                                        canvasSize = Size(size.width.toFloat(), size.height.toFloat()),
                                        segmentValues = view.segments.map { it.value },
                                        total = total,
                                    )
                                    selectedSegmentIndex = if (selectedSegmentIndex == tappedIndex) {
                                        null
                                    } else {
                                        tappedIndex
                                    }
                                }
                            }
                            .testTag("query_pie_chart"),
                    ) {
                        val diameter = min(size.width, size.height)
                        val topLeft = Offset(
                            x = (size.width - diameter) / 2f,
                            y = (size.height - diameter) / 2f,
                        )
                        val arcSize = Size(diameter, diameter)
                        var startAngle = -90f
                        view.segments.forEachIndexed { index, segment ->
                            val sweep = (segment.value / total * 360.0).toFloat()
                            val selected = selectedSegmentIndex == index
                            val expand = if (selected) 10.dp.toPx() else 0f
                            drawArc(
                                color = palette[index],
                                startAngle = startAngle,
                                sweepAngle = sweep,
                                useCenter = true,
                                topLeft = Offset(
                                    x = topLeft.x - (expand / 2f),
                                    y = topLeft.y - (expand / 2f),
                                ),
                                size = Size(
                                    width = arcSize.width + expand,
                                    height = arcSize.height + expand,
                                ),
                                style = Fill,
                            )
                            startAngle += sweep
                        }
                        drawArc(
                            color = centerFillColor,
                            startAngle = 0f,
                            sweepAngle = 360f,
                            useCenter = true,
                            topLeft = Offset(
                                x = topLeft.x + diameter * 0.22f,
                                y = topLeft.y + diameter * 0.22f,
                            ),
                            size = Size(diameter * 0.56f, diameter * 0.56f),
                            style = Fill,
                        )
                        drawArc(
                            color = outlineColor,
                            startAngle = 0f,
                            sweepAngle = 360f,
                            useCenter = false,
                            topLeft = topLeft,
                            size = arcSize,
                            style = Stroke(width = 1.dp.toPx(), cap = StrokeCap.Round),
                        )
                    }
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(4.dp),
                        modifier = Modifier.testTag("query_pie_chart_center"),
                    ) {
                        Text(
                            text = "Total Expense",
                            style = MaterialTheme.typography.labelMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                        Text(
                            text = formatAmount(total),
                            style = MaterialTheme.typography.titleMedium,
                            fontFamily = FontFamily.Monospace,
                            color = MaterialTheme.colorScheme.onSurface,
                        )
                    }
                }
            }
            selectedSegment?.let { segment ->
                val share = segment.value / total
                PieChartSelectionCard(
                    label = segment.label,
                    amount = "${formatAmount(segment.value)} ${view.unit}".trim(),
                    percentage = "${(share * 100).toInt()}%",
                    color = palette[selectedSegmentIndex ?: 0],
                )
            }
            Column(
                verticalArrangement = Arrangement.spacedBy(10.dp),
                modifier = Modifier.testTag("query_pie_chart_legend"),
            ) {
                view.segments.forEachIndexed { index, segment ->
                    val share = segment.value / total
                    ChartLegendRow(
                        label = segment.label,
                        value = "${formatAmount(segment.value)} ${view.unit}".trim(),
                        supporting = "${(share * 100).toInt()}%",
                        color = palette[index],
                        highlighted = selectedSegmentIndex == index,
                        onClick = {
                            selectedSegmentIndex = if (selectedSegmentIndex == index) {
                                null
                            } else {
                                index
                            }
                        },
                    )
                }
            }
        }
    }
}

@Composable
private fun ChartLegendPill(
    label: String,
    color: Color,
    backgroundAlpha: Float,
) {
    Surface(
        shape = RoundedCornerShape(999.dp),
        color = color.copy(alpha = backgroundAlpha),
    ) {
        Row(
            modifier = Modifier.padding(horizontal = 10.dp, vertical = 8.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Surface(
                modifier = Modifier.size(10.dp),
                shape = CircleShape,
                color = color,
                content = {},
            )
            Text(
                text = label,
                style = MaterialTheme.typography.labelLarge,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
        }
    }
}

@Composable
private fun ChartLegendRow(
    label: String,
    value: String,
    supporting: String,
    color: Color,
    highlighted: Boolean = false,
    onClick: (() -> Unit)? = null,
) {
    Surface(
        shape = RoundedCornerShape(16.dp),
        color = if (highlighted) {
            color.copy(alpha = 0.12f)
        } else {
            MaterialTheme.colorScheme.surface.copy(alpha = 0.92f)
        },
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .then(
                    if (onClick != null) {
                        Modifier.clickable(onClick = onClick)
                    } else {
                        Modifier
                    },
                )
                .padding(horizontal = 12.dp, vertical = 10.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Row(
                modifier = Modifier.padding(end = 12.dp),
                horizontalArrangement = Arrangement.spacedBy(10.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Surface(
                    modifier = Modifier.size(12.dp),
                    shape = CircleShape,
                    color = color,
                    content = {},
                )
                Column(verticalArrangement = Arrangement.spacedBy(2.dp)) {
                    Text(
                        text = label.ifBlank { "uncategorized" },
                        style = MaterialTheme.typography.bodyMedium,
                    )
                    Text(
                        text = supporting,
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
            }
            Text(
                text = value,
                style = MaterialTheme.typography.labelLarge,
                fontFamily = FontFamily.Monospace,
                textAlign = TextAlign.End,
            )
        }
    }
}

@Composable
private fun PieChartSelectionCard(
    label: String,
    amount: String,
    percentage: String,
    color: Color,
) {
    Surface(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("query_pie_chart_selection"),
        shape = RoundedCornerShape(16.dp),
        color = color.copy(alpha = 0.12f),
    ) {
        Row(
            modifier = Modifier.padding(horizontal = 14.dp, vertical = 12.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Row(
                horizontalArrangement = Arrangement.spacedBy(10.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Surface(
                    modifier = Modifier.size(12.dp),
                    shape = CircleShape,
                    color = color,
                    content = {},
                )
                Column(verticalArrangement = Arrangement.spacedBy(2.dp)) {
                    Text(
                        text = label.ifBlank { "uncategorized" },
                        style = MaterialTheme.typography.titleSmall,
                    )
                    Text(
                        text = percentage,
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
            }
            Text(
                text = amount,
                style = MaterialTheme.typography.labelLarge,
                fontFamily = FontFamily.Monospace,
            )
        }
    }
}

private fun detectPieChartSegmentIndex(
    tapOffset: Offset,
    canvasSize: Size,
    segmentValues: List<Double>,
    total: Double,
): Int? {
    if (segmentValues.isEmpty() || total <= 0.0) {
        return null
    }

    val diameter = min(canvasSize.width, canvasSize.height)
    val center = Offset(canvasSize.width / 2f, canvasSize.height / 2f)
    val radius = diameter / 2f
    val innerRadius = radius * 0.56f
    val dx = tapOffset.x - center.x
    val dy = tapOffset.y - center.y
    val distance = hypot(dx.toDouble(), dy.toDouble()).toFloat()
    if (distance > radius || distance < innerRadius) {
        return null
    }

    val degrees = Math.toDegrees(atan2(dy.toDouble(), dx.toDouble())).toFloat()
    val normalizedAngle = ((degrees + 90f) + 360f) % 360f
    var accumulatedSweep = 0f
    segmentValues.forEachIndexed { index, value ->
        val sweep = (value / total * 360.0).toFloat()
        accumulatedSweep += sweep
        if (normalizedAngle <= accumulatedSweep || index == segmentValues.lastIndex) {
            return index
        }
    }
    return null
}
