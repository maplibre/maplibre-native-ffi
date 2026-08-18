package org.maplibre.nativeffi.map

import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CValue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.ptr
import kotlinx.cinterop.staticCFunction
import kotlinx.cinterop.useContents
import org.maplibre.nativeffi.internal.c.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM
import org.maplibre.nativeffi.internal.c.mln_canonical_tile_id
import org.maplibre.nativeffi.internal.c.mln_custom_mvt_vector_source_options
import org.maplibre.nativeffi.internal.c.mln_custom_mvt_vector_source_options_default
import org.maplibre.nativeffi.internal.callback.CallbackGate
import org.maplibre.nativeffi.internal.struct.StyleStructs
import org.maplibre.nativeffi.style.CustomMvtVectorSourceOptions

/**
 * Owns map/style-scoped custom MVT vector source callback state.
 *
 * [onReleased] runs on the map owner thread when native stops referencing this state, which is what
 * drops it from its map's registry and closes it.
 */
@OptIn(ExperimentalForeignApi::class)
internal class CustomMvtVectorSourceState(
  private val options: CustomMvtVectorSourceOptions,
  private val onReleased: () -> Unit,
) : AutoCloseable {
  private val selfRef = StableRef.create(this)
  private val descriptor = nativeHeap.alloc<mln_custom_mvt_vector_source_options>()
  private val gate = CallbackGate("custom MVT vector callbacks") { closeNative() }

  init {
    mln_custom_mvt_vector_source_options_default().place(descriptor.ptr)
    descriptor.fetch_tile = staticCFunction(::customMvtVectorFetchTile)
    descriptor.cancel_tile = staticCFunction(::customMvtVectorCancelTile)
    descriptor.release_user_data = staticCFunction(::customMvtVectorReleaseUserData)
    descriptor.user_data = selfRef.asCPointer()
    writeFields(descriptor)
  }

  fun descriptor(): CPointer<mln_custom_mvt_vector_source_options> = descriptor.ptr

  internal fun fetch(tileId: CValue<mln_canonical_tile_id>) {
    val lease = gate.enter() ?: return
    try {
      options.callback.fetchTile(tileId.useContents { StyleStructs.canonicalTileId(this) })
    } catch (_: Throwable) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      lease.close()
    }
  }

  /** Runs when native stops referencing this state, on the map owner thread. */
  internal fun released() {
    onReleased()
  }

  internal fun cancel(tileId: CValue<mln_canonical_tile_id>) {
    val lease = gate.enter() ?: return
    try {
      options.callback.cancelTile(tileId.useContents { StyleStructs.canonicalTileId(this) })
    } catch (_: Throwable) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      lease.close()
    }
  }

  private fun writeFields(native: mln_custom_mvt_vector_source_options) {
    native.fields = 0U
    options.minZoom?.let {
      native.fields = native.fields or MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM
      native.min_zoom = it
    }
    options.maxZoom?.let {
      native.fields = native.fields or MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM
      native.max_zoom = it
    }
  }

  override fun close() = gate.close()

  private fun closeNative() {
    selfRef.dispose()
    nativeHeap.free(descriptor.rawPtr)
  }
}

@Suppress("UNUSED_PARAMETER")
@OptIn(ExperimentalForeignApi::class)
private fun customMvtVectorFetchTile(
  userData: COpaquePointer?,
  tileId: CValue<mln_canonical_tile_id>,
) {
  userData?.asStableRef<CustomMvtVectorSourceState>()?.get()?.fetch(tileId)
}

@Suppress("UNUSED_PARAMETER")
@OptIn(ExperimentalForeignApi::class)
private fun customMvtVectorCancelTile(
  userData: COpaquePointer?,
  tileId: CValue<mln_canonical_tile_id>,
) {
  userData?.asStableRef<CustomMvtVectorSourceState>()?.get()?.cancel(tileId)
}

@OptIn(ExperimentalForeignApi::class)
private fun customMvtVectorReleaseUserData(userData: COpaquePointer?) {
  try {
    userData?.asStableRef<CustomMvtVectorSourceState>()?.get()?.released()
  } catch (_: Throwable) {
    // Native callbacks must not unwind through the C ABI.
  }
}
