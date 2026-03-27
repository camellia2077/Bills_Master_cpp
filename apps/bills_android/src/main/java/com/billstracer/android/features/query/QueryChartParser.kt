package com.billstracer.android.features.query

import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.doubleOrNull
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

internal data class QueryChartUiModel(
    val schemaVersion: String,
    val views: List<QueryChartViewUiModel>,
)

internal sealed interface QueryChartViewUiModel {
    val id: String
    val title: String
}

internal data class GroupedBarChartViewUiModel(
    override val id: String,
    override val title: String,
    val xLabels: List<String>,
    val series: List<GroupedBarChartSeriesUiModel>,
) : QueryChartViewUiModel

internal data class GroupedBarChartSeriesUiModel(
    val id: String,
    val label: String,
    val unit: String,
    val colorHex: String?,
    val values: List<Double>,
)

internal data class PieChartViewUiModel(
    override val id: String,
    override val title: String,
    val unit: String,
    val segments: List<PieChartSegmentUiModel>,
) : QueryChartViewUiModel

internal data class PieChartSegmentUiModel(
    val id: String,
    val label: String,
    val value: Double,
    val colorHex: String?,
)

internal fun parseQueryChartData(rawJson: String?): QueryChartUiModel? {
    val content = rawJson?.takeIf { it.isNotBlank() } ?: return null

    return try {
        val root = standardReportJsonParser.parseToJsonElement(content).jsonObject
        val chartData = root["extensions"]?.jsonObject?.get("chart_data")?.jsonObject ?: return null
        val rawViews = chartData["views"]?.jsonArray ?: return null
        if (rawViews.isEmpty()) {
            return QueryChartUiModel(
                schemaVersion = chartData.string("schema_version"),
                views = emptyList(),
            )
        }
        val views = rawViews.mapNotNull { viewElement ->
            val view = viewElement.jsonObject
            when (view.string("chart_type")) {
                "grouped_bar" -> parseGroupedBarChartView(view)
                "pie" -> parsePieChartView(view)
                else -> null
            }
        }
        if (views.isEmpty()) {
            return null
        }

        QueryChartUiModel(
            schemaVersion = chartData.string("schema_version"),
            views = views,
        )
    } catch (_: Exception) {
        null
    }
}

private fun parseGroupedBarChartView(
    view: kotlinx.serialization.json.JsonObject,
): GroupedBarChartViewUiModel? {
    val xLabels = view["x_labels"]?.jsonArray?.mapNotNull { label ->
        label.jsonPrimitive.contentOrNull
    }.orEmpty()
    if (xLabels.isEmpty()) {
        return null
    }

    val series = view["series"]?.jsonArray?.mapNotNull { seriesElement ->
        val seriesObject = seriesElement.jsonObject
        val values = seriesObject["values"]?.jsonArray?.mapNotNull { value ->
            value.jsonPrimitive.doubleOrNull
        }.orEmpty()
        if (values.size != xLabels.size) {
            return@mapNotNull null
        }
        GroupedBarChartSeriesUiModel(
            id = seriesObject.string("id"),
            label = seriesObject.string("label"),
            unit = seriesObject.string("unit"),
            colorHex = seriesObject["color"]?.jsonPrimitive?.contentOrNull,
            values = values,
        )
    }.orEmpty()
    if (series.isEmpty()) {
        return null
    }

    return GroupedBarChartViewUiModel(
        id = view.string("id"),
        title = view.string("title"),
        xLabels = xLabels,
        series = series,
    )
}

private fun parsePieChartView(
    view: kotlinx.serialization.json.JsonObject,
): PieChartViewUiModel? {
    val segments = view["segments"]?.jsonArray?.mapNotNull { segmentElement ->
        val segment = segmentElement.jsonObject
        val value = segment["value"]?.jsonPrimitive?.doubleOrNull ?: return@mapNotNull null
        PieChartSegmentUiModel(
            id = segment.string("id"),
            label = segment.string("label"),
            value = value,
            colorHex = segment["color"]?.jsonPrimitive?.contentOrNull,
        )
    }.orEmpty()
    if (segments.isEmpty()) {
        return null
    }

    return PieChartViewUiModel(
        id = view.string("id"),
        title = view.string("title"),
        unit = view.string("unit"),
        segments = segments,
    )
}
