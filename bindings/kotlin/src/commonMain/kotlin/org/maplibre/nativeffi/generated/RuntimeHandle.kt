// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * A runtime: the native scheduler thread and event store for its maps.
 *
 * See `mln_runtime` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class RuntimeHandle
internal constructor(
  handle: Long,
) : GeneratedRuntimeOperations(), org.maplibre.nativeffi.runtime.AsyncReleasable, AutoCloseable {
  internal override val binding =
    HandleState("RuntimeHandle", handle, GeneratedOwnerDisposal::runtime)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /**
   * The native handle value, which runtime events report as their source. It stays readable after
   * close, so events drained later still match this handle.
   */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public override val isClosed: Boolean
    get() = binding.isReleased

  public override fun close() {
    dispose()
  }

  override fun toString(): String =
    "RuntimeHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}
