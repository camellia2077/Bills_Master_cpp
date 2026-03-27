package com.billstracer.android.data.services

import java.io.File

internal class WorkspaceTempStorage(
    private val cacheDir: File,
) {
    fun createTempBundleFile(prefix: String): File {
        val tempRoot = File(cacheDir, "parse_bundle")
        tempRoot.mkdirs()
        return File.createTempFile(prefix, ".zip", tempRoot)
    }

    fun createTempRecordImportDir(): File {
        val tempRoot = File(cacheDir, "record_directory_import")
        tempRoot.mkdirs()
        return File.createTempFile("record_directory_", "", tempRoot).apply {
            delete()
            mkdirs()
        }
    }

    fun stageSourceTxtDocuments(
        tempRoot: File,
        sourceDocuments: List<SourceTxtDocument>,
    ) {
        sourceDocuments.forEach { document ->
            val tempFile = File(tempRoot, document.relativePath)
            tempFile.parentFile?.mkdirs()
            tempFile.writeText(document.rawText, Charsets.UTF_8)
        }
    }
}
