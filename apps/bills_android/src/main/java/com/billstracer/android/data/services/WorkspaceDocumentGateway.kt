package com.billstracer.android.data.services

import android.content.Context
import android.net.Uri
import androidx.documentfile.provider.DocumentFile
import java.io.File
import java.util.Locale

internal data class SourceTxtDocument(
    val relativePath: String,
    val rawText: String,
)

internal class WorkspaceDocumentGateway(
    context: Context,
) {
    private val applicationContext = context.applicationContext

    fun loadSourceTxtDocuments(sourceDirectoryUri: Uri): List<SourceTxtDocument> {
        val documents = if (sourceDirectoryUri.scheme == "file") {
            loadFileSchemeTxtDocuments(sourceDirectoryUri)
        } else {
            loadTreeSchemeTxtDocuments(sourceDirectoryUri)
        }
        return documents.sortedWith(
            compareBy<SourceTxtDocument>(
                { document -> document.relativePath.lowercase(Locale.ROOT) },
                { document -> document.relativePath },
            ),
        )
    }

    fun copyUriToFile(
        sourceUri: Uri,
        destinationFile: File,
        failureMessage: String,
    ) {
        if (sourceUri.scheme == "file") {
            val sourceFile = requireNotNull(sourceUri.path).let(::File)
            require(sourceFile.isFile) { failureMessage }
            sourceFile.inputStream().use { input ->
                destinationFile.outputStream().use { output -> input.copyTo(output) }
            }
            return
        }
        applicationContext.contentResolver.openInputStream(sourceUri)?.use { input ->
            destinationFile.outputStream().use { output -> input.copyTo(output) }
        } ?: error(failureMessage)
    }

    fun copyFileToUri(
        sourceFile: File,
        targetUri: Uri,
        failureMessage: String,
    ) {
        if (targetUri.scheme == "file") {
            val targetFile = requireNotNull(targetUri.path).let(::File)
            targetFile.parentFile?.mkdirs()
            sourceFile.inputStream().use { input ->
                targetFile.outputStream().use { output -> input.copyTo(output) }
            }
            return
        }
        applicationContext.contentResolver.openOutputStream(targetUri, "w")?.use { output ->
            sourceFile.inputStream().use { input -> input.copyTo(output) }
        } ?: error(failureMessage)
    }

    fun deleteSingleDocument(uri: Uri) {
        if (uri.scheme == "file") {
            uri.path?.let(::File)?.delete()
            return
        }
        DocumentFile.fromSingleUri(applicationContext, uri)?.delete()
    }

    fun displayPathForUri(uri: Uri, fallback: String): String =
        if (uri.scheme == "file") {
            uri.path?.let(::File)?.name?.takeIf { it.isNotBlank() } ?: fallback
        } else {
            DocumentFile.fromSingleUri(applicationContext, uri)
                ?.name
                ?.takeIf { it.isNotBlank() }
                ?: fallback
        }

    private fun loadFileSchemeTxtDocuments(sourceDirectoryUri: Uri): List<SourceTxtDocument> {
        val directoryPath = requireNotNull(sourceDirectoryUri.path) {
            "Failed to resolve the selected directory."
        }
        val rootDirectory = File(directoryPath)
        require(rootDirectory.isDirectory) { "Selected location is not a directory." }
        return rootDirectory.walkTopDown()
            .filter { file -> file.isFile && file.extension.equals("txt", ignoreCase = true) }
            .map { file ->
                SourceTxtDocument(
                    relativePath = file.relativeTo(rootDirectory).invariantSeparatorsPath,
                    rawText = file.readText(Charsets.UTF_8),
                )
            }
            .toList()
    }

    private fun loadTreeSchemeTxtDocuments(sourceDirectoryUri: Uri): List<SourceTxtDocument> {
        val rootDocument = DocumentFile.fromTreeUri(applicationContext, sourceDirectoryUri)
            ?: error("Failed to open the selected directory.")
        if (!rootDocument.isDirectory) {
            error("Selected location is not a directory.")
        }
        val documents = mutableListOf<SourceTxtDocument>()
        collectTreeTxtDocuments(rootDocument, emptyList(), documents)
        return documents
    }

    private fun collectTreeTxtDocuments(
        document: DocumentFile,
        pathSegments: List<String>,
        output: MutableList<SourceTxtDocument>,
    ) {
        document.listFiles().forEach { child ->
            val childName = child.name ?: return@forEach
            if (child.isDirectory) {
                collectTreeTxtDocuments(child, pathSegments + childName, output)
                return@forEach
            }
            if (!child.isFile || !childName.endsWith(".txt", ignoreCase = true)) {
                return@forEach
            }
            val relativePath = (pathSegments + childName).joinToString("/")
            output += SourceTxtDocument(
                relativePath = relativePath,
                rawText = readTextFromUri(child.uri, relativePath),
            )
        }
    }

    private fun readTextFromUri(uri: Uri, label: String): String =
        applicationContext.contentResolver.openInputStream(uri)?.use { input ->
            input.readBytes().toString(Charsets.UTF_8)
        } ?: error("Failed to open TXT file '$label'.")
}
