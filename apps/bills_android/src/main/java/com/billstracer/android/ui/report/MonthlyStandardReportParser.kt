package com.billstracer.android.ui.report

import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.booleanOrNull
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.doubleOrNull
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

private val standardReportJsonParser = Json { ignoreUnknownKeys = true }

internal data class MonthlyStandardReportUiModel(
    val periodStart: String,
    val periodEnd: String,
    val remark: String,
    val dataFound: Boolean,
    val totalIncome: Double,
    val totalExpense: Double,
    val balance: Double,
    val categories: List<MonthlyStandardCategoryUiModel>,
)

internal data class MonthlyStandardCategoryUiModel(
    val name: String,
    val total: Double,
    val subCategories: List<MonthlyStandardSubCategoryUiModel>,
)

internal data class MonthlyStandardSubCategoryUiModel(
    val name: String,
    val subtotal: Double,
    val transactions: List<MonthlyStandardTransactionUiModel>,
)

internal data class MonthlyStandardTransactionUiModel(
    val description: String,
    val source: String,
    val comment: String,
    val transactionType: String,
    val amount: Double,
)

internal fun parseMonthlyStandardReport(rawJson: String?): MonthlyStandardReportUiModel? {
    val content = rawJson?.takeIf { it.isNotBlank() } ?: return null

    return try {
        val root = standardReportJsonParser.parseToJsonElement(content).jsonObject
        val meta = root["meta"]?.jsonObject ?: JsonObject(emptyMap())
        if (meta.string("report_type") != "monthly") {
            return null
        }

        val scope = root["scope"]?.jsonObject ?: JsonObject(emptyMap())
        val summary = root["summary"]?.jsonObject ?: JsonObject(emptyMap())
        val items = root["items"]?.jsonObject ?: JsonObject(emptyMap())
        val categories = items["categories"]?.jsonArray?.map { categoryElement ->
            val category = categoryElement.jsonObject
            MonthlyStandardCategoryUiModel(
                name = category.string("name"),
                total = category.double("total"),
                subCategories = category["sub_categories"]?.jsonArray?.map { subCategoryElement ->
                    val subCategory = subCategoryElement.jsonObject
                    MonthlyStandardSubCategoryUiModel(
                        name = subCategory.string("name"),
                        subtotal = subCategory.double("subtotal"),
                        transactions = subCategory["transactions"]?.jsonArray?.map { transactionElement ->
                            val transaction = transactionElement.jsonObject
                            MonthlyStandardTransactionUiModel(
                                description = transaction.string("description"),
                                source = transaction.string("source"),
                                comment = transaction.string("comment"),
                                transactionType = transaction.string("transaction_type"),
                                amount = transaction.double("amount"),
                            )
                        }.orEmpty(),
                    )
                }.orEmpty(),
            )
        }.orEmpty()

        MonthlyStandardReportUiModel(
            periodStart = scope.string("period_start"),
            periodEnd = scope.string("period_end"),
            remark = scope.string("remark"),
            dataFound = scope.boolean("data_found"),
            totalIncome = summary.double("total_income"),
            totalExpense = summary.double("total_expense"),
            balance = summary.double("balance"),
            categories = categories,
        )
    } catch (_: Exception) {
        null
    }
}

private fun JsonObject.boolean(key: String): Boolean =
    this[key]?.jsonPrimitive?.booleanOrNull ?: false

private fun JsonObject.string(key: String): String =
    this[key]?.jsonPrimitive?.contentOrNull.orEmpty()

private fun JsonObject.double(key: String): Double =
    this[key]?.jsonPrimitive?.doubleOrNull ?: 0.0
