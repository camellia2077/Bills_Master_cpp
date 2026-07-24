package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.QueryNativeBindings
import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.double
import com.billstracer.android.data.nativebridge.int
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.model.MonthlySummaryItem
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.intOrNull
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

internal data class ParsedQueryResultPayload(
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
)

internal fun parseQueryResultPayload(data: JsonObject): ParsedQueryResultPayload {
    val periodStart = data.string("period_start")
    val periodEnd = data.string("period_end")
    val monthlySummary = data["monthly_summary"]?.jsonArray?.mapNotNull { item ->
        val entry = item.jsonObject
        val period = entry["period"]?.jsonPrimitive?.contentOrNull ?: return@mapNotNull null
        MonthlySummaryItem(
            period = period,
            income = entry.double("income"),
            expense = entry.double("expense"),
            balance = entry.double("balance"),
        )
    }.orEmpty()

    val derivedPeriod = periodStart.ifBlank { periodEnd }
    val derivedYear = data["year"]?.jsonPrimitive?.intOrNull
        ?: derivedPeriod.substringBefore('-', missingDelimiterValue = "").toIntOrNull()
    val derivedMonth = data["month"]?.jsonPrimitive?.intOrNull
        ?: derivedPeriod.substringAfter('-', missingDelimiterValue = "").toIntOrNull()

    return ParsedQueryResultPayload(
        periodStart = periodStart,
        periodEnd = periodEnd,
        transactionCount = data.int("transaction_count"),
        remark = data.string("remark"),
        year = derivedYear,
        month = derivedMonth,
        matchedBills = data.int("matched_bills"),
        totalIncome = data.double("total_income"),
        totalExpense = data.double("total_expense"),
        balance = data.double("balance"),
        monthlySummary = monthlySummary,
        standardReportMarkdown = data["report_markdown"]?.jsonPrimitive?.contentOrNull,
        standardReportJson = data["standard_report"]?.toString(),
    )
}

internal class DefaultQueryService(
    private val runtime: AndroidWorkspaceRuntime,
) : QueryService {
    override suspend fun listAvailablePeriods(): List<String> = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        parseAvailablePeriods(
            QueryNativeBindings.listAvailablePeriodsNative(
                workspace.dbFile.absolutePath,
            ),
        )
    }

    override suspend fun queryYear(isoYear: String): QueryResult = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        parseQueryResult(
            rawJson = QueryNativeBindings.queryYearNative(
                workspace.dbFile.absolutePath,
                isoYear,
            ),
            type = QueryType.YEAR,
        )
    }

    override suspend fun queryMonth(isoMonth: String): QueryResult = withContext(Dispatchers.IO) {
        val workspace = runtime.initializeWorkspace()
        parseQueryResult(
            rawJson = QueryNativeBindings.queryMonthNative(
                workspace.dbFile.absolutePath,
                isoMonth,
            ),
            type = QueryType.MONTH,
        )
    }

    override suspend fun queryRange(startIsoMonth: String, endIsoMonth: String): QueryResult =
        withContext(Dispatchers.IO) {
            val workspace = runtime.initializeWorkspace()
            parseQueryResult(
                rawJson = QueryNativeBindings.queryRangeNative(
                    workspace.dbFile.absolutePath,
                    startIsoMonth,
                    endIsoMonth,
                ),
                type = QueryType.RANGE,
            )
        }

    private fun parseQueryResult(rawJson: String, type: QueryType): QueryResult {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        val payload = parseQueryResultPayload(data)

        return QueryResult(
            ok = root.boolean("ok"),
            message = root.string("message"),
            type = type,
            periodStart = payload.periodStart,
            periodEnd = payload.periodEnd,
            transactionCount = payload.transactionCount,
            remark = payload.remark,
            year = payload.year,
            month = if (type == QueryType.MONTH) payload.month else null,
            matchedBills = payload.matchedBills,
            totalIncome = payload.totalIncome,
            totalExpense = payload.totalExpense,
            balance = payload.balance,
            monthlySummary = payload.monthlySummary,
            standardReportMarkdown = payload.standardReportMarkdown,
            standardReportJson = payload.standardReportJson,
            rawJson = rawJson,
        )
    }

    private fun parseAvailablePeriods(rawJson: String): List<String> {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        if (!root.boolean("ok")) {
            error(root.string("message"))
        }
        return data["periods"]?.jsonArray?.mapNotNull { item ->
            item.jsonPrimitive.contentOrNull
        }.orEmpty().distinct().sortedDescending()
    }
}
