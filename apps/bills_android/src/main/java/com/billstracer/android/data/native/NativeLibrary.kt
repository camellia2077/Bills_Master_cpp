package com.billstracer.android.data.nativebridge

internal object NativeLibrary {
    init {
        System.loadLibrary("bills_android_native")
    }

    fun ensureLoaded() = Unit
}
