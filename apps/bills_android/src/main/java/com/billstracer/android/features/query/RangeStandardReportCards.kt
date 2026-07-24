package com.billstracer.android.features.query

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp

@Composable
internal fun RangeStandardReportCard(
    report: RangeStandardReportUiModel,
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("query_range_standard_card"),
        shape = RoundedCornerShape(22.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.tertiaryContainer.copy(alpha = 0.50f),
        ),
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.Start,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(text = "Range Report", style = MaterialTheme.typography.titleMedium)
            }
            Text(
                text = formatPeriodLabel(report.periodStart, report.periodEnd),
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onTertiaryContainer,
            )
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .horizontalScroll(rememberScrollState()),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                SummaryStatPill(label = "income", value = formatAmount(report.totalIncome))
                SummaryStatPill(label = "expense", value = formatAmount(report.totalExpense))
                SummaryStatPill(label = "balance", value = formatAmount(report.balance))
                SummaryStatPill(label = "months", value = report.monthlySummary.size.toString())
            }
            if (report.remark.isNotBlank()) {
                Surface(
                    shape = RoundedCornerShape(16.dp),
                    color = MaterialTheme.colorScheme.surface.copy(alpha = 0.92f),
                ) {
                    Text(
                        text = report.remark,
                        modifier = Modifier.padding(horizontal = 12.dp, vertical = 10.dp),
                        style = MaterialTheme.typography.bodySmall,
                    )
                }
            }
            if (!report.dataFound || report.monthlySummary.isEmpty()) {
                Text(
                    text = "No monthly summaries found for this range.",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onTertiaryContainer,
                    modifier = Modifier.testTag("query_range_empty_message"),
                )
            } else {
                RangeMonthlySummaryTable(monthlySummary = report.monthlySummary)
            }
        }
    }
}

@Composable
private fun RangeMonthlySummaryTable(
    monthlySummary: List<RangeMonthlySummaryUiModel>,
) {
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Text(text = "Monthly Summary", style = MaterialTheme.typography.labelLarge)
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(6.dp),
        ) {
            RangeMonthlySummaryRow(
                period = "period",
                income = "income",
                expense = "expense",
                balance = "balance",
                isHeader = true,
            )
            monthlySummary.forEach { entry ->
                RangeMonthlySummaryRow(
                    period = entry.period,
                    income = formatAmount(entry.income),
                    expense = formatAmount(entry.expense),
                    balance = formatAmount(entry.balance),
                    isHeader = false,
                )
            }
        }
    }
}

@Composable
private fun RangeMonthlySummaryRow(
    period: String,
    income: String,
    expense: String,
    balance: String,
    isHeader: Boolean,
) {
    Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
        RangeSummaryCell(text = period, isHeader = isHeader)
        RangeSummaryCell(text = income, isHeader = isHeader)
        RangeSummaryCell(text = expense, isHeader = isHeader)
        RangeSummaryCell(text = balance, isHeader = isHeader)
    }
}

@Composable
private fun RangeSummaryCell(
    text: String,
    isHeader: Boolean,
) {
    Surface(
        shape = RoundedCornerShape(12.dp),
        color = if (isHeader) {
            MaterialTheme.colorScheme.primary.copy(alpha = 0.12f)
        } else {
            MaterialTheme.colorScheme.surface.copy(alpha = 0.92f)
        },
    ) {
        Text(
            text = text,
            modifier = Modifier
                .widthIn(min = 88.dp)
                .padding(horizontal = 10.dp, vertical = 8.dp),
            style = if (isHeader) {
                MaterialTheme.typography.labelLarge
            } else {
                MaterialTheme.typography.bodySmall
            },
            fontFamily = FontFamily.Monospace,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
        )
    }
}
