package com.billstracer.android.data.runtime

import android.content.Context
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext

internal class AndroidWorkspaceRuntime(
    context: Context,
) {
    private val assetBundleManager = AssetBundleManager(context.applicationContext)
    private val mutex = Mutex()

    @Volatile
    private var cachedPaths: AppWorkspacePaths? = null

    suspend fun initializeWorkspace(): AppWorkspacePaths = withContext(Dispatchers.IO) {
        mutex.withLock {
            cachedPaths ?: assetBundleManager.materializeWorkspace().also { paths ->
                cachedPaths = paths
            }
        }
    }

    suspend fun clearDatabase(): Boolean = withContext(Dispatchers.IO) {
        val workspace = initializeWorkspace()
        assetBundleManager.clearDatabaseFiles(workspace.dbFile)
    }
}
