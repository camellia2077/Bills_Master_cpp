package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.boolean
import com.billstracer.android.data.nativebridge.double
import com.billstracer.android.data.nativebridge.int
import com.billstracer.android.data.nativebridge.parseRoot
import com.billstracer.android.data.nativebridge.string
import com.billstracer.android.model.RecordPreviewFile
import com.billstracer.android.model.RecordPreviewResult
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.intOrNull
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

internal fun parseRecordPreviewResult(rawJson: String): RecordPreviewResult {
    val root = parseRoot(rawJson)
    val data = root["data"]?.jsonObject
    val files = data?.get("files")?.jsonArray?.map { item ->
        val entry = item.jsonObject
        RecordPreviewFile(
            path = entry.string("path"),
            ok = entry.boolean("ok"),
            period = entry["period"]?.jsonPrimitive?.contentOrNull,
            year = entry["year"]?.jsonPrimitive?.intOrNull,
            month = entry["month"]?.jsonPrimitive?.intOrNull,
            transactionCount = entry.int("transaction_count"),
            totalIncome = entry.double("total_income"),
            totalExpense = entry.double("total_expense"),
            balance = entry.double("balance"),
            error = entry["error"]?.jsonPrimitive?.contentOrNull,
        )
    }.orEmpty()

    return RecordPreviewResult(
        ok = root.boolean("ok"),
        code = root.string("code"),
        message = root.string("message"),
        processed = data?.int("processed") ?: 0,
        success = data?.int("success") ?: 0,
        failure = data?.int("failure") ?: 0,
        periods = data?.get("periods")?.jsonArray?.mapNotNull { item ->
            item.jsonPrimitive.contentOrNull
        }.orEmpty(),
        files = files,
        rawJson = rawJson,
    )
}
