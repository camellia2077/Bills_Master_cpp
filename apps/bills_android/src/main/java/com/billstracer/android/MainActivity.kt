package com.billstracer.android

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.billstracer.android.data.BillsNativeRepository
import com.billstracer.android.ui.BillsApp
import com.billstracer.android.ui.BillsViewModel
import com.billstracer.android.ui.BillsViewModelFactory
import com.billstracer.android.ui.theme.BillsAndroidTheme

class MainActivity : ComponentActivity() {
    private val viewModel: BillsViewModel by viewModels {
        BillsViewModelFactory(BillsNativeRepository(applicationContext))
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            val uiState = viewModel.uiState.collectAsStateWithLifecycle()
            BillsAndroidTheme(themePreferences = uiState.value.themePreferences) {
                BillsApp(viewModel = viewModel)
            }
        }
    }
}
