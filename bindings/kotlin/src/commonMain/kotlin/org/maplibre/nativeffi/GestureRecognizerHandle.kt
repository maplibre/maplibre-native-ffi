// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlin.jvm.JvmName
import org.maplibre.nativeffi.internal.JvmSynthetic
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A gesture recognizer that moves a map's camera.
 *
 * See `mln_gesture_recognizer` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class GestureRecognizerHandle
private constructor(
  handle: Long,
) : AutoCloseable {
  @get:JvmSynthetic
  internal val binding =
    ReadHandleState("GestureRecognizerHandle", handle, GeneratedOwnerDisposal::gestureRecognizer)
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
   * Handles one input at `timeMs`, in milliseconds on any clock that only goes forward, the same
   * for every input and frame.
   *
   * See `mln_gesture_recognizer_handle` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_ARGUMENT` when `input` isn't a valid input.
   */
  public fun handle(input: GestureInput, timeMs: Double): GestureResult =
    nativeRead(this, binding) {
      val out = sized(w(200, 200, 156), w(8, 8, 4))
      check(
        C.mln_gesture_recognizer_handle(handle, writeGestureInput(input), timeMs, out, diagnostic),
      )
      readGestureResult(out)
    }

  /**
   * Releases a gesture recognizer. A glide it started keeps going.
   *
   * See `mln_gesture_recognizer_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
   */
  public fun release(): Unit =
    nativeClose(this, binding) { C.mln_gesture_recognizer_release(handle) }

  /**
   * Changes the settings whose bits are set, from the next input on.
   *
   * See `mln_gesture_recognizer_set_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
   *
   * Errors:
   *
   * - `Status.INVALID_ARGUMENT` when an option is out of range.
   */
  public fun setOptions(options: GestureOptions): Unit =
    nativeCall(this, binding) {
      check(C.mln_gesture_recognizer_set_options(handle, writeGestureOptions(options), diagnostic))
    }

  /**
   * Times long presses, steps the eases of discrete deltas, ends deltas once they idle, and samples
   * the camera for notches, at `timeMs` on the inputs' clock. Call it when the last result's
   * `next_tick` says: on the next display frame, or at its time with a frame or a timer. A tick at
   * any other time does no harm.
   *
   * See `mln_gesture_recognizer_tick` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
   */
  public fun tick(timeMs: Double): GestureResult =
    nativeRead(this, binding) {
      val out = sized(w(200, 200, 156), w(8, 8, 4))
      check(C.mln_gesture_recognizer_tick(handle, timeMs, out, diagnostic))
      readGestureResult(out)
    }

  override fun toString(): String =
    "GestureRecognizerHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  internal companion object {
    /** Creates the owner of `handle`. */
    @JvmSynthetic
    internal fun wrap(handle: Long): GestureRecognizerHandle = GestureRecognizerHandle(handle)
  }
}
