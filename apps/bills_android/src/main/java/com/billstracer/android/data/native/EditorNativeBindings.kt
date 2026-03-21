package com.billstracer.android.data.nativebridge

internal object EditorNativeBindings {
    init {
        NativeLibrary.ensureLoaded()
    }

    external fun generateRecordTemplateNative(
        configDir: String,
        isoMonth: String,
    ): String

    external fun previewRecordPathNative(
        inputPath: String,
        configDir: String,
    ): String

    external fun listRecordPeriodsNative(
        inputPath: String,
    ): String
}
