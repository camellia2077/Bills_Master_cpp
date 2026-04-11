package com.billstracer.android

import com.billstracer.android.model.AppEnvironment
import java.io.File

internal fun fakeAppEnvironment(): AppEnvironment = AppEnvironment(
    configRoot = File("config"),
    recordsRoot = File("records"),
    dbFile = File("db.sqlite3"),
)
