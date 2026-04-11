package com.billstracer.android

import android.net.Uri
import com.billstracer.android.data.services.BackupService
import com.billstracer.android.data.services.WorkspaceService
import com.billstracer.android.model.ExportedBackupBundleResult
import com.billstracer.android.model.ExportedParseBundleResult
import com.billstracer.android.model.ImportedBackupBundleResult
import com.billstracer.android.model.ImportedParseBundleResult
import com.billstracer.android.model.RecordDirectoryImportResult

internal class FakeWorkspaceService : WorkspaceService {
    var environment = fakeAppEnvironment()
    var exportedResult = ExportedParseBundleResult(
        exportedRecordFiles = 1,
        exportedConfigFiles = 3,
        destinationDisplayPath = "parse_bundle.zip",
        rawJson = """{"ok":true}""",
    )
    var importedBundleResult = ImportedParseBundleResult(
        ok = true,
        code = "ok",
        message = "Parse bundle import finished.",
        importedRecordFiles = 1,
        importedConfigFiles = 3,
        importedBills = 1,
        sourceDisplayPath = "parse_bundle.zip",
        rawJson = """{"ok":true}""",
    )
    var recordDirectoryImportResult = RecordDirectoryImportResult(
        processed = 2,
        imported = 2,
        overwritten = 0,
        failure = 0,
        invalid = 0,
        duplicatePeriodConflicts = 0,
    )
    var clearedRecordFiles = 1
    var clearedDatabase = true

    override suspend fun initializeEnvironment() = environment

    override suspend fun importTxtDirectoryAndSyncDatabase(sourceDirectoryUri: Uri): RecordDirectoryImportResult =
        recordDirectoryImportResult

    override suspend fun exportParseBundle(targetDocumentUri: Uri): ExportedParseBundleResult =
        exportedResult

    override suspend fun importParseBundle(sourceDocumentUri: Uri): ImportedParseBundleResult =
        importedBundleResult

    override suspend fun clearRecordFiles(): Int = clearedRecordFiles

    override suspend fun clearDatabase(): Boolean = clearedDatabase
}

internal class FakeBackupService : BackupService {
    var exportedResult = ExportedBackupBundleResult(
        exportedRecordFiles = 1,
        exportedConfigFiles = 3,
        destinationDisplayPath = "bills_backup.zip",
        rawJson = """{"ok":true}""",
    )
    var importedResult = ImportedBackupBundleResult(
        ok = true,
        code = "ok",
        message = "Backup bundle restore finished.",
        restoredRecordFiles = 1,
        restoredConfigFiles = 3,
        restoredBills = 1,
        firstErrorMessage = null,
        sourceDisplayPath = "bills_backup.zip",
        rawJson = """{"ok":true}""",
    )

    override suspend fun exportBackupBundle(targetDocumentUri: Uri): ExportedBackupBundleResult =
        exportedResult

    override suspend fun importBackupBundle(sourceDocumentUri: Uri): ImportedBackupBundleResult =
        importedResult
}
