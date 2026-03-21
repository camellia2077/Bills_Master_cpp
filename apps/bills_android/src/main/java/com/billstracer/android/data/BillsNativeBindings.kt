package com.billstracer.android.data

internal object BillsNativeBindings {
    init {
        System.loadLibrary("bills_android_native")
    }

    external fun importBundledSampleNative(
        inputPath: String,
        configDir: String,
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

    external fun generateRecordTemplateNative(
        configDir: String,
        isoMonth: String,
    ): String

    external fun previewRecordPathNative(
        inputPath: String,
        configDir: String,
    ): String

    external fun validateConfigBundleNative(
        validatorConfigText: String,
        modifierConfigText: String,
        exportFormatsText: String,
        validatorDisplayPath: String,
        modifierDisplayPath: String,
        exportFormatsDisplayPath: String,
    ): String

    external fun listRecordPeriodsNative(
        inputPath: String,
    ): String

    external fun coreVersionNative(): String
}
