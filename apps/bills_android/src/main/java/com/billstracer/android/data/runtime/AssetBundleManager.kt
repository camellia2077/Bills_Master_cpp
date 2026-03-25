package com.billstracer.android.data.runtime

import android.content.Context
import java.io.File
import java.nio.charset.StandardCharsets

internal class AssetBundleManager(
    private val context: Context,
) {
    private companion object {
        val bundledConfigFiles = listOf(
            "validator_config.toml",
            "modifier_config.toml",
            "export_formats.toml",
        )
    }

    fun materializeWorkspace(): AppWorkspacePaths {
        val workspaceRoot = File(context.noBackupFilesDir, "bills_android")
        val assetsRoot = File(workspaceRoot, "bundled_assets")
        val configRoot = File(workspaceRoot, "runtime_config")
        val recordsRoot = File(workspaceRoot, "records")
        val dbRoot = File(workspaceRoot, "db")
        val dbFile = File(dbRoot, "bills.sqlite3")
        val bundledSampleSpec = BundledSampleSpecProvider.current()

        assetsRoot.deleteRecursively()
        dbRoot.mkdirs()
        recordsRoot.mkdirs()
        extractAssetTree("notices", File(assetsRoot, "notices"))
        bundledSampleSpec?.let { sample ->
            materializeBundledSampleRecords(
                workspaceRoot = workspaceRoot,
                sample = sample,
                recordsRoot = recordsRoot,
            )
        }
        materializePersistentConfig(configRoot)

        return AppWorkspacePaths(
            bundledSampleInputPath = bundledSampleSpec?.let { sample ->
                File(
                    recordsRoot,
                    sample.relativeInputPath,
                )
            },
            bundledSampleSpec = bundledSampleSpec,
            configRoot = configRoot,
            recordsRoot = recordsRoot,
            noticesRoot = File(assetsRoot, "notices"),
            dbFile = dbFile,
        )
    }

    fun clearDatabaseFiles(dbFile: File): Boolean {
        val walFile = File("${dbFile.absolutePath}-wal")
        val shmFile = File("${dbFile.absolutePath}-shm")
        val existed = dbFile.exists() || walFile.exists() || shmFile.exists()
        dbFile.delete()
        walFile.delete()
        shmFile.delete()
        return existed
    }

    private fun materializeBundledSampleRecords(
        workspaceRoot: File,
        sample: BundledSampleSpec,
        recordsRoot: File,
    ) {
        val seedMarker = File(
            workspaceRoot,
            ".bundled_sample_seeded_${sample.relativeInputPath.replace('/', '_').replace('\\', '_')}",
        )
        if (seedMarker.exists()) {
            return
        }

        copyAssetTreeIfMissing(
            assetPath = "testdata/bills/${sample.relativeInputPath}",
            destination = File(recordsRoot, sample.relativeInputPath),
        )
        seedMarker.writeText(sample.relativeInputPath, StandardCharsets.UTF_8)
    }

    private fun extractAssetTree(assetPath: String, destination: File) {
        val entries = context.assets.list(assetPath).orEmpty()
        if (entries.isEmpty()) {
            destination.parentFile?.mkdirs()
            context.assets.open(assetPath).use { input ->
                destination.outputStream().use { output ->
                    input.copyTo(output)
                }
            }
            return
        }

        destination.mkdirs()
        for (entry in entries) {
            extractAssetTree("$assetPath/$entry", File(destination, entry))
        }
    }

    private fun copyAssetTreeIfMissing(assetPath: String, destination: File) {
        val entries = context.assets.list(assetPath).orEmpty()
        if (entries.isEmpty()) {
            if (destination.exists()) {
                return
            }
            destination.parentFile?.mkdirs()
            context.assets.open(assetPath).use { input ->
                destination.outputStream().use { output ->
                    input.copyTo(output)
                }
            }
            return
        }

        destination.mkdirs()
        for (entry in entries) {
            copyAssetTreeIfMissing("$assetPath/$entry", File(destination, entry))
        }
    }

    private fun materializePersistentConfig(configRoot: File) {
        configRoot.mkdirs()
        for (fileName in bundledConfigFiles) {
            copyAssetFileIfMissing(
                assetPath = "config/$fileName",
                destination = File(configRoot, fileName),
            )
        }
    }

    private fun copyAssetFileIfMissing(assetPath: String, destination: File) {
        if (destination.exists()) {
            return
        }
        destination.parentFile?.mkdirs()
        context.assets.open(assetPath).use { input ->
            destination.outputStream().use { output ->
                input.copyTo(output)
            }
        }
    }
}

internal data class AppWorkspacePaths(
    val bundledSampleInputPath: File?,
    val bundledSampleSpec: BundledSampleSpec?,
    val configRoot: File,
    val recordsRoot: File,
    val noticesRoot: File,
    val dbFile: File,
)
