package com.billstracer.android.data.services

import java.io.File

internal fun recordFileForPeriod(recordsRoot: File, period: String): File {
    val year = period.substringBefore('-', missingDelimiterValue = period)
    // records/YYYY/YYYY-MM.txt only improves human browsing and parse bundle exports.
    // Database ingest still works by recursively scanning TXT files from the records root.
    return File(File(recordsRoot, year), "$period.txt")
}
