package com.billstracer.android.features.query

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp

@Composable
internal fun YearlyStandardReportCard(
    report: YearlyStandardReportUiModel,
    toggleLabel: String,
    onToggle: () -> Unit,
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("query_yearly_standard_card"),
        shape = RoundedCornerShape(22.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.secondaryContainer.copy(alpha = 0.50f),
        ),
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(text = "Yearly Report", style = MaterialTheme.typography.titleMedium)
                TextButton(
                    onClick = onToggle,
                    modifier = Modifier.testTag("query_toggle_button"),
                ) {
                    Text(toggleLabel)
                }
            }
            Text(
                text = formatPeriodLabel(report.periodStart, report.periodEnd),
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSecondaryContainer,
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
                    text = "No monthly summary available in standardReportJson.",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSecondaryContainer,
                )
            } else {
                YearlyMonthlySummaryTable(monthlySummary = report.monthlySummary)
            }
        }
    }
}

@Composable
private fun YearlyMonthlySummaryTable(
    monthlySummary: List<YearlyMonthlySummaryUiModel>,
) {
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Text(text = "Monthly Summary", style = MaterialTheme.typography.labelLarge)
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(6.dp),
        ) {
            YearlyMonthlySummaryRow(
                month = "month",
                income = "income",
                expense = "expense",
                balance = "balance",
                isHeader = true,
            )
            monthlySummary.forEach { entry ->
                YearlyMonthlySummaryRow(
                    month = entry.month.toString().padStart(2, '0'),
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
private fun YearlyMonthlySummaryRow(
    month: String,
    income: String,
    expense: String,
    balance: String,
    isHeader: Boolean,
) {
    Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
        YearlySummaryCell(text = month, isHeader = isHeader)
        YearlySummaryCell(text = income, isHeader = isHeader)
        YearlySummaryCell(text = expense, isHeader = isHeader)
        YearlySummaryCell(text = balance, isHeader = isHeader)
    }
}

@Composable
private fun YearlySummaryCell(
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
