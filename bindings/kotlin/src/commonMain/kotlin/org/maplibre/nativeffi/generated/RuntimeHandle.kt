// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

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

  public companion object {
    /**
     * Creates a runtime.
     *
     * Suspends until the call's thread starts. On the JVM, Android, and Kotlin/Native the call
     * waits on the calling thread, which it blocks until then. On Kotlin/Wasm it runs on a thread
     * of its own, so the page can call it. Cancellation does not interrupt the wait: a caller
     * cancelled meanwhile receives the cancellation, and the binding disposes the result.
     *
     * See `mln_runtime_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
     */
    public suspend fun create(options: RuntimeOptions): RuntimeHandle =
      nativeThreadStart({ it.dispose() }) {
        val out = allocate(8, 8)
        threadStart { diagnostic ->
          C.mln_runtime_create(writeRuntimeOptions(options), out, diagnostic)
        }
        adopt(out, GeneratedOwnerDisposal::runtime) { RuntimeHandle(it) }
      }
  }
}
