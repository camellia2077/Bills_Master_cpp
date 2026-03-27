package com.billstracer.android

import android.content.Context
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File

@RunWith(AndroidJUnit4::class)
class EditorServiceInstrumentedTest {
    @Test
    fun listCommitOpenAndPreviewUseEditorService() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val services = createAndroidServiceBundle(context)
        val period = "2026-03"
        val rawText = "date:2026-03\nremark:test\n\nmeal\nmeal_low 12 lunch\n"

        services.workspaceService.clearRecordFiles()
        services.workspaceService.clearDatabase()

        val initialPeriods = services.editorService.listPersistedRecordPeriods()
        val saved = services.editorService.commitRecordDocument(period, rawText)
        val periods = services.editorService.listPersistedRecordPeriods()
        val opened = services.editorService.openPersistedRecordPeriod(period)
        val preview = services.editorService.previewRecordDocument(period, opened.rawText)

        assertTrue(initialPeriods.isEmpty())
        assertTrue(saved.ok)
        assertEquals("2026/2026-03.txt", saved.document?.relativePath)
        assertTrue(periods.contains(period))
        assertTrue(services.queryService.listAvailablePeriods().contains(period))
        assertEquals(period, opened.period)
        assertTrue(opened.rawText.contains("date:$period"))
        assertTrue(preview.ok)
    }

    @Test
    fun commitFailureKeepsPreviousTxtAndDatabaseState() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val services = createAndroidServiceBundle(context)
        val period = "2026-03"
        val originalText = "date:2026-03\nremark:stable\n\nmeal\nmeal_low 12 lunch\n"

        services.workspaceService.clearRecordFiles()
        services.workspaceService.clearDatabase()
        val initialSave = services.editorService.commitRecordDocument(period, originalText)
        assertTrue(initialSave.ok)

        val failedSave = services.editorService.commitRecordDocument(
            period,
            "date:2026-04\nremark:broken\n\nmeal\nmeal_low 12 lunch\n",
        )
        val opened = services.editorService.openPersistedRecordPeriod(period)
        val periods = services.queryService.listAvailablePeriods()

        assertFalse(failedSave.ok)
        assertTrue(failedSave.errorMessage?.contains("does not match selected period") == true)
        assertEquals(originalText, opened.rawText)
        assertTrue(periods.contains(period))
        assertFalse(periods.contains("2026-04"))
    }

    @Test
    fun openingDatabaseBackedPeriodFailsWhenCanonicalTxtIsMissing() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val services = createAndroidServiceBundle(context)
        val period = "2026-03"

        services.workspaceService.clearRecordFiles()
        services.workspaceService.clearDatabase()
        val environment = services.workspaceService.initializeEnvironment()
        val saved = services.editorService.commitRecordDocument(
            period,
            "date:2026-03\nremark:test\n\nmeal\nmeal_low 12 lunch\n",
        )
        assertTrue(saved.ok)
        File(environment.recordsRoot, "2026/2026-03.txt").delete()

        assertTrue(services.editorService.listPersistedRecordPeriods().contains(period))
        try {
            services.editorService.openPersistedRecordPeriod(period)
            fail("Expected out-of-sync error when canonical TXT is missing.")
        } catch (error: Throwable) {
            assertTrue(error.message?.contains("out of sync") == true)
        }
    }
}
