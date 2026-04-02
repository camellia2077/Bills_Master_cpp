package com.billstracer.android.data.nativebridge

internal object SettingsNativeBindings {
    init {
        NativeLibrary.ensureLoaded()
    }

    external fun coreVersionNative(): String

    external fun validateConfigTextsNative(
        validatorText: String,
        modifierText: String,
        exportFormatsText: String,
    ): String
}
