// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlin.jvm.JvmName
import kotlin.jvm.JvmStatic
import org.maplibre.nativeffi.internal.JvmSynthetic
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Demands render sessions' frames at a display's refreshes.
 *
 * See `mln_frame_pacer` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class FramePacerHandle
private constructor(
  handle: Long,
) : AutoCloseable {
  @get:JvmSynthetic
  internal val binding = HandleState("FramePacerHandle", handle, GeneratedOwnerDisposal::framePacer)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after release. */
  @get:JvmName("getId")
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  /** Calls [release], for the standard library's `use`. */
  public override fun close() {
    release()
  }

  /**
   * Releases a frame pacer.
   *
   * Render sessions that use the pacer stay paced until you release them or set another pacer.
   *
   * See `mln_frame_pacer_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun release(): Unit = nativeClose(this, binding) { C.mln_frame_pacer_release(handle) }

  /**
   * Changes a frame pacer's settings.
   *
   * Settings that `options` doesn't set keep their current values. To remove the frame rate limit,
   * set `FramePacerOptions.unlimitedFramesPerSecond`. You can call it on any thread.
   *
   * The sessions that the pacer serves follow the new settings from the next display refresh,
   * including a refresh that they already wait for. The pacer counts a new limit from the last
   * frames it demanded, so a higher limit can demand the next frames sooner. On iOS, macOS, and
   * tvOS, the pacer also asks the system for the new limit's refresh rate, or for the display's own
   * rate once the limit is removed.
   *
   * See `mln_frame_pacer_set_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_STATE` when you released `pacer`, even while sessions still use it.
   */
  public fun setOptions(options: FramePacerOptions): Unit =
    nativeCall(this, binding) {
      check(C.mln_frame_pacer_set_options(handle, writeFramePacerOptions(options), diagnostic))
    }

  override fun toString(): String =
    "FramePacerHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  public companion object {
    /**
     * Creates a frame pacer that follows the platform's display.
     *
     * On iOS, macOS, and tvOS, the pacer follows the display through a `CADisplayLink`. You can
     * call it on any thread, and the pacer needs no run loop of yours. On macOS, it follows the
     * main display, the one with the menu bar. With `FramePacerOptions.maximumFramesPerSecond`, the
     * pacer also asks the system for that refresh rate. On iPhone, a rate above 60 needs
     * `CADisableMinimumFrameDurationOnPhone` in the app's `Info.plist`.
     *
     * On Android, the pacer follows the display through the calling thread's `AChoreographer`. Call
     * it on a thread with an `ALooper`, such as the app's main thread, and keep that thread running
     * its looper while sessions use the pacer. `AChoreographer` can stop while the phone's display
     * is off, even while a Surface on another display, such as an Android Auto projection, still
     * shows the map. The pacer then keeps demanding frames from a timer on the looper, within the
     * frame rate limit, until vsyncs arrive again.
     *
     * On OpenHarmony, the pacer follows the display through `OH_NativeVSync`. You can call it on
     * any thread, and the pacer needs no event loop of yours.
     *
     * See `mln_frame_pacer_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
     *
     * Errors:
     *
     * - `Status.UNSUPPORTED` on the other platforms, such as visionOS, Linux, and Windows, where
     *   the library has no source of display refreshes.
     * - `Status.INVALID_STATE` on Android when the calling thread has no `ALooper`.
     * - `Status.NATIVE_ERROR` on iOS, macOS, and tvOS when the system has no display to follow, and
     *   on OpenHarmony when the system can't report the display's refreshes to the process.
     */
    @JvmStatic
    public fun create(options: FramePacerOptions? = null): FramePacerHandle =
      nativeCall(null, null) {
        val out = allocate(8, w(8, 8, 4))
        check(
          C.mln_frame_pacer_create(
            options?.let { writeFramePacerOptions(it) } ?: 0L,
            out,
            diagnostic,
          ),
        )
        adopt(out, GeneratedOwnerDisposal::framePacer) { FramePacerHandle.wrap(it) }
      }

    /** Creates the owner of `handle`. */
    @JvmSynthetic internal fun wrap(handle: Long): FramePacerHandle = FramePacerHandle(handle)
  }
}
