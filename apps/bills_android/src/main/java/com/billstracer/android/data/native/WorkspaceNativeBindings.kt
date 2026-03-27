package com.billstracer.android.data.nativebridge

internal object WorkspaceNativeBindings {
    init {
        NativeLibrary.ensureLoaded()
    }

    external fun importRecordsToDatabaseNative(
        configDir: String,
        recordsDir: String,
        dbPath: String,
    ): String

    external fun importTxtDirectoryAndSyncDatabaseNative(
        sourceRecordsDir: String,
        configDir: String,
        recordsRoot: String,
        dbPath: String,
    ): String

    external fun exportParseBundleNative(
        configDir: String,
        recordsDir: String,
        outputZipPath: String,
    ): String

    external fun importParseBundleNative(
        bundleZipPath: String,
        configDir: String,
        recordsDir: String,
        dbPath: String,
    ): String
}
