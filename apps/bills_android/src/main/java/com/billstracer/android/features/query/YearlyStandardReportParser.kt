package com.billstracer.android.features.query

import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject

internal data class YearlyStandardReportUiModel(
    val periodStart: String,
    val periodEnd: String,
    val remark: String,
    val dataFound: Boolean,
    val totalIncome: Double,
    val totalExpense: Double,
    val balance: Double,
    val monthlySummary: List<YearlyMonthlySummaryUiModel>,
)

internal data class YearlyMonthlySummaryUiModel(
    val month: Int,
    val income: Double,
    val expense: Double,
    val balance: Double,
)

internal fun parseYearlyStandardReport(rawJson: String?): YearlyStandardReportUiModel? {
    val content = rawJson?.takeIf { it.isNotBlank() } ?: return null

    return try {
        val root = standardReportJsonParser.parseToJsonElement(content).jsonObject
        val meta = root["meta"]?.jsonObject ?: JsonObject(emptyMap())
        if (meta.string("report_type") != "yearly") {
            return null
        }

        val scope = root["scope"]?.jsonObject ?: JsonObject(emptyMap())
        val summary = root["summary"]?.jsonObject ?: JsonObject(emptyMap())
        val items = root["items"]?.jsonObject ?: JsonObject(emptyMap())
        val monthlySummary = items["monthly_summary"]?.jsonArray?.map { entry ->
            val monthData = entry.jsonObject
            YearlyMonthlySummaryUiModel(
                month = monthData.int("month"),
                income = monthData.double("income"),
                expense = monthData.double("expense"),
                balance = monthData.double("balance"),
            )
        }.orEmpty()

        YearlyStandardReportUiModel(
            periodStart = scope.string("period_start"),
            periodEnd = scope.string("period_end"),
            remark = scope.string("remark"),
            dataFound = scope.boolean("data_found"),
            totalIncome = summary.double("total_income"),
            totalExpense = summary.double("total_expense"),
            balance = summary.double("balance"),
            monthlySummary = monthlySummary,
        )
    } catch (_: Exception) {
        null
    }
}
