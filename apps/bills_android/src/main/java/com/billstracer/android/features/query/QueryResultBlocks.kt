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
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
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
    val hasStructuredView = monthlyStandardReport != null || yearlyStandardReport != null
    val effectiveViewMode = if (selectedViewMode == QueryViewMode.STRUCTURED && !hasStructuredView) {
        QueryViewMode.MARKDOWN
    } else {
        selectedViewMode
    }

    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(14.dp),
    ) {
        if (result.type == QueryType.MONTH && monthlyStandardReport != null) {
            when (selectedViewMode) {
                QueryViewMode.STRUCTURED -> MonthlyStandardReportCard(
                    report = monthlyStandardReport,
                    toggleLabel = "Show Markdown",
                    onToggle = { onSelectViewMode(QueryViewMode.MARKDOWN) },
                )
                QueryViewMode.RAW_JSON -> QuerySourceCard(
                    title = "Raw JSON",
                    content = result.rawJson,
                    toggleLabel = "Show Native View",
                    onToggle = { onSelectViewMode(QueryViewMode.STRUCTURED) },
                    isRawJson = true,
                    renderMarkdown = false,
                )
                QueryViewMode.MARKDOWN -> QuerySourceCard(
                    title = "Markdown Report",
                    content = markdown,
                    toggleLabel = "Show Native View",
                    onToggle = { onSelectViewMode(QueryViewMode.STRUCTURED) },
                    isRawJson = false,
                    renderMarkdown = true,
                )
            }
        } else if (result.type == QueryType.YEAR && yearlyStandardReport != null) {
            when (effectiveViewMode) {
                QueryViewMode.STRUCTURED -> YearlyStandardReportCard(
                    report = yearlyStandardReport,
                    toggleLabel = "Show Markdown",
                    onToggle = { onSelectViewMode(QueryViewMode.MARKDOWN) },
                )
                QueryViewMode.RAW_JSON -> QuerySourceCard(
                    title = "Raw JSON",
                    content = result.rawJson,
                    toggleLabel = "Show Native View",
                    onToggle = { onSelectViewMode(QueryViewMode.STRUCTURED) },
                    isRawJson = true,
                    renderMarkdown = false,
                )
                QueryViewMode.MARKDOWN -> QuerySourceCard(
                    title = "Markdown Report",
                    content = markdown,
                    toggleLabel = "Show Native View",
                    onToggle = { onSelectViewMode(QueryViewMode.STRUCTURED) },
                    isRawJson = false,
                    renderMarkdown = true,
                )
            }
        } else {
            QuerySourceCard(
                title = if (effectiveViewMode == QueryViewMode.RAW_JSON) "Raw JSON" else "Markdown Report",
                content = if (effectiveViewMode == QueryViewMode.RAW_JSON) result.rawJson else markdown,
                toggleLabel = if (effectiveViewMode == QueryViewMode.RAW_JSON) "Show Markdown" else "Show Raw JSON",
                onToggle = {
                    onSelectViewMode(
                        if (effectiveViewMode == QueryViewMode.RAW_JSON) {
                            QueryViewMode.MARKDOWN
                        } else {
                            QueryViewMode.RAW_JSON
                        },
                    )
                },
                isRawJson = effectiveViewMode == QueryViewMode.RAW_JSON,
                renderMarkdown = effectiveViewMode != QueryViewMode.RAW_JSON,
            )
        }
    }
}

internal fun buildFallbackMarkdown(result: QueryResult): String = buildString {
    appendLine("# Markdown Report")
    appendLine()
    append(
        result.message.ifBlank {
            "No markdown content returned by the native bridge."
        },
    )
}

@Composable
private fun QuerySourceCard(
    title: String,
    content: String,
    toggleLabel: String?,
    onToggle: (() -> Unit)?,
    isRawJson: Boolean,
    renderMarkdown: Boolean,
) {
    val scrollState = rememberScrollState()
    val cardTag = if (isRawJson) "query_json_card" else "query_markdown_card"
    val textTag = if (isRawJson) "query_json_text" else "query_markdown_text"

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag(cardTag),
        shape = RoundedCornerShape(22.dp),
        colors = CardDefaults.cardColors(
            containerColor = if (isRawJson) {
                MaterialTheme.colorScheme.surface.copy(alpha = 0.92f)
            } else {
                MaterialTheme.colorScheme.secondaryContainer.copy(alpha = 0.50f)
            },
        ),
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(text = title, style = MaterialTheme.typography.titleMedium)
                if (toggleLabel != null && onToggle != null) {
                    TextButton(
                        onClick = onToggle,
                        modifier = Modifier.testTag("query_toggle_button"),
                    ) {
                        Text(toggleLabel)
                    }
                }
            }
            Spacer(modifier = Modifier.height(12.dp))
            Surface(
                modifier = Modifier
                    .fillMaxWidth()
                    .heightIn(max = 320.dp),
                shape = RoundedCornerShape(18.dp),
                color = if (isRawJson) {
                    MaterialTheme.colorScheme.scrim.copy(alpha = 0.88f)
                } else {
                    MaterialTheme.colorScheme.surface.copy(alpha = 0.92f)
                },
            ) {
                if (renderMarkdown && !isRawJson) {
                    MarkdownText(
                        markdown = content,
                        modifier = Modifier
                            .fillMaxWidth()
                            .verticalScroll(scrollState)
                            .padding(14.dp)
                            .testTag(textTag),
                        color = MaterialTheme.colorScheme.onSurface,
                        style = MaterialTheme.typography.bodyMedium,
                    )
                } else {
                    SelectionContainer {
                        Text(
                            text = content,
                            modifier = Modifier
                                .fillMaxWidth()
                                .verticalScroll(scrollState)
                                .padding(14.dp)
                                .testTag(textTag),
                            color = if (isRawJson) {
                                MaterialTheme.colorScheme.surface
                            } else {
                                MaterialTheme.colorScheme.onSurface
                            },
                            style = if (isRawJson) {
                                MaterialTheme.typography.bodySmall
                            } else {
                                MaterialTheme.typography.bodyMedium
                            },
                            fontFamily = FontFamily.Monospace,
                        )
                    }
                }
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
