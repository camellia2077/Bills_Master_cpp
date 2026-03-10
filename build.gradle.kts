plugins {
    id("com.android.application") version "9.0.0" apply false
    id("org.jetbrains.kotlin.plugin.compose") version "2.2.10" apply false
}

allprojects {
    if (tasks.findByName("prepareKotlinBuildScriptModel") == null) {
        tasks.register("prepareKotlinBuildScriptModel") {
            group = "ide"
            description = "Compatibility no-op for IDE sync requests."
        }
    }
    if (tasks.findByName("updateDaemonJvm") == null) {
        tasks.register("updateDaemonJvm") {
            group = "ide"
            description = "Compatibility no-op for IDE sync requests."
        }
    }
}

tasks.register<Delete>("clean") {
    delete(layout.buildDirectory)
    subprojects.forEach { project ->
        delete(project.layout.buildDirectory)
    }
    delete(layout.projectDirectory.dir("apps/bills_android/.cxx"))
}
