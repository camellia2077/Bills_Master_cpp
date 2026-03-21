package com.billstracer.android

import android.content.Context
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class QueryServiceInstrumentedTest {
    @Test
    fun queryYearReturnsReportData() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val services = createAndroidServiceBundle(context)
        val environment = services.workspaceService.initializeEnvironment()
        services.workspaceService.importBundledSample()

        val query = services.queryService.queryYear(environment.bundledSampleYear)

        assertTrue(query.ok)
        assertEquals(environment.bundledSampleYear.toInt(), query.year)
        assertTrue(!query.standardReportMarkdown.isNullOrBlank())
    }
}
