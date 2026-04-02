package com.billstracer.android.data.services

import java.io.File

internal fun findRecordFileForPeriod(recordsRoot: File, period: String): File? {
    val datePrefix = "date:$period"
    return recordsRoot.walkTopDown()
        .firstOrNull { file ->
            file.isFile &&
            file.extension.equals("txt", ignoreCase = true) &&
            file.useLines { lines -> lines.firstOrNull()?.trim() == datePrefix }
        }
}

internal fun defaultRecordFileForPeriod(recordsRoot: File, period: String): File {
    val year = period.substringBefore('-', missingDelimiterValue = period)
    return File(File(recordsRoot, year), "$period.txt")
}

internal fun recordFileForPeriod(recordsRoot: File, period: String): File =
    defaultRecordFileForPeriod(recordsRoot, period)
