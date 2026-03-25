package com.billstracer.android.data.services

import com.billstracer.android.data.nativebridge.BundledSampleNativeBindings
import com.billstracer.android.model.AppEnvironment

internal object BundledSampleImportSupport {
    fun importBundledSample(environment: AppEnvironment): String {
        val sampleInputPath = environment.bundledSampleInputPath
            ?: error("Bundled sample is unavailable in this build.")
        return BundledSampleNativeBindings.importBundledSampleNative(
            sampleInputPath.absolutePath,
            environment.configRoot.absolutePath,
            environment.dbFile.absolutePath,
        )
    }
}
