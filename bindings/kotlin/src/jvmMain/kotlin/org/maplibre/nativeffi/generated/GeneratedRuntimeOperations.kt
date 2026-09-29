// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import java.lang.foreign.Arena
import java.lang.foreign.ValueLayout
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.async.adoptOwned
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.loader.CompletionBridge
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

public actual abstract class GeneratedRuntimeOperations internal actual constructor() {
  internal actual val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()

  internal abstract fun bindingRuntimeHandle(): Long

  internal abstract fun bindingCloseRuntime(call: (Long) -> Unit)

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
            NativeAccess.completionValue(result, ValueLayout.JAVA_LONG.byteSize())
              .get(ValueLayout.JAVA_LONG, 0),
            this@GeneratedRuntimeOperations as org.maplibre.nativeffi.generated.RuntimeHandle,
          )
        },
        { it.dispose() },
        { result ->
          GeneratedOwnerDisposal.map(
            NativeAccess.completionValue(result, ValueLayout.JAVA_LONG.byteSize())
              .get(ValueLayout.JAVA_LONG, 0)
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_create(
                bindingRuntimeHandle(),
                GeneratedValues.writeMapOptions(arena, options),
                completion,
                diagnostic,
              )
            }
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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_runtime_barrier(bindingRuntimeHandle(), completion, diagnostic)
          }
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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_runtime_clear_http_header_transform(
              bindingRuntimeHandle(),
              completion,
              diagnostic,
            )
          }
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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_runtime_clear_resource_provider(
              bindingRuntimeHandle(),
              completion,
              diagnostic,
            )
          }
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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_runtime_clear_resource_transform(
              bindingRuntimeHandle(),
              completion,
              diagnostic,
            )
          }
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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_runtime_dispose(owner, diagnostic)
          }
        }
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
      return Arena.ofConfined().use { arena ->
        val output = arena.allocate(ValueLayout.JAVA_LONG)
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_runtime_drain_events(bindingRuntimeHandle(), output, diagnostic)
        }
        adoptOwned(
          output.get(ValueLayout.JAVA_LONG, 0),
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
      return Arena.ofConfined().use { arena ->
        val output = arena.allocate(ValueLayout.JAVA_LONG)
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_runtime_get_event_mask(bindingRuntimeHandle(), output, diagnostic)
        }
        RuntimeEventMask(output.get(ValueLayout.JAVA_LONG, 0).toULong())
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
            NativeAccess.completionValue(result, mln_offline_region_info.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_runtime_offline_region_create(
                bindingRuntimeHandle(),
                GeneratedValues.writeOfflineRegionDefinition(arena, definition),
                GeneratedValues.rawBytes(arena, metadata),
                metadata.size.toLong(),
                completion,
                diagnostic,
              )
            }
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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_runtime_offline_region_delete(
              bindingRuntimeHandle(),
              regionId,
              completion,
              diagnostic,
            )
          }
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
          if (mln_completion_result.value_count(result) == 0L) null
          else
            GeneratedValues.readOfflineRegionInfo(
              NativeAccess.completionValue(result, mln_offline_region_info.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_runtime_offline_region_get(
                bindingRuntimeHandle(),
                regionId,
                completion,
                diagnostic,
              )
            }
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
            NativeAccess.completionValue(result, mln_offline_region_status.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_runtime_offline_region_get_status(
                bindingRuntimeHandle(),
                regionId,
                completion,
                diagnostic,
              )
            }
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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_runtime_offline_region_invalidate(
              bindingRuntimeHandle(),
              regionId,
              completion,
              diagnostic,
            )
          }
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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_runtime_offline_region_set_download_state(
              bindingRuntimeHandle(),
              regionId,
              state.rawValue.toInt(),
              completion,
              diagnostic,
            )
          }
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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_runtime_offline_region_set_observed(
              bindingRuntimeHandle(),
              regionId,
              observed,
              completion,
              diagnostic,
            )
          }
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
            NativeAccess.completionValue(result, mln_offline_region_info.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_runtime_offline_region_update_metadata(
                bindingRuntimeHandle(),
                regionId,
                GeneratedValues.rawBytes(arena, metadata),
                metadata.size.toLong(),
                completion,
                diagnostic,
              )
            }
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
            mln_completion_result.value(result),
            mln_completion_result.value_count(result),
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_runtime_offline_regions_list(
                bindingRuntimeHandle(),
                completion,
                diagnostic,
              )
            }
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
            mln_completion_result.value(result),
            mln_completion_result.value_count(result),
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_runtime_offline_regions_merge_database(
                bindingRuntimeHandle(),
                GeneratedValues.cString(arena, sideDatabasePath),
                completion,
                diagnostic,
              )
            }
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
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_runtime_release(owner, completion, diagnostic)
            }
          }
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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_runtime_run_ambient_cache_operation(
              bindingRuntimeHandle(),
              operation.rawValue.toInt(),
              completion,
              diagnostic,
            )
          }
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
      return Arena.ofConfined().use { arena ->
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_runtime_set_event_mask(
            bindingRuntimeHandle(),
            mask.rawValue.toLong(),
            diagnostic,
          )
        }
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
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_runtime_set_http_header_transform(
                bindingRuntimeHandle(),
                GeneratedCallbacks.prepareHttpHeaderTransform(arena, transform, registrations),
                completion,
                diagnostic,
              )
            }
            registrations.accept(bindingCallbacks)
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
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_runtime_set_maximum_ambient_cache_size(
              bindingRuntimeHandle(),
              size.toLong(),
              completion,
              diagnostic,
            )
          }
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
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_runtime_set_resource_provider(
                bindingRuntimeHandle(),
                GeneratedCallbacks.prepareResourceProvider(arena, provider, registrations),
                completion,
                diagnostic,
              )
            }
            registrations.accept(bindingCallbacks)
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
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_runtime_set_resource_transform(
                bindingRuntimeHandle(),
                GeneratedCallbacks.prepareResourceTransform(arena, transform, registrations),
                completion,
                diagnostic,
              )
            }
            registrations.accept(bindingCallbacks)
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
