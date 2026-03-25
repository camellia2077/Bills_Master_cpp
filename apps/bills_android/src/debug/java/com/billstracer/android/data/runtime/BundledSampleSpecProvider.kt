package com.billstracer.android.data.runtime

internal object BundledSampleSpecProvider {
    fun current(): BundledSampleSpec? = BundledSampleSpec(
        relativeInputPath = "2025",
        label = "2025 full-year sample",
        year = "2025",
        month = "2025-01",
    )
}
