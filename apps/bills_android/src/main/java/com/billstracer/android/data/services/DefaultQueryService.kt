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

internal class DefaultQueryService(
    private val runtime: AndroidWorkspaceRuntime,
) : QueryService {
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

    private fun parseQueryResult(rawJson: String, type: QueryType): QueryResult {
        val root = parseRoot(rawJson)
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        val monthlySummary = data["monthly_summary"]?.jsonArray?.mapNotNull { item ->
            val entry = item.jsonObject
            val month = entry["month"]?.jsonPrimitive?.intOrNull ?: return@mapNotNull null
            MonthlySummaryItem(
                month = month,
                income = entry.double("income"),
                expense = entry.double("expense"),
                balance = entry.double("balance"),
            )
        }.orEmpty()

        return QueryResult(
            ok = root.boolean("ok"),
            message = root.string("message"),
            type = type,
            year = data["year"]?.jsonPrimitive?.intOrNull,
            month = data["month"]?.jsonPrimitive?.intOrNull,
            matchedBills = data.int("matched_bills"),
            totalIncome = data.double("total_income"),
            totalExpense = data.double("total_expense"),
            balance = data.double("balance"),
            monthlySummary = monthlySummary,
            standardReportMarkdown = data["report_markdown"]?.jsonPrimitive?.contentOrNull,
            standardReportJson = data["standard_report"]?.toString(),
            rawJson = rawJson,
        )
    }
}
