package com.billstracer.android.data.nativebridge

internal object SettingsNativeBindings {
    init {
        NativeLibrary.ensureLoaded()
    }

    external fun coreVersionNative(): String
}
