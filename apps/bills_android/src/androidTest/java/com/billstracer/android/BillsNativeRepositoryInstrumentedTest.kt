package com.billstracer.android

import android.content.Context
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.billstracer.android.data.BillsNativeRepository
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePalette
import com.billstracer.android.model.ThemePreferences
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File

@RunWith(AndroidJUnit4::class)
class BillsNativeRepositoryInstrumentedTest {
    @Test
    fun bundledImportAndYearQuerySucceed() = runBlocking {
        val repository = BillsNativeRepository(ApplicationProvider.getApplicationContext<Context>())
        val environment = repository.initialize()

        val imported = repository.importBundledSample()
        val yearResult = repository.queryBundledYear()

        assertTrue(imported.processed >= 1)
        assertTrue(environment.bundledSampleFile.exists())
        assertEquals("2025", environment.bundledSampleYear)
        assertEquals("0.4.2", environment.coreVersion.versionName)
        assertEquals("2026-03-10", environment.coreVersion.lastUpdated)
        assertEquals(BuildConfig.PRESENTATION_VERSION_NAME, environment.androidVersion.versionName)
        assertEquals(BuildConfig.PRESENTATION_VERSION_CODE, environment.androidVersion.versionCode)
        assertEquals(
            listOf("validator_config.toml", "modifier_config.toml", "export_formats.toml"),
            environment.bundledConfigs.map { it.fileName },
        )
        val exportFormats = environment.bundledConfigs.last().rawText
        assertTrue(exportFormats.contains("\"json\""))
        assertTrue(exportFormats.contains("\"md\""))
        assertTrue(!exportFormats.contains("\"rst\""))
        assertTrue(!exportFormats.contains("\"tex\""))
        assertTrue(!exportFormats.contains("\"typ\""))
        assertTrue(environment.bundledNotices.markdownText.contains("Open Source Notices"))
        assertTrue(environment.bundledNotices.rawJson.contains("\"target_id\": \"android\""))
        assertTrue(yearResult.rawJson.isNotBlank())
        assertTrue(!yearResult.standardReportMarkdown.isNullOrBlank())
    }

    @Test
    fun updateBundledConfigWritesRuntimeToml() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val repository = BillsNativeRepository(context)
        val originalText = repository.initialize().bundledConfigs.last().rawText
        val updatedText = originalText.trimEnd() + "\n# persisted from android test\n"

        try {
            val updatedConfigs = repository.updateBundledConfig(
                fileName = "export_formats.toml",
                rawText = updatedText,
            )
            val reopenedRepository = BillsNativeRepository(context)
            val reopenedEnvironment = reopenedRepository.initialize()

            assertEquals(updatedText, updatedConfigs.last().rawText)
            assertEquals(updatedText, reopenedEnvironment.bundledConfigs.last().rawText)
        } finally {
            BillsNativeRepository(context).updateBundledConfig(
                fileName = "export_formats.toml",
                rawText = originalText,
            )
        }
    }

    @Test
    fun updateThemePreferencesPersistAcrossRepositoryReload() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val repository = BillsNativeRepository(context)
        val originalTheme = repository.initialize().themePreferences
        val updatedTheme = ThemePreferences(
            mode = ThemeMode.DARK,
            palette = ThemePalette.HARBOR,
        )

        try {
            repository.updateThemePreferences(updatedTheme)
            val reopenedRepository = BillsNativeRepository(context)
            val reopenedTheme = reopenedRepository.initialize().themePreferences
            assertEquals(updatedTheme, reopenedTheme)
        } finally {
            BillsNativeRepository(context).updateThemePreferences(originalTheme)
        }
    }

    @Test
    fun recordTemplateSaveAndListUseCoreRecordService() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val repository = BillsNativeRepository(context)
        val environment = repository.initialize()
        val period = "2026-03"
        val recordFile = File(environment.recordsRoot, "2026/2026-03.txt")
        val originalText = recordFile.takeIf { it.isFile }?.readText(Charsets.UTF_8)

        try {
            val opened = repository.openRecordPeriod(period)
            val saved = repository.saveRecordDocument(period, opened.rawText)
            val listed = repository.listRecordPeriods()
            val reopened = repository.openRecordPeriod(period)

            assertEquals(period, opened.period)
            assertTrue(opened.rawText.contains("date:$period"))
            assertEquals("2026/2026-03.txt", saved.relativePath)
            assertTrue(listed.periods.contains(period))
            assertTrue(reopened.persisted)
        } finally {
            if (originalText == null) {
                recordFile.delete()
                recordFile.parentFile?.delete()
            } else {
                recordFile.parentFile?.mkdirs()
                recordFile.writeText(originalText, Charsets.UTF_8)
            }
        }
    }
}
