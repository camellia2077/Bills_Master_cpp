package com.billstracer.android.ui

import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.Alignment
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.billstracer.android.model.MonthlySummaryItem
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType

@Composable
internal fun QueryResultDisplay(
    result: QueryResult,
    modifier: Modifier = Modifier,
) {
    val markdown = result.standardReportMarkdown?.takeIf { it.isNotBlank() }
        ?: buildFallbackMarkdown(result)
    var showRawJson by rememberSaveable(
        result.rawJson,
        result.standardReportMarkdown,
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
        QuerySourceCard(
            title = if (showRawJson) "Raw JSON" else "Markdown Report",
            content = if (showRawJson) result.rawJson else markdown,
            toggleLabel = if (showRawJson) "Show Markdown" else "Show Raw JSON",
            onToggle = { showRawJson = !showRawJson },
            isRawJson = showRawJson,
        )
        if (result.type == QueryType.MONTH) {
            StructuredSummaryCard(
                result = result,
                monthlySummary = result.monthlySummary,
            )
        }
    }
}

private fun buildFallbackMarkdown(result: QueryResult): String {
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

@Composable
private fun QuerySourceCard(
    title: String,
    content: String,
    toggleLabel: String,
    onToggle: () -> Unit,
    isRawJson: Boolean,
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
                Text(
                    text = title,
                    style = MaterialTheme.typography.titleMedium,
                )
                TextButton(
                    onClick = onToggle,
                    modifier = Modifier.testTag("query_toggle_button"),
                ) {
                    Text(toggleLabel)
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

@Composable
private fun StructuredSummaryCard(
    result: QueryResult,
    monthlySummary: List<MonthlySummaryItem>,
) {
    val summaryItems = buildList {
        add("type" to if (result.type == QueryType.YEAR) "year" else "month")
        add("year" to (result.year?.toString() ?: "-"))
        if (result.month != null) {
            add("month" to result.month.toString().padStart(2, '0'))
        }
        add("matched" to result.matchedBills.toString())
        add("income" to result.totalIncome.toString())
        add("expense" to result.totalExpense.toString())
        add("balance" to result.balance.toString())
    }

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("query_summary_card"),
        shape = RoundedCornerShape(22.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.92f),
        ),
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = "Structured Summary",
                style = MaterialTheme.typography.titleMedium,
            )
            Spacer(modifier = Modifier.height(12.dp))
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .horizontalScroll(rememberScrollState()),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                summaryItems.forEach { (label, value) ->
                    SummaryStatPill(label = label, value = value)
                }
            }
            if (monthlySummary.isNotEmpty()) {
                Spacer(modifier = Modifier.height(14.dp))
                Text(
                    text = "Monthly Summary",
                    style = MaterialTheme.typography.labelLarge,
                )
                Spacer(modifier = Modifier.height(8.dp))
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .horizontalScroll(rememberScrollState()),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    monthlySummary.forEach { entry ->
                        SummaryStatPill(
                            label = entry.month.toString().padStart(2, '0'),
                            value = entry.balance.toString(),
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun SummaryStatPill(
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
