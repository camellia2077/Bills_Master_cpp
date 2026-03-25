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
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import com.billstracer.android.platform.PaneContent
import com.billstracer.android.platform.SectionGroupCard

@Composable
internal fun QueryScreen(
    state: QueryUiState,
    onSelectQueryYear: (String) -> Unit,
    onSelectQueryPeriodYear: (String) -> Unit,
    onSelectQueryPeriodMonth: (String) -> Unit,
    onRunYearQuery: () -> Unit,
    onRunMonthQuery: () -> Unit,
    onSelectQueryViewMode: (QueryViewMode) -> Unit,
    modifier: Modifier = Modifier,
) {
    val availableYears = state.availablePeriods
        .mapNotNull { period -> period.substringBefore('-').takeIf { it.length == 4 } }
        .distinct()
    val availableMonths = state.availablePeriods
        .filter { period ->
            state.queryPeriodYearInput.isNotBlank() &&
                period.startsWith("${state.queryPeriodYearInput}-") &&
                period.length == 7
        }
        .map { period -> period.substringAfter('-') }
        .distinct()
    val hasAvailablePeriods = state.availablePeriods.isNotEmpty()
    var yearQueryExpanded by rememberSaveable { mutableStateOf(false) }
    var monthYearExpanded by rememberSaveable { mutableStateOf(false) }
    var monthExpanded by rememberSaveable { mutableStateOf(false) }

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
        SectionGroupCard(title = "Year Query") {
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
                Text("Query Year")
            }
        }
        SectionGroupCard(title = "Month Query") {
            Text(
                text = "Month options are derived from the same imported TXT periods already stored in SQLite.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                Box(modifier = Modifier.weight(1f)) {
                    OutlinedButton(
                        onClick = { monthYearExpanded = true },
                        enabled = !state.isInitializing && !state.isWorking && availableYears.isNotEmpty(),
                        modifier = Modifier
                            .fillMaxWidth()
                            .testTag("query_month_year_selector_button"),
                    ) {
                        Text(
                            text = state.queryPeriodYearInput.ifBlank { "Select year" },
                            fontFamily = FontFamily.Monospace,
                        )
                    }
                    DropdownMenu(
                        expanded = monthYearExpanded,
                        onDismissRequest = { monthYearExpanded = false },
                        modifier = Modifier.fillMaxWidth(0.9f),
                    ) {
                        availableYears.forEach { year ->
                            DropdownMenuItem(
                                text = { Text(text = year, fontFamily = FontFamily.Monospace) },
                                onClick = {
                                    monthYearExpanded = false
                                    onSelectQueryPeriodYear(year)
                                },
                            )
                        }
                    }
                }
                Box(modifier = Modifier.weight(1f)) {
                    OutlinedButton(
                        onClick = { monthExpanded = true },
                        enabled = !state.isInitializing && !state.isWorking && availableMonths.isNotEmpty(),
                        modifier = Modifier
                            .fillMaxWidth()
                            .testTag("query_month_month_selector_button"),
                    ) {
                        Text(
                            text = state.queryPeriodMonthInput.ifBlank { "Select month" },
                            fontFamily = FontFamily.Monospace,
                        )
                    }
                    DropdownMenu(
                        expanded = monthExpanded,
                        onDismissRequest = { monthExpanded = false },
                        modifier = Modifier.fillMaxWidth(0.9f),
                    ) {
                        availableMonths.forEach { month ->
                            DropdownMenuItem(
                                text = { Text(text = month, fontFamily = FontFamily.Monospace) },
                                onClick = {
                                    monthExpanded = false
                                    onSelectQueryPeriodMonth(month)
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
                Text("Query Month")
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
