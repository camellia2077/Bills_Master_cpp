package com.billstracer.android

import android.content.Context
import android.net.Uri
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.billstracer.android.model.AppEnvironment
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File

@RunWith(AndroidJUnit4::class)
class WorkspaceServiceInstrumentedTest {
    @Test
    fun initializeAndImportBundledSampleSucceed() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        File(context.noBackupFilesDir, "bills_android").deleteRecursively()
        val services = createAndroidServiceBundle(context)

        val environment = services.workspaceService.initializeEnvironment()
        val imported = services.workspaceService.importBundledSample()
        val opened = services.editorService.openPersistedRecordPeriod("2025-01")

        assertTrue(requireNotNull(environment.bundledSampleInputPath).exists())
        assertTrue(File(environment.recordsRoot, "2025/2025-01.txt").isFile)
        assertEquals("2025", environment.bundledSampleYear)
        assertEquals(12, imported.processed)
        assertEquals(12, imported.imported)
        assertEquals("2025/2025-01.txt", opened.relativePath)
        assertTrue(opened.rawText.startsWith("date:2025-01"))
    }

    @Test
    fun importCurrentRecordFilesToDatabaseSucceed() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        File(context.noBackupFilesDir, "bills_android").deleteRecursively()
        val services = createAndroidServiceBundle(context)

        val environment = services.workspaceService.initializeEnvironment()
        val imported = services.workspaceService.importRecordFilesToDatabase()
        val periods = services.queryService.listAvailablePeriods()

        assertTrue(File(environment.recordsRoot, "2025/2025-01.txt").isFile)
        assertEquals(12, imported.processed)
        assertEquals(12, imported.imported)
        assertTrue(periods.contains("2025-01"))
    }

    @Test
    fun importTxtDirectoryToRecordsUsesHeaderPeriodWithoutDatabaseIngest() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        File(context.noBackupFilesDir, "bills_android").deleteRecursively()
        val services = createAndroidServiceBundle(context)

        val environment = services.workspaceService.initializeEnvironment()
        val sourceDir = createSourceDirectory(context, "header_period")
        File(sourceDir, "renamed-source.txt").writeText(
            sampleRecordText(environment).replaceFirst("date:2025-01", "date:2026-04"),
            Charsets.UTF_8,
        )

        val imported = services.workspaceService.importTxtDirectoryToRecords(Uri.fromFile(sourceDir))
        val periods = services.queryService.listAvailablePeriods()
        val importedFile = File(environment.recordsRoot, "2026/2026-04.txt")

        assertEquals(1, imported.processed)
        assertEquals(1, imported.imported)
        assertEquals(0, imported.overwritten)
        assertEquals(0, imported.failure)
        assertTrue(importedFile.isFile)
        assertTrue(importedFile.readText(Charsets.UTF_8).startsWith("date:2026-04"))
        assertFalse(periods.contains("2026-04"))
    }

    @Test
    fun importTxtDirectoryToRecordsOverwritesExistingPeriodFile() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        File(context.noBackupFilesDir, "bills_android").deleteRecursively()
        val services = createAndroidServiceBundle(context)

        val environment = services.workspaceService.initializeEnvironment()
        val firstDir = createSourceDirectory(context, "overwrite_first")
        val secondDir = createSourceDirectory(context, "overwrite_second")
        File(firstDir, "first.txt").writeText(
            sampleRecordText(environment).replaceFirst("date:2025-01", "date:2026-05"),
            Charsets.UTF_8,
        )
        File(secondDir, "second.txt").writeText(
            sampleRecordText(environment)
                .replaceFirst("date:2025-01", "date:2026-05")
                .replace("西瓜", "草莓"),
            Charsets.UTF_8,
        )

        services.workspaceService.importTxtDirectoryToRecords(Uri.fromFile(firstDir))
        val overwritten = services.workspaceService.importTxtDirectoryToRecords(Uri.fromFile(secondDir))
        val importedFile = File(environment.recordsRoot, "2026/2026-05.txt")

        assertEquals(1, overwritten.imported)
        assertEquals(1, overwritten.overwritten)
        assertEquals(0, overwritten.failure)
        assertTrue(importedFile.readText(Charsets.UTF_8).contains("草莓"))
    }

    @Test
    fun importTxtDirectoryToRecordsSkipsInvalidFilesAndKeepsValidOnes() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        File(context.noBackupFilesDir, "bills_android").deleteRecursively()
        val services = createAndroidServiceBundle(context)

        val environment = services.workspaceService.initializeEnvironment()
        val sourceDir = createSourceDirectory(context, "mixed_validity")
        File(sourceDir, "valid.txt").writeText(
            sampleRecordText(environment).replaceFirst("date:2025-01", "date:2026-06"),
            Charsets.UTF_8,
        )
        File(sourceDir, "invalid.txt").writeText("oops", Charsets.UTF_8)

        val imported = services.workspaceService.importTxtDirectoryToRecords(Uri.fromFile(sourceDir))

        assertEquals(2, imported.processed)
        assertEquals(1, imported.imported)
        assertEquals(1, imported.failure)
        assertEquals(1, imported.invalid)
        assertEquals(0, imported.duplicatePeriodConflicts)
        assertTrue(File(environment.recordsRoot, "2026/2026-06.txt").isFile)
        assertFalse(File(environment.recordsRoot, "2026/2026-07.txt").exists())
    }

    @Test
    fun importTxtDirectoryToRecordsRejectsDuplicatePeriodsWithinSelection() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        File(context.noBackupFilesDir, "bills_android").deleteRecursively()
        val services = createAndroidServiceBundle(context)

        val environment = services.workspaceService.initializeEnvironment()
        val sourceDir = createSourceDirectory(context, "duplicate_periods")
        val duplicateText = sampleRecordText(environment).replaceFirst("date:2025-01", "date:2026-08")
        File(sourceDir, "one.txt").writeText(duplicateText, Charsets.UTF_8)
        File(sourceDir, "two.txt").writeText(
            duplicateText.replace("蜜雪冰城", "霸王茶姬"),
            Charsets.UTF_8,
        )

        val imported = services.workspaceService.importTxtDirectoryToRecords(Uri.fromFile(sourceDir))

        assertEquals(2, imported.processed)
        assertEquals(0, imported.imported)
        assertEquals(2, imported.failure)
        assertEquals(0, imported.invalid)
        assertEquals(2, imported.duplicatePeriodConflicts)
        assertFalse(File(environment.recordsRoot, "2026/2026-08.txt").exists())
    }
}

private fun createSourceDirectory(context: Context, name: String): File =
    File(context.cacheDir, "workspace_service_test/$name").apply {
        deleteRecursively()
        mkdirs()
    }

private fun sampleRecordText(environment: AppEnvironment): String =
    File(environment.recordsRoot, "2025/2025-01.txt").readText(Charsets.UTF_8)
