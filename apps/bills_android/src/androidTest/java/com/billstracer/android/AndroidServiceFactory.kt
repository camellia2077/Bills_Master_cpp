package com.billstracer.android

import android.content.Context
import com.billstracer.android.data.prefs.ThemePreferenceStore
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.data.services.DefaultEditorService
import com.billstracer.android.data.services.DefaultQueryService
import com.billstracer.android.data.services.DefaultSettingsService
import com.billstracer.android.data.services.DefaultWorkspaceService
import com.billstracer.android.data.services.EditorService
import com.billstracer.android.data.services.QueryService
import com.billstracer.android.data.services.SettingsDataSource
import com.billstracer.android.data.services.SettingsService
import com.billstracer.android.data.services.WorkspaceService

internal data class AndroidServiceBundle(
    val workspaceService: WorkspaceService,
    val queryService: QueryService,
    val editorService: EditorService,
    val settingsService: SettingsService,
)

internal fun createAndroidServiceBundle(context: Context): AndroidServiceBundle {
    val runtime = AndroidWorkspaceRuntime(context)
    val settingsDataSource = SettingsDataSource(runtime, ThemePreferenceStore(context))
    return AndroidServiceBundle(
        workspaceService = DefaultWorkspaceService(context, runtime),
        queryService = DefaultQueryService(runtime),
        editorService = DefaultEditorService(runtime),
        settingsService = DefaultSettingsService(settingsDataSource),
    )
}
