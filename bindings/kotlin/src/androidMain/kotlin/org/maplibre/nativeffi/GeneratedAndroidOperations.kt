// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Creates a frame pacer that follows the display through the calling thread's looper.
 *
 * Call it on a thread with an `ALooper`, such as the app's main thread. The pacer demands frames
 * from that thread at the display refreshes that the looper's `AChoreographer` reports, so keep the
 * thread running its looper while sessions use the pacer.
 *
 * See `mln_android_frame_pacer_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
 *
 * Errors:
 *
 * - `Status.INVALID_STATE` when the calling thread has no `ALooper`.
 */
public fun androidFramePacerCreate(options: FramePacerOptions? = null): FramePacerHandle =
  nativeCall(null, null) {
    val out = allocate(8, w(8, 8, 4))
    check(
      C.mln_android_frame_pacer_create(
        options?.let { writeFramePacerOptions(it) } ?: 0L,
        out,
        diagnostic,
      ),
    )
    adopt(out, GeneratedOwnerDisposal::framePacer) { FramePacerHandle(it) }
  }
