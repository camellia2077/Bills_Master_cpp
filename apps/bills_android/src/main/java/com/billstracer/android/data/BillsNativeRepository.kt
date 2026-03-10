package com.billstracer.android.data

import android.content.Context
import com.billstracer.android.BuildConfig
import com.billstracer.android.model.AppEnvironment
import com.billstracer.android.model.BundledConfigFile
import com.billstracer.android.model.BundledNotices
import com.billstracer.android.model.ImportResult
import com.billstracer.android.model.InvalidRecordFile
import com.billstracer.android.model.ListedRecordPeriodsResult
import com.billstracer.android.model.MonthlySummaryItem
import com.billstracer.android.model.QueryResult
import com.billstracer.android.model.QueryType
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewFile
import com.billstracer.android.model.RecordPreviewResult
import com.billstracer.android.model.ThemePreferences
import com.billstracer.android.model.VersionInfo
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.booleanOrNull
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.doubleOrNull
import kotlinx.serialization.json.intOrNull
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import java.io.File

class BillsNativeRepository(
    context: Context,
) : BillsRepository {
    private companion object {
        val bundledConfigOrder = listOf(
            "validator_config.toml",
            "modifier_config.toml",
            "export_formats.toml",
        )
    }

    private val applicationContext = context.applicationContext
    private val assetBundleManager = AssetBundleManager(applicationContext)
    private val themePreferenceStore = ThemePreferenceStore(applicationContext)
    private val mutex = Mutex()
    private val json = Json { ignoreUnknownKeys = true }

    @Volatile
    private var environment: AppEnvironment? = null

    override suspend fun initialize(): AppEnvironment = mutex.withLock {
        environment ?: prepareEnvironment().also { environment = it }
    }

    override suspend fun updateBundledConfig(
        fileName: String,
        rawText: String,
    ): List<BundledConfigFile> = withContext(Dispatchers.IO) {
        mutex.withLock {
            require(fileName in bundledConfigOrder) {
                "Unsupported bundled config file: $fileName"
            }
            val currentEnvironment = environment ?: prepareEnvironment().also { environment = it }
            File(currentEnvironment.configRoot, fileName).writeText(rawText, Charsets.UTF_8)
            val updatedConfigs = loadBundledConfigs(currentEnvironment.configRoot)
            environment = currentEnvironment.copy(bundledConfigs = updatedConfigs)
            updatedConfigs
        }
    }

    override suspend fun updateThemePreferences(
        preferences: ThemePreferences,
    ): ThemePreferences = withContext(Dispatchers.IO) {
        mutex.withLock {
            val persistedPreferences = themePreferenceStore.save(preferences)
            environment = environment?.copy(themePreferences = persistedPreferences)
            persistedPreferences
        }
    }

    override suspend fun importBundledSample(): ImportResult = withContext(Dispatchers.IO) {
        val env = initialize()
        parseImportResult(
            BillsNativeBindings.importBundledSampleNative(
                env.bundledSampleFile.absolutePath,
                env.configRoot.absolutePath,
                env.dbFile.absolutePath,
            ),
        )
    }

    override suspend fun queryBundledYear(): QueryResult = withContext(Dispatchers.IO) {
        val env = initialize()
        parseQueryResult(
            rawJson = BillsNativeBindings.queryYearNative(
                env.dbFile.absolutePath,
                env.bundledSampleYear,
            ),
            type = QueryType.YEAR,
        )
    }

    override suspend fun queryBundledMonth(): QueryResult = withContext(Dispatchers.IO) {
        val env = initialize()
        parseQueryResult(
            rawJson = BillsNativeBindings.queryMonthNative(
                env.dbFile.absolutePath,
                env.bundledSampleMonth,
            ),
            type = QueryType.MONTH,
        )
    }

    override suspend fun openRecordPeriod(period: String): RecordEditorDocument = withContext(Dispatchers.IO) {
        val env = initialize()
        val persistedRecordFile = recordFileForPeriod(env.recordsRoot, period)
        if (persistedRecordFile.isFile) {
            return@withContext RecordEditorDocument(
                period = period,
                relativePath = persistedRecordFile.relativeTo(env.recordsRoot).invariantSeparatorsPath,
                rawText = persistedRecordFile.readText(Charsets.UTF_8),
                persisted = true,
            )
        }

        parseRecordEditorDocument(
            BillsNativeBindings.generateRecordTemplateNative(
                env.configRoot.absolutePath,
                period,
            ),
        )
    }

    override suspend fun saveRecordDocument(
        period: String,
        rawText: String,
    ): RecordEditorDocument = withContext(Dispatchers.IO) {
        val env = initialize()
        val targetFile = recordFileForPeriod(env.recordsRoot, period)
        targetFile.parentFile?.mkdirs()
        targetFile.writeText(rawText, Charsets.UTF_8)
        RecordEditorDocument(
            period = period,
            relativePath = targetFile.relativeTo(env.recordsRoot).invariantSeparatorsPath,
            rawText = rawText,
            persisted = true,
        )
    }

    override suspend fun previewRecordDocument(
        period: String,
        rawText: String,
    ): RecordPreviewResult = withContext(Dispatchers.IO) {
        val env = initialize()
        val previewDir = File(env.recordsRoot, ".preview")
        previewDir.mkdirs()
        val previewFile = File.createTempFile(
            "record-preview-${period.replace("-", "_")}-",
            ".txt",
            previewDir,
        )
        try {
            previewFile.writeText(rawText, Charsets.UTF_8)
            parseRecordPreviewResult(
                BillsNativeBindings.previewRecordPathNative(
                    previewFile.absolutePath,
                    env.configRoot.absolutePath,
                ),
            )
        } finally {
            previewFile.delete()
        }
    }

    override suspend fun listRecordPeriods(): ListedRecordPeriodsResult = withContext(Dispatchers.IO) {
        val env = initialize()
        parseListedRecordPeriodsResult(
            BillsNativeBindings.listRecordPeriodsNative(env.recordsRoot.absolutePath),
        )
    }

    override suspend fun clearDatabase(): Boolean = withContext(Dispatchers.IO) {
        val env = initialize()
        assetBundleManager.clearDatabaseFiles(env.dbFile)
    }

    private suspend fun prepareEnvironment(): AppEnvironment {
        val workspace = assetBundleManager.materializeWorkspace()
        return AppEnvironment(
            bundledSampleFile = workspace.bundledSampleFile,
            bundledSampleLabel = BuildConfig.BUNDLED_SAMPLE_LABEL,
            bundledSampleYear = BuildConfig.BUNDLED_SAMPLE_YEAR,
            bundledSampleMonth = BuildConfig.BUNDLED_SAMPLE_MONTH,
            configRoot = workspace.configRoot,
            recordsRoot = workspace.recordsRoot,
            dbFile = workspace.dbFile,
            coreVersion = loadCoreVersion(),
            androidVersion = VersionInfo(
                versionName = BuildConfig.PRESENTATION_VERSION_NAME,
                versionCode = BuildConfig.PRESENTATION_VERSION_CODE,
            ),
            bundledConfigs = loadBundledConfigs(workspace.configRoot),
            themePreferences = themePreferenceStore.load(),
            bundledNotices = loadBundledNotices(workspace.noticesRoot),
        )
    }

    private fun loadBundledConfigs(configRoot: File): List<BundledConfigFile> {
        return bundledConfigOrder.map { fileName ->
            val configFile = File(configRoot, fileName)
            BundledConfigFile(
                fileName = fileName,
                rawText = configFile.readText(Charsets.UTF_8),
            )
        }
    }

    private fun loadBundledNotices(noticesRoot: File): BundledNotices = BundledNotices(
        markdownText = File(noticesRoot, "NOTICE.md").readText(Charsets.UTF_8),
        rawJson = File(noticesRoot, "notices.json").readText(Charsets.UTF_8),
    )

    private fun loadCoreVersion(): VersionInfo {
        val root = json.parseToJsonElement(BillsNativeBindings.coreVersionNative()).jsonObject
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        if (!root.boolean("ok")) {
            error(root.string("message").ifBlank { "Failed to load core version." })
        }
        return VersionInfo(
            versionName = data.string("version_name"),
            lastUpdated = data["last_updated"]?.jsonPrimitive?.contentOrNull,
        )
    }

    private fun parseImportResult(rawJson: String): ImportResult {
        val root = json.parseToJsonElement(rawJson).jsonObject
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        return ImportResult(
            ok = root.boolean("ok"),
            code = root.string("code"),
            message = root.string("message"),
            processed = data.int("processed"),
            success = data.int("success"),
            failure = data.int("failure"),
            imported = data.int("imported"),
            rawJson = rawJson,
        )
    }

    private fun parseQueryResult(rawJson: String, type: QueryType): QueryResult {
        val root = json.parseToJsonElement(rawJson).jsonObject
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        val monthlySummary = data["monthly_summary"]?.jsonArray?.mapNotNull { item ->
            val entry = item.jsonObject
            val month = entry["month"]?.jsonPrimitive?.intOrNull ?: return@mapNotNull null
            MonthlySummaryItem(
                month = month,
                income = entry.double("income"),
                expense = entry.double("expense"),
                balance = entry.double("balance"),
            )
        }.orEmpty()

        return QueryResult(
            ok = root.boolean("ok"),
            message = root.string("message"),
            type = type,
            year = data["year"]?.jsonPrimitive?.intOrNull,
            month = data["month"]?.jsonPrimitive?.intOrNull,
            matchedBills = data.int("matched_bills"),
            totalIncome = data.double("total_income"),
            totalExpense = data.double("total_expense"),
            balance = data.double("balance"),
            monthlySummary = monthlySummary,
            standardReportMarkdown = data["report_markdown"]?.jsonPrimitive?.contentOrNull,
            standardReportJson = data["standard_report"]?.toString(),
            rawJson = rawJson,
        )
    }

    private fun parseRecordEditorDocument(rawJson: String): RecordEditorDocument {
        val root = json.parseToJsonElement(rawJson).jsonObject
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        if (!root.boolean("ok")) {
            error(root.string("message"))
        }
        return RecordEditorDocument(
            period = data.string("period"),
            relativePath = data.string("relative_path"),
            rawText = data.string("text"),
            persisted = data.boolean("persisted"),
        )
    }

    private fun parseRecordPreviewResult(rawJson: String): RecordPreviewResult {
        val root = json.parseToJsonElement(rawJson).jsonObject
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        val files = data["files"]?.jsonArray?.map { item ->
            val entry = item.jsonObject
            RecordPreviewFile(
                path = entry.string("path"),
                ok = entry.boolean("ok"),
                period = entry["period"]?.jsonPrimitive?.contentOrNull,
                year = entry["year"]?.jsonPrimitive?.intOrNull,
                month = entry["month"]?.jsonPrimitive?.intOrNull,
                transactionCount = entry.int("transaction_count"),
                totalIncome = entry.double("total_income"),
                totalExpense = entry.double("total_expense"),
                balance = entry.double("balance"),
                error = entry["error"]?.jsonPrimitive?.contentOrNull,
            )
        }.orEmpty()

        return RecordPreviewResult(
            ok = root.boolean("ok"),
            code = root.string("code"),
            message = root.string("message"),
            processed = data.int("processed"),
            success = data.int("success"),
            failure = data.int("failure"),
            periods = data["periods"]?.jsonArray?.mapNotNull { item ->
                item.jsonPrimitive.contentOrNull
            }.orEmpty(),
            files = files,
            rawJson = rawJson,
        )
    }

    private fun parseListedRecordPeriodsResult(rawJson: String): ListedRecordPeriodsResult {
        val root = json.parseToJsonElement(rawJson).jsonObject
        val data = root["data"]?.jsonObject ?: JsonObject(emptyMap())
        val invalidFiles = data["invalid_files"]?.jsonArray?.map { item ->
            val entry = item.jsonObject
            InvalidRecordFile(
                path = entry.string("path"),
                error = entry.string("error"),
            )
        }.orEmpty()

        return ListedRecordPeriodsResult(
            ok = root.boolean("ok"),
            code = root.string("code"),
            message = root.string("message"),
            processed = data.int("processed"),
            valid = data.int("valid"),
            invalid = data.int("invalid"),
            periods = data["periods"]?.jsonArray?.mapNotNull { item ->
                item.jsonPrimitive.contentOrNull
            }.orEmpty(),
            invalidFiles = invalidFiles,
            rawJson = rawJson,
        )
    }

    private fun JsonObject.boolean(key: String): Boolean =
        this[key]?.jsonPrimitive?.booleanOrNull ?: false

    private fun JsonObject.string(key: String): String =
        this[key]?.jsonPrimitive?.content ?: ""

    private fun JsonObject.int(key: String): Int =
        this[key]?.jsonPrimitive?.intOrNull ?: 0

    private fun JsonObject.double(key: String): Double =
        this[key]?.jsonPrimitive?.doubleOrNull ?: 0.0

    private fun recordFileForPeriod(recordsRoot: File, period: String): File {
        val year = period.substringBefore('-', missingDelimiterValue = period)
        return File(File(recordsRoot, year), "$period.txt")
    }
}
