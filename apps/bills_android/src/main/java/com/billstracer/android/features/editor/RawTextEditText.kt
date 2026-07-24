package com.billstracer.android.features.editor

import android.content.Context
import android.text.Editable
import android.text.InputType
import android.text.TextWatcher
import android.util.TypedValue
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.widget.EditText
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView

private class RawEditorEditText(
    context: Context,
) : EditText(context) {
    var onValueChange: ((String) -> Unit)? = null
    var isApplyingExternalText: Boolean = false

    private val watcher = object : TextWatcher {
        override fun beforeTextChanged(
            s: CharSequence?,
            start: Int,
            count: Int,
            after: Int,
        ) = Unit

        override fun onTextChanged(
            s: CharSequence?,
            start: Int,
            before: Int,
            count: Int,
        ) = Unit

        override fun afterTextChanged(s: Editable?) {
            if (!isApplyingExternalText) {
                onValueChange?.invoke(s?.toString().orEmpty())
            }
        }
    }

    init {
        addTextChangedListener(watcher)
    }
}

@Composable
internal fun RawTextEditText(
    value: String,
    onValueChange: (String) -> Unit,
    modifier: Modifier = Modifier,
    backgroundColor: Color = Color.Transparent,
    textColorOverride: Color? = null,
) {
    val density = LocalDensity.current
    val textStyle = MaterialTheme.typography.bodyMedium
    val textColor = (textColorOverride ?: MaterialTheme.colorScheme.onSurface).toArgb()
    val textSizePx = with(density) { textStyle.fontSize.toPx() }
    val contentPaddingPx = with(density) { 12.dp.roundToPx() }

    AndroidView(
        modifier = modifier
            .fillMaxWidth()
            .testTag("editor_record_field"),
        factory = { context ->
            RawEditorEditText(context).apply {
                layoutParams = ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                )
                minLines = 1
                maxLines = Int.MAX_VALUE
                gravity = Gravity.TOP or Gravity.START
                setHorizontallyScrolling(false)
                isVerticalScrollBarEnabled = true
                overScrollMode = View.OVER_SCROLL_IF_CONTENT_SCROLLS
                typeface = android.graphics.Typeface.MONOSPACE
                setTextSize(TypedValue.COMPLEX_UNIT_PX, textSizePx)
                setTextColor(textColor)
                setBackgroundColor(backgroundColor.toArgb())
                inputType = InputType.TYPE_CLASS_TEXT or
                    InputType.TYPE_TEXT_FLAG_MULTI_LINE or
                    InputType.TYPE_TEXT_FLAG_CAP_SENTENCES
                imeOptions = EditorInfo.IME_FLAG_NO_ENTER_ACTION
                setPadding(
                    contentPaddingPx,
                    contentPaddingPx,
                    contentPaddingPx,
                    contentPaddingPx,
                )
                isLongClickable = true
                contentDescription = "editor_record_field_native"
                setTag("editor_record_field_native")
                this.onValueChange = onValueChange
                setText(value)
                setSelection(0)
                scrollTo(0, 0)
            }
        },
        update = { editText ->
            editText.onValueChange = onValueChange
            editText.setTextColor(textColor)
            editText.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSizePx)
            editText.setBackgroundColor(backgroundColor.toArgb())
            if (editText.text.toString() != value) {
                editText.isApplyingExternalText = true
                editText.setText(value)
                editText.setSelection(0)
                editText.scrollTo(0, 0)
                editText.isApplyingExternalText = false
            }
        },
    )
}
