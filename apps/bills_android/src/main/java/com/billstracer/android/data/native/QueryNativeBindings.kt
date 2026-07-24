package com.billstracer.android.data.nativebridge

internal object QueryNativeBindings {
    init {
        NativeLibrary.ensureLoaded()
    }

    external fun listAvailablePeriodsNative(
        dbPath: String,
    ): String

    external fun queryYearNative(
        dbPath: String,
        isoYear: String,
    ): String

    external fun queryMonthNative(
        dbPath: String,
        isoMonth: String,
    ): String

    external fun queryRangeNative(
        dbPath: String,
        startIsoMonth: String,
        endIsoMonth: String,
    ): String
}
