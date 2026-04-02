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
import com.billstracer.android.model.RecordSaveResult
import com.billstracer.android.platform.yearMonthOrNull
import java.time.YearMonth
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
)

class EditorViewModel(
    private val editorService: EditorService,
    private val sessionBus: AppSessionBus,
    private val workspaceDataChangeBus: WorkspaceDataChangeBus,
    private val currentPeriodProvider: () -> String = { YearMonth.now().toString() },
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

    fun onEditorScreenShown() {
        val currentPeriod = currentPeriodProvider()
        val normalizedPeriods = normalizeRecordPeriods(
            state.value.persistedRecordPeriods,
            currentPeriod = currentPeriod,
        )
        mutableState.update { current ->
            applyExistingRecordSelection(
                current,
                periods = normalizedPeriods,
                preferredPeriod = currentPeriod,
            )
        }

        val currentYear = currentPeriod.substringBefore('-')
        val currentMonth = currentPeriod.substringAfter('-')
        val currentState = state.value
        if (
            currentState.activeRecordDocument?.period == currentPeriod &&
            currentState.selectedExistingRecordYear == currentYear &&
            currentState.selectedExistingRecordMonth == currentMonth
        ) {
            return
        }

        openRecordPeriod(period = currentPeriod, persistIfMissing = true)
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
        openSelectedExistingRecord()
    }

    fun selectExistingRecordMonth(month: String) {
        mutableState.update { current ->
            current.copy(selectedExistingRecordMonth = month)
        }
        openSelectedExistingRecord()
    }

    fun openSelectedExistingRecord() {
        val period = yearMonthOrNull(
            yearInput = state.value.selectedExistingRecordYear,
            monthInput = state.value.selectedExistingRecordMonth,
        ) ?: return
        openRecordPeriod(period = period, persistIfMissing = period == currentPeriodProvider())
    }

    fun updateRecordDraft(rawText: String) {
        mutableState.update { current ->
            current.copy(recordDraftText = rawText)
        }
    }

    fun saveRecordDraft() {
        val activeRecord = state.value.activeRecordDocument ?: return
        val draft = state.value.recordDraftText
        commitRecordDraft(activeRecord, draft)
    }

    fun saveRawRecordText(rawText: String) {
        val activeRecord = state.value.activeRecordDocument ?: return
        mutableState.update { current ->
            current.copy(
                recordDraftText = rawText,
            )
        }
        commitRecordDraft(activeRecord, rawText)
    }

    private fun commitRecordDraft(
        activeRecord: RecordEditorDocument,
        draft: String,
    ) {
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
                applyCommittedRecordResult(
                    saveResult = saveResult,
                    failureMessage = "Failed to save record draft.",
                    successMessage = "Saved ${activeRecord.relativePath} and synced it to the database.",
                    refreshFailureMessage =
                        "Saved ${activeRecord.relativePath} and synced it to the database, but failed to refresh imported periods.",
                )
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
                    val normalizedPeriods = normalizeRecordPeriods(
                        periods,
                        currentPeriod = currentPeriodProvider(),
                    )
                    val message = if (normalizedPeriods.isEmpty()) {
                        "No imported months found in SQLite."
                    } else {
                        "Loaded ${normalizedPeriods.size} imported/openable period(s)."
                    }
                    mutableState.update { current ->
                        applyExistingRecordSelection(
                            current.copy(
                                isInitializing = false,
                                isWorking = false,
                                errorMessage = null,
                                statusMessage = message,
                                persistedRecordPeriods = normalizedPeriods,
                            ),
                            periods = normalizedPeriods,
                            preferredPeriod = preferredPeriod ?: currentPeriodProvider().takeIf { initialLoad },
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

    private fun openRecordPeriod(
        period: String,
        persistIfMissing: Boolean,
    ) {
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
                    if (!document.persisted && persistIfMissing) {
                        persistGeneratedRecordPeriod(period, document)
                        return@onSuccess
                    }
                    val message = if (document.persisted) {
                        "Loaded TXT source for ${document.period}."
                    } else {
                        "Created an unsaved TXT template for ${document.period}."
                    }
                    sessionBus.publishStatus(message)
                    mutableState.update { current ->
                        current.copy(
                            isWorking = false,
                            activeRecordDocument = document,
                            recordDraftText = document.rawText,
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

    private suspend fun persistGeneratedRecordPeriod(
        period: String,
        document: RecordEditorDocument,
    ) {
        val pendingMessage = "Creating TXT source for $period..."
        mutableState.update { current ->
            current.copy(
                isWorking = true,
                errorMessage = null,
                statusMessage = pendingMessage,
            )
        }
        sessionBus.publishStatus(pendingMessage)
        runCatching { editorService.commitRecordDocument(period, document.rawText) }
            .onSuccess { saveResult ->
                applyCommittedRecordResult(
                    saveResult = saveResult,
                    failureMessage = "Failed to create record period.",
                    successMessage = "Created ${document.relativePath} and synced it to the database.",
                    refreshFailureMessage =
                        "Created ${document.relativePath} and synced it to the database, but failed to refresh imported periods.",
                )
            }
            .onFailure { error ->
                val message = error.message ?: "Failed to create record period."
                sessionBus.publishError(message, "Failed to create record period.")
                mutableState.update { current ->
                    current.copy(
                        isWorking = false,
                        errorMessage = message,
                        statusMessage = "Failed to create record period.",
                    )
                }
            }
    }

    private suspend fun applyCommittedRecordResult(
        saveResult: RecordSaveResult,
        failureMessage: String,
        successMessage: String,
        refreshFailureMessage: String,
    ) {
        if (!saveResult.ok || saveResult.document == null) {
            val message = saveResult.message.ifBlank { failureMessage }
            val errorMessage = saveResult.errorMessage ?: message
            sessionBus.publishError(errorMessage, failureMessage)
            mutableState.update { current ->
                current.copy(
                    isWorking = false,
                    errorMessage = errorMessage,
                    statusMessage = message,
                )
            }
            return
        }

        val savedDocument = saveResult.document
        workspaceDataChangeBus.notifyChanged()

        runCatching { editorService.listPersistedRecordPeriods() }
            .onSuccess { periods ->
                val normalizedPeriods = normalizeRecordPeriods(
                    periods,
                    currentPeriod = currentPeriodProvider(),
                )
                val message = saveResult.message.ifBlank { successMessage }
                sessionBus.publishStatus(message)
                mutableState.update { current ->
                    applyExistingRecordSelection(
                        current.copy(
                            isWorking = false,
                            activeRecordDocument = savedDocument,
                            recordDraftText = savedDocument.rawText,
                            errorMessage = null,
                            statusMessage = message,
                            persistedRecordPeriods = normalizedPeriods,
                        ),
                        periods = normalizedPeriods,
                        preferredPeriod = savedDocument.period,
                    )
                }
            }
            .onFailure { error ->
                val errorMessage = error.message ?: "Failed to refresh imported periods."
                sessionBus.publishError(errorMessage, refreshFailureMessage)
                mutableState.update { current ->
                    current.copy(
                        isWorking = false,
                        activeRecordDocument = savedDocument,
                        recordDraftText = savedDocument.rawText,
                        errorMessage = errorMessage,
                        statusMessage = refreshFailureMessage,
                    )
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

private fun normalizeRecordPeriods(
    periods: List<String>,
    currentPeriod: String,
): List<String> = (periods + currentPeriod).distinct().sortedDescending()
