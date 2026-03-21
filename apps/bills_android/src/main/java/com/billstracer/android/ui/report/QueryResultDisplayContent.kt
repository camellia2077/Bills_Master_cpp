package com.billstracer.android.ui.report

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType

@Composable
internal fun queryResultDisplayContent(
    result: QueryResult,
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
    var showAlternateView by rememberSaveable(
        result.rawJson,
        result.standardReportMarkdown,
        result.standardReportJson,
        result.type,
        result.year,
        result.month,
    ) {
        mutableStateOf(false)
    }

    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(14.dp),
    ) {
        if (result.type == QueryType.MONTH) {
            when {
                monthlyStandardReport == null -> QuerySourceCard(
                    title = "Markdown Report",
                    content = markdown,
                    toggleLabel = null,
                    onToggle = null,
                    isRawJson = false,
                    renderMarkdown = true,
                )

                showAlternateView -> QuerySourceCard(
                    title = "Markdown Report",
                    content = markdown,
                    toggleLabel = "Show Native View",
                    onToggle = { showAlternateView = false },
                    isRawJson = false,
                    renderMarkdown = true,
                )

                else -> MonthlyStandardReportCard(
                    report = monthlyStandardReport,
                    toggleLabel = "Show Markdown",
                    onToggle = { showAlternateView = true },
                )
            }

            StructuredSummaryCard(
                result = result,
                monthlySummary = result.monthlySummary,
            )
        } else {
            QuerySourceCard(
                title = if (showAlternateView) "Raw JSON" else "Markdown Report",
                content = if (showAlternateView) result.rawJson else markdown,
                toggleLabel = if (showAlternateView) "Show Markdown" else "Show Raw JSON",
                onToggle = { showAlternateView = !showAlternateView },
                isRawJson = showAlternateView,
                renderMarkdown = false,
            )
        }
    }
}

internal fun buildFallbackMarkdown(result: QueryResult): String {
    return buildString {
        appendLine("# Markdown Report")
        appendLine()
        append(
            result.message.ifBlank {
                "No markdown content returned by the native bridge."
            },
        )
    }
}
