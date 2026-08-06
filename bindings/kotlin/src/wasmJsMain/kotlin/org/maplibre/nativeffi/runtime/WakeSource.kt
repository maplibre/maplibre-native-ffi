package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeWakeSource
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.BrowserModule

// `(i64) -> i32` and `(i64) -> ()`: a handle is a 64-bit generational identifier, so it crosses as
// a BigInt rather than a number, which is what the module's own i64 interface expects.
@JsFun("(handle) => globalThis.__maplibreNativeC._mln_wake_source_signal(handle)")
private external fun signalWakeSource(handle: Long): Int

@JsFun("(handle) => globalThis.__maplibreNativeC._mln_wake_source_destroy(handle)")
private external fun destroyWakeSource(handle: Long)

/**
 * Releases a runtime owner thread parked in [RuntimeHandle.pump].
 *
 * Unlike almost everything else in this binding, these two calls are not proxied to the runtime's
 * owner thread. The C API documents a wake source as callable from any thread -- it takes one small
 * lock and returns -- and proxying it would defeat its purpose, since the thread it exists to
 * release is the one that would have to run the proxied call.
 *
 * That also makes it usable from a callback stack, which cannot suspend, and from JavaScript event
 * handlers a host wires up outside a scope.
 */
public actual class WakeSource private constructor(private val source: NativeWakeSource) :
  AutoCloseable {
  private val core = HandleStateCore("WakeSource", source.raw)

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun signal() {
    BrowserModule.attach()
    core.withLive { Status.check(signalWakeSource(source.raw)) }
  }

  /**
   * Releases the wake source.
   *
   * A wake source is not owner-affine, so a host may hold one past the end of every runtime and
   * close it whenever it likes, which is the behaviour every other target has.
   */
  public actual override fun close() {
    // Destruction returns nothing, so the close reports success on its behalf.
    core.closeOnce(
      destroy = {
        destroyWakeSource(source.raw)
        MaplibreStatus.OK.nativeCode
      }
    )
  }

  internal companion object {
    fun fromNative(source: NativeWakeSource): WakeSource = WakeSource(source)
  }
}
