// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.async.adoptOwned
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

@OptIn(ExperimentalForeignApi::class)
public actual abstract class GeneratedRuntimeOperations internal actual constructor() {
  internal actual val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()

  internal abstract fun bindingRuntimeHandle(): ULong

  internal abstract fun bindingCloseRuntime(call: (ULong) -> Int)

  internal abstract fun bindingRetireRuntime(call: (ULong) -> Deferred<Unit>): Deferred<Unit>

  public actual fun mapCreate(
    options: MapOptions
  ): Deferred<org.maplibre.nativeffi.generated.MapHandle> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_map_create",
      )
      return CompletionBridge.submitOwned(
        { result ->
          MapHandle(
            result.pointed.value!!.reinterpret<ULongVar>().pointed.value,
            this@GeneratedRuntimeOperations as org.maplibre.nativeffi.generated.RuntimeHandle,
          )
        },
        { it.dispose() },
        { result ->
          GeneratedOwnerDisposal.map(
            result.pointed.value!!.reinterpret<ULongVar>().pointed.value.toLong()
          )
        },
        { completion ->
          memScoped {
            val arena = this
            mln_map_create(
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_barrier",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_runtime_barrier(bindingRuntimeHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun clearHttpHeaderTransform(): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_clear_http_header_transform",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_runtime_clear_http_header_transform(bindingRuntimeHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun clearResourceProvider(): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_clear_resource_provider",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_runtime_clear_resource_provider(bindingRuntimeHandle(), completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun clearResourceTransform(): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_clear_resource_transform",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_runtime_clear_resource_transform(bindingRuntimeHandle(), completion)
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
        memScoped {
          val arena = this
          mln_runtime_dispose(owner)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun drainEvents(): org.maplibre.nativeffi.generated.EventBatchHandle {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_drain_events",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<ULongVar>().also { it.value = 0uL }
        BindingStatus.check(mln_runtime_drain_events(bindingRuntimeHandle(), output.ptr))
        adoptOwned(
          output.value,
          { GeneratedOwnerDisposal.eventBatch(it.toLong()) },
          { EventBatchHandle(it) },
        )
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getEventMask(): RuntimeEventMask {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_get_event_mask",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<ULongVar>()
        BindingStatus.check(mln_runtime_get_event_mask(bindingRuntimeHandle(), output.ptr))
        RuntimeEventMask(output.value)
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_create",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readOfflineRegionInfo(
            result.pointed.value!!.reinterpret<mln_offline_region_info>().pointed
          )
        },
        { completion ->
          memScoped {
            val arena = this
            mln_runtime_offline_region_create(
              bindingRuntimeHandle(),
              GeneratedValues.writeOfflineRegionDefinition(arena, definition),
              GeneratedValues.rawBytes(arena, metadata).reinterpret(),
              metadata.size.convert(),
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_delete",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_runtime_offline_region_delete(bindingRuntimeHandle(), regionId, completion)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionGet(regionId: Long): Deferred<OfflineRegionInfo?> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_get",
      )
      return CompletionBridge.submit(
        { result ->
          if (result.pointed.value_count.toLong() == 0L) null
          else
            GeneratedValues.readOfflineRegionInfo(
              result.pointed.value!!.reinterpret<mln_offline_region_info>().pointed
            )
        },
        { completion ->
          memScoped {
            val arena = this
            mln_runtime_offline_region_get(bindingRuntimeHandle(), regionId, completion)
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionGetStatus(regionId: Long): Deferred<OfflineRegionStatus> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_get_status",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readOfflineRegionStatus(
            result.pointed.value!!.reinterpret<mln_offline_region_status>().pointed
          )
        },
        { completion ->
          memScoped {
            val arena = this
            mln_runtime_offline_region_get_status(bindingRuntimeHandle(), regionId, completion)
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun offlineRegionInvalidate(regionId: Long): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_invalidate",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_runtime_offline_region_invalidate(bindingRuntimeHandle(), regionId, completion)
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_set_download_state",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_runtime_offline_region_set_download_state(
            bindingRuntimeHandle(),
            regionId,
            state.rawValue,
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_set_observed",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_runtime_offline_region_set_observed(
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_region_update_metadata",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readOfflineRegionInfo(
            result.pointed.value!!.reinterpret<mln_offline_region_info>().pointed
          )
        },
        { completion ->
          memScoped {
            val arena = this
            mln_runtime_offline_region_update_metadata(
              bindingRuntimeHandle(),
              regionId,
              GeneratedValues.rawBytes(arena, metadata).reinterpret(),
              metadata.size.convert(),
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_regions_list",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readOfflineRegionInfoArray(
            result.pointed.value?.reinterpret<mln_offline_region_info>(),
            result.pointed.value_count.toULong(),
          )
        },
        { completion ->
          memScoped {
            val arena = this
            mln_runtime_offline_regions_list(bindingRuntimeHandle(), completion)
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_offline_regions_merge_database",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readOfflineRegionInfoArray(
            result.pointed.value?.reinterpret<mln_offline_region_info>(),
            result.pointed.value_count.toULong(),
          )
        },
        { completion ->
          memScoped {
            val arena = this
            mln_runtime_offline_regions_merge_database(
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
          memScoped {
            val arena = this
            mln_runtime_release(owner, completion)
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun runAmbientCacheOperation(operation: AmbientCacheOperation): Deferred<Unit> {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_run_ambient_cache_operation",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_runtime_run_ambient_cache_operation(
            bindingRuntimeHandle(),
            operation.rawValue,
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_set_event_mask",
      )
      return memScoped {
        val arena = this
        BindingStatus.check(mln_runtime_set_event_mask(bindingRuntimeHandle(), mask.rawValue))
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
          memScoped {
            val arena = this
            val status =
              mln_runtime_set_http_header_transform(
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRuntimeHandle().toLong(),
        "mln_runtime_set_maximum_ambient_cache_size",
      )
      return CompletionBridge.unit { completion ->
        memScoped {
          val arena = this
          mln_runtime_set_maximum_ambient_cache_size(bindingRuntimeHandle(), size, completion)
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
          memScoped {
            val arena = this
            val status =
              mln_runtime_set_resource_provider(
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
          memScoped {
            val arena = this
            val status =
              mln_runtime_set_resource_transform(
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
