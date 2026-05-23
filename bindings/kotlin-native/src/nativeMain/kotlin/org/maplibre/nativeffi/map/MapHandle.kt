package org.maplibre.nativeffi.map

import cnames.structs.mln_map
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVarOf
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.internal.c.mln_map_create
import org.maplibre.nativeffi.internal.c.mln_map_destroy
import org.maplibre.nativeffi.internal.c.mln_map_options
import org.maplibre.nativeffi.internal.c.mln_map_options_default
import org.maplibre.nativeffi.internal.c.mln_map_set_style_json
import org.maplibre.nativeffi.internal.c.mln_map_set_style_url
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.runtime.RuntimeHandle

/** Owned native map handle. Close it on the map owner thread. */
@OptIn(ExperimentalForeignApi::class)
public class MapHandle
private constructor(private val runtime: RuntimeHandle, handle: CPointer<mln_map>) : AutoCloseable {
  private val state = HandleState("MapHandle", handle, runtime)

  public fun setStyleUrl(url: String) {
    MemoryUtil.requireValidCString(url)
    Status.check(mln_map_set_style_url(state.requireLive(), url))
  }

  public fun setStyleJson(json: String) {
    MemoryUtil.requireValidCString(json)
    Status.check(mln_map_set_style_json(state.requireLive(), json))
  }

  override fun close() {
    state.closeOnce(::mln_map_destroy)
  }

  public fun isClosed(): Boolean = state.isReleased()

  public fun runtime(): RuntimeHandle = runtime

  internal fun nativeHandle(): CPointer<mln_map> = state.requireLive()

  internal fun nativeAddress(): Long = state.address()

  public companion object {
    public fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle = memScoped {
      val nativeOptions = alloc<mln_map_options>()
      mln_map_options_default().place(nativeOptions.ptr)
      options.width?.let { nativeOptions.width = it }
      options.height?.let { nativeOptions.height = it }
      options.scaleFactor?.let { nativeOptions.scale_factor = it }
      options.mapMode?.let { nativeOptions.map_mode = it.nativeValue }

      val outMap = alloc<CPointerVarOf<CPointer<mln_map>>>()
      outMap.value = null
      Status.check(mln_map_create(runtime.nativeHandle(), nativeOptions.ptr, outMap.ptr))
      MapHandle(runtime, requireNotNull(outMap.value) { "mln_map_create returned null" })
    }
  }
}
