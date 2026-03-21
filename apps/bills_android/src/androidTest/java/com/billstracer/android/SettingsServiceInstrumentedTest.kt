package com.billstracer.android

import android.content.Context
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.billstracer.android.model.ThemeColor
import com.billstracer.android.model.ThemeMode
import com.billstracer.android.model.ThemePreferences
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class SettingsServiceInstrumentedTest {
    @Test
    fun loadAndPersistSettingsData() = runBlocking {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val services = createAndroidServiceBundle(context)
        val originalTheme = services.settingsService.loadThemePreferences()
        val originalConfig = services.settingsService.loadBundledConfigs().last().rawText
        val updatedTheme = ThemePreferences(
            color = ThemeColor.EMERALD,
            mode = ThemeMode.DARK,
        )
        val updatedConfig = originalConfig.trimEnd() + "\n# persisted from android test\n"

        try {
            val updatedConfigs = services.settingsService.updateBundledConfig(
                fileName = "export_formats.toml",
                rawText = updatedConfig,
            )
            val (coreVersion, androidVersion) = services.settingsService.loadVersionInfo()
            val notices = services.settingsService.loadBundledNotices()
            val persistedTheme = services.settingsService.updateThemePreferences(updatedTheme)

            assertEquals(updatedConfig, updatedConfigs.last().rawText)
            assertEquals(updatedTheme, persistedTheme)
            assertEquals("0.4.2", coreVersion.versionName)
            assertEquals(BuildConfig.PRESENTATION_VERSION_NAME, androidVersion.versionName)
            assertTrue(notices.markdownText.contains("Open Source Notices"))
        } finally {
            services.settingsService.updateBundledConfig(
                fileName = "export_formats.toml",
                rawText = originalConfig,
            )
            services.settingsService.updateThemePreferences(originalTheme)
        }
    }
}
