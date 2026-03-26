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

val bundledSampleRelativePath = "2025"
val bundledSampleLabel = "2025 full-year sample"
val bundledSampleYear = "2025"
val bundledSampleMonth = "2025-01"
val androidPresentationVersionCode = 2
val androidPresentationVersionName = "0.1.2"

private object AndroidUiDependencyVersions {
    const val composeBom = "2025.08.01"
    const val activityCompose = "1.10.1"
    const val documentFile = "1.1.0"
    const val lifecycle = "2.10.0"
}

val generatedCommonAssetsDir = layout.buildDirectory.dir("generated/assets/common")
val generatedCommonAssetsPath = generatedCommonAssetsDir.get().asFile
val generatedDebugSampleAssetsDir = layout.buildDirectory.dir("generated/assets/debugSample")
val generatedDebugSampleAssetsPath = generatedDebugSampleAssetsDir.get().asFile
val distributedAndroidConfigDir = rootProject.layout.projectDirectory.dir("dist/config/android")
val distributedAndroidNoticesDir = rootProject.layout.projectDirectory.dir("dist/notices/android")
val noticesMetadataDir = layout.buildDirectory.dir("generated/noticesMetadata")
val releaseRuntimeArtifactsFile = noticesMetadataDir.map { it.file("release-runtime-artifacts.json") }
val debugRuntimeArtifactsFile = noticesMetadataDir.map { it.file("debug-runtime-artifacts.json") }
val releaseSigningStoreFile = providers.gradleProperty("BILLS_ANDROID_RELEASE_STORE_FILE").orNull
val releaseSigningStorePassword = providers.gradleProperty("BILLS_ANDROID_RELEASE_STORE_PASSWORD").orNull
val releaseSigningKeyAlias = providers.gradleProperty("BILLS_ANDROID_RELEASE_KEY_ALIAS").orNull
val releaseSigningKeyPassword = providers.gradleProperty("BILLS_ANDROID_RELEASE_KEY_PASSWORD").orNull
val localNlohmannJsonSourceDir = providers.gradleProperty("BILLS_ANDROID_NLOHMANN_JSON_SOURCE_DIR").orNull
val localTomlplusplusSourceDir = providers.gradleProperty("BILLS_ANDROID_TOMLPLUSPLUS_SOURCE_DIR").orNull
val localSqliteAmalgamationSourceDir = providers.gradleProperty("BILLS_ANDROID_SQLITE_AMALGAMATION_SOURCE_DIR").orNull
val hasExplicitReleaseSigning =
    !releaseSigningStoreFile.isNullOrBlank() &&
        !releaseSigningStorePassword.isNullOrBlank() &&
        !releaseSigningKeyAlias.isNullOrBlank() &&
        !releaseSigningKeyPassword.isNullOrBlank()

fun cmakePathOrNull(rawPath: String?): String? =
    rawPath
        ?.takeIf { it.isNotBlank() }
        ?.let { File(it).absolutePath.replace('\\', '/') }

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
        minSdk = 35
        targetSdk = 36
        versionCode = androidPresentationVersionCode
        versionName = androidPresentationVersionName
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        buildConfigField("String", "PRESENTATION_VERSION_NAME", "\"$androidPresentationVersionName\"")
        buildConfigField("int", "PRESENTATION_VERSION_CODE", androidPresentationVersionCode.toString())

        ndk {
            abiFilters += listOf("arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DBILLS_ENABLE_MODULES=OFF",
                    "-DBILLS_CORE_BUILD_SHARED=OFF",
                    "-DBILLS_ANDROID_ENABLE_BUNDLED_SAMPLE=ON",
                )
                cmakePathOrNull(localNlohmannJsonSourceDir)?.let { sourceDir ->
                    arguments += "-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=$sourceDir"
                }
                cmakePathOrNull(localTomlplusplusSourceDir)?.let { sourceDir ->
                    arguments += "-DFETCHCONTENT_SOURCE_DIR_TOMLPLUSPLUS=$sourceDir"
                }
                cmakePathOrNull(localSqliteAmalgamationSourceDir)?.let { sourceDir ->
                    arguments += "-DFETCHCONTENT_SOURCE_DIR_SQLITE_AMALGAMATION=$sourceDir"
                }
            }
        }
    }

    buildFeatures {
        compose = true
        buildConfig = true
    }

    signingConfigs {
        create("release") {
            if (hasExplicitReleaseSigning) {
                storeFile = rootProject.file(requireNotNull(releaseSigningStoreFile))
                storePassword = requireNotNull(releaseSigningStorePassword)
                keyAlias = requireNotNull(releaseSigningKeyAlias)
                keyPassword = requireNotNull(releaseSigningKeyPassword)
            }
        }
    }

    buildTypes {
        getByName("debug") {
            isDebuggable = true
        }
        getByName("release") {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
            signingConfig = if (hasExplicitReleaseSigning) {
                signingConfigs.getByName("release")
            } else {
                signingConfigs.getByName("debug")
            }
            externalNativeBuild {
                cmake {
                    arguments += "-DBILLS_ANDROID_ENABLE_BUNDLED_SAMPLE=OFF"
                }
            }
        }
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
    sourceSets["main"].assets.srcDir(generatedCommonAssetsPath)
    @Suppress("DEPRECATION")
    sourceSets["debug"].assets.srcDir(generatedDebugSampleAssetsPath)

    testOptions {
        unitTests.isReturnDefaultValues = true
    }
}

dependencies {
    val composeBom = enforcedPlatform(
        "androidx.compose:compose-bom:${AndroidUiDependencyVersions.composeBom}",
    )

    implementation(composeBom)
    debugImplementation(composeBom)
    androidTestImplementation(composeBom)

    implementation("androidx.activity:activity-compose:${AndroidUiDependencyVersions.activityCompose}")
    implementation("androidx.documentfile:documentfile:${AndroidUiDependencyVersions.documentFile}")
    implementation("androidx.lifecycle:lifecycle-runtime-compose:${AndroidUiDependencyVersions.lifecycle}")
    implementation("androidx.lifecycle:lifecycle-viewmodel-ktx:${AndroidUiDependencyVersions.lifecycle}")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:${AndroidUiDependencyVersions.lifecycle}")
    implementation("androidx.datastore:datastore-preferences:1.2.0")
    implementation("androidx.compose.foundation:foundation")
    implementation("androidx.compose.foundation:foundation-layout")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.runtime:runtime")
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-tooling-preview")
    implementation("com.google.android.material:material:1.12.0")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.10.2")
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.10.0")

    debugImplementation("androidx.compose.ui:ui-tooling")
    debugImplementation("androidx.compose.ui:ui-test-manifest")

    testImplementation("junit:junit:4.13.2")
    testImplementation("org.mockito:mockito-core:5.14.2")
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
        rootProject.layout.projectDirectory.file("libs/core/notices.toml"),
        rootProject.layout.projectDirectory.file("libs/io/notices.toml"),
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

val syncBundledCommonAssets by tasks.registering(Sync::class) {
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
    into(generatedCommonAssetsPath)
}

val syncBundledDebugSampleAssets by tasks.registering(Sync::class) {
    from(rootProject.layout.projectDirectory.dir("testdata/bills/$bundledSampleRelativePath")) {
        into("testdata/bills/$bundledSampleRelativePath")
    }
    into(generatedDebugSampleAssetsPath)
}

tasks.named("preBuild").configure {
    dependsOn(syncBundledCommonAssets)
}

tasks.configureEach {
    if (name == "preDebugBuild") {
        dependsOn(syncBundledDebugSampleAssets)
    }
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
