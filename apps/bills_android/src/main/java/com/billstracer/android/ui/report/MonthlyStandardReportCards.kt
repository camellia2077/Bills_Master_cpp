package com.billstracer.android.ui.report

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
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
internal fun MonthlyStandardReportCard(
    report: MonthlyStandardReportUiModel,
    toggleLabel: String,
    onToggle: () -> Unit,
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .testTag("query_monthly_standard_card"),
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
                Text(
                    text = "Monthly Report",
                    style = MaterialTheme.typography.titleMedium,
                )
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
                SummaryStatPill(label = "categories", value = report.categories.size.toString())
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
            if (!report.dataFound || report.categories.isEmpty()) {
                Text(
                    text = "No category details available in standardReportJson.",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSecondaryContainer,
                )
            } else {
                Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                    report.categories.forEach { category ->
                        MonthlyStandardCategoryCard(category = category)
                    }
                }
            }
        }
    }
}

@Composable
private fun MonthlyStandardCategoryCard(category: MonthlyStandardCategoryUiModel) {
    Surface(
        shape = RoundedCornerShape(18.dp),
        color = MaterialTheme.colorScheme.surface.copy(alpha = 0.92f),
    ) {
        Column(
            modifier = Modifier.padding(14.dp),
            verticalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(
                    text = category.name.ifBlank { "uncategorized" },
                    style = MaterialTheme.typography.titleSmall,
                )
                AmountText(amount = category.total)
            }
            if (category.subCategories.isEmpty()) {
                Text(
                    text = "No sub-categories",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            } else {
                Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    category.subCategories.forEach { subCategory ->
                        MonthlyStandardSubCategoryCard(subCategory = subCategory)
                    }
                }
            }
        }
    }
}

@Composable
private fun MonthlyStandardSubCategoryCard(subCategory: MonthlyStandardSubCategoryUiModel) {
    Surface(
        shape = RoundedCornerShape(16.dp),
        color = MaterialTheme.colorScheme.primary.copy(alpha = 0.08f),
    ) {
        Column(
            modifier = Modifier.padding(12.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(
                    text = subCategory.name.ifBlank { "misc" },
                    style = MaterialTheme.typography.labelLarge,
                )
                AmountText(amount = subCategory.subtotal)
            }
            if (subCategory.transactions.isEmpty()) {
                Text(
                    text = "No transactions",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            } else {
                Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    subCategory.transactions.forEach { transaction ->
                        MonthlyStandardTransactionRow(transaction = transaction)
                    }
                }
            }
        }
    }
}

@Composable
private fun MonthlyStandardTransactionRow(transaction: MonthlyStandardTransactionUiModel) {
    Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
        Text(
            text = transaction.description.ifBlank { "Untitled transaction" },
            style = MaterialTheme.typography.bodyMedium,
        )
        val details = listOfNotNull(
            transaction.transactionType.takeIf { it.isNotBlank() },
            transaction.source.takeIf { it.isNotBlank() },
            transaction.comment.takeIf { it.isNotBlank() },
        ).joinToString(" · ")
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.Bottom,
        ) {
            if (details.isNotBlank()) {
                Text(
                    text = details,
                    modifier = Modifier.padding(end = 12.dp),
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            } else {
                Spacer(modifier = Modifier.height(0.dp))
            }
            AmountText(amount = transaction.amount)
        }
    }
}

@Composable
private fun AmountText(amount: Double) {
    Text(
        text = formatAmount(amount),
        style = MaterialTheme.typography.labelLarge,
        fontFamily = FontFamily.Monospace,
        maxLines = 1,
        overflow = TextOverflow.Ellipsis,
    )
}
