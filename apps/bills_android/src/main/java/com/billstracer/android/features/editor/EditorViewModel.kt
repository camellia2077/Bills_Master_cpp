package com.billstracer.android.features.editor

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.data.services.EditorService
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewResult
import com.billstracer.android.platform.yearMonthOrNull
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

data class EditorUiState(
    val isInitializing: Boolean = true,
    val isWorking: Boolean = false,
    val statusMessage: String = "Loading editable months from database...",
    val errorMessage: String? = null,
    val databaseRecordPeriods: List<String> = emptyList(),
    val selectedExistingRecordYear: String = "",
    val selectedExistingRecordMonth: String = "",
    val activeRecordDocument: RecordEditorDocument? = null,
    val recordDraftText: String = "",
    val recordPreviewResult: RecordPreviewResult? = null,
)

class EditorViewModel(
    private val editorService: EditorService,
    private val sessionBus: AppSessionBus,
) : ViewModel() {
    private val mutableState = MutableStateFlow(EditorUiState())
    val state: StateFlow<EditorUiState> = mutableState.asStateFlow()

    init {
        refreshDatabaseRecordPeriods(initialLoad = true)
    }

    fun refreshDatabaseRecordPeriods() {
        refreshDatabaseRecordPeriods(initialLoad = false)
    }

    fun selectExistingRecordYear(year: String) {
        mutableState.update { current ->
            val months = monthsForYear(current.databaseRecordPeriods, year)
            val selectedMonth = if (months.contains(current.selectedExistingRecordMonth)) {
                current.selectedExistingRecordMonth
            } else {
                months.firstOrNull().orEmpty()
            }
            current.copy(
                selectedExistingRecordYear = year,
                selectedExistingRecordMonth = selectedMonth,
            )
        }
    }

    fun selectExistingRecordMonth(month: String) {
        mutableState.update { current ->
            current.copy(selectedExistingRecordMonth = month)
        }
    }

    fun openSelectedExistingRecord() {
        val period = yearMonthOrNull(
            yearInput = state.value.selectedExistingRecordYear,
            monthInput = state.value.selectedExistingRecordMonth,
        ) ?: return
        viewModelScope.launch {
            val pendingMessage = "Opening record period $period..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { editorService.openPersistedRecordPeriod(period) }
                .onSuccess { document ->
                    val message = "Loaded persisted record ${document.period}."
                    sessionBus.publishStatus(message)
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            activeRecordDocument = document,
                            recordDraftText = document.rawText,
                            recordPreviewResult = null,
                            statusMessage = message,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to open record period."
                    sessionBus.publishError(message, "Failed to open record period.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to open record period.",
                        )
                    }
                }
        }
    }

    fun updateRecordDraft(rawText: String) {
        mutableState.update { current ->
            current.copy(recordDraftText = rawText)
        }
    }

    fun resetRecordDraft() {
        mutableState.update { current ->
            val activeRecord = current.activeRecordDocument ?: return@update current
            current.copy(
                recordDraftText = activeRecord.rawText,
                statusMessage = "Restored the draft for ${activeRecord.period}.",
            )
        }
    }

    fun previewRecordDraft() {
        val activeRecord = state.value.activeRecordDocument ?: return
        val draft = state.value.recordDraftText
        viewModelScope.launch {
            val pendingMessage =
                "Previewing ${activeRecord.period} through core record_preview..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching { editorService.previewRecordDocument(activeRecord.period, draft) }
                .onSuccess { preview ->
                    val message = if (preview.ok) {
                        "Previewed ${preview.success} record file(s) successfully."
                    } else {
                        preview.message
                    }
                    if (preview.ok) {
                        sessionBus.publishStatus(message)
                    } else {
                        sessionBus.publishError(preview.message, message)
                    }
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            recordPreviewResult = preview,
                            errorMessage = if (preview.ok) null else preview.message,
                            statusMessage = message,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to preview record draft."
                    sessionBus.publishError(message, "Failed to preview record draft.")
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to preview record draft.",
                        )
                    }
                }
        }
    }

    fun saveRecordDraft() {
        val activeRecord = state.value.activeRecordDocument ?: return
        val draft = state.value.recordDraftText
        viewModelScope.launch {
            val pendingMessage = "Saving ${activeRecord.period} into private records..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching {
                editorService.saveRecordDocument(activeRecord.period, draft)
            }.onSuccess { savedDocument ->
                runCatching {
                    editorService.syncSavedRecordToDatabase(savedDocument.period)
                }.onSuccess { syncResult ->
                    if (!syncResult.ok) {
                        val statusMessage = "Saved ${savedDocument.relativePath}, but database sync failed."
                        val errorMessage = syncResult.errorMessage ?: syncResult.message
                        sessionBus.publishError(errorMessage, statusMessage)
                        mutableState.update { current ->
                            current.copy(
                                isWorking = false,
                                activeRecordDocument = savedDocument,
                                recordDraftText = savedDocument.rawText,
                                errorMessage = errorMessage,
                                statusMessage = statusMessage,
                            )
                        }
                        return@onSuccess
                    }

                    runCatching { editorService.listDatabaseRecordPeriods() }
                        .onSuccess { periods ->
                            val message = syncResult.message.ifBlank {
                                "Saved ${savedDocument.relativePath} and synced it to the database."
                            }
                            sessionBus.publishStatus(message)
                            mutableState.update { current ->
                                applyExistingRecordSelection(
                                    current.copy(
                                        isWorking = false,
                                        activeRecordDocument = savedDocument,
                                        recordDraftText = savedDocument.rawText,
                                        errorMessage = null,
                                        statusMessage = message,
                                        databaseRecordPeriods = periods,
                                    ),
                                    periods = periods,
                                    preferredPeriod = savedDocument.period,
                                )
                            }
                        }
                        .onFailure { error ->
                            val message =
                                "Saved ${savedDocument.relativePath} and synced it to the database, but failed to refresh editor months."
                            val errorMessage = error.message ?: "Failed to refresh editor months."
                            sessionBus.publishError(errorMessage, message)
                            mutableState.update { current ->
                                current.copy(
                                    isWorking = false,
                                    activeRecordDocument = savedDocument,
                                    recordDraftText = savedDocument.rawText,
                                    errorMessage = errorMessage,
                                    statusMessage = message,
                                )
                            }
                        }
                }.onFailure { error ->
                    val message = "Saved ${savedDocument.relativePath}, but database sync failed."
                    val errorMessage = error.message ?: "Failed to sync the saved TXT into the database."
                    sessionBus.publishError(errorMessage, message)
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            activeRecordDocument = savedDocument,
                            recordDraftText = savedDocument.rawText,
                            errorMessage = errorMessage,
                            statusMessage = message,
                        )
                    }
                }
            }.onFailure { error ->
                val message = error.message ?: "Failed to save record draft."
                sessionBus.publishError(message, "Failed to save record draft.")
                mutableState.update { current ->
                    current.copy(
                        isWorking = false,
                        errorMessage = message,
                        statusMessage = "Failed to save record draft.",
                    )
                }
            }
        }
    }

    private fun refreshDatabaseRecordPeriods(
        initialLoad: Boolean,
        preferredPeriod: String? = state.value.activeRecordDocument?.period,
    ) {
        viewModelScope.launch {
            if (!initialLoad) {
                mutableState.update { current ->
                    current.copy(
                        isWorking = true,
                        errorMessage = null,
                        statusMessage = "Loading editable months from database...",
                    )
                }
            }
            runCatching { editorService.listDatabaseRecordPeriods() }
                .onSuccess { periods ->
                    val message = if (periods.isEmpty()) {
                        "No imported months found in database."
                    } else {
                        "Loaded ${periods.size} editable month(s) from database."
                    }
                    mutableState.update { current ->
                        applyExistingRecordSelection(
                            current.copy(
                                isInitializing = false,
                                isWorking = false,
                                errorMessage = null,
                                statusMessage = message,
                                databaseRecordPeriods = periods,
                            ),
                            periods = periods,
                            preferredPeriod = preferredPeriod,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to load database record periods."
                    sessionBus.publishError(message, "Failed to load editor months.")
                    mutableState.update { current ->
                        current.copy(
                            isInitializing = false,
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to load database record periods.",
                        )
                    }
                }
        }
    }
}

class EditorViewModelFactory(
    private val editorService: EditorService,
    private val sessionBus: AppSessionBus,
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return EditorViewModel(editorService, sessionBus) as T
    }
}

private data class ExistingRecordSelection(
    val year: String = "",
    val month: String = "",
)

private fun applyExistingRecordSelection(
    state: EditorUiState,
    periods: List<String>,
    preferredPeriod: String? = state.activeRecordDocument?.period,
): EditorUiState {
    val selection = resolveExistingRecordSelection(
        currentYear = state.selectedExistingRecordYear,
        currentMonth = state.selectedExistingRecordMonth,
        periods = periods,
        preferredPeriod = preferredPeriod,
    )
    return state.copy(
        databaseRecordPeriods = periods,
        selectedExistingRecordYear = selection.year,
        selectedExistingRecordMonth = selection.month,
    )
}

private fun resolveExistingRecordSelection(
    currentYear: String,
    currentMonth: String,
    periods: List<String>,
    preferredPeriod: String?,
): ExistingRecordSelection {
    val years = yearsFromPeriods(periods)
    if (years.isEmpty()) {
        return ExistingRecordSelection()
    }

    val preferredYear = preferredPeriod?.substringBefore('-', "")
    val preferredMonth = preferredPeriod?.substringAfter('-', "")
    val selectedYear = when {
        !preferredYear.isNullOrBlank() && years.contains(preferredYear) -> preferredYear
        currentYear.isNotBlank() && years.contains(currentYear) -> currentYear
        else -> years.first()
    }
    val months = monthsForYear(periods, selectedYear)
    val selectedMonth = when {
        !preferredMonth.isNullOrBlank() &&
            preferredYear == selectedYear &&
            months.contains(preferredMonth) -> preferredMonth
        currentMonth.isNotBlank() && months.contains(currentMonth) -> currentMonth
        else -> months.firstOrNull().orEmpty()
    }

    return ExistingRecordSelection(
        year = selectedYear,
        month = selectedMonth,
    )
}

private fun yearsFromPeriods(periods: List<String>): List<String> =
    periods.mapNotNull { period -> period.substringBefore('-').takeIf { it.length == 4 } }
        .distinct()

private fun monthsForYear(
    periods: List<String>,
    year: String,
): List<String> = periods
    .filter { period -> period.startsWith("$year-") && period.length == 7 }
    .map { period -> period.substringAfter('-') }
    .distinct()
