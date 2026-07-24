package com.billstracer.android.platform

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.billstracer.android.app.navigation.FloatingBottomNavigationHeight

@Composable
internal fun PaneContent(
    modifier: Modifier = Modifier,
    scrollEnabled: Boolean = true,
    content: @Composable ColumnScope.() -> Unit,
) {
    Column(
        // This padding is intentionally applied after verticalScroll: it becomes scrollable
        // tail space for the final item, rather than a fixed, opaque strip under the capsule.
        modifier = modifier
            .verticalScroll(
                state = rememberScrollState(),
                enabled = scrollEnabled,
            )
            .padding(bottom = floatingBottomNavigationContentPadding()),
        verticalArrangement = Arrangement.spacedBy(12.dp),
        content = content,
    )
}

@Composable
private fun floatingBottomNavigationContentPadding() =
    FloatingBottomNavigationHeight + maxOf(
        WindowInsets.navigationBars.asPaddingValues().calculateBottomPadding(),
        20.dp,
    )

@Composable
internal fun SectionGroupCard(
    title: String,
    content: @Composable ColumnScope.() -> Unit,
) {
    Surface(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(22.dp),
        color = MaterialTheme.colorScheme.surfaceContainer,
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            Text(
                text = title,
                style = MaterialTheme.typography.titleMedium,
                fontFamily = FontFamily.Monospace,
            )
            content()
        }
    }
}

@Composable
internal fun StatPill(label: String, value: String) {
    Surface(
        shape = RoundedCornerShape(999.dp),
        color = MaterialTheme.colorScheme.primary.copy(alpha = 0.12f),
    ) {
        Column(
            modifier = Modifier
                .widthIn(min = 88.dp)
                .padding(horizontal = 12.dp, vertical = 8.dp),
        ) {
            Text(label, style = MaterialTheme.typography.labelSmall)
            Text(
                text = value,
                style = MaterialTheme.typography.labelLarge,
                fontFamily = FontFamily.Monospace,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
        }
    }
}
