package com.billstracer.android

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.app.navigation.AppSessionViewModel
import com.billstracer.android.app.navigation.AppSessionViewModelFactory
import com.billstracer.android.app.navigation.BillsAndroidApp
import com.billstracer.android.app.navigation.WorkspaceDataChangeBus
import com.billstracer.android.app.theme.BillsAndroidTheme
import com.billstracer.android.data.prefs.ThemePreferenceStore
import com.billstracer.android.data.runtime.AndroidWorkspaceRuntime
import com.billstracer.android.data.services.DefaultEditorService
import com.billstracer.android.data.services.DefaultQueryService
import com.billstracer.android.data.services.DefaultSettingsService
import com.billstracer.android.data.services.DefaultWorkspaceService
import com.billstracer.android.data.services.SettingsDataSource
import com.billstracer.android.features.editor.EditorViewModel
import com.billstracer.android.features.editor.EditorViewModelFactory
import com.billstracer.android.features.query.QueryViewModel
import com.billstracer.android.features.query.QueryViewModelFactory
import com.billstracer.android.features.settings.SettingsViewModel
import com.billstracer.android.features.settings.SettingsViewModelFactory
import com.billstracer.android.features.workspace.WorkspaceViewModel
import com.billstracer.android.features.workspace.WorkspaceViewModelFactory

class MainActivity : ComponentActivity() {
    private val sessionBus by lazy { AppSessionBus() }
    private val workspaceDataChangeBus by lazy { WorkspaceDataChangeBus() }
    private val workspaceRuntime by lazy { AndroidWorkspaceRuntime(applicationContext) }
    private val themePreferenceStore by lazy { ThemePreferenceStore(applicationContext) }
    private val settingsDataSource by lazy {
        SettingsDataSource(
            runtime = workspaceRuntime,
            themePreferenceStore = themePreferenceStore,
        )
    }
    private val workspaceService by lazy {
        DefaultWorkspaceService(
            context = applicationContext,
            runtime = workspaceRuntime,
        )
    }
    private val queryService by lazy { DefaultQueryService(runtime = workspaceRuntime) }
    private val editorService by lazy { DefaultEditorService(runtime = workspaceRuntime) }
    private val settingsService by lazy { DefaultSettingsService(settingsDataSource) }

    private val sessionViewModel: AppSessionViewModel by viewModels {
        AppSessionViewModelFactory(
            workspaceService = workspaceService,
            settingsService = settingsService,
            sessionBus = sessionBus,
        )
    }
    private val workspaceViewModel: WorkspaceViewModel by viewModels {
        WorkspaceViewModelFactory(
            workspaceService = workspaceService,
            sessionBus = sessionBus,
            workspaceDataChangeBus = workspaceDataChangeBus,
        )
    }
    private val queryViewModel: QueryViewModel by viewModels {
        QueryViewModelFactory(
            workspaceService = workspaceService,
            queryService = queryService,
            sessionBus = sessionBus,
            workspaceDataChangeBus = workspaceDataChangeBus,
        )
    }
    private val editorViewModel: EditorViewModel by viewModels {
        EditorViewModelFactory(
            editorService = editorService,
            sessionBus = sessionBus,
            workspaceDataChangeBus = workspaceDataChangeBus,
        )
    }
    private val settingsViewModel: SettingsViewModel by viewModels {
        SettingsViewModelFactory(
            settingsService = settingsService,
            sessionBus = sessionBus,
        )
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            val sessionState = sessionViewModel.state.collectAsStateWithLifecycle()
            BillsAndroidTheme(themePreferences = sessionState.value.themePreferences) {
                BillsAndroidApp(
                    sessionViewModel = sessionViewModel,
                    workspaceViewModel = workspaceViewModel,
                    queryViewModel = queryViewModel,
                    editorViewModel = editorViewModel,
                    settingsViewModel = settingsViewModel,
                )
            }
        }
    }
}
