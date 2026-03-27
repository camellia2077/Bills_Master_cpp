package com.billstracer.android.app.navigation

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update

class WorkspaceDataChangeBus {
    private val mutableVersion = MutableStateFlow(0)
    val version: StateFlow<Int> = mutableVersion.asStateFlow()

    fun notifyChanged() {
        mutableVersion.update { current -> current + 1 }
    }
}
