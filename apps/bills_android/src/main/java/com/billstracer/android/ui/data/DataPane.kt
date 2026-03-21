package com.billstracer.android.ui.data

import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontFamily
import com.billstracer.android.BuildConfig
import com.billstracer.android.model.BillsUiState
import com.billstracer.android.ui.common.PaneContent
import com.billstracer.android.ui.common.SectionGroupCard

@Composable
internal fun DataPane(
    state: BillsUiState,
    onImport: () -> Unit,
    onExportRecordFiles: () -> Unit,
    onClearRecordFiles: () -> Unit,
    onClearDatabase: () -> Unit,
    modifier: Modifier = Modifier,
) {
    PaneContent(modifier = modifier) {
        if (BuildConfig.DEBUG) {
            SectionGroupCard(title = "Debug") {
                Text(
                    text = "Debug build only. Sample import and export tools are excluded from release builds.",
                    style = MaterialTheme.typography.bodySmall,
                    fontFamily = FontFamily.Monospace,
                )
                Button(
                    onClick = onImport,
                    enabled = !state.isInitializing && !state.isWorking,
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Text("Import bundled sample")
                }
                Button(
                    onClick = onExportRecordFiles,
                    enabled = !state.isInitializing && !state.isWorking,
                    modifier = Modifier
                        .fillMaxWidth()
                        .testTag("data_export_txt_button"),
                ) {
                    Text("Export TXT and configs")
                }
                state.importResult?.let { result ->
                    Text(
                        text = "processed ${result.processed}  imported ${result.imported}  failure ${result.failure}",
                        style = MaterialTheme.typography.bodySmall,
                        fontFamily = FontFamily.Monospace,
                    )
                    if (!result.ok && result.message.isNotBlank()) {
                        Text(
                            text = result.message,
                            color = MaterialTheme.colorScheme.error,
                            style = MaterialTheme.typography.bodyMedium,
                        )
                    }
                }
            }
        }
        SectionGroupCard(title = "Workspace") {
            Button(
                onClick = onClearRecordFiles,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier
                    .fillMaxWidth()
                    .testTag("data_clear_txt_button"),
            ) {
                Text("Clear all TXT files")
            }
            Button(
                onClick = onClearDatabase,
                enabled = !state.isInitializing && !state.isWorking,
                modifier = Modifier.fillMaxWidth(),
            ) {
                Text("Clear database")
            }
        }
    }
}
