package com.billstracer.android.features.editor

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.billstracer.android.app.navigation.AppSessionBus
import com.billstracer.android.data.services.EditorService
import com.billstracer.android.model.ListedRecordPeriodsResult
import com.billstracer.android.model.RecordEditorDocument
import com.billstracer.android.model.RecordPreviewResult
import com.billstracer.android.platform.sanitizeMonthInput
import com.billstracer.android.platform.sanitizeYearInput
import com.billstracer.android.platform.splitYearMonth
import com.billstracer.android.platform.yearMonthOrNull
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import java.time.YearMonth

data class EditorUiState(
    val isInitializing: Boolean = true,
    val isWorking: Boolean = false,
    val statusMessage: String = "",
    val errorMessage: String? = null,
    val selectedExistingRecordYear: String = "",
    val selectedExistingRecordMonth: String = "",
    val recordPeriodYearInput: String = "",
    val recordPeriodMonthInput: String = "",
    val activeRecordDocument: RecordEditorDocument? = null,
    val recordDraftText: String = "",
    val listedRecordPeriods: ListedRecordPeriodsResult? = null,
    val recordPreviewResult: RecordPreviewResult? = null,
)

class EditorViewModel(
    private val editorService: EditorService,
    private val sessionBus: AppSessionBus,
) : ViewModel() {
    private val mutableState = MutableStateFlow(
        EditorUiState().withManualRecordPeriod(YearMonth.now().toString()),
    )
    val state: StateFlow<EditorUiState> = mutableState.asStateFlow()

    init {
        refreshRecordPeriods(initialLoad = true)
    }

    fun updateRecordPeriodYearInput(year: String) {
        mutableState.update { current ->
            current.copy(recordPeriodYearInput = sanitizeYearInput(year))
        }
    }

    fun updateRecordPeriodMonthInput(month: String) {
        mutableState.update { current ->
            current.copy(recordPeriodMonthInput = sanitizeMonthInput(month))
        }
    }

    fun selectExistingRecordYear(year: String) {
        mutableState.update { current ->
            val months = monthsForYear(current.listedRecordPeriods, year)
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

    fun openManualRecord() {
        val period = yearMonthOrNull(
            yearInput = state.value.recordPeriodYearInput,
            monthInput = state.value.recordPeriodMonthInput,
        )
        if (period == null) {
            mutableState.update { current ->
                current.copy(
                    errorMessage = "Manual period must use YYYY-MM, and month must be between 01 and 12.",
                    statusMessage = "Manual period input is invalid.",
                )
            }
            return
        }
        openRecordPeriod(period)
    }

    fun openSelectedExistingRecord() {
        val period = yearMonthOrNull(
            yearInput = state.value.selectedExistingRecordYear,
            monthInput = state.value.selectedExistingRecordMonth,
        ) ?: return
        openRecordPeriod(period)
    }

    private fun openRecordPeriod(period: String) {
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
            runCatching {
                val document = editorService.openRecordPeriod(period)
                val listedPeriods = editorService.listRecordPeriods()
                document to listedPeriods
            }.onSuccess { (document, listedPeriods) ->
                val message = if (document.persisted) {
                    "Loaded persisted record ${document.period}."
                } else {
                    "Generated a fresh record template for ${document.period}."
                }
                sessionBus.publishStatus(message)
                mutableState.update { current ->
                    applyExistingRecordSelection(
                        current.copy(
                            isWorking = false,
                            activeRecordDocument = document,
                            recordDraftText = document.rawText,
                            recordPreviewResult = null,
                            statusMessage = message,
                        ).withManualRecordPeriod(document.period),
                        listedPeriods = listedPeriods,
                        preferredPeriod = document.period,
                    )
                }
            }.onFailure { error ->
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
                val savedDocument = editorService.saveRecordDocument(activeRecord.period, draft)
                val listedPeriods = editorService.listRecordPeriods()
                savedDocument to listedPeriods
            }.onSuccess { (savedDocument, listedPeriods) ->
                sessionBus.publishStatus("Saved ${savedDocument.relativePath}.")
                mutableState.update { current ->
                    applyExistingRecordSelection(
                        current.copy(
                            isWorking = false,
                            activeRecordDocument = savedDocument,
                            recordDraftText = savedDocument.rawText,
                            statusMessage = "Saved ${savedDocument.relativePath}.",
                        ),
                        listedPeriods = listedPeriods,
                        preferredPeriod = savedDocument.period,
                    )
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

    fun refreshRecordPeriods(initialLoad: Boolean = false) {
        viewModelScope.launch {
            if (!initialLoad) {
                mutableState.update { current ->
                    current.copy(
                        isWorking = true,
                        errorMessage = null,
                        statusMessage = "Scanning private record periods...",
                    )
                }
                sessionBus.publishStatus("Scanning private record periods...")
            }
            runCatching { editorService.listRecordPeriods() }
                .onSuccess { listedPeriods ->
                    val message = listedPeriods.message
                    if (listedPeriods.ok) {
                        sessionBus.publishStatus(message.ifBlank { "Record periods refreshed." })
                    } else {
                        sessionBus.publishError(message, "Record periods refresh failed.")
                    }
                    mutableState.update { current ->
                        applyExistingRecordSelection(
                            current.copy(
                                isInitializing = false,
                                isWorking = false,
                                errorMessage = if (listedPeriods.ok) null else listedPeriods.message,
                                statusMessage = message,
                            ),
                            listedPeriods = listedPeriods,
                            preferredPeriod = current.activeRecordDocument?.period,
                        )
                    }
                }
                .onFailure { error ->
                    val message = error.message ?: "Failed to scan record periods."
                    sessionBus.publishError(message, "Failed to scan record periods.")
                    mutableState.update { current ->
                        current.copy(
                            isInitializing = false,
                            isWorking = false,
                            errorMessage = message,
                            statusMessage = "Failed to scan record periods.",
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
    listedPeriods: ListedRecordPeriodsResult?,
    preferredPeriod: String? = state.activeRecordDocument?.period,
): EditorUiState {
    val selection = resolveExistingRecordSelection(
        currentYear = state.selectedExistingRecordYear,
        currentMonth = state.selectedExistingRecordMonth,
        listedPeriods = listedPeriods,
        preferredPeriod = preferredPeriod,
    )
    return state.copy(
        listedRecordPeriods = listedPeriods,
        selectedExistingRecordYear = selection.year,
        selectedExistingRecordMonth = selection.month,
    )
}

private fun resolveExistingRecordSelection(
    currentYear: String,
    currentMonth: String,
    listedPeriods: ListedRecordPeriodsResult?,
    preferredPeriod: String?,
): ExistingRecordSelection {
    val years = yearsFromPeriods(listedPeriods)
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
    val months = monthsForYear(listedPeriods, selectedYear)
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

private fun yearsFromPeriods(listedPeriods: ListedRecordPeriodsResult?): List<String> =
    listedPeriods?.periods.orEmpty()
        .mapNotNull { period -> period.substringBefore('-').takeIf { it.length == 4 } }
        .distinct()

private fun monthsForYear(
    listedPeriods: ListedRecordPeriodsResult?,
    year: String,
): List<String> = listedPeriods?.periods.orEmpty()
    .filter { period -> period.startsWith("$year-") && period.length == 7 }
    .map { period -> period.substringAfter('-') }
    .distinct()

private fun EditorUiState.withManualRecordPeriod(period: String): EditorUiState {
    val (year, month) = splitYearMonth(period)
    return copy(
        recordPeriodYearInput = year,
        recordPeriodMonthInput = month,
    )
}
