package com.billstracer.android

import android.content.Context
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertFalse
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.Json

@RunWith(AndroidJUnit4::class)
class QueryServiceInstrumentedTest {
    @Test
    fun listAvailablePeriodsReturnsImportedMonths() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        File(context.noBackupFilesDir, "bills_android").deleteRecursively()
        val services = createAndroidServiceBundle(context)
        val imported = services.workspaceService.importBundledSample()

        assertTrue(imported.ok)
        val periods = services.queryService.listAvailablePeriods()

        assertTrue(periods.isNotEmpty())
    }

    @Test
    fun queryYearReturnsReportData() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        File(context.noBackupFilesDir, "bills_android").deleteRecursively()
        val services = createAndroidServiceBundle(context)
        val environment = services.workspaceService.initializeEnvironment()
        val imported = services.workspaceService.importBundledSample()
        val bundledSampleYear = requireNotNull(environment.bundledSampleYear)

        assertTrue(imported.ok)
        val query = services.queryService.queryYear(bundledSampleYear)

        assertTrue(query.rawJson, query.ok)
        assertEquals(bundledSampleYear.toInt(), query.year)
        assertFalse(query.standardReportMarkdown.isNullOrBlank())
        val reportJson = Json.parseToJsonElement(requireNotNull(query.standardReportJson)).jsonObject
        val chartViews = reportJson["extensions"]?.jsonObject
            ?.get("chart_data")?.jsonObject
            ?.get("views")?.jsonArray
        assertNotNull(chartViews)
        assertTrue(chartViews!!.isNotEmpty())
        val yearSeries = chartViews.first().jsonObject["series"]?.jsonArray
        assertNotNull(yearSeries)
        assertTrue(yearSeries!!.all { series ->
            !series.jsonObject["color"]?.jsonPrimitive?.content.isNullOrBlank()
        })
    }

    @Test
    fun queryMonthReturnsExpensePieChartData() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        File(context.noBackupFilesDir, "bills_android").deleteRecursively()
        val services = createAndroidServiceBundle(context)
        val environment = services.workspaceService.initializeEnvironment()
        val imported = services.workspaceService.importBundledSample()
        val bundledSampleMonth = requireNotNull(environment.bundledSampleMonth)

        assertTrue(imported.ok)
        val query = services.queryService.queryMonth(bundledSampleMonth)

        assertTrue(query.rawJson, query.ok)
        val reportJson = Json.parseToJsonElement(requireNotNull(query.standardReportJson)).jsonObject
        val chartViews = reportJson["extensions"]?.jsonObject
            ?.get("chart_data")?.jsonObject
            ?.get("views")?.jsonArray
        assertNotNull(chartViews)
        assertTrue(chartViews!!.isNotEmpty())
        val monthSegments = chartViews.first().jsonObject["segments"]?.jsonArray
        assertNotNull(monthSegments)
        assertTrue(monthSegments!!.all { segment ->
            !segment.jsonObject["color"]?.jsonPrimitive?.content.isNullOrBlank()
        })
    }
}
