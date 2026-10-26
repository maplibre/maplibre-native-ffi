// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A parsed style expression, immutable and independent of any map.
 *
 * See `mln_style_expression` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class StyleExpressionHandle
internal constructor(
  handle: Long,
) : AutoCloseable {
  internal val binding =
    HandleState("StyleExpressionHandle", handle, GeneratedOwnerDisposal::styleExpression)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after close. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

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
   * Copies what a parsed expression depends on.
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
  public override fun close(): Unit =
    nativeClose(this, binding) { C.mln_style_expression_release(handle) }

  override fun toString(): String =
    "StyleExpressionHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  public companion object {
    /**
     * Parses a style expression.
     *
     * See `mln_style_expression_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
     */
    public fun create(
      expression: String,
      options: StyleExpressionOptions? = null,
    ): StyleExpressionHandle =
      nativeCall(null, null) {
        val out = allocate(8, 8)
        check(
          C.mln_style_expression_create(
            view(expression),
            options?.let { writeStyleExpressionOptions(it) } ?: 0L,
            out,
            diagnostic,
          ),
        )
        adopt(out, GeneratedOwnerDisposal::styleExpression) { StyleExpressionHandle(it) }
      }
  }
}
