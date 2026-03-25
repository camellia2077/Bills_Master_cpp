package com.billstracer.android.data.services

import com.billstracer.android.model.AppEnvironment

internal object BundledSampleImportSupport {
    fun importBundledSample(environment: AppEnvironment): String {
        return """
            {
              "ok": false,
              "code": "business.unsupported",
              "message": "Bundled sample import is unavailable in release builds.",
              "data": {
                "processed": 0,
                "success": 0,
                "failure": 0,
                "imported": 0
              }
            }
        """.trimIndent()
    }
}
