package com.billstracer.android.features.query

import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject

internal data class RangeStandardReportUiModel(
    val periodStart: String,
    val periodEnd: String,
    val remark: String,
    val dataFound: Boolean,
    val totalIncome: Double,
    val totalExpense: Double,
    val balance: Double,
    val monthlySummary: List<RangeMonthlySummaryUiModel>,
)

internal data class RangeMonthlySummaryUiModel(
    val period: String,
    val income: Double,
    val expense: Double,
    val balance: Double,
)

internal fun parseRangeStandardReport(rawJson: String?): RangeStandardReportUiModel? {
    val content = rawJson?.takeIf { it.isNotBlank() } ?: return null

    return try {
        val root = standardReportJsonParser.parseToJsonElement(content).jsonObject
        val meta = root["meta"]?.jsonObject ?: JsonObject(emptyMap())
        if (meta.string("report_type") != "range") {
            return null
        }

        val scope = root["scope"]?.jsonObject ?: JsonObject(emptyMap())
        val summary = root["summary"]?.jsonObject ?: JsonObject(emptyMap())
        val items = root["items"]?.jsonObject ?: JsonObject(emptyMap())
        val monthlySummary = items["monthly_summary"]?.jsonArray?.map { entry ->
            val monthData = entry.jsonObject
            RangeMonthlySummaryUiModel(
                period = monthData.string("period"),
                income = monthData.double("income"),
                expense = monthData.double("expense"),
                balance = monthData.double("balance"),
            )
        }.orEmpty()

        RangeStandardReportUiModel(
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
