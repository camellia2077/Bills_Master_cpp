package com.billstracer.android.model

enum class QueryType {
    YEAR,
    MONTH,
}

data class MonthlySummaryItem(
    val month: Int,
    val income: Double,
    val expense: Double,
    val balance: Double,
)

data class QueryResult(
    val ok: Boolean,
    val message: String,
    val type: QueryType,
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
