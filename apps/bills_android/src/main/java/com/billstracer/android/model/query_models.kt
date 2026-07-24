package com.billstracer.android.model

enum class QueryType {
    YEAR,
    MONTH,
    RANGE,
}

data class MonthlySummaryItem(
    val period: String,
    val income: Double,
    val expense: Double,
    val balance: Double,
)

data class QueryResult(
    val ok: Boolean,
    val message: String,
    val type: QueryType,
    val periodStart: String,
    val periodEnd: String,
    val transactionCount: Int,
    val remark: String,
    val year: Int?,
    val month: Int?,
    val matchedBills: Int,
    val totalIncome: Double,
    val totalExpense: Double,
    val balance: Double,
    val monthlySummary: List<MonthlySummaryItem>,
    val standardReportMarkdown: String?,
    val standardReportJson: String?,
    val rawJson: String,
)
