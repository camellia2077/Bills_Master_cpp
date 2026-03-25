package com.billstracer.android.data.nativebridge

internal object EditorNativeBindings {
    init {
        NativeLibrary.ensureLoaded()
    }

    external fun generateRecordTemplateNative(
        configDir: String,
        isoMonth: String,
    ): String

    external fun listDatabaseRecordPeriodsNative(
        dbPath: String,
    ): String

    external fun previewRecordPathNative(
        inputPath: String,
        configDir: String,
    ): String

    external fun syncSavedRecordToDatabaseNative(
        inputPath: String,
        configDir: String,
        dbPath: String,
        expectedPeriod: String,
    ): String
}
