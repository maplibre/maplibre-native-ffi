package org.maplibre.nativeffi.runtime

import cnames.structs.mln_runtime
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVarOf
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE
import org.maplibre.nativeffi.internal.c.mln_runtime_create
import org.maplibre.nativeffi.internal.c.mln_runtime_destroy
import org.maplibre.nativeffi.internal.c.mln_runtime_options
import org.maplibre.nativeffi.internal.c.mln_runtime_options_default
import org.maplibre.nativeffi.internal.c.mln_runtime_run_once
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.status.Status

/** Owned native runtime handle. Close it on the owner thread. */
@OptIn(ExperimentalForeignApi::class)
public class RuntimeHandle private constructor(handle: CPointer<mln_runtime>) : AutoCloseable {
  private val state = HandleState("RuntimeHandle", handle)

  public fun runOnce() {
    Status.check(mln_runtime_run_once(state.requireLive()))
  }

  override fun close() {
    state.closeOnce(::mln_runtime_destroy)
  }

  public fun isClosed(): Boolean = state.isReleased()

  internal fun nativeHandle(): CPointer<mln_runtime> = state.requireLive()

  internal fun nativeAddress(): Long = state.address()

  public companion object {
    public fun create(): RuntimeHandle = create(RuntimeOptions())

    public fun create(options: RuntimeOptions): RuntimeHandle = memScoped {
      val nativeOptions = alloc<mln_runtime_options>()
      mln_runtime_options_default().place(nativeOptions.ptr)
      options.assetPath?.let { nativeOptions.asset_path = MemoryUtil.cString(this, it) }
      options.cachePath?.let { nativeOptions.cache_path = MemoryUtil.cString(this, it) }
      options.maximumCacheSize?.let {
        nativeOptions.flags = nativeOptions.flags or MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE
        nativeOptions.maximum_cache_size = it.toULong()
      }

      val outRuntime = alloc<CPointerVarOf<CPointer<mln_runtime>>>()
      outRuntime.value = null
      Status.check(mln_runtime_create(nativeOptions.ptr, outRuntime.ptr))
      RuntimeHandle(requireNotNull(outRuntime.value) { "mln_runtime_create returned null" })
    }
  }
}
