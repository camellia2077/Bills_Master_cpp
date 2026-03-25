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
class EditorServiceInstrumentedTest {
    @Test
    fun listSaveSyncOpenAndPreviewUseEditorService() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val services = createAndroidServiceBundle(context)
        val period = "2026-03"
        val rawText = "date:2026-03\nremark:test\n\nmeal\nmeal_low 12 lunch\n"

        services.workspaceService.clearRecordFiles()
        services.workspaceService.clearDatabase()

        val initialPeriods = services.editorService.listDatabaseRecordPeriods()
        val saved = services.editorService.saveRecordDocument(period, rawText)
        val synced = services.editorService.syncSavedRecordToDatabase(period)
        val periods = services.editorService.listDatabaseRecordPeriods()
        val opened = services.editorService.openPersistedRecordPeriod(period)
        val preview = services.editorService.previewRecordDocument(period, opened.rawText)

        assertTrue(initialPeriods.isEmpty())
        assertEquals("2026/2026-03.txt", saved.relativePath)
        assertTrue(synced.ok)
        assertTrue(periods.contains(period))
        assertEquals(period, opened.period)
        assertTrue(opened.rawText.contains("date:$period"))
        assertTrue(preview.ok)
    }
}
