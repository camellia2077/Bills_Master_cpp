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
class WorkspaceServiceInstrumentedTest {
    @Test
    fun initializeAndImportBundledSampleSucceed() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val services = createAndroidServiceBundle(context)

        val environment = services.workspaceService.initializeEnvironment()
        val imported = services.workspaceService.importBundledSample()

        assertTrue(environment.bundledSampleInputPath.exists())
        assertEquals("2025", environment.bundledSampleYear)
        assertEquals(12, imported.processed)
        assertEquals(12, imported.imported)
    }
}
