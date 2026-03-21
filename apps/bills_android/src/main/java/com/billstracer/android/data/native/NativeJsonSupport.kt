package com.billstracer.android.data.nativebridge

import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.booleanOrNull
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.doubleOrNull
import kotlinx.serialization.json.intOrNull
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

internal val nativeJson: Json = Json { ignoreUnknownKeys = true }

internal fun parseRoot(rawJson: String): JsonObject =
    nativeJson.parseToJsonElement(rawJson).jsonObject

internal fun JsonObject.boolean(key: String): Boolean =
    this[key]?.jsonPrimitive?.booleanOrNull ?: false

internal fun JsonObject.string(key: String): String =
    this[key]?.jsonPrimitive?.contentOrNull.orEmpty()

internal fun JsonObject.int(key: String): Int =
    this[key]?.jsonPrimitive?.intOrNull ?: 0

internal fun JsonObject.double(key: String): Double =
    this[key]?.jsonPrimitive?.doubleOrNull ?: 0.0
