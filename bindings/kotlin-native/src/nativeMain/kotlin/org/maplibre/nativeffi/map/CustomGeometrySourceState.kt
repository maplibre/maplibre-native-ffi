package org.maplibre.nativeffi.map

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
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
import org.maplibre.nativeffi.internal.c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
import org.maplibre.nativeffi.internal.c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
import org.maplibre.nativeffi.internal.c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
import org.maplibre.nativeffi.internal.c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
import org.maplibre.nativeffi.internal.c.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
import org.maplibre.nativeffi.internal.c.mln_canonical_tile_id
import org.maplibre.nativeffi.internal.c.mln_custom_geometry_source_options
import org.maplibre.nativeffi.internal.c.mln_custom_geometry_source_options_default
import org.maplibre.nativeffi.internal.struct.StyleStructs
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions

/** Owns map/style-scoped custom geometry source callback state. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class)
internal class CustomGeometrySourceState(private val options: CustomGeometrySourceOptions) :
  AutoCloseable {
  private val selfRef = StableRef.create(this)
  private val descriptor = nativeHeap.alloc<mln_custom_geometry_source_options>()
  private val callbackLock = AtomicInt(0)
  private var activeCallbacks = 0
  private var closeRequested = false
  private var closed = false

  init {
    mln_custom_geometry_source_options_default().place(descriptor.ptr)
    descriptor.fetch_tile = staticCFunction(::customGeometryFetchTile)
    descriptor.cancel_tile = staticCFunction(::customGeometryCancelTile)
    descriptor.user_data = selfRef.asCPointer()
    writeFields(descriptor)
  }

  fun descriptor(): CPointer<mln_custom_geometry_source_options> = descriptor.ptr

  internal fun fetch(tileId: CValue<mln_canonical_tile_id>) {
    if (!enterCallback()) return
    try {
      options.callback.fetchTile(tileId.useContents { StyleStructs.canonicalTileId(this) })
    } catch (_: Throwable) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      exitCallback()
    }
  }

  internal fun cancel(tileId: CValue<mln_canonical_tile_id>) {
    if (!enterCallback()) return
    try {
      options.callback.cancelTile(tileId.useContents { StyleStructs.canonicalTileId(this) })
    } catch (_: Throwable) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      exitCallback()
    }
  }

  private fun writeFields(native: mln_custom_geometry_source_options) {
    native.fields = 0U
    options.minZoom?.let {
      native.fields = native.fields or MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
      native.min_zoom = it
    }
    options.maxZoom?.let {
      native.fields = native.fields or MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
      native.max_zoom = it
    }
    options.tolerance?.let {
      native.fields = native.fields or MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
      native.tolerance = it
    }
    options.tileSize?.let {
      native.fields = native.fields or MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
      native.tile_size = it.toUInt()
    }
    options.buffer?.let {
      native.fields = native.fields or MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
      native.buffer = it.toUInt()
    }
    options.clip?.let {
      native.fields = native.fields or MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
      native.clip = it
    }
    options.wrap?.let {
      native.fields = native.fields or MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
      native.wrap = it
    }
  }

  override fun close() {
    val shouldClose = withCallbackLock {
      if (closed || closeRequested) {
        false
      } else {
        closeRequested = true
        val canClose = activeCallbacks == 0
        if (canClose) closed = true
        canClose
      }
    }
    if (shouldClose) closeNative()
  }

  private fun enterCallback(): Boolean = withCallbackLock {
    if (closed || closeRequested) {
      false
    } else {
      activeCallbacks++
      true
    }
  }

  private fun exitCallback() {
    val shouldClose = withCallbackLock {
      activeCallbacks--
      val canClose = closeRequested && activeCallbacks == 0 && !closed
      if (canClose) closed = true
      canClose
    }
    if (shouldClose) closeNative()
  }

  private fun closeNative() {
    selfRef.dispose()
    nativeHeap.free(descriptor.rawPtr)
  }

  private inline fun <T> withCallbackLock(block: () -> T): T {
    while (!callbackLock.compareAndSet(0, 1)) {
      // Custom geometry callbacks can arrive concurrently on worker threads.
    }
    try {
      return block()
    } finally {
      callbackLock.store(0)
    }
  }
}

@Suppress("UNUSED_PARAMETER")
@OptIn(ExperimentalForeignApi::class)
private fun customGeometryFetchTile(
  userData: COpaquePointer?,
  tileId: CValue<mln_canonical_tile_id>,
) {
  userData?.asStableRef<CustomGeometrySourceState>()?.get()?.fetch(tileId)
}

@Suppress("UNUSED_PARAMETER")
@OptIn(ExperimentalForeignApi::class)
private fun customGeometryCancelTile(
  userData: COpaquePointer?,
  tileId: CValue<mln_canonical_tile_id>,
) {
  userData?.asStableRef<CustomGeometrySourceState>()?.get()?.cancel(tileId)
}
