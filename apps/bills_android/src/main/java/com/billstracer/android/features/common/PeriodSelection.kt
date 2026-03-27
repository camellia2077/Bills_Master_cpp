package com.billstracer.android.features.common

internal data class YearMonthSelection(
    val year: String = "",
    val month: String = "",
)

internal fun resolveYearSelection(
    currentYear: String,
    periods: List<String>,
    preferredYear: String? = null,
): String {
    val years = yearsFromPeriods(periods)
    if (years.isEmpty()) {
        return ""
    }
    return when {
        !preferredYear.isNullOrBlank() && years.contains(preferredYear) -> preferredYear
        currentYear.isNotBlank() && years.contains(currentYear) -> currentYear
        else -> years.first()
    }
}

internal fun resolveYearMonthSelection(
    currentYear: String,
    currentMonth: String,
    periods: List<String>,
    preferredPeriod: String? = null,
): YearMonthSelection {
    val selectedYear = resolveYearSelection(
        currentYear = currentYear,
        periods = periods,
        preferredYear = preferredPeriod?.substringBefore('-', ""),
    )
    if (selectedYear.isBlank()) {
        return YearMonthSelection()
    }

    val months = monthsForYear(periods, selectedYear)
    val preferredMonth = preferredPeriod?.substringAfter('-', "")
    val preferredYear = preferredPeriod?.substringBefore('-', "")
    val selectedMonth = when {
        !preferredMonth.isNullOrBlank() &&
            preferredYear == selectedYear &&
            months.contains(preferredMonth) -> preferredMonth
        currentMonth.isNotBlank() && months.contains(currentMonth) -> currentMonth
        else -> months.firstOrNull().orEmpty()
    }

    return YearMonthSelection(
        year = selectedYear,
        month = selectedMonth,
    )
}

internal fun yearsFromPeriods(periods: List<String>): List<String> =
    periods.mapNotNull { period -> period.substringBefore('-').takeIf { it.length == 4 } }
        .distinct()

internal fun monthsForYear(
    periods: List<String>,
    year: String,
): List<String> = periods
    .filter { period -> period.startsWith("$year-") && period.length == 7 }
    .map { period -> period.substringAfter('-') }
    .distinct()
