package com.billstracer.android.data.nativebridge

internal object WorkspaceNativeBindings {
    init {
        NativeLibrary.ensureLoaded()
    }

    external fun importBundledSampleNative(
        inputPath: String,
        configDir: String,
        dbPath: String,
    ): String
}
