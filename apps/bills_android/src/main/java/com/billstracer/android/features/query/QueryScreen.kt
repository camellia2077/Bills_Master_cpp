package com.billstracer.android.features.query

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.SegmentedButton
import androidx.compose.material3.SegmentedButtonDefaults
import androidx.compose.material3.SingleChoiceSegmentedButtonRow
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.billstracer.android.features.common.YearMonthPickerAvailability
import com.billstracer.android.features.common.YearMonthPickerSheet
import com.billstracer.android.platform.PaneContent
import com.billstracer.android.platform.SectionGroupCard

@Composable
internal fun QueryScreen(
    state: QueryUiState,
    onSelectQueryYear: (String) -> Unit,
    onSelectQueryPeriodYear: (String) -> Unit,
    onSelectQueryPeriodMonth: (String) -> Unit,
    onSelectQueryRangeStart: (String) -> Unit,
    onSelectQueryRangeEnd: (String) -> Unit,
    onRunYearQuery: () -> Unit,
    onRunMonthQuery: () -> Unit,
    onRunRangeQuery: () -> Unit,
    onSelectQueryInputMode: (QueryInputMode) -> Unit = {},
    onSelectQueryViewMode: (QueryViewMode) -> Unit,
    modifier: Modifier = Modifier,
) {
    val availableYears = state.availablePeriods
        .mapNotNull { period -> period.substringBefore('-').takeIf { it.length == 4 } }
        .distinct()
    val hasAvailablePeriods = state.availablePeriods.isNotEmpty()
    val monthPickerAvailability = remember(state.availablePeriods) {
        YearMonthPickerAvailability.fromPeriods(state.availablePeriods)
    }
    val selectedPeriod = listOf(
        state.queryPeriodYearInput,
        state.queryPeriodMonthInput,
    ).filter { it.isNotBlank() }.joinToString("-")
    var yearQueryExpanded by rememberSaveable { mutableStateOf(false) }
    var monthPickerVisible by rememberSaveable { mutableStateOf(false) }
    var rangeStartExpanded by rememberSaveable { mutableStateOf(false) }
    var rangeEndExpanded by rememberSaveable { mutableStateOf(false) }

    PaneContent(modifier = modifier) {
        if (state.statusMessage.isNotBlank()) {
            Text(
                text = state.statusMessage,
                style = MaterialTheme.typography.bodyMedium,
                fontFamily = FontFamily.Monospace,
                modifier = Modifier.testTag("query_status_message"),
            )
        }
        state.errorMessage?.takeIf { it.isNotBlank() }?.let { errorMessage ->
            Surface(
                color = MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.92f),
                shape = MaterialTheme.shapes.medium,
            ) {
                Text(
                    text = errorMessage,
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(12.dp)
                        .testTag("query_error_message"),
                    color = MaterialTheme.colorScheme.onErrorContainer,
                    style = MaterialTheme.typography.bodyMedium,
                    fontFamily = FontFamily.Monospace,
                )
            }
        }
        SingleChoiceSegmentedButtonRow(modifier = Modifier.fillMaxWidth()) {
            QueryInputMode.entries.forEachIndexed { index, mode ->
                val selected = state.queryInputMode == mode
                SegmentedButton(
                    shape = SegmentedButtonDefaults.itemShape(
                        index = index,
                        count = QueryInputMode.entries.size,
                    ),
                    onClick = { onSelectQueryInputMode(mode) },
                    selected = selected,
                    modifier = Modifier
                        .weight(1f)
                        .testTag("query_mode_${mode.name.lowercase()}_chip"),
                    colors = SegmentedButtonDefaults.colors(
                        activeContainerColor = MaterialTheme.colorScheme.primaryContainer,
                        activeContentColor = MaterialTheme.colorScheme.onPrimaryContainer,
                        inactiveContainerColor = MaterialTheme.colorScheme.surface,
                        inactiveContentColor = MaterialTheme.colorScheme.onSurfaceVariant,
                        activeBorderColor = MaterialTheme.colorScheme.primary,
                        inactiveBorderColor = MaterialTheme.colorScheme.outlineVariant,
                    ),
                    label = {
                        Text(
                            text = mode.label,
                            maxLines = 1,
                            overflow = TextOverflow.Ellipsis,
                            fontFamily = FontFamily.Monospace,
                            fontWeight = if (selected) FontWeight.SemiBold else FontWeight.Medium,
                        )
                    },
                )
            }
        }
        if (state.queryInputMode == QueryInputMode.YEAR) {
            SectionGroupCard(title = "Year") {
            Text(
                text = "Query years come from months already parsed from TXT and inserted into SQLite.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            Box(modifier = Modifier.fillMaxWidth()) {
                OutlinedButton(
                    onClick = { yearQueryExpanded = true },
                    enabled = !state.isInitializing && !state.isWorking && availableYears.isNotEmpty(),
                    modifier = Modifier
                        .fillMaxWidth()
                        .testTag("query_year_selector_button"),
                ) {
                    Text(
                        text = state.queryYearInput.ifBlank { "Select year" },
                        fontFamily = FontFamily.Monospace,
                    )
                }
                DropdownMenu(
                    expanded = yearQueryExpanded,
                    onDismissRequest = { yearQueryExpanded = false },
                    modifier = Modifier.fillMaxWidth(0.9f),
                ) {
                    availableYears.forEach { year ->
                        DropdownMenuItem(
                            text = { Text(text = year, fontFamily = FontFamily.Monospace) },
                            onClick = {
                                yearQueryExpanded = false
                                onSelectQueryYear(year)
                            },
                        )
                    }
                }
            }
            if (state.isInitializing) {
                Text(
                    text = "Loading imported months from database...",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            } else if (!hasAvailablePeriods) {
                Text(
                    text = "No imported months found in SQLite yet. Import or sync TXT files first.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                    modifier = Modifier.testTag("query_year_empty_state_message"),
                )
            } else {
                Text(
                    text = "Choose an existing year from the database-backed list.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
            Button(
                onClick = onRunYearQuery,
                enabled = !state.isInitializing && !state.isWorking && state.queryYearInput.isNotBlank(),
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("query_run_year_button"),
            ) {
                Text("Run Year")
            }
            }
        }
        if (state.queryInputMode == QueryInputMode.MONTH) {
            SectionGroupCard(title = "Month") {
            Text(
                text = "Month options are derived from the same imported TXT periods already stored in SQLite.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            OutlinedButton(
                onClick = { monthPickerVisible = true },
                enabled = !state.isInitializing && !state.isWorking && hasAvailablePeriods,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("query_month_picker_button"),
            ) {
                Text(
                    text = selectedPeriod.ifBlank { "Select year and month" },
                    style = MaterialTheme.typography.titleMedium,
                    fontFamily = FontFamily.Monospace,
                )
            }
            if (monthPickerVisible) {
                YearMonthPickerSheet(
                    selectedYearMonth = selectedPeriod,
                    availability = monthPickerAvailability,
                    title = "Select report month",
                    currentText = "Current selection: ${selectedPeriod.ifBlank { "None" }}",
                    onYearMonthSelected = { period ->
                        onSelectQueryPeriodYear(period.substringBefore('-'))
                        onSelectQueryPeriodMonth(period.substringAfter('-'))
                    },
                    onDismissRequest = { monthPickerVisible = false },
                )
            }
            if (state.isInitializing) {
                Text(
                    text = "Loading imported months from database...",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            } else if (!hasAvailablePeriods) {
                Text(
                    text = "No imported months found in SQLite yet. Import or sync TXT files first.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                    modifier = Modifier.testTag("query_month_empty_state_message"),
                )
            } else {
                Text(
                    text = "Select an imported year/month and run the report for that saved period.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
            Button(
                onClick = onRunMonthQuery,
                enabled = !state.isInitializing &&
                    !state.isWorking &&
                    state.queryPeriodYearInput.isNotBlank() &&
                    state.queryPeriodMonthInput.isNotBlank(),
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("query_run_month_button"),
            ) {
                Text("Run Month")
            }
            }
        }
        if (state.queryInputMode == QueryInputMode.RANGE) {
            SectionGroupCard(title = "Range") {
            Text(
                text = "Range queries reuse the same imported month list and summarize an inclusive start/end period.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                Box(modifier = Modifier.weight(1f)) {
                    OutlinedButton(
                        onClick = { rangeStartExpanded = true },
                        enabled = !state.isInitializing && !state.isWorking && hasAvailablePeriods,
                        modifier = Modifier
                            .fillMaxWidth()
                            .testTag("query_range_start_selector_button"),
                    ) {
                        Text(
                            text = state.queryRangeStartInput.ifBlank { "Select start" },
                            fontFamily = FontFamily.Monospace,
                        )
                    }
                    DropdownMenu(
                        expanded = rangeStartExpanded,
                        onDismissRequest = { rangeStartExpanded = false },
                        modifier = Modifier.fillMaxWidth(0.9f),
                    ) {
                        state.availablePeriods.forEach { period ->
                            DropdownMenuItem(
                                text = { Text(text = period, fontFamily = FontFamily.Monospace) },
                                onClick = {
                                    rangeStartExpanded = false
                                    onSelectQueryRangeStart(period)
                                },
                            )
                        }
                    }
                }
                Box(modifier = Modifier.weight(1f)) {
                    OutlinedButton(
                        onClick = { rangeEndExpanded = true },
                        enabled = !state.isInitializing && !state.isWorking && hasAvailablePeriods,
                        modifier = Modifier
                            .fillMaxWidth()
                            .testTag("query_range_end_selector_button"),
                    ) {
                        Text(
                            text = state.queryRangeEndInput.ifBlank { "Select end" },
                            fontFamily = FontFamily.Monospace,
                        )
                    }
                    DropdownMenu(
                        expanded = rangeEndExpanded,
                        onDismissRequest = { rangeEndExpanded = false },
                        modifier = Modifier.fillMaxWidth(0.9f),
                    ) {
                        state.availablePeriods.forEach { period ->
                            DropdownMenuItem(
                                text = { Text(text = period, fontFamily = FontFamily.Monospace) },
                                onClick = {
                                    rangeEndExpanded = false
                                    onSelectQueryRangeEnd(period)
                                },
                            )
                        }
                    }
                }
            }
            if (state.isInitializing) {
                Text(
                    text = "Loading imported months from database...",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            } else if (!hasAvailablePeriods) {
                Text(
                    text = "No imported months found in SQLite yet. Import or sync TXT files first.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                    modifier = Modifier.testTag("query_range_empty_state_message"),
                )
            } else {
                Text(
                    text = "Choose an inclusive start/end month range from the imported periods.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
            }
            Button(
                onClick = onRunRangeQuery,
                enabled = !state.isInitializing &&
                    !state.isWorking &&
                    state.queryRangeStartInput.isNotBlank() &&
                    state.queryRangeEndInput.isNotBlank(),
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("query_run_range_button"),
            ) {
                Text("Run Range")
            }
            }
        }
        state.queryResult?.let { result ->
            QueryResultDisplayContent(
                result = result,
                selectedViewMode = state.selectedQueryViewMode,
                onSelectViewMode = onSelectQueryViewMode,
            )
        }
    }
}
