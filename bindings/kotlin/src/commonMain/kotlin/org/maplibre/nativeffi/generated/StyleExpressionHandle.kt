// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

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
}
