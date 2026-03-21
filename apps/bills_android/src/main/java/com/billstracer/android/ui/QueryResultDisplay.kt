package com.billstracer.android.ui

import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import com.billstracer.android.model.QueryResult
import com.billstracer.android.ui.report.queryResultDisplayContent

@Composable
internal fun queryResultDisplay(
    result: QueryResult,
    modifier: Modifier = Modifier,
) {
    queryResultDisplayContent(
        result = result,
        modifier = modifier,
    )
}
