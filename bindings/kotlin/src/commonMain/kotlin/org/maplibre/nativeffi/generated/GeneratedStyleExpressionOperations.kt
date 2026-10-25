// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedStyleExpressionOperations internal constructor() {
  internal abstract val binding: HandleState

  /**
   * Evaluates a parsed expression and writes its result as UTF-8 JSON.
   *
   * See `mln_style_expression_evaluate` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  public fun evaluate(inputs: StyleExpressionInputs? = null): String =
    nativeCall(this, binding) {
      val nativeInputs = inputs?.let { writeStyleExpressionInputs(it) } ?: 0L
      val textLength = allocate(w(4, 8), w(4, 8))
      var textCapacity = 256L
      var textStorage = allocate(textCapacity, 1)
      while (true) {
        check(
          C.mln_style_expression_evaluate(
            handle,
            nativeInputs,
            textStorage,
            textCapacity,
            textLength,
            diagnostic,
          ),
        )
        val written = readSize(textLength).toLong()
        if (written < textCapacity) break
        textCapacity = written + 1
        textStorage = allocate(textCapacity, 1)
      }
      readBytes(textStorage, readSize(textLength)).decodeToString()
    }

  /**
   * Copies what a parsed expression depends on into out_info.
   *
   * See `mln_style_expression_get_info` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  public fun getInfo(): StyleExpressionInfo =
    nativeCall(this, binding) {
      val out = sized(8, 4)
      check(C.mln_style_expression_get_info(handle, out, diagnostic))
      readStyleExpressionInfo(out)
    }

  /**
   * Releases a style expression.
   *
   * See `mln_style_expression_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  public fun release(): Unit = nativeClose(this, binding) { C.mln_style_expression_release(handle) }
}
