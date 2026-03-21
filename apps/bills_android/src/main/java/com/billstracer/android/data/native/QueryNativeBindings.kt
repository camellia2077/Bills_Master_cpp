package com.billstracer.android.data.nativebridge

internal object QueryNativeBindings {
    init {
        NativeLibrary.ensureLoaded()
    }

    external fun queryYearNative(
        dbPath: String,
        isoYear: String,
    ): String

    external fun queryMonthNative(
        dbPath: String,
        isoMonth: String,
    ): String
}
