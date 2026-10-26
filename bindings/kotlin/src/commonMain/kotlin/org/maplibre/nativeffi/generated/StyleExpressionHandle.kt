// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

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
) : GeneratedStyleExpressionOperations(), AutoCloseable {
  internal override val binding =
    HandleState("StyleExpressionHandle", handle, GeneratedOwnerDisposal::styleExpression)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after close. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  public override fun close() {
    release()
  }

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
