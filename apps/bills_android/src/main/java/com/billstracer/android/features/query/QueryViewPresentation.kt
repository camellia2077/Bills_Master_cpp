package com.billstracer.android.features.query

import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType

internal data class QueryModeAvailability(
    val hasStructuredView: Boolean,
    val hasChartView: Boolean,
) {
    fun availableModes(): List<QueryViewMode> = buildList {
        if (hasStructuredView) {
            add(QueryViewMode.STRUCTURED)
        }
        add(QueryViewMode.TEXT)
        if (hasChartView) {
            add(QueryViewMode.CHART)
        }
    }
}

internal fun resolveQueryModeAvailability(result: QueryResult?): QueryModeAvailability {
    if (result == null) {
        return QueryModeAvailability(
            hasStructuredView = false,
            hasChartView = false,
        )
    }

    val hasStructuredView = when (result.type) {
        QueryType.YEAR -> parseYearlyStandardReport(result.standardReportJson) != null
        QueryType.MONTH -> parseMonthlyStandardReport(result.standardReportJson) != null
    }
    val hasChartView = parseQueryChartData(result.standardReportJson)?.views?.isNotEmpty() == true
    return QueryModeAvailability(
        hasStructuredView = hasStructuredView,
        hasChartView = hasChartView,
    )
}

internal fun resolveQueryViewMode(
    hasStructuredView: Boolean,
    hasChartView: Boolean,
    preferredMode: QueryViewMode? = null,
): QueryViewMode {
    if (preferredMode == QueryViewMode.STRUCTURED && hasStructuredView) {
        return QueryViewMode.STRUCTURED
    }
    if (preferredMode == QueryViewMode.CHART && hasChartView) {
        return QueryViewMode.CHART
    }
    if (preferredMode == QueryViewMode.TEXT) {
        return QueryViewMode.TEXT
    }
    if (hasStructuredView) {
        return QueryViewMode.STRUCTURED
    }
    if (hasChartView) {
        return QueryViewMode.CHART
    }
    return QueryViewMode.TEXT
}

internal fun resolvePreferredQueryViewMode(
    result: QueryResult,
    preferredMode: QueryViewMode? = null,
): QueryViewMode {
    val availability = resolveQueryModeAvailability(result)
    return resolveQueryViewMode(
        hasStructuredView = availability.hasStructuredView,
        hasChartView = availability.hasChartView,
        preferredMode = preferredMode,
    )
}
