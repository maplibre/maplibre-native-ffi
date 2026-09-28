// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.bytedeco.javacpp.*
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.async.adoptOwned
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

public actual abstract class GeneratedRuntimeOperations internal actual constructor() {
  internal actual val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()

  internal abstract fun bindingRuntimeHandle(): Long

  internal abstract fun bindingCloseRuntime(call: (Long) -> Int)

  internal abstract fun bindingRetireRuntime(call: (Long) -> Deferred<Unit>): Deferred<Unit>

  public actual fun mapCreate(
    options: MapOptions
  ): Deferred<org.maplibre.nativeffi.generated.MapHandle> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_map_create",
      )
      return CompletionBridge.submitOwned(
        { result ->
          MapHandle(
            LongPointer(result.value()).get(),
            this@GeneratedRuntimeOperations as org.maplibre.nativeffi.generated.RuntimeHandle,
          )
        },
        { it.dispose() },
        { result -> GeneratedOwnerDisposal.map(LongPointer(result.value()).get()) },
        { completion ->
          PointerScope().use { arena ->
            MaplibreNativeC.mln_map_create(
              bindingRuntimeHandle(),
              GeneratedValues.writeMapOptions(arena, options),
              completion,
            )
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun barrier(): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_barrier",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_runtime_barrier(bindingRuntimeHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun clearHttpHeaderTransform(): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_clear_http_header_transform",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_runtime_clear_http_header_transform(
            bindingRuntimeHandle(),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun clearResourceProvider(): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_clear_resource_provider",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_runtime_clear_resource_provider(bindingRuntimeHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun clearResourceTransform(): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_clear_resource_transform",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_runtime_clear_resource_transform(bindingRuntimeHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun dispose(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_runtime_dispose"
      )
      return bindingCloseRuntime { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_runtime_dispose",
        )
        PointerScope().use { arena -> MaplibreNativeC.mln_runtime_dispose(owner) }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun drainEvents(): org.maplibre.nativeffi.generated.EventBatchHandle {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_drain_events",
      )
      return PointerScope().use { arena ->
        val output = LongPointer(1L).put(0L)
        BindingStatus.check(
          MaplibreNativeC.mln_runtime_drain_events(bindingRuntimeHandle(), output)
        )
        adoptOwned(
          output.get(),
          { GeneratedOwnerDisposal.eventBatch(it) },
          { EventBatchHandle(it) },
        )
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getEventMask(): RuntimeEventMask {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_get_event_mask",
      )
      return PointerScope().use { arena ->
        val output = LongPointer(1L)
        BindingStatus.check(
          MaplibreNativeC.mln_runtime_get_event_mask(bindingRuntimeHandle(), output)
        )
        RuntimeEventMask(output.get().toULong())
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionCreate(
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_create",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readOfflineRegionInfo(
            MaplibreNativeC.mln_offline_region_info(result.value())
          )
        },
        { completion ->
          PointerScope().use { arena ->
            MaplibreNativeC.mln_runtime_offline_region_create(
              bindingRuntimeHandle(),
              GeneratedValues.writeOfflineRegionDefinition(arena, definition),
              GeneratedValues.rawBytes(arena, metadata),
              metadata.size.toLong(),
              completion,
            )
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionDelete(regionId: Long): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_delete",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_runtime_offline_region_delete(
            bindingRuntimeHandle(),
            regionId,
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionGet(regionId: Long): Deferred<OfflineRegionInfo?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_get",
      )
      return CompletionBridge.submit(
        { result ->
          if (result.value_count() == 0L) null
          else
            GeneratedValues.readOfflineRegionInfo(
              MaplibreNativeC.mln_offline_region_info(result.value())
            )
        },
        { completion ->
          PointerScope().use { arena ->
            MaplibreNativeC.mln_runtime_offline_region_get(
              bindingRuntimeHandle(),
              regionId,
              completion,
            )
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionGetStatus(regionId: Long): Deferred<OfflineRegionStatus> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_get_status",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readOfflineRegionStatus(
            MaplibreNativeC.mln_offline_region_status(result.value())
          )
        },
        { completion ->
          PointerScope().use { arena ->
            MaplibreNativeC.mln_runtime_offline_region_get_status(
              bindingRuntimeHandle(),
              regionId,
              completion,
            )
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionInvalidate(regionId: Long): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_invalidate",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_runtime_offline_region_invalidate(
            bindingRuntimeHandle(),
            regionId,
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionSetDownloadState(
    regionId: Long,
    state: OfflineRegionDownloadState,
  ): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_set_download_state",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_runtime_offline_region_set_download_state(
            bindingRuntimeHandle(),
            regionId,
            state.rawValue.toInt(),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionSetObserved(regionId: Long, observed: Boolean): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_set_observed",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_runtime_offline_region_set_observed(
            bindingRuntimeHandle(),
            regionId,
            observed,
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionUpdateMetadata(
    regionId: Long,
    metadata: ByteArray,
  ): Deferred<OfflineRegionInfo> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_update_metadata",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readOfflineRegionInfo(
            MaplibreNativeC.mln_offline_region_info(result.value())
          )
        },
        { completion ->
          PointerScope().use { arena ->
            MaplibreNativeC.mln_runtime_offline_region_update_metadata(
              bindingRuntimeHandle(),
              regionId,
              GeneratedValues.rawBytes(arena, metadata),
              metadata.size.toLong(),
              completion,
            )
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionsList(): Deferred<List<OfflineRegionInfo>> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_regions_list",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readOfflineRegionInfoArray(
            MaplibreNativeC.mln_offline_region_info(result.value()),
            result.value_count(),
          )
        },
        { completion ->
          PointerScope().use { arena ->
            MaplibreNativeC.mln_runtime_offline_regions_list(bindingRuntimeHandle(), completion)
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionsMergeDatabase(
    sideDatabasePath: String
  ): Deferred<List<OfflineRegionInfo>> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_regions_merge_database",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readOfflineRegionInfoArray(
            MaplibreNativeC.mln_offline_region_info(result.value()),
            result.value_count(),
          )
        },
        { completion ->
          PointerScope().use { arena ->
            MaplibreNativeC.mln_runtime_offline_regions_merge_database(
              bindingRuntimeHandle(),
              GeneratedValues.cString(arena, sideDatabasePath),
              completion,
            )
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun release(): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_runtime_release"
      )
      return bindingRetireRuntime { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_runtime_release",
        )
        CompletionBridge.unitChecked { completion ->
          PointerScope().use { arena -> MaplibreNativeC.mln_runtime_release(owner, completion) }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun runAmbientCacheOperation(operation: AmbientCacheOperation): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_run_ambient_cache_operation",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_runtime_run_ambient_cache_operation(
            bindingRuntimeHandle(),
            operation.rawValue.toInt(),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun setEventMask(mask: RuntimeEventMask): Unit {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_set_event_mask",
      )
      return PointerScope().use { arena ->
        BindingStatus.check(
          MaplibreNativeC.mln_runtime_set_event_mask(bindingRuntimeHandle(), mask.rawValue.toLong())
        )
        Unit
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun setHttpHeaderTransform(transform: HttpHeaderTransform): Deferred<Unit> {
    try {
      return org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope().use {
        registrations ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingRuntimeHandle().toLong(),
          "mln_runtime_set_http_header_transform",
        )
        CompletionBridge.unit { completion ->
          PointerScope().use { arena ->
            val status =
              MaplibreNativeC.mln_runtime_set_http_header_transform(
                bindingRuntimeHandle(),
                GeneratedCallbacks.prepareHttpHeaderTransform(arena, transform, registrations),
                completion,
              )
            if (status == 0) registrations.accept(bindingCallbacks)
            status
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun setMaximumAmbientCacheSize(size: ULong): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_set_maximum_ambient_cache_size",
      )
      return CompletionBridge.unit { completion ->
        PointerScope().use { arena ->
          MaplibreNativeC.mln_runtime_set_maximum_ambient_cache_size(
            bindingRuntimeHandle(),
            size.toLong(),
            completion,
          )
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun setResourceProvider(provider: ResourceProvider): Deferred<Unit> {
    try {
      return org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope().use {
        registrations ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingRuntimeHandle().toLong(),
          "mln_runtime_set_resource_provider",
        )
        CompletionBridge.unit { completion ->
          PointerScope().use { arena ->
            val status =
              MaplibreNativeC.mln_runtime_set_resource_provider(
                bindingRuntimeHandle(),
                GeneratedCallbacks.prepareResourceProvider(arena, provider, registrations),
                completion,
              )
            if (status == 0) registrations.accept(bindingCallbacks)
            status
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun setResourceTransform(transform: ResourceTransform): Deferred<Unit> {
    try {
      return org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope().use {
        registrations ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingRuntimeHandle().toLong(),
          "mln_runtime_set_resource_transform",
        )
        CompletionBridge.unit { completion ->
          PointerScope().use { arena ->
            val status =
              MaplibreNativeC.mln_runtime_set_resource_transform(
                bindingRuntimeHandle(),
                GeneratedCallbacks.prepareResourceTransform(arena, transform, registrations),
                completion,
              )
            if (status == 0) registrations.accept(bindingCallbacks)
            status
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
