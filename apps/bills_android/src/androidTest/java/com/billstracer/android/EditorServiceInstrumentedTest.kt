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
    fun openSavePreviewAndListUseEditorService() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val services = createAndroidServiceBundle(context)
        val period = "2026-03"

        val opened = services.editorService.openRecordPeriod(period)
        val saved = services.editorService.saveRecordDocument(period, opened.rawText)
        val preview = services.editorService.previewRecordDocument(period, opened.rawText)
        val listed = services.editorService.listRecordPeriods()

        assertEquals(period, opened.period)
        assertTrue(opened.rawText.contains("date:$period"))
        assertEquals("2026/2026-03.txt", saved.relativePath)
        assertTrue(preview.ok)
        assertTrue(listed.periods.contains(period))
    }
}
