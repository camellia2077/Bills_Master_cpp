package com.billstracer.android.ui.app

import androidx.annotation.DrawableRes
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import com.billstracer.android.model.BillsUiState

internal data class AppSectionItem(
    val label: String,
    @field:DrawableRes val iconResId: Int,
)

@Composable
internal fun BottomSectionBar(
    sections: List<AppSectionItem>,
    selectedIndex: Int,
    onSelectedIndexChange: (Int) -> Unit,
) {
    NavigationBar(
        containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.97f),
        tonalElevation = 8.dp,
    ) {
        sections.forEachIndexed { index, section ->
            NavigationBarItem(
                selected = selectedIndex == index,
                onClick = { onSelectedIndexChange(index) },
                icon = {
                    Icon(
                        painter = painterResource(id = section.iconResId),
                        contentDescription = section.label,
                    )
                },
                label = { Text(section.label) },
                colors = NavigationBarItemDefaults.colors(
                    selectedIconColor = MaterialTheme.colorScheme.onSecondaryContainer,
                    selectedTextColor = MaterialTheme.colorScheme.onSurface,
                    indicatorColor = MaterialTheme.colorScheme.secondaryContainer,
                ),
            )
        }
    }
}

@Composable
internal fun HeroHeader(
    state: BillsUiState,
) {
    Column(
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        Text(
            text = state.statusMessage,
            style = MaterialTheme.typography.bodyLarge,
            fontFamily = FontFamily.Monospace,
        )
        if (!state.errorMessage.isNullOrBlank()) {
            Surface(
                shape = RoundedCornerShape(18.dp),
                color = MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.92f),
            ) {
                Text(
                    text = state.errorMessage ?: "",
                    color = MaterialTheme.colorScheme.onErrorContainer,
                    style = MaterialTheme.typography.bodyMedium,
                    modifier = Modifier.padding(horizontal = 14.dp, vertical = 12.dp),
                )
            }
        }
    }
}
