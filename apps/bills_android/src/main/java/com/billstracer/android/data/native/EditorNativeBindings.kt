package com.billstracer.android.data.nativebridge

internal object EditorNativeBindings {
    init {
        NativeLibrary.ensureLoaded()
    }

    external fun generateRecordTemplateJsonNative(
        configDir: String,
        isoMonth: String,
    ): String

    external fun parseRecordEditorDocumentJsonNative(
        rawText: String,
    ): String

    external fun serializeRecordEditorDocumentJsonNative(
        documentJson: String,
    ): String

    external fun previewRecordPathNative(
        inputPath: String,
        configDir: String,
    ): String

    external fun commitRecordDocumentJsonNative(
        expectedPeriod: String,
        rawText: String,
        configDir: String,
        recordsRoot: String,
        dbPath: String,
    ): String
}
