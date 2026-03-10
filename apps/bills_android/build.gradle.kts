import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import groovy.json.JsonOutput
import org.gradle.api.DefaultTask
import org.gradle.api.artifacts.component.ModuleComponentIdentifier
import org.gradle.api.artifacts.result.ResolvedArtifactResult
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.provider.ListProperty
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.InputFiles
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.PathSensitive
import org.gradle.api.tasks.PathSensitivity
import org.gradle.api.tasks.TaskAction
import org.gradle.work.DisableCachingByDefault
import java.io.File

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.plugin.compose")
}

val bundledSampleRelativePath = "2025/2025-01.txt"
val bundledSampleLabel = "2025-01"
val bundledSampleYear = "2025"
val bundledSampleMonth = "2025-01"
val androidPresentationVersionCode = 1
val androidPresentationVersionName = "0.1.0"
val generatedAssetsDir = layout.buildDirectory.dir("generated/assets/main")
val generatedAssetsPath = generatedAssetsDir.get().asFile
val distributedAndroidConfigDir = rootProject.layout.projectDirectory.dir("dist/config/android")
val distributedAndroidNoticesDir = rootProject.layout.projectDirectory.dir("dist/notices/android")
val noticesMetadataDir = layout.buildDirectory.dir("generated/noticesMetadata")
val releaseRuntimeArtifactsFile = noticesMetadataDir.map { it.file("release-runtime-artifacts.json") }
val debugRuntimeArtifactsFile = noticesMetadataDir.map { it.file("debug-runtime-artifacts.json") }

@DisableCachingByDefault(because = "Writes resolved runtime artifact metadata to a generated JSON file.")
abstract class WriteResolvedRuntimeArtifactsTask : DefaultTask() {
    private data class RuntimeArtifactMetadata(
        val group: String,
        val name: String,
        val version: String,
        val fileName: String,
    ) {
        val coordinate: String
            get() = "$group:$name:$version"
    }

    companion object {
        private val runtimeArtifactFieldSeparator = "\t"

        private fun encodeRuntimeArtifactMetadata(metadata: RuntimeArtifactMetadata): String =
            listOf(
                metadata.group,
                metadata.name,
                metadata.version,
                metadata.fileName,
            ).joinToString(runtimeArtifactFieldSeparator)

        private fun decodeRuntimeArtifactMetadata(encoded: String): RuntimeArtifactMetadata {
            val fields = encoded.split(runtimeArtifactFieldSeparator, limit = 4)
            require(fields.size == 4) {
                "Invalid runtime artifact metadata entry: $encoded"
            }
            return RuntimeArtifactMetadata(
                group = fields[0],
                name = fields[1],
                version = fields[2],
                fileName = fields[3],
            )
        }

        private fun toRuntimeArtifactMetadata(artifact: ResolvedArtifactResult): RuntimeArtifactMetadata {
            val owner = artifact.variant.owner
            require(owner is ModuleComponentIdentifier) {
                "Unsupported runtime artifact owner '${owner.displayName}' for ${artifact.file.name}."
            }
            return RuntimeArtifactMetadata(
                group = owner.group,
                name = owner.module,
                version = owner.version,
                fileName = artifact.file.name,
            )
        }
    }

    @get:Input
    abstract val configurationName: Property<String>

    @get:Input
    abstract val artifactMetadata: ListProperty<String>

    @get:InputFiles
    @get:PathSensitive(PathSensitivity.NAME_ONLY)
    abstract val artifactFiles: ListProperty<File>

    @get:OutputFile
    abstract val outputFile: RegularFileProperty

    fun useResolvedArtifacts(artifacts: Provider<Set<ResolvedArtifactResult>>) {
        val metadataProvider = artifacts.map { resolvedArtifacts ->
            resolvedArtifacts.asSequence()
                .filterNot { artifact -> artifact.file.extension.equals("pom", ignoreCase = true) }
                .map { artifact -> toRuntimeArtifactMetadata(artifact) }
                .distinctBy { metadata -> metadata.coordinate }
                .sortedBy { metadata -> metadata.coordinate }
                .map { metadata -> encodeRuntimeArtifactMetadata(metadata) }
                .toList()
        }
        artifactMetadata.set(metadataProvider)
        artifactFiles.set(
            artifacts.map { resolvedArtifacts ->
                resolvedArtifacts.asSequence()
                    .filterNot { artifact -> artifact.file.extension.equals("pom", ignoreCase = true) }
                    .map { artifact -> artifact.file }
                    .distinct()
                    .toList()
            },
        )
    }

    @TaskAction
    fun writeArtifacts() {
        val artifacts = artifactMetadata.get()
            .map(::decodeRuntimeArtifactMetadata)
            .map { artifact ->
                mapOf(
                    "group" to artifact.group,
                    "name" to artifact.name,
                    "version" to artifact.version,
                    "fileName" to artifact.fileName,
                )
            }
            .toList()

        val targetFile = outputFile.get().asFile
        targetFile.parentFile.mkdirs()
        targetFile.writeText(
            JsonOutput.prettyPrint(
                JsonOutput.toJson(
                    mapOf(
                        "configuration" to configurationName.get(),
                        "artifacts" to artifacts,
                    ),
                ),
            ),
            Charsets.UTF_8,
        )
    }
}

val generateDistributedAndroidConfig by tasks.registering(Exec::class) {
    inputs.files(
        rootProject.layout.projectDirectory.file("config/validator_config.toml"),
        rootProject.layout.projectDirectory.file("config/modifier_config.toml"),
        rootProject.layout.projectDirectory.file("config/export_formats.toml"),
    )
    outputs.dir(distributedAndroidConfigDir)
    workingDir = rootProject.layout.projectDirectory.asFile
    commandLine(
        "python",
        "tools/flows/distribute_configs.py",
        "--targets",
        "android",
    )
}

android {
    namespace = "com.billstracer.android"
    compileSdk = 36
    ndkVersion = "29.0.14206865"

    defaultConfig {
        applicationId = "com.billstracer.android"
        minSdk = 26
        targetSdk = 36
        versionCode = androidPresentationVersionCode
        versionName = androidPresentationVersionName
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        buildConfigField("String", "BUNDLED_SAMPLE_RELATIVE_PATH", "\"$bundledSampleRelativePath\"")
        buildConfigField("String", "BUNDLED_SAMPLE_LABEL", "\"$bundledSampleLabel\"")
        buildConfigField("String", "BUNDLED_SAMPLE_YEAR", "\"$bundledSampleYear\"")
        buildConfigField("String", "BUNDLED_SAMPLE_MONTH", "\"$bundledSampleMonth\"")
        buildConfigField("String", "PRESENTATION_VERSION_NAME", "\"$androidPresentationVersionName\"")
        buildConfigField("int", "PRESENTATION_VERSION_CODE", androidPresentationVersionCode.toString())

        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DBILLS_ENABLE_MODULES=OFF",
                    "-DBILLS_CORE_BUILD_SHARED=OFF",
                )
            }
        }
    }

    buildFeatures {
        compose = true
        buildConfig = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }

    kotlin {
        compilerOptions {
            jvmTarget.set(JvmTarget.JVM_21)
        }
    }

    packaging {
        resources {
            excludes += "/META-INF/{AL2.0,LGPL2.1}"
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "4.1.2"
        }
    }

    @Suppress("DEPRECATION")
    sourceSets["main"].assets.srcDir(generatedAssetsPath)

    testOptions {
        unitTests.isReturnDefaultValues = true
    }
}

dependencies {
    val composeBom = platform("androidx.compose:compose-bom:2024.12.01")

    implementation(composeBom)
    androidTestImplementation(composeBom)

    implementation("androidx.activity:activity-compose:1.10.1")
    implementation("androidx.lifecycle:lifecycle-runtime-compose:2.10.0")
    implementation("androidx.lifecycle:lifecycle-viewmodel-ktx:2.10.0")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.10.0")
    implementation("androidx.datastore:datastore-preferences:1.2.0")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.material:material-icons-extended")
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-tooling-preview")
    implementation("com.google.android.material:material:1.12.0")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.10.2")
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.10.0")

    debugImplementation("androidx.compose.ui:ui-tooling")
    debugImplementation("androidx.compose.ui:ui-test-manifest")

    testImplementation("junit:junit:4.13.2")
    testImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.10.2")

    androidTestImplementation("androidx.test.ext:junit:1.2.1")
    androidTestImplementation("androidx.test:core-ktx:1.6.1")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.6.1")
    androidTestImplementation("androidx.compose.ui:ui-test-junit4")
}

val writeReleaseRuntimeArtifacts by tasks.registering(WriteResolvedRuntimeArtifactsTask::class) {
    configurationName.set("releaseRuntimeClasspath")
    outputFile.set(releaseRuntimeArtifactsFile)
}

val writeDebugRuntimeArtifacts by tasks.registering(WriteResolvedRuntimeArtifactsTask::class) {
    configurationName.set("debugRuntimeClasspath")
    outputFile.set(debugRuntimeArtifactsFile)
}

val generateBundledNotices by tasks.registering(Exec::class) {
    dependsOn(writeReleaseRuntimeArtifacts, writeDebugRuntimeArtifacts)
    inputs.dir(rootProject.layout.projectDirectory.dir("third_party"))
    inputs.files(
        rootProject.layout.projectDirectory.file("libs/bills_core/notices.toml"),
        rootProject.layout.projectDirectory.file("libs/bills_io/notices.toml"),
        rootProject.layout.projectDirectory.file("apps/bills_cli/notices.toml"),
        rootProject.layout.projectDirectory.file("apps/bills_android/notices.toml"),
        releaseRuntimeArtifactsFile,
        debugRuntimeArtifactsFile,
    )
    outputs.dir(distributedAndroidNoticesDir)
    workingDir = rootProject.layout.projectDirectory.asFile
    commandLine(
        "python",
        "tools/notices/generate_notices.py",
        "--targets",
        "android",
        "--android-release-artifacts",
        releaseRuntimeArtifactsFile.get().asFile.absolutePath,
        "--android-debug-artifacts",
        debugRuntimeArtifactsFile.get().asFile.absolutePath,
    )
}

val syncBundledAssets by tasks.registering(Sync::class) {
    from(rootProject.layout.projectDirectory.file("testdata/bills/$bundledSampleRelativePath")) {
        into("testdata/bills/2025")
    }
    dependsOn(generateDistributedAndroidConfig)
    from(distributedAndroidConfigDir) {
        include("validator_config.toml", "modifier_config.toml", "export_formats.toml")
        into("config")
    }
    dependsOn(generateBundledNotices)
    from(distributedAndroidNoticesDir) {
        include("NOTICE.md", "notices.json")
        into("notices")
    }
    into(generatedAssetsPath)
}

tasks.named("preBuild").configure {
    dependsOn(syncBundledAssets)
}

afterEvaluate {
    val releaseRuntimeArtifacts = configurations.named("releaseRuntimeClasspath")
        .flatMap { configuration -> configuration.incoming.artifacts.resolvedArtifacts }
    val debugRuntimeArtifacts = configurations.named("debugRuntimeClasspath")
        .flatMap { configuration -> configuration.incoming.artifacts.resolvedArtifacts }

    writeReleaseRuntimeArtifacts.configure {
        useResolvedArtifacts(releaseRuntimeArtifacts)
    }
    writeDebugRuntimeArtifacts.configure {
        useResolvedArtifacts(debugRuntimeArtifacts)
    }
}
