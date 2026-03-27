package com.billstracer.android.features.editor

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.app.navigation.WorkspaceDataChangeBus
import com.billstracer.android.data.services.EditorService
import com.billstracer.android.features.common.monthsForYear
import com.billstracer.android.features.common.resolveYearMonthSelection
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
    val statusMessage: String = "Loading imported periods from SQLite...",
    val errorMessage: String? = null,
    val persistedRecordPeriods: List<String> = emptyList(),
    val selectedExistingRecordYear: String = "",
    val selectedExistingRecordMonth: String = "",
    val activeRecordDocument: RecordEditorDocument? = null,
    val recordDraftText: String = "",
    val recordPreviewResult: RecordPreviewResult? = null,
)

class EditorViewModel(
    private val editorService: EditorService,
    private val sessionBus: AppSessionBus,
    private val workspaceDataChangeBus: WorkspaceDataChangeBus,
) : ViewModel() {
    private val mutableState = MutableStateFlow(EditorUiState())
    val state: StateFlow<EditorUiState> = mutableState.asStateFlow()
    private var observedWorkspaceDataVersion = workspaceDataChangeBus.version.value

    init {
        observeWorkspaceDataChanges()
        refreshPersistedRecordPeriods(initialLoad = true)
    }

    fun refreshPersistedRecordPeriods() {
        refreshPersistedRecordPeriods(initialLoad = false)
    }

    fun selectExistingRecordYear(year: String) {
        mutableState.update { current ->
            val months = monthsForYear(current.persistedRecordPeriods, year)
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
                    val message = "Loaded TXT source for ${document.period}."
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
            val pendingMessage = "Saving ${activeRecord.period} into private records and SQLite..."
            mutableState.update { current ->
                current.copy(
                    isWorking = true,
                    errorMessage = null,
                    statusMessage = pendingMessage,
                )
            }
            sessionBus.publishStatus(pendingMessage)
            runCatching {
                editorService.commitRecordDocument(activeRecord.period, draft)
            }.onSuccess { saveResult ->
                if (!saveResult.ok || saveResult.document == null) {
                    val message = saveResult.message.ifBlank {
                        "Failed to save record draft."
                    }
                    val errorMessage = saveResult.errorMessage ?: message
                    sessionBus.publishError(errorMessage, message)
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            errorMessage = errorMessage,
                            statusMessage = message,
                        )
                    }
                    return@onSuccess
                }

                val savedDocument = saveResult.document
                runCatching { editorService.listPersistedRecordPeriods() }
                    .onSuccess { periods ->
                        val message = saveResult.message.ifBlank {
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
                                    persistedRecordPeriods = periods,
                                ),
                                periods = periods,
                                preferredPeriod = savedDocument.period,
                            )
                        }
                    }
                    .onFailure { error ->
                        val message =
                            "Saved ${savedDocument.relativePath} and synced it to the database, but failed to refresh imported periods."
                        val errorMessage = error.message ?: "Failed to refresh imported periods."
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

    private fun observeWorkspaceDataChanges() {
        viewModelScope.launch {
            workspaceDataChangeBus.version.collect { version ->
                if (version == observedWorkspaceDataVersion) {
                    return@collect
                }
                observedWorkspaceDataVersion = version
                refreshPersistedRecordPeriods(initialLoad = false)
            }
        }
    }

    private fun refreshPersistedRecordPeriods(
        initialLoad: Boolean,
        preferredPeriod: String? = state.value.activeRecordDocument?.period,
    ) {
        viewModelScope.launch {
            if (!initialLoad) {
                mutableState.update { current ->
                    current.copy(
                        isWorking = true,
                        errorMessage = null,
                        statusMessage = "Loading imported periods from SQLite...",
                    )
                }
            }
            runCatching { editorService.listPersistedRecordPeriods() }
                .onSuccess { periods ->
                    val message = if (periods.isEmpty()) {
                        "No imported months found in SQLite."
                    } else {
                        "Loaded ${periods.size} imported period(s) from SQLite."
                    }
                    mutableState.update { current ->
                        applyExistingRecordSelection(
                            current.copy(
                                isInitializing = false,
                                isWorking = false,
                                errorMessage = null,
                                statusMessage = message,
                                persistedRecordPeriods = periods,
                            ),
                            periods = periods,
                            preferredPeriod = preferredPeriod,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to load imported periods from SQLite."
                    sessionBus.publishError(message, "Failed to load editor periods.")
                    mutableState.update { current ->
                        current.copy(
                            isInitializing = false,
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to load imported periods from SQLite.",
                        )
                    }
                }
        }
    }
}

class EditorViewModelFactory(
    private val editorService: EditorService,
    private val sessionBus: AppSessionBus,
    private val workspaceDataChangeBus: WorkspaceDataChangeBus,
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return EditorViewModel(editorService, sessionBus, workspaceDataChangeBus) as T
    }
}

private fun applyExistingRecordSelection(
    state: EditorUiState,
    periods: List<String>,
    preferredPeriod: String? = state.activeRecordDocument?.period,
): EditorUiState {
    val selection = resolveYearMonthSelection(
        currentYear = state.selectedExistingRecordYear,
        currentMonth = state.selectedExistingRecordMonth,
        periods = periods,
        preferredPeriod = preferredPeriod,
    )
    return state.copy(
        persistedRecordPeriods = periods,
        selectedExistingRecordYear = selection.year,
        selectedExistingRecordMonth = selection.month,
    )
}
