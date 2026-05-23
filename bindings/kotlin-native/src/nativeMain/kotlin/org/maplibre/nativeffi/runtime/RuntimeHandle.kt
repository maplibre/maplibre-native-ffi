package org.maplibre.nativeffi.runtime

import cnames.structs.mln_runtime
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVarOf
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.UIntVar
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.toLong
import kotlinx.cinterop.value
import org.maplibre.nativeffi.internal.c.MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE
import org.maplibre.nativeffi.internal.c.mln_network_status_get
import org.maplibre.nativeffi.internal.c.mln_network_status_set
import org.maplibre.nativeffi.internal.c.mln_runtime_create
import org.maplibre.nativeffi.internal.c.mln_runtime_destroy
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_offline_operation_discard
import org.maplibre.nativeffi.internal.c.mln_runtime_options
import org.maplibre.nativeffi.internal.c.mln_runtime_options_default
import org.maplibre.nativeffi.internal.c.mln_runtime_poll_event
import org.maplibre.nativeffi.internal.c.mln_runtime_run_ambient_cache_operation_start
import org.maplibre.nativeffi.internal.c.mln_runtime_run_once
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.RuntimeStructs
import org.maplibre.nativeffi.map.MapHandle

/** Owned native runtime handle. Close it on the owner thread. */
@OptIn(ExperimentalForeignApi::class)
public class RuntimeHandle private constructor(handle: CPointer<mln_runtime>) : AutoCloseable {
  private val state = HandleState("RuntimeHandle", handle)
  private val liveMaps = mutableMapOf<Long, MapHandle>()

  public fun runOnce() {
    Status.check(mln_runtime_run_once(state.requireLive()))
  }

  public fun startAmbientCacheOperation(
    operation: AmbientCacheOperation
  ): OfflineOperationHandle<Unit> = memScoped {
    val outOperationId = alloc<ULongVar>()
    Status.check(
      mln_runtime_run_ambient_cache_operation_start(
        state.requireLive(),
        operation.nativeValue,
        outOperationId.ptr,
      )
    )
    OfflineOperationHandle(
      this@RuntimeHandle,
      outOperationId.value,
      OfflineOperationKind.AMBIENT_CACHE,
      OfflineOperationResultKind.NONE,
    )
  }

  internal fun discardOfflineOperation(operation: OfflineOperationHandle<*>) {
    val id = operation.requireLive(this)
    Status.check(mln_runtime_offline_operation_discard(state.requireLive(), id))
    operation.markConsumed()
  }

  public fun pollEvent(): RuntimeEvent? = memScoped {
    val event = alloc<mln_runtime_event>()
    event.size = sizeOf<mln_runtime_event>().toUInt()
    val hasEvent = alloc<BooleanVar>()
    hasEvent.value = false
    Status.check(mln_runtime_poll_event(state.requireLive(), event.ptr, hasEvent.ptr))
    if (!hasEvent.value) {
      return@memScoped null
    }

    val sourceType = RuntimeEventSourceType.fromNative(event.source_type)
    val sourceAddress = event.source?.rawValue?.toLong()
    RuntimeEvent(
      RuntimeEventType.fromNative(event.type),
      event.type,
      sourceType,
      event.source_type,
      if (sourceType == RuntimeEventSourceType.RUNTIME) this@RuntimeHandle else null,
      if (sourceType == RuntimeEventSourceType.MAP && sourceAddress != null) liveMaps[sourceAddress]
      else null,
      event.code,
      event.payload_type,
      RuntimeStructs.payload(event),
      RuntimeStructs.message(event),
    )
  }

  override fun close() {
    state.closeOnce(::mln_runtime_destroy)
  }

  public fun isClosed(): Boolean = state.isReleased()

  internal fun nativeHandle(): CPointer<mln_runtime> = state.requireLive()

  internal fun nativeAddress(): Long = state.address()

  internal fun registerMap(map: MapHandle) {
    liveMaps[map.nativeAddress()] = map
  }

  internal fun unregisterMap(map: MapHandle) {
    val address = map.nativeAddress()
    if (liveMaps[address] === map) {
      liveMaps.remove(address)
    }
  }

  public companion object {
    public fun networkStatus(): NetworkStatus = memScoped {
      val outStatus = alloc<UIntVar>()
      Status.check(mln_network_status_get(outStatus.ptr))
      NetworkStatus.fromNative(outStatus.value)
    }

    public fun setNetworkStatus(status: NetworkStatus) {
      Status.check(mln_network_status_set(status.nativeValue))
    }

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
