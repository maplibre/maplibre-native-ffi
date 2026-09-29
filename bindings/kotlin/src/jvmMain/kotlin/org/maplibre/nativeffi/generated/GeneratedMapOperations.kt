// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
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
import org.maplibre.nativeffi.runtime.CommandCompletion

public actual abstract class GeneratedMapOperations internal actual constructor() {
  internal actual val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()

  internal abstract fun bindingMapHandle(): Long

  internal abstract fun bindingCloseMap(call: (Long) -> Unit)

  internal abstract fun bindingRetireMap(call: (Long) -> Deferred<Unit>): Deferred<Unit>

  public actual fun addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_color_relief_layer",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_color_relief_layer(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.stringView(arena, (beforeLayerId ?: "")),
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

  public actual fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): Deferred<CommandCompletion> {
    try {
      return org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope().use {
        registrations ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingMapHandle().toLong(),
          "mln_map_add_custom_geometry_source",
        )
        CompletionBridge.command { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_add_custom_geometry_source(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, sourceId),
                GeneratedCallbacks.prepareCustomGeometrySourceOptions(
                  arena,
                  options,
                  registrations,
                ),
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

  public actual fun addCustomMvtVectorSource(
    sourceId: String,
    options: CustomMvtVectorSourceOptions,
  ): Deferred<CommandCompletion> {
    try {
      return org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope().use {
        registrations ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingMapHandle().toLong(),
          "mln_map_add_custom_mvt_vector_source",
        )
        CompletionBridge.command { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_add_custom_mvt_vector_source(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, sourceId),
                GeneratedCallbacks.prepareCustomMvtVectorSourceOptions(
                  arena,
                  options,
                  registrations,
                ),
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

  public actual fun addGeojsonSourceData(
    sourceId: String,
    data: org.maplibre.nativeffi.generated.GeojsonSourceDataHandle,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_geojson_source_data",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_geojson_source_data(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              data.bindingGeojsonSourceDataHandle(),
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

  public actual fun addGeojsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeojsonSourceOptions?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_geojson_source_url",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_geojson_source_url(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.stringView(arena, url),
              if (options == null) MemorySegment.NULL
              else GeneratedValues.writeGeojsonSourceOptions(arena, options!!),
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

  public actual fun addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_hillshade_layer",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_hillshade_layer(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.stringView(arena, (beforeLayerId ?: "")),
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

  public actual fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_image_source_image",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_image_source_image(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeLatLngArray(arena, coordinates),
              coordinates.size.toLong(),
              GeneratedValues.writePremultipliedRgba8Image(arena, image),
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

  public actual fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_image_source_url",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_image_source_url(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeLatLngArray(arena, coordinates),
              coordinates.size.toLong(),
              GeneratedValues.stringView(arena, url),
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

  public actual fun addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_location_indicator_layer",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_location_indicator_layer(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              GeneratedValues.stringView(arena, (beforeLayerId ?: "")),
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

  public actual fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_raster_dem_source_tiles",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_raster_dem_source_tiles(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeBufferViewArray(arena, tiles),
              tiles.size.toLong(),
              if (options == null) MemorySegment.NULL
              else GeneratedValues.writeStyleTileSourceOptions(arena, options!!),
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

  public actual fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_raster_dem_source_url",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_raster_dem_source_url(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.stringView(arena, url),
              if (options == null) MemorySegment.NULL
              else GeneratedValues.writeStyleTileSourceOptions(arena, options!!),
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

  public actual fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_raster_source_tiles",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_raster_source_tiles(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeBufferViewArray(arena, tiles),
              tiles.size.toLong(),
              if (options == null) MemorySegment.NULL
              else GeneratedValues.writeStyleTileSourceOptions(arena, options!!),
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

  public actual fun addRasterSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_raster_source_url",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_raster_source_url(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.stringView(arena, url),
              if (options == null) MemorySegment.NULL
              else GeneratedValues.writeStyleTileSourceOptions(arena, options!!),
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

  public actual fun addStyleLayerJson(
    layerJson: ByteArray,
    beforeLayerId: String?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_style_layer_json",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_style_layer_json(
              bindingMapHandle(),
              GeneratedValues.byteView(arena, layerJson),
              GeneratedValues.stringView(arena, (beforeLayerId ?: "")),
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

  public actual fun addStyleSourceJson(
    sourceId: String,
    sourceJson: ByteArray,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_style_source_json",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_style_source_json(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.byteView(arena, sourceJson),
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

  public actual fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_vector_source_tiles",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_vector_source_tiles(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeBufferViewArray(arena, tiles),
              tiles.size.toLong(),
              if (options == null) MemorySegment.NULL
              else GeneratedValues.writeStyleTileSourceOptions(arena, options!!),
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

  public actual fun addVectorSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_add_vector_source_url",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_add_vector_source_url(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.stringView(arena, url),
              if (options == null) MemorySegment.NULL
              else GeneratedValues.writeStyleTileSourceOptions(arena, options!!),
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

  public actual fun applyCameraDelta(delta: CameraDelta): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_apply_camera_delta",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_apply_camera_delta(
              bindingMapHandle(),
              GeneratedValues.writeCameraDelta(arena, delta),
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

  public actual fun cameraForGeometry(
    geometry: ByteArray,
    fitOptions: CameraFitOptions?,
  ): Deferred<CameraOptions> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_camera_for_geometry",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readCameraOptions(
            NativeAccess.completionValue(result, mln_camera_options.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_camera_for_geometry(
                bindingMapHandle(),
                GeneratedValues.byteView(arena, geometry),
                if (fitOptions == null) MemorySegment.NULL
                else GeneratedValues.writeCameraFitOptions(arena, fitOptions!!),
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

  public actual fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): Deferred<CameraOptions> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_camera_for_lat_lng_bounds",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readCameraOptions(
            NativeAccess.completionValue(result, mln_camera_options.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_camera_for_lat_lng_bounds(
                bindingMapHandle(),
                GeneratedValues.writeLatLngBounds(arena, bounds),
                if (fitOptions == null) MemorySegment.NULL
                else GeneratedValues.writeCameraFitOptions(arena, fitOptions!!),
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

  public actual fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): Deferred<CameraOptions> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_camera_for_lat_lngs",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readCameraOptions(
            NativeAccess.completionValue(result, mln_camera_options.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_camera_for_lat_lngs(
                bindingMapHandle(),
                GeneratedValues.writeLatLngArray(arena, coordinates),
                coordinates.size.toLong(),
                if (fitOptions == null) MemorySegment.NULL
                else GeneratedValues.writeCameraFitOptions(arena, fitOptions!!),
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

  public actual fun cameraQuery(): Deferred<CameraQueryResult> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_camera_query",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readCameraQueryResult(
            NativeAccess.completionValue(result, mln_camera_query_result.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_camera_query(bindingMapHandle(), completion, diagnostic)
            }
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun cameraSnapshotGet(): MapCameraSnapshotGetResult {
    try {
      return Arena.ofConfined().use { arena ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          bindingMapHandle().toLong(),
          "mln_map_camera_snapshot_get",
        )
        val out0 = mln_camera_options.allocate(arena)
        mln_camera_options.size(out0, mln_camera_options.sizeof().toInt())
        val out1 = arena.allocate(ValueLayout.JAVA_LONG)
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_map_camera_snapshot_get(bindingMapHandle(), out0, out1, diagnostic)
        }
        MapCameraSnapshotGetResult(
          camera = GeneratedValues.readCameraOptions(out0),
          generation = out1.get(ValueLayout.JAVA_LONG, 0).toULong(),
        )
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun cancelTransitions(): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_cancel_transitions",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_cancel_transitions(bindingMapHandle(), completion, diagnostic)
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun copyLayerSourceId(layerId: String): Deferred<String?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_copy_layer_source_id",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readString(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
            .takeIf { it.isNotEmpty() }
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_copy_layer_source_id(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, layerId),
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

  public actual fun copyLayerSourceLayer(layerId: String): Deferred<String?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_copy_layer_source_layer",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readString(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
            .takeIf { it.isNotEmpty() }
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_copy_layer_source_layer(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, layerId),
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

  public actual fun copyStyleImagePremultipliedRgba8(imageId: String): Deferred<ByteArray?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_copy_style_image_premultiplied_rgba8",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else if (
            mln_buffer_view
              .data(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
              .address() == 0L
          )
            null
          else
            GeneratedValues.readBytes(
              NativeAccess.completionValue(result, mln_buffer_view.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_copy_style_image_premultiplied_rgba8(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, imageId),
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

  public actual fun copyStyleImageStretches(imageId: String): Deferred<StyleImageStretchesResult?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_copy_style_image_stretches",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else
            GeneratedValues.readStyleImageStretchesResult(
              NativeAccess.completionValue(result, mln_style_image_stretches_result.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_copy_style_image_stretches(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, imageId),
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

  public actual fun copyStyleSourceAttribution(sourceId: String): Deferred<String?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_copy_style_source_attribution",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else if (
            mln_buffer_view
              .data(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
              .address() == 0L
          )
            null
          else
            GeneratedValues.readString(
              NativeAccess.completionValue(result, mln_buffer_view.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_copy_style_source_attribution(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, sourceId),
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

  public actual fun copyStyleSourceUrl(sourceId: String): Deferred<String?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_copy_style_source_url",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else if (
            mln_buffer_view
              .data(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
              .address() == 0L
          )
            null
          else
            GeneratedValues.readString(
              NativeAccess.completionValue(result, mln_buffer_view.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_copy_style_source_url(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, sourceId),
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

  public actual fun dispose(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation("mln_map_dispose")
      return bindingCloseMap { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_map_dispose",
        )
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_dispose(owner, diagnostic)
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun dumpDebugLogs(): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_dump_debug_logs",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_dump_debug_logs(bindingMapHandle(), completion, diagnostic)
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getFeatureState(selector: FeatureStateSelector): Deferred<ByteArray> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_feature_state",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readBytes(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_feature_state(
                bindingMapHandle(),
                GeneratedValues.writeFeatureStateSelector(arena, selector),
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

  public actual fun getGlobalState(): Deferred<ByteArray> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_global_state",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readBytes(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_global_state(bindingMapHandle(), completion, diagnostic)
            }
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getImageSourceCoordinates(sourceId: String): Deferred<List<LatLng>?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_image_source_coordinates",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value(result).address() == 0L) null
          else
            GeneratedValues.readLatLngArray(
              mln_completion_result.value(result),
              mln_completion_result.value_count(result),
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_image_source_coordinates(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, sourceId),
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

  public actual fun getLayerFilter(layerId: String): Deferred<ByteArray?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_layer_filter",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else if (
            mln_buffer_view
              .data(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
              .address() == 0L
          )
            null
          else
            GeneratedValues.readBytes(
              NativeAccess.completionValue(result, mln_buffer_view.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_layer_filter(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, layerId),
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

  public actual fun getLayerProperty(layerId: String, propertyName: String): Deferred<ByteArray?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_layer_property",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else if (
            mln_buffer_view
              .data(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
              .address() == 0L
          )
            null
          else
            GeneratedValues.readBytes(
              NativeAccess.completionValue(result, mln_buffer_view.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_layer_property(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, layerId),
                GeneratedValues.stringView(arena, propertyName),
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

  public actual fun getStyleImageInfo(imageId: String): Deferred<StyleImageResult?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_style_image_info",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else
            GeneratedValues.readStyleImageResult(
              NativeAccess.completionValue(result, mln_style_image_result.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_style_image_info(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, imageId),
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

  public actual fun getStyleLayerInfo(layerId: String): Deferred<StyleLayerResult?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_style_layer_info",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else
            GeneratedValues.readStyleLayerResult(
              NativeAccess.completionValue(result, mln_style_layer_result.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_style_layer_info(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, layerId),
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

  public actual fun getStyleLayerJson(layerId: String): Deferred<ByteArray?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_style_layer_json",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else if (
            mln_buffer_view
              .data(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
              .address() == 0L
          )
            null
          else
            GeneratedValues.readBytes(
              NativeAccess.completionValue(result, mln_buffer_view.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_style_layer_json(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, layerId),
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

  public actual fun getStyleLightProperty(propertyName: String): Deferred<ByteArray?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_style_light_property",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else if (
            mln_buffer_view
              .data(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
              .address() == 0L
          )
            null
          else
            GeneratedValues.readBytes(
              NativeAccess.completionValue(result, mln_buffer_view.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_style_light_property(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, propertyName),
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

  public actual fun getStyleSourceInfo(sourceId: String): Deferred<StyleSourceResult?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_style_source_info",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else
            GeneratedValues.readStyleSourceResult(
              NativeAccess.completionValue(result, mln_style_source_result.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_style_source_info(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, sourceId),
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

  public actual fun getStyleSourceTileUrls(sourceId: String): Deferred<StyleSourceTileUrlsResult?> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_style_source_tile_urls",
      )
      return CompletionBridge.submit(
        { result ->
          if (mln_completion_result.value_count(result) == 0L) null
          else
            GeneratedValues.readStyleSourceTileUrlsResult(
              NativeAccess.completionValue(result, mln_style_source_tile_urls_result.sizeof())
            )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_style_source_tile_urls(
                bindingMapHandle(),
                GeneratedValues.stringView(arena, sourceId),
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

  public actual fun getStyleTransitionOptions(): Deferred<StyleTransitionOptions> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_get_style_transition_options",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readStyleTransitionOptions(
            NativeAccess.completionValue(result, mln_style_transition_options.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_get_style_transition_options(
                bindingMapHandle(),
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

  public actual fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_invalidate_custom_geometry_source_region",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_invalidate_custom_geometry_source_region(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeLatLngBounds(arena, bounds),
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

  public actual fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_invalidate_custom_geometry_source_tile",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_invalidate_custom_geometry_source_tile(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeCanonicalTileId(arena, tileId),
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

  public actual fun invalidateCustomMvtVectorSourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_invalidate_custom_mvt_vector_source_tile",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_invalidate_custom_mvt_vector_source_tile(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeCanonicalTileId(arena, tileId),
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

  public actual fun latLngBoundsForCamera(camera: CameraOptions): Deferred<LatLngBounds> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_lat_lng_bounds_for_camera",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readLatLngBounds(
            NativeAccess.completionValue(result, mln_lat_lng_bounds.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_lat_lng_bounds_for_camera(
                bindingMapHandle(),
                GeneratedValues.writeCameraOptions(arena, camera),
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

  public actual fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): Deferred<LatLngBounds> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_lat_lng_bounds_for_camera_unwrapped",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readLatLngBounds(
            NativeAccess.completionValue(result, mln_lat_lng_bounds.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_lat_lng_bounds_for_camera_unwrapped(
                bindingMapHandle(),
                GeneratedValues.writeCameraOptions(arena, camera),
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

  public actual fun latLngForPixel(point: ScreenPoint): Deferred<LatLng> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_lat_lng_for_pixel",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readLatLng(NativeAccess.completionValue(result, mln_lat_lng.sizeof()))
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_lat_lng_for_pixel(
                bindingMapHandle(),
                GeneratedValues.writeScreenPoint(arena, point),
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

  public actual fun latLngForPixelUnwrapped(point: ScreenPoint): Deferred<LatLng> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_lat_lng_for_pixel_unwrapped",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readLatLng(NativeAccess.completionValue(result, mln_lat_lng.sizeof()))
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_lat_lng_for_pixel_unwrapped(
                bindingMapHandle(),
                GeneratedValues.writeScreenPoint(arena, point),
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

  public actual fun latLngsForPixels(points: List<ScreenPoint>): Deferred<List<LatLng>> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_lat_lngs_for_pixels",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readLatLngArray(
            mln_completion_result.value(result),
            mln_completion_result.value_count(result),
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_lat_lngs_for_pixels(
                bindingMapHandle(),
                GeneratedValues.writeScreenPointArray(arena, points),
                points.size.toLong(),
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

  public actual fun latLngsForPixelsUnwrapped(points: List<ScreenPoint>): Deferred<List<LatLng>> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_lat_lngs_for_pixels_unwrapped",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readLatLngArray(
            mln_completion_result.value(result),
            mln_completion_result.value_count(result),
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_lat_lngs_for_pixels_unwrapped(
                bindingMapHandle(),
                GeneratedValues.writeScreenPointArray(arena, points),
                points.size.toLong(),
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

  public actual fun listStyleLayerIds(): Deferred<List<String>> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_list_style_layer_ids",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readBufferViewArray(
            mln_completion_result.value(result),
            mln_completion_result.value_count(result),
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_list_style_layer_ids(
                bindingMapHandle(),
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

  public actual fun listStyleLayers(): Deferred<List<StyleLayerEntry>> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_list_style_layers",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readStyleLayerEntryArray(
            mln_completion_result.value(result),
            mln_completion_result.value_count(result),
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_list_style_layers(bindingMapHandle(), completion, diagnostic)
            }
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun listStyleSourceIds(): Deferred<List<String>> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_list_style_source_ids",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readBufferViewArray(
            mln_completion_result.value(result),
            mln_completion_result.value_count(result),
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_list_style_source_ids(
                bindingMapHandle(),
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

  public actual fun loadedStyleJson(): Deferred<ByteArray> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_loaded_style_json",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readBytes(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_loaded_style_json(bindingMapHandle(), completion, diagnostic)
            }
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun metersPerPixelAtLatitude(latitude: Double): Deferred<Double> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_meters_per_pixel_at_latitude",
      )
      return CompletionBridge.submit(
        { result ->
          NativeAccess.completionValue(result, ValueLayout.JAVA_DOUBLE.byteSize())
            .get(ValueLayout.JAVA_DOUBLE, 0)
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_meters_per_pixel_at_latitude(
                bindingMapHandle(),
                latitude,
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

  public actual fun moveStyleLayer(
    layerId: String,
    beforeLayerId: String?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_move_style_layer",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_move_style_layer(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              GeneratedValues.stringView(arena, (beforeLayerId ?: "")),
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

  public actual fun pixelForLatLng(coordinate: LatLng): Deferred<ScreenPoint> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_pixel_for_lat_lng",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readScreenPoint(
            NativeAccess.completionValue(result, mln_screen_point.sizeof())
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_pixel_for_lat_lng(
                bindingMapHandle(),
                GeneratedValues.writeLatLng(arena, coordinate),
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

  public actual fun pixelsForLatLngs(coordinates: List<LatLng>): Deferred<List<ScreenPoint>> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_pixels_for_lat_lngs",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readScreenPointArray(
            mln_completion_result.value(result),
            mln_completion_result.value_count(result),
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_pixels_for_lat_lngs(
                bindingMapHandle(),
                GeneratedValues.writeLatLngArray(arena, coordinates),
                coordinates.size.toLong(),
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

  public actual fun projectionCreate():
    Deferred<org.maplibre.nativeffi.generated.MapProjectionHandle> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_projection_create",
      )
      return CompletionBridge.submitOwned(
        { result ->
          MapProjectionHandle(
            NativeAccess.completionValue(result, ValueLayout.JAVA_LONG.byteSize())
              .get(ValueLayout.JAVA_LONG, 0)
          )
        },
        { it.close() },
        { result ->
          GeneratedOwnerDisposal.mapProjection(
            NativeAccess.completionValue(result, ValueLayout.JAVA_LONG.byteSize())
              .get(ValueLayout.JAVA_LONG, 0)
          )
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_projection_create(bindingMapHandle(), completion, diagnostic)
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation("mln_map_release")
      return bindingRetireMap { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_map_release",
        )
        CompletionBridge.unitChecked { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_release(owner, completion, diagnostic)
            }
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun removeFeatureState(
    selector: FeatureStateSelector
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_remove_feature_state",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_remove_feature_state(
              bindingMapHandle(),
              GeneratedValues.writeFeatureStateSelector(arena, selector),
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

  public actual fun removeStyleImage(imageId: String): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_remove_style_image",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_remove_style_image(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, imageId),
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

  public actual fun removeStyleLayer(layerId: String): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_remove_style_layer",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_remove_style_layer(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
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

  public actual fun removeStyleSource(sourceId: String): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_remove_style_source",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_remove_style_source(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
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

  public actual fun requestRepaint(): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_request_repaint",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_request_repaint(bindingMapHandle(), completion, diagnostic)
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun requestStillImage(): Deferred<Unit> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_request_still_image",
      )
      return CompletionBridge.unit { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_request_still_image(bindingMapHandle(), completion, diagnostic)
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun resize(extent: LogicalExtent): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_resize",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_resize(
              bindingMapHandle(),
              GeneratedValues.writeLogicalExtent(arena, extent),
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

  public actual fun setBounds(options: BoundOptions): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_bounds",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_bounds(
              bindingMapHandle(),
              GeneratedValues.writeBoundOptions(arena, options),
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

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_custom_geometry_source_tile_data",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_custom_geometry_source_tile_data(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeCanonicalTileId(arena, tileId),
              GeneratedValues.byteView(arena, data),
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

  public actual fun setCustomMvtVectorSourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_custom_mvt_vector_source_tile_data",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_custom_mvt_vector_source_tile_data(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeCanonicalTileId(arena, tileId),
              GeneratedValues.byteView(arena, data),
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

  public actual fun setCustomMvtVectorSourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_custom_mvt_vector_source_tile_error",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_custom_mvt_vector_source_tile_error(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeCanonicalTileId(arena, tileId),
              GeneratedValues.stringView(arena, message),
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

  public actual fun setDebugOptions(options: MapDebugOption): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_debug_options",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_debug_options(
              bindingMapHandle(),
              options.rawValue.toInt(),
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

  public actual fun setEventMask(mask: RuntimeEventMask): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_event_mask",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_event_mask(
              bindingMapHandle(),
              mask.rawValue.toLong(),
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

  public actual fun setFeatureState(
    selector: FeatureStateSelector,
    state: ByteArray,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_feature_state",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_feature_state(
              bindingMapHandle(),
              GeneratedValues.writeFeatureStateSelector(arena, selector),
              GeneratedValues.byteView(arena, state),
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

  public actual fun setFreeCameraOptions(options: FreeCameraOptions): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_free_camera_options",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_free_camera_options(
              bindingMapHandle(),
              GeneratedValues.writeFreeCameraOptions(arena, options),
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

  public actual fun setGeojsonSourceData(
    sourceId: String,
    data: org.maplibre.nativeffi.generated.GeojsonSourceDataHandle,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_geojson_source_data",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_geojson_source_data(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              data.bindingGeojsonSourceDataHandle(),
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

  public actual fun setGeojsonSourceSynchronousTiling(
    sourceId: String,
    enabled: Boolean,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_geojson_source_synchronous_tiling",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_geojson_source_synchronous_tiling(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              enabled,
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

  public actual fun setGeojsonSourceUrl(
    sourceId: String,
    url: String,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_geojson_source_url",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_geojson_source_url(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.stringView(arena, url),
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

  public actual fun setGlobalStateProperty(
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_global_state_property",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_global_state_property(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, propertyName),
              GeneratedValues.byteView(arena, valueValue),
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

  public actual fun setImageSourceCoordinates(
    sourceId: String,
    coordinates: List<LatLng>,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_image_source_coordinates",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_image_source_coordinates(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writeLatLngArray(arena, coordinates),
              coordinates.size.toLong(),
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

  public actual fun setImageSourceImage(
    sourceId: String,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_image_source_image",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_image_source_image(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.writePremultipliedRgba8Image(arena, image),
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

  public actual fun setImageSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_image_source_url",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_image_source_url(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              GeneratedValues.stringView(arena, url),
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

  public actual fun setLayerFilter(
    layerId: String,
    filter: ByteArray?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_layer_filter",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_layer_filter(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              if (filter == null) MemorySegment.NULL else GeneratedValues.byteView(arena, filter!!),
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

  public actual fun setLayerMaxZoom(layerId: String, maxZoom: Double): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_layer_max_zoom",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_layer_max_zoom(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              maxZoom,
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

  public actual fun setLayerMinZoom(layerId: String, minZoom: Double): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_layer_min_zoom",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_layer_min_zoom(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              minZoom,
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

  public actual fun setLayerProperty(
    layerId: String,
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_layer_property",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_layer_property(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              GeneratedValues.stringView(arena, propertyName),
              GeneratedValues.byteView(arena, valueValue),
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

  public actual fun setLayerSourceId(
    layerId: String,
    sourceId: String,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_layer_source_id",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_layer_source_id(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              GeneratedValues.stringView(arena, sourceId),
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

  public actual fun setLayerSourceLayer(
    layerId: String,
    sourceLayer: String?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_layer_source_layer",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_layer_source_layer(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              GeneratedValues.stringView(arena, (sourceLayer ?: "")),
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

  public actual fun setLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_layer_visibility",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_layer_visibility(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              visibility.rawValue.toInt(),
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

  public actual fun setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_location_indicator_accuracy_radius",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_location_indicator_accuracy_radius(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              radius,
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

  public actual fun setLocationIndicatorBearing(
    layerId: String,
    bearing: Double,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_location_indicator_bearing",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_location_indicator_bearing(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              bearing,
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

  public actual fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_location_indicator_image_name",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_location_indicator_image_name(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              imageKind.rawValue.toInt(),
              GeneratedValues.stringView(arena, imageId),
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

  public actual fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_location_indicator_location",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_location_indicator_location(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, layerId),
              GeneratedValues.writeLatLng(arena, coordinate),
              altitude,
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

  public actual fun setProjectionMode(mode: ProjectionMode): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_projection_mode",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_projection_mode(
              bindingMapHandle(),
              GeneratedValues.writeProjectionMode(arena, mode),
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

  public actual fun setRenderingStatsViewEnabled(enabled: Boolean): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_rendering_stats_view_enabled",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_rendering_stats_view_enabled(
              bindingMapHandle(),
              enabled,
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

  public actual fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions?,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_style_image",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_style_image(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, imageId),
              GeneratedValues.writePremultipliedRgba8Image(arena, image),
              if (options == null) MemorySegment.NULL
              else GeneratedValues.writeStyleImageOptions(arena, options!!),
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

  public actual fun setStyleJson(json: ByteArray): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_style_json",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_style_json(
              bindingMapHandle(),
              GeneratedValues.byteView(arena, json),
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

  public actual fun setStyleLightJson(lightJson: ByteArray): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_style_light_json",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_style_light_json(
              bindingMapHandle(),
              GeneratedValues.byteView(arena, lightJson),
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

  public actual fun setStyleLightProperty(
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_style_light_property",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_style_light_property(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, propertyName),
              GeneratedValues.byteView(arena, valueValue),
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

  public actual fun setStyleSourceVolatile(
    sourceId: String,
    isVolatile: Boolean,
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_style_source_volatile",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_style_source_volatile(
              bindingMapHandle(),
              GeneratedValues.stringView(arena, sourceId),
              isVolatile,
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

  public actual fun setStyleTransitionOptions(
    options: StyleTransitionOptions
  ): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_style_transition_options",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_style_transition_options(
              bindingMapHandle(),
              GeneratedValues.writeStyleTransitionOptions(arena, options),
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

  public actual fun setStyleUrl(url: String): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_style_url",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_style_url(
              bindingMapHandle(),
              GeneratedValues.cString(arena, url),
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

  public actual fun setTileOptions(options: MapTileOptions): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_tile_options",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_tile_options(
              bindingMapHandle(),
              GeneratedValues.writeMapTileOptions(arena, options),
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

  public actual fun setViewportOptions(options: MapViewportOptions): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_set_viewport_options",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_set_viewport_options(
              bindingMapHandle(),
              GeneratedValues.writeMapViewportOptions(arena, options),
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

  public actual fun snapshotGet(): MapSnapshot {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_snapshot_get",
      )
      return Arena.ofConfined().use { arena ->
        val output = mln_map_snapshot.allocate(arena)
        mln_map_snapshot.size(output, mln_map_snapshot.sizeof().toInt())
        NativeDiagnostics.check { diagnostic ->
          MapLibreNativeC.mln_map_snapshot_get(bindingMapHandle(), output, diagnostic)
        }
        GeneratedValues.readMapSnapshot(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun styleUrl(): Deferred<String> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_style_url",
      )
      return CompletionBridge.submit(
        { result ->
          GeneratedValues.readString(NativeAccess.completionValue(result, mln_buffer_view.sizeof()))
        },
        { completion ->
          Arena.ofConfined().use { arena ->
            NativeDiagnostics.check { diagnostic ->
              MapLibreNativeC.mln_map_style_url(bindingMapHandle(), completion, diagnostic)
            }
          }
        },
      )
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun updateCamera(update: CameraUpdate): Deferred<CommandCompletion> {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_map_update_camera",
      )
      return CompletionBridge.command { completion ->
        Arena.ofConfined().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_map_update_camera(
              bindingMapHandle(),
              GeneratedValues.writeCameraUpdate(arena, update),
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

  public actual fun metalBorrowedTextureAttach(
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_metal_borrowed_texture_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_metal_borrowed_texture_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeMetalBorrowedTextureDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun metalOwnedTextureAttach(
    descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_metal_owned_texture_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_metal_owned_texture_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeMetalOwnedTextureDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun metalSurfaceAttach(
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_metal_surface_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_metal_surface_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeMetalSurfaceDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun openglBorrowedTextureAttach(
    descriptor: OpenglBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_opengl_borrowed_texture_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_opengl_borrowed_texture_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeOpenglBorrowedTextureDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun openglOwnedTextureAttach(
    descriptor: OpenglOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_opengl_owned_texture_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_opengl_owned_texture_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeOpenglOwnedTextureDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun openglSurfaceAttach(
    descriptor: OpenglSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_opengl_surface_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_opengl_surface_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeOpenglSurfaceDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun vulkanBorrowedTextureAttach(
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_vulkan_borrowed_texture_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_vulkan_borrowed_texture_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeVulkanBorrowedTextureDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun vulkanOwnedTextureAttach(
    descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_vulkan_owned_texture_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_vulkan_owned_texture_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeVulkanOwnedTextureDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun vulkanSurfaceAttach(
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_vulkan_surface_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_vulkan_surface_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeVulkanSurfaceDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun webgpuBorrowedTextureAttach(
    descriptor: WebgpuBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_webgpu_borrowed_texture_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_webgpu_borrowed_texture_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeWebgpuBorrowedTextureDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun webgpuOwnedTextureAttach(
    descriptor: WebgpuOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_webgpu_owned_texture_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_webgpu_owned_texture_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeWebgpuOwnedTextureDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun webgpuSurfaceAttach(
    descriptor: WebgpuSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapHandle().toLong(),
        "mln_webgpu_surface_attach",
      )
      return run {
        val registrations = CallbackRegistrationScope()
        try {
          Arena.ofConfined().use { arena ->
            val output = arena.allocate(ValueLayout.JAVA_LONG)
            val ready = CompletionBridge.unitChecked { completion ->
              NativeDiagnostics.check { diagnostic ->
                MapLibreNativeC.mln_webgpu_surface_attach(
                  bindingMapHandle(),
                  GeneratedValues.writeWebgpuSurfaceDescriptor(arena, descriptor),
                  GeneratedValues.writeRenderSessionAttachOptions(arena, options, registrations),
                  output,
                  completion,
                  diagnostic,
                )
              }
            }
            run {
              val owner =
                adoptOwned(
                  output.get(ValueLayout.JAVA_LONG, 0),
                  { GeneratedOwnerDisposal.renderSession(it) },
                  {
                    RenderSessionHandle(
                      it,
                      this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle,
                    )
                  },
                )
              try {
                registrations.accept(owner.bindingCallbacks)
                RenderSessionAttachment(owner, ready)
              } catch (failure: Throwable) {
                try {
                  owner.dispose()
                } catch (cleanup: Throwable) {
                  failure.addSuppressed(cleanup)
                }
                throw failure
              }
            }
          }
        } finally {
          registrations.close()
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
