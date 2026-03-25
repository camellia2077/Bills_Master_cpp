package com.billstracer.android.features.query

import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.booleanOrNull
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.doubleOrNull
import kotlinx.serialization.json.intOrNull
import kotlinx.serialization.json.jsonPrimitive

internal val standardReportJsonParser = Json { ignoreUnknownKeys = true }

internal fun JsonObject.boolean(key: String): Boolean =
    this[key]?.jsonPrimitive?.booleanOrNull ?: false

internal fun JsonObject.string(key: String): String =
    this[key]?.jsonPrimitive?.contentOrNull.orEmpty()

internal fun JsonObject.double(key: String): Double =
    this[key]?.jsonPrimitive?.doubleOrNull ?: 0.0

internal fun JsonObject.int(key: String): Int =
    this[key]?.jsonPrimitive?.intOrNull ?: 0
