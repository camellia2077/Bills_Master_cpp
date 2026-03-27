package com.billstracer.android.data.services

import org.junit.Assert.assertEquals
import org.junit.Test
import java.nio.file.Files

class RecordWorkspacePathsTest {
    @Test
    fun recordFileForPeriodBuildsCanonicalRecordsLayout() {
        val tempDir = Files.createTempDirectory("record-workspace-paths-test").toFile()
        try {
            val recordsRoot = tempDir.resolve("records").apply { mkdirs() }
            assertEquals(
                recordsRoot.resolve("2026/2026-04.txt").absolutePath,
                recordFileForPeriod(recordsRoot, "2026-04").absolutePath,
            )
        } finally {
            tempDir.deleteRecursively()
        }
    }
}
