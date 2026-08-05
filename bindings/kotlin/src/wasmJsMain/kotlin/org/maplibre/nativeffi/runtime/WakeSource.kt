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
    BrowserModule.require()
    core.withLive { Status.check(signalWakeSource(source.raw)) }
  }

  /**
   * Releases the wake source, or retires the handle when the module has already gone.
   *
   * A wake source is not owner-affine, so it is deliberately not one of the handles a shutdown
   * refuses to leave open — a host may hold one past the end of every runtime and close it whenever
   * it likes, which is the behaviour every other target has. A shutdown that released the module
   * took this source's storage with the heap it lived in, so there is nothing left to destroy and
   * the handle simply retires. Calling native there would reach a null module and fail a close that
   * has nothing to fail at.
   */
  public actual override fun close() {
    core.closeOnce(
      destroy = {
        if (BrowserModule.isLoaded()) destroyWakeSource(source.raw)
        MaplibreStatus.OK.nativeCode
      }
    )
  }

  internal companion object {
    fun fromNative(source: NativeWakeSource): WakeSource = WakeSource(source)
  }
}
