package com.billstracer.android.features.query

import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import com.billstracer.android.platform.PaneContent
import com.billstracer.android.platform.SectionGroupCard
import com.billstracer.android.platform.YearDigitsField
import com.billstracer.android.platform.YearMonthDigitsRow
import com.billstracer.android.platform.yearInputOrNull
import com.billstracer.android.platform.yearInputValidationMessage
import com.billstracer.android.platform.yearMonthOrNull
import com.billstracer.android.platform.yearMonthValidationMessage

@Composable
internal fun QueryScreen(
    state: QueryUiState,
    onQueryYearChange: (String) -> Unit,
    onQueryPeriodYearChange: (String) -> Unit,
    onQueryPeriodMonthChange: (String) -> Unit,
    onRunYearQuery: () -> Unit,
    onRunMonthQuery: () -> Unit,
    onSelectQueryViewMode: (QueryViewMode) -> Unit,
    modifier: Modifier = Modifier,
) {
    val yearQueryError = yearInputValidationMessage(state.queryYearInput)
    val isYearQueryValid = yearInputOrNull(state.queryYearInput) != null
    val monthQueryError = yearMonthValidationMessage(
        yearInput = state.queryPeriodYearInput,
        monthInput = state.queryPeriodMonthInput,
    )
    val isMonthQueryValid = yearMonthOrNull(
        yearInput = state.queryPeriodYearInput,
        monthInput = state.queryPeriodMonthInput,
    ) != null

    PaneContent(modifier = modifier) {
        SectionGroupCard(title = "Year Query") {
            YearDigitsField(
                value = state.queryYearInput,
                onValueChange = onQueryYearChange,
                enabled = !state.isInitializing && !state.isWorking,
                label = "Year",
                placeholder = "YYYY",
                testTag = "query_year_field",
                modifier = Modifier.fillMaxWidth(),
            )
            yearQueryError?.let { validationMessage ->
                Text(
                    text = validationMessage,
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                    color = MaterialTheme.colorScheme.error,
                )
            }
            Text(
                text = "Use a 4-digit year, for example ${state.bundledSampleYear}.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            Button(
                onClick = onRunYearQuery,
                enabled = !state.isInitializing && !state.isWorking && isYearQueryValid,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("query_run_year_button"),
            ) {
                Text("Query Year")
            }
        }
        SectionGroupCard(title = "Month Query") {
            YearMonthDigitsRow(
                yearValue = state.queryPeriodYearInput,
                monthValue = state.queryPeriodMonthInput,
                onYearValueChange = onQueryPeriodYearChange,
                onMonthValueChange = onQueryPeriodMonthChange,
                enabled = !state.isInitializing && !state.isWorking,
                rowTag = "query_month_period_field",
                yearFieldTag = "query_month_year_field",
                monthFieldTag = "query_month_month_field",
                separatorTag = "query_month_separator",
                modifier = Modifier.fillMaxWidth(),
            )
            monthQueryError?.let { validationMessage ->
                Text(
                    text = validationMessage,
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                    color = MaterialTheme.colorScheme.error,
                )
            }
            Text(
                text = "Use numeric year/month only. The dash is fixed and month auto-pads to 2 digits.",
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
            )
            Button(
                onClick = onRunMonthQuery,
                enabled = !state.isInitializing && !state.isWorking && isMonthQueryValid,
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
