package com.billstracer.android.features.query

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import com.billstracer.android.platform.MarkdownText

@Composable
internal fun QueryResultDisplayContent(
    result: QueryResult,
    selectedViewMode: QueryViewMode,
    onSelectViewMode: (QueryViewMode) -> Unit,
    modifier: Modifier = Modifier,
) {
    val markdown = result.standardReportMarkdown?.takeIf { it.isNotBlank() }
        ?: buildFallbackMarkdown(result)
    val monthlyStandardReport = remember(result.type, result.standardReportJson) {
        if (result.type == QueryType.MONTH) {
            parseMonthlyStandardReport(result.standardReportJson)
        } else {
            null
        }
    }
    val yearlyStandardReport = remember(result.type, result.standardReportJson) {
        if (result.type == QueryType.YEAR) {
            parseYearlyStandardReport(result.standardReportJson)
        } else {
            null
        }
    }
    val chartData = remember(result.standardReportJson) {
        parseQueryChartData(result.standardReportJson)
    }
    val hasStructuredView = monthlyStandardReport != null || yearlyStandardReport != null
    val hasChartView = chartData?.views?.isNotEmpty() == true
    val availableModes = remember(hasStructuredView, hasChartView) {
        QueryModeAvailability(
            hasStructuredView = hasStructuredView,
            hasChartView = hasChartView,
        ).availableModes()
    }
    val effectiveViewMode = remember(selectedViewMode, hasStructuredView, hasChartView) {
        resolveQueryViewMode(
            hasStructuredView = hasStructuredView,
            hasChartView = hasChartView,
            preferredMode = selectedViewMode,
        )
    }

    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(14.dp),
    ) {
        QueryResultModeSwitch(
            availableModes = availableModes,
            selectedViewMode = effectiveViewMode,
            onSelectViewMode = onSelectViewMode,
        )

        when (effectiveViewMode) {
            QueryViewMode.STRUCTURED -> {
                when {
                    result.type == QueryType.MONTH && monthlyStandardReport != null -> {
                        MonthlyStandardReportCard(report = monthlyStandardReport)
                    }
                    result.type == QueryType.YEAR && yearlyStandardReport != null -> {
                        YearlyStandardReportCard(report = yearlyStandardReport)
                    }
                    else -> QueryTextReportCard(content = markdown)
                }
            }

            QueryViewMode.CHART -> {
                if (chartData != null && chartData.views.isNotEmpty()) {
                    QueryChartContent(chartData = chartData)
                } else {
                    QueryTextReportCard(content = markdown)
                }
            }

            QueryViewMode.TEXT -> QueryTextReportCard(content = markdown)
        }
    }
}

internal fun buildFallbackMarkdown(result: QueryResult): String = buildString {
    appendLine("# Text Report")
    appendLine()
    append(
        result.message.ifBlank {
            queryTextEmptyMessage()
        },
    )
}

@Composable
private fun QueryResultModeSwitch(
    availableModes: List<QueryViewMode>,
    selectedViewMode: QueryViewMode,
    onSelectViewMode: (QueryViewMode) -> Unit,
) {
    Surface(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("query_result_mode_switch"),
        shape = RoundedCornerShape(18.dp),
        color = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.72f),
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState())
                .padding(8.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            availableModes.forEach { viewMode ->
                val selected = viewMode == selectedViewMode
                val buttonModifier = Modifier.testTag(
                    when (viewMode) {
                        QueryViewMode.STRUCTURED -> "query_mode_structured_button"
                        QueryViewMode.TEXT -> "query_mode_text_button"
                        QueryViewMode.CHART -> "query_mode_chart_button"
                    },
                )
                if (selected) {
                    Button(
                        onClick = { onSelectViewMode(viewMode) },
                        modifier = buttonModifier,
                    ) {
                        Text(text = queryViewModeLabel(viewMode))
                    }
                } else {
                    OutlinedButton(
                        onClick = { onSelectViewMode(viewMode) },
                        modifier = buttonModifier,
                    ) {
                        Text(text = queryViewModeLabel(viewMode))
                    }
                }
            }
        }
    }
}

private fun queryViewModeLabel(viewMode: QueryViewMode): String = when (viewMode) {
    QueryViewMode.STRUCTURED -> "Structured"
    QueryViewMode.TEXT -> "Text"
    QueryViewMode.CHART -> "Chart"
}

@Composable
private fun QueryTextReportCard(
    content: String,
) {
    val scrollState = rememberScrollState()

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("query_text_card"),
        shape = RoundedCornerShape(22.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.secondaryContainer.copy(alpha = 0.50f),
        ),
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = "Text Report",
                style = MaterialTheme.typography.titleMedium,
            )
            Spacer(modifier = Modifier.height(12.dp))
            Surface(
                modifier = Modifier
                    .fillMaxWidth()
                    .heightIn(max = 320.dp),
                shape = RoundedCornerShape(18.dp),
                color = MaterialTheme.colorScheme.surface.copy(alpha = 0.92f),
            ) {
                MarkdownText(
                    markdown = content,
                    modifier = Modifier
                        .fillMaxWidth()
                        .verticalScroll(scrollState)
                        .padding(14.dp)
                        .testTag("query_text_content"),
                    color = MaterialTheme.colorScheme.onSurface,
                    style = MaterialTheme.typography.bodyMedium,
                )
            }
        }
    }
}

internal fun formatPeriodLabel(periodStart: String, periodEnd: String): String {
    if (periodStart.isBlank() && periodEnd.isBlank()) {
        return "Monthly period"
    }
    if (periodStart.isBlank()) {
        return periodEnd
    }
    if (periodEnd.isBlank()) {
        return periodStart
    }
    return "$periodStart to $periodEnd"
}

internal fun formatAmount(amount: Double): String =
    java.lang.String.format(java.util.Locale.US, "%,.2f", amount)

@Composable
internal fun SummaryStatPill(
    label: String,
    value: String,
) {
    Surface(
        shape = RoundedCornerShape(999.dp),
        color = MaterialTheme.colorScheme.primary.copy(alpha = 0.12f),
    ) {
        Column(
            modifier = Modifier
                .widthIn(min = 88.dp)
                .padding(horizontal = 12.dp, vertical = 8.dp),
        ) {
            Text(label, style = MaterialTheme.typography.labelSmall)
            Text(
                text = value,
                style = MaterialTheme.typography.labelLarge,
                fontFamily = FontFamily.Monospace,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
        }
    }
}
