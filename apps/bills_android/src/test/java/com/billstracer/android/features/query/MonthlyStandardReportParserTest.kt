package com.billstracer.android.features.query

import com.billstracer.android.fakeMonthStandardReportJson
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Test

class MonthlyStandardReportParserTest {
    @Test
    fun parseMonthlyStandardReportReadsCategoryRows() {
        val report = parseMonthlyStandardReport(fakeMonthStandardReportJson("2026-03"))

        assertNotNull(report)
        assertEquals("2026-03", report?.periodStart)
        assertEquals("2026-03", report?.periodEnd)
        assertEquals(10.0, report?.totalIncome ?: 0.0, 0.0)
        assertEquals(-5.0, report?.totalExpense ?: 0.0, 0.0)
        assertEquals(1, report?.categories?.size)
        assertEquals("meal", report?.categories?.first()?.name)
    }
}
