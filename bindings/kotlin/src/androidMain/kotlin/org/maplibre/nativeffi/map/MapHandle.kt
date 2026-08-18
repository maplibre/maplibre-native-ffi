package org.maplibre.nativeffi.map

import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicLong
import kotlinx.coroutines.Deferred
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.LongPointer
import org.bytedeco.javacpp.Pointer
import org.bytedeco.javacpp.SizeTPointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.BoundsConstraint
import org.maplibre.nativeffi.camera.CameraDelta
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraSnapshot
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.geo.Vec3
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.async.mapDeferred
import org.maplibre.nativeffi.internal.callback.CallbackGate
import org.maplibre.nativeffi.internal.javacpp.ByteArrayViewScope
import org.maplibre.nativeffi.internal.javacpp.GeoJsonSourceOptionsScope
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.render.RenderSessionAttachOptions
import org.maplibre.nativeffi.render.RenderSessionAttachment
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.runtime.CommandCompletion
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.GeoJsonSourceDataHandle
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.ImageContent
import org.maplibre.nativeffi.style.ImageStretch
import org.maplibre.nativeffi.style.LayerInfo
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.RasterDemEncoding
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImage
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleImageTextFit
import org.maplibre.nativeffi.style.StyleLayerVisibility
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileJson
import org.maplibre.nativeffi.style.TileScheme
import org.maplibre.nativeffi.style.TileSourceOptions
import org.maplibre.nativeffi.style.VectorTileEncoding

/** Owned Android JNI map handle. */
public actual class MapHandle
private constructor(
  private val runtime: RuntimeHandle,
  private val handleId: Long,
  cachedEventMask: RuntimeEventMask,
) {
  private var cachedEventMask =
    RuntimeEventMask(cachedEventMask.nativeValue and RuntimeEventMask.ALL_MAP_EVENTS.nativeValue)
  private val core = HandleStateCore("MapHandle", handleId)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  private val customGeometrySources =
    CustomGeometrySourceRegistry<CustomGeometrySourceState>(::releaseCallbackRoot)

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun runtime(): RuntimeHandle = runtime

  public actual val eventMask: RuntimeEventMask
    get() = cachedEventMask

  public actual fun setEventMask(value: RuntimeEventMask): Deferred<CommandCompletion> {
    NativeAccess.ensureLoaded()
    return command { completion ->
        Status.check(
          MaplibreNativeC.mln_map_set_event_mask(requireLiveHandle(), value.nativeValue, completion)
        )
      }
      .mapDeferred {
        cachedEventMask =
          RuntimeEventMask(value.nativeValue and RuntimeEventMask.ALL_MAP_EVENTS.nativeValue)
        it
      }
  }

  public actual fun setStyleUrl(url: String): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    optionalCString(url).use { nativeUrl ->
      Status.check(
        MaplibreNativeC.mln_map_set_style_url(requireLiveHandle(), nativeUrl, completion)
      )
    }
  }

  public actual fun setStyleJson(json: ByteArray): Deferred<CommandCompletion> =
    command { completion ->
      NativeAccess.ensureLoaded()
      ByteArrayViewScope(json).use { nativeJson ->
        Status.check(
          MaplibreNativeC.mln_map_set_style_json(requireLiveHandle(), nativeJson.view, completion)
        )
      }
    }

  public actual fun loadedStyleJson(): Deferred<ByteArray> =
    CompletionBridge.submit(::requiredBuffer) { completion ->
      MaplibreNativeC.mln_map_loaded_style_json(requireLiveHandle(), completion)
    }

  public actual fun styleUrl(): Deferred<String> =
    CompletionBridge.submit({ result -> requiredBuffer(result).decodeToString() }) { completion ->
      MaplibreNativeC.mln_map_style_url(requireLiveHandle(), completion)
    }

  public actual fun addStyleSourceJson(
    sourceId: String,
    sourceJson: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      ByteArrayViewScope(sourceJson).use { nativeSourceJson ->
        Status.check(
          MaplibreNativeC.mln_map_add_style_source_json(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeSourceJson.view,
            completion,
          )
        )
      }
    }
  }

  public actual fun removeStyleSource(sourceId: String): Deferred<CommandCompletion> =
    command { completion ->
      NativeAccess.ensureLoaded()
      StringViewScope(sourceId).use { nativeSourceId ->
        Status.check(
          MaplibreNativeC.mln_map_remove_style_source(
            requireLiveHandle(),
            nativeSourceId.view,
            completion,
          )
        )
      }
    }

  public actual fun styleSourceInfo(sourceId: String): Deferred<SourceInfo?> {
    NativeAccess.ensureLoaded()
    return StringViewScope(sourceId).use { nativeSourceId ->
      CompletionBridge.submit(
        { result ->
          if (result.value_count() == 0L) null
          else {
            val raw = MaplibreNativeC.mln_style_source_result(result.value())
            val info = raw.info()
            val fields = info.fields()
            val tileUrls =
              List(Math.toIntExact(raw.tile_url_count())) { index ->
                stringView(raw.tile_urls().position(index.toLong()))
              }
            SourceInfo(
              SourceType.fromNative(info.type()),
              info.is_volatile(),
              if (info.has_attribution()) stringView(raw.attribution()) else null,
              if (fields and STYLE_SOURCE_INFO_URL != 0) stringView(raw.url()) else null,
              if (fields and STYLE_SOURCE_INFO_TILEJSON != 0)
                TileJson(
                  tileUrls,
                  info.min_zoom(),
                  info.max_zoom(),
                  TileScheme.fromNative(info.scheme()),
                  if (fields and STYLE_SOURCE_INFO_BOUNDS != 0) latLngBounds(info.bounds())
                  else null,
                )
              else null,
              if (fields and STYLE_SOURCE_INFO_TILE_SIZE != 0)
                Math.toIntExact(Integer.toUnsignedLong(info.tile_size()))
              else null,
              if (fields and STYLE_SOURCE_INFO_VECTOR_ENCODING != 0)
                VectorTileEncoding.fromNative(info.vector_encoding())
              else null,
              if (fields and STYLE_SOURCE_INFO_RASTER_ENCODING != 0)
                RasterDemEncoding.fromNative(info.raster_encoding())
              else null,
            )
          }
        },
        { completion ->
          MaplibreNativeC.mln_map_get_style_source_info(
            requireLiveHandle(),
            nativeSourceId.view,
            completion,
          )
        },
      )
    }
  }

  public actual fun styleSourceIds(): Deferred<List<String>> =
    CompletionBridge.submit(::stringList) { completion ->
      MaplibreNativeC.mln_map_list_style_source_ids(requireLiveHandle(), completion)
    }

  public actual fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      StringViewScope(url).use { nativeUrl ->
        GeoJsonSourceOptionsScope(options).use { nativeOptions ->
          Status.check(
            MaplibreNativeC.mln_map_add_geojson_source_url(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeUrl.view,
              nativeOptions.options,
              completion,
            )
          )
        }
      }
    }
  }

  public actual fun addGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    data.withNativeHandle { nativeData ->
      StringViewScope(sourceId).use { nativeSourceId ->
        Status.check(
          MaplibreNativeC.mln_map_add_geojson_source_data(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeData,
            completion,
          )
        )
      }
    }
  }

  public actual fun setGeoJsonSourceUrl(
    sourceId: String,
    url: String,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      StringViewScope(url).use { nativeUrl ->
        Status.check(
          MaplibreNativeC.mln_map_set_geojson_source_url(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeUrl.view,
            completion,
          )
        )
      }
    }
  }

  public actual fun setGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    data.withNativeHandle { nativeData ->
      StringViewScope(sourceId).use { nativeSourceId ->
        Status.check(
          MaplibreNativeC.mln_map_set_geojson_source_data(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeData,
            completion,
          )
        )
      }
    }
  }

  public actual fun setGeoJsonSourceSynchronousTiling(
    sourceId: String,
    enabled: Boolean,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      Status.check(
        MaplibreNativeC.mln_map_set_geojson_source_synchronous_tiling(
          requireLiveHandle(),
          nativeSourceId.view,
          enabled,
          completion,
        )
      )
    }
  }

  public actual fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    // The release callback captures the registry rather than this map, so a map a
    // host leaks with a live source still reports as leaked.
    val registry = customGeometrySources
    val sourceState = CustomGeometrySourceState(options) { registry.remove(sourceId) }
    registry.install(sourceId, sourceState) {
      StringViewScope(sourceId).use { nativeSourceId ->
        Status.check(
          MaplibreNativeC.mln_map_add_custom_geometry_source(
            requireLiveHandle(),
            nativeSourceId.view,
            sourceState.descriptor,
            completion,
          )
        )
      }
      HandleLeakCleaner.retainNativeCallbackRoot(sourceState)
    }
  }

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      CanonicalTileIdScope(tileId).use { nativeTileId ->
        ByteArrayViewScope(data).use { nativeData ->
          Status.check(
            MaplibreNativeC.mln_map_set_custom_geometry_source_tile_data(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeTileId.tileId,
              nativeData.view,
              completion,
            )
          )
        }
      }
    }
  }

  public actual fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      CanonicalTileIdScope(tileId).use { nativeTileId ->
        Status.check(
          MaplibreNativeC.mln_map_invalidate_custom_geometry_source_tile(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeTileId.tileId,
            completion,
          )
        )
      }
    }
  }

  public actual fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      Status.check(
        MaplibreNativeC.mln_map_invalidate_custom_geometry_source_region(
          requireLiveHandle(),
          nativeSourceId.view,
          latLngBounds(bounds),
          completion,
        )
      )
    }
  }

  public actual fun addVectorSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    addTileSourceUrl(
      MaplibreNativeC::mln_map_add_vector_source_url,
      sourceId,
      url,
      options,
      completion,
    )
  }

  public actual fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    addTileSourceTiles(
      MaplibreNativeC::mln_map_add_vector_source_tiles,
      sourceId,
      tiles,
      options,
      completion,
    )
  }

  public actual fun addRasterSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    addTileSourceUrl(
      MaplibreNativeC::mln_map_add_raster_source_url,
      sourceId,
      url,
      options,
      completion,
    )
  }

  public actual fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    addTileSourceTiles(
      MaplibreNativeC::mln_map_add_raster_source_tiles,
      sourceId,
      tiles,
      options,
      completion,
    )
  }

  public actual fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    addTileSourceUrl(
      MaplibreNativeC::mln_map_add_raster_dem_source_url,
      sourceId,
      url,
      options,
      completion,
    )
  }

  public actual fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    addTileSourceTiles(
      MaplibreNativeC::mln_map_add_raster_dem_source_tiles,
      sourceId,
      tiles,
      options,
      completion,
    )
  }

  public actual fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(imageId).use { nativeImageId ->
      PremultipliedImageScope(image).use { nativeImage ->
        StyleImageOptionsScope(options).use { nativeOptions ->
          Status.check(
            MaplibreNativeC.mln_map_set_style_image(
              requireLiveHandle(),
              nativeImageId.view,
              nativeImage.image,
              nativeOptions.options,
              completion,
            )
          )
        }
      }
    }
  }

  public actual fun removeStyleImage(imageId: String): Deferred<CommandCompletion> =
    command { completion ->
      NativeAccess.ensureLoaded()
      StringViewScope(imageId).use { nativeImageId ->
        Status.check(
          MaplibreNativeC.mln_map_remove_style_image(
            requireLiveHandle(),
            nativeImageId.view,
            completion,
          )
        )
      }
    }

  public actual fun styleImageInfo(imageId: String): Deferred<StyleImageInfo?> =
    StringViewScope(imageId).use { nativeImageId ->
      CompletionBridge.submit(
        { result ->
          if (result.value_count() == 0L) null
          else styleImageInfo(MaplibreNativeC.mln_style_image_result(result.value()).info())
        },
        { completion ->
          MaplibreNativeC.mln_map_get_style_image_info(
            requireLiveHandle(),
            nativeImageId.view,
            completion,
          )
        },
      )
    }

  public actual fun copyStyleImagePremultipliedRgba8(imageId: String): Deferred<StyleImage?> =
    StringViewScope(imageId).use { nativeImageId ->
      CompletionBridge.submit(
        { result ->
          if (result.value_count() == 0L) null
          else {
            val raw = MaplibreNativeC.mln_style_image_result(result.value())
            val info = styleImageInfo(raw.info())
            StyleImage(
              PremultipliedRgba8Image(
                info.width,
                info.height,
                info.stride,
                bufferView(raw.pixels()),
              ),
              info.pixelRatio,
              info.sdf,
            )
          }
        },
        { completion ->
          MaplibreNativeC.mln_map_get_style_image_info(
            requireLiveHandle(),
            nativeImageId.view,
            completion,
          )
        },
      )
    }

  public actual fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      LatLngArrayScope(coordinates).use { nativeCoordinates ->
        StringViewScope(url).use { nativeUrl ->
          Status.check(
            MaplibreNativeC.mln_map_add_image_source_url(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeCoordinates.coordinates,
              nativeCoordinates.count,
              nativeUrl.view,
              completion,
            )
          )
        }
      }
    }
  }

  public actual fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      LatLngArrayScope(coordinates).use { nativeCoordinates ->
        PremultipliedImageScope(image).use { nativeImage ->
          Status.check(
            MaplibreNativeC.mln_map_add_image_source_image(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeCoordinates.coordinates,
              nativeCoordinates.count,
              nativeImage.image,
              completion,
            )
          )
        }
      }
    }
  }

  public actual fun setImageSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> =
    command { completion ->
      NativeAccess.ensureLoaded()
      StringViewScope(sourceId).use { nativeSourceId ->
        StringViewScope(url).use { nativeUrl ->
          Status.check(
            MaplibreNativeC.mln_map_set_image_source_url(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeUrl.view,
              completion,
            )
          )
        }
      }
    }

  public actual fun setImageSourceImage(
    sourceId: String,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      PremultipliedImageScope(image).use { nativeImage ->
        Status.check(
          MaplibreNativeC.mln_map_set_image_source_image(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeImage.image,
            completion,
          )
        )
      }
    }
  }

  public actual fun setImageSourceCoordinates(
    sourceId: String,
    coordinates: List<LatLng>,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(sourceId).use { nativeSourceId ->
      LatLngArrayScope(coordinates).use { nativeCoordinates ->
        Status.check(
          MaplibreNativeC.mln_map_set_image_source_coordinates(
            requireLiveHandle(),
            nativeSourceId.view,
            nativeCoordinates.coordinates,
            nativeCoordinates.count,
            completion,
          )
        )
      }
    }
  }

  public actual fun imageSourceCoordinates(sourceId: String): Deferred<List<LatLng>?> =
    StringViewScope(sourceId).use { nativeSourceId ->
      CompletionBridge.submit(
        { result ->
          if (result.value_count() == 0L) null
          else {
            val values = MaplibreNativeC.mln_lat_lng(result.value())
            List(Math.toIntExact(result.value_count())) { index ->
              latLng(values.position(index.toLong()))
            }
          }
        },
        { completion ->
          MaplibreNativeC.mln_map_get_image_source_coordinates(
            requireLiveHandle(),
            nativeSourceId.view,
            completion,
          )
        },
      )
    }

  public actual fun addStyleLayerJson(
    layerJson: ByteArray,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    ByteArrayViewScope(layerJson).use { nativeLayerJson ->
      StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_add_style_layer_json(
            requireLiveHandle(),
            nativeLayerJson.view,
            nativeBeforeLayerId.view,
            completion,
          )
        )
      }
    }
  }

  public actual fun addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(sourceId).use { nativeSourceId ->
        StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
          Status.check(
            MaplibreNativeC.mln_map_add_hillshade_layer(
              requireLiveHandle(),
              nativeLayerId.view,
              nativeSourceId.view,
              nativeBeforeLayerId.view,
              completion,
            )
          )
        }
      }
    }
  }

  public actual fun addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(sourceId).use { nativeSourceId ->
        StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
          Status.check(
            MaplibreNativeC.mln_map_add_color_relief_layer(
              requireLiveHandle(),
              nativeLayerId.view,
              nativeSourceId.view,
              nativeBeforeLayerId.view,
              completion,
            )
          )
        }
      }
    }
  }

  public actual fun addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_add_location_indicator_layer(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeBeforeLayerId.view,
            completion,
          )
        )
      }
    }
  }

  public actual fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      latLng(coordinate).use { nativeCoordinate ->
        Status.check(
          MaplibreNativeC.mln_map_set_location_indicator_location(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeCoordinate,
            altitude,
            completion,
          )
        )
      }
    }
  }

  public actual fun setLocationIndicatorBearing(
    layerId: String,
    bearing: Double,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_set_location_indicator_bearing(
          requireLiveHandle(),
          nativeLayerId.view,
          bearing,
          completion,
        )
      )
    }
  }

  public actual fun setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_set_location_indicator_accuracy_radius(
          requireLiveHandle(),
          nativeLayerId.view,
          radius,
          completion,
        )
      )
    }
  }

  public actual fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(imageId).use { nativeImageId ->
        Status.check(
          MaplibreNativeC.mln_map_set_location_indicator_image_name(
            requireLiveHandle(),
            nativeLayerId.view,
            imageKind.nativeValue,
            nativeImageId.view,
            completion,
          )
        )
      }
    }
  }

  public actual fun removeStyleLayer(layerId: String): Deferred<CommandCompletion> =
    command { completion ->
      NativeAccess.ensureLoaded()
      StringViewScope(layerId).use { nativeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_remove_style_layer(
            requireLiveHandle(),
            nativeLayerId.view,
            completion,
          )
        )
      }
    }

  public actual fun styleLayerInfo(layerId: String): Deferred<LayerInfo?> =
    StringViewScope(layerId).use { nativeLayerId ->
      CompletionBridge.submit(
        { result ->
          if (result.value_count() == 0L) null
          else {
            val raw = MaplibreNativeC.mln_style_layer_result(result.value())
            val info = raw.info()
            val fields = info.fields()
            LayerInfo(
              stringView(info.type()),
              info.min_zoom(),
              info.max_zoom(),
              StyleLayerVisibility.fromNative(info.visibility()),
              if (fields and MaplibreNativeC.MLN_STYLE_LAYER_INFO_SOURCE_ID != 0)
                stringView(raw.source_id())
              else null,
              if (fields and MaplibreNativeC.MLN_STYLE_LAYER_INFO_SOURCE_LAYER != 0)
                stringView(raw.source_layer())
              else null,
            )
          }
        },
        { completion ->
          MaplibreNativeC.mln_map_get_style_layer_info(
            requireLiveHandle(),
            nativeLayerId.view,
            completion,
          )
        },
      )
    }

  public actual fun styleLayerIds(): Deferred<List<String>> =
    CompletionBridge.submit(::stringList) { completion ->
      MaplibreNativeC.mln_map_list_style_layer_ids(requireLiveHandle(), completion)
    }

  public actual fun moveStyleLayer(
    layerId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(beforeLayerId).use { nativeBeforeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_move_style_layer(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeBeforeLayerId.view,
            completion,
          )
        )
      }
    }
  }

  public actual fun styleLayerJson(layerId: String): Deferred<ByteArray?> =
    StringViewScope(layerId).use { nativeLayerId ->
      CompletionBridge.submit(
        ::optionalBuffer,
        { completion ->
          MaplibreNativeC.mln_map_get_style_layer_json(
            requireLiveHandle(),
            nativeLayerId.view,
            completion,
          )
        },
      )
    }

  public actual fun setStyleLightJson(lightJson: ByteArray): Deferred<CommandCompletion> =
    command { completion ->
      NativeAccess.ensureLoaded()
      ByteArrayViewScope(lightJson).use { nativeLightJson ->
        Status.check(
          MaplibreNativeC.mln_map_set_style_light_json(
            requireLiveHandle(),
            nativeLightJson.view,
            completion,
          )
        )
      }
    }

  public actual fun setStyleLightProperty(
    propertyName: String,
    value: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(propertyName).use { nativePropertyName ->
      ByteArrayViewScope(value).use { nativeValue ->
        Status.check(
          MaplibreNativeC.mln_map_set_style_light_property(
            requireLiveHandle(),
            nativePropertyName.view,
            nativeValue.view,
            completion,
          )
        )
      }
    }
  }

  public actual fun styleLightProperty(propertyName: String): Deferred<ByteArray?> =
    StringViewScope(propertyName).use { nativePropertyName ->
      CompletionBridge.submit(
        ::optionalBuffer,
        { completion ->
          MaplibreNativeC.mln_map_get_style_light_property(
            requireLiveHandle(),
            nativePropertyName.view,
            completion,
          )
        },
      )
    }

  public actual fun setStyleTransitionOptions(
    options: StyleTransitionOptions
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StyleTransitionOptionsScope(options).use { nativeOptions ->
      Status.check(
        MaplibreNativeC.mln_map_set_style_transition_options(
          requireLiveHandle(),
          nativeOptions.options,
          completion,
        )
      )
    }
  }

  public actual fun styleTransitionOptions(): Deferred<StyleTransitionOptions> =
    CompletionBridge.submit(
      { result ->
        styleTransitionOptions(MaplibreNativeC.mln_style_transition_options(result.value()))
      },
      { completion ->
        MaplibreNativeC.mln_map_get_style_transition_options(requireLiveHandle(), completion)
      },
    )

  public actual fun setLayerProperty(
    layerId: String,
    propertyName: String,
    value: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(propertyName).use { nativePropertyName ->
        ByteArrayViewScope(value).use { nativeValue ->
          Status.check(
            MaplibreNativeC.mln_map_set_layer_property(
              requireLiveHandle(),
              nativeLayerId.view,
              nativePropertyName.view,
              nativeValue.view,
              completion,
            )
          )
        }
      }
    }
  }

  public actual fun layerProperty(layerId: String, propertyName: String): Deferred<ByteArray?> =
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(propertyName).use { nativePropertyName ->
        CompletionBridge.submit(
          ::optionalBuffer,
          { completion ->
            MaplibreNativeC.mln_map_get_layer_property(
              requireLiveHandle(),
              nativeLayerId.view,
              nativePropertyName.view,
              completion,
            )
          },
        )
      }
    }

  public actual fun setLayerFilter(
    layerId: String,
    filter: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      ByteArrayViewScope(filter).use { nativeFilter ->
        Status.check(
          MaplibreNativeC.mln_map_set_layer_filter(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeFilter.view,
            completion,
          )
        )
      }
    }
  }

  public actual fun clearLayerFilter(layerId: String): Deferred<CommandCompletion> =
    command { completion ->
      NativeAccess.ensureLoaded()
      StringViewScope(layerId).use { nativeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_set_layer_filter(
            requireLiveHandle(),
            nativeLayerId.view,
            null,
            completion,
          )
        )
      }
    }

  public actual fun layerFilter(layerId: String): Deferred<ByteArray?> =
    StringViewScope(layerId).use { nativeLayerId ->
      CompletionBridge.submit(
        ::optionalBuffer,
        { completion ->
          MaplibreNativeC.mln_map_get_layer_filter(
            requireLiveHandle(),
            nativeLayerId.view,
            completion,
          )
        },
      )
    }

  public actual fun styleImageStretches(
    imageId: String
  ): Deferred<Pair<List<ImageStretch>, List<ImageStretch>>?> =
    StringViewScope(imageId).use { nativeImageId ->
      CompletionBridge.submit(
        { result ->
          if (result.value_count() == 0L) null
          else {
            val raw = MaplibreNativeC.mln_style_image_stretches_result(result.value())
            readStretches(raw.stretch_x(), Math.toIntExact(raw.stretch_x_count())) to
              readStretches(raw.stretch_y(), Math.toIntExact(raw.stretch_y_count()))
          }
        },
        { completion ->
          MaplibreNativeC.mln_map_copy_style_image_stretches(
            requireLiveHandle(),
            nativeImageId.view,
            completion,
          )
        },
      )
    }

  public actual fun setLayerSourceLayer(
    layerId: String,
    sourceLayer: String,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(sourceLayer).use { nativeSourceLayer ->
        Status.check(
          MaplibreNativeC.mln_map_set_layer_source_layer(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeSourceLayer.view,
            completion,
          )
        )
      }
    }
  }

  public actual fun layerSourceLayer(layerId: String): Deferred<String> =
    StringViewScope(layerId).use { nativeLayerId ->
      CompletionBridge.submit({ result -> requiredBuffer(result).decodeToString() }) { completion ->
        MaplibreNativeC.mln_map_copy_layer_source_layer(
          requireLiveHandle(),
          nativeLayerId.view,
          completion,
        )
      }
    }

  public actual fun setLayerSourceId(
    layerId: String,
    sourceId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      StringViewScope(sourceId).use { nativeSourceId ->
        Status.check(
          MaplibreNativeC.mln_map_set_layer_source_id(
            requireLiveHandle(),
            nativeLayerId.view,
            nativeSourceId.view,
            completion,
          )
        )
      }
    }
  }

  public actual fun layerSourceId(layerId: String): Deferred<String> =
    StringViewScope(layerId).use { nativeLayerId ->
      CompletionBridge.submit({ result -> requiredBuffer(result).decodeToString() }) { completion ->
        MaplibreNativeC.mln_map_copy_layer_source_id(
          requireLiveHandle(),
          nativeLayerId.view,
          completion,
        )
      }
    }

  public actual fun setLayerMinZoom(layerId: String, minZoom: Double): Deferred<CommandCompletion> =
    command { completion ->
      NativeAccess.ensureLoaded()
      StringViewScope(layerId).use { nativeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_set_layer_min_zoom(
            requireLiveHandle(),
            nativeLayerId.view,
            minZoom,
            completion,
          )
        )
      }
    }

  public actual fun setLayerMaxZoom(layerId: String, maxZoom: Double): Deferred<CommandCompletion> =
    command { completion ->
      NativeAccess.ensureLoaded()
      StringViewScope(layerId).use { nativeLayerId ->
        Status.check(
          MaplibreNativeC.mln_map_set_layer_max_zoom(
            requireLiveHandle(),
            nativeLayerId.view,
            maxZoom,
            completion,
          )
        )
      }
    }

  public actual fun setLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility,
  ): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    StringViewScope(layerId).use { nativeLayerId ->
      Status.check(
        MaplibreNativeC.mln_map_set_layer_visibility(
          requireLiveHandle(),
          nativeLayerId.view,
          visibility.nativeValue,
          completion,
        )
      )
    }
  }

  public actual fun requestRepaint(): Deferred<CommandCompletion> = command { completion ->
    NativeAccess.ensureLoaded()
    Status.check(MaplibreNativeC.mln_map_request_repaint(requireLiveHandle(), completion))
  }

  public actual fun requestStillImage(): Deferred<CommandCompletion> =
    CompletionBridge.command { completion ->
      MaplibreNativeC.mln_map_request_still_image(requireLiveHandle(), completion)
    }

  public actual fun snapshot(): MapSnapshot {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_map_snapshot().use { value ->
      value.size(value.sizeof())
      Status.check(MaplibreNativeC.mln_map_snapshot_get(requireLiveHandle(), value))
      val extent = value.logical_extent()
      return MapSnapshot(
        value.generation(),
        debugOptions(value.debug_options()),
        cameraOptions(value.camera()),
        MapSize(extent.width(), extent.height(), extent.scale_factor()),
        projectionModeOptions(value.projection_mode()),
        viewportOptions(value.viewport()),
        value.fully_loaded(),
        value.rendering_stats_view_enabled(),
        value.repaint_demand(),
        value.latest_render_update_generation(),
        tileOptions(value.tile()),
        boundOptions(value.bounds()),
        freeCameraOptions(value.free_camera()),
      )
    }
  }

  public actual fun setDebugOptions(options: Set<DebugOption>): Deferred<CommandCompletion> =
    command { completion ->
      NativeAccess.ensureLoaded()
      Status.check(
        MaplibreNativeC.mln_map_set_debug_options(
          requireLiveHandle(),
          debugOptionMask(options),
          completion,
        )
      )
    }

  public actual fun setRenderingStatsViewEnabled(enabled: Boolean): Deferred<CommandCompletion> =
    command { completion ->
      NativeAccess.ensureLoaded()
      Status.check(
        MaplibreNativeC.mln_map_set_rendering_stats_view_enabled(
          requireLiveHandle(),
          enabled,
          completion,
        )
      )
    }

  public actual fun setViewportOptions(options: ViewportOptions): Deferred<CommandCompletion> =
    ViewportOptionsScope(options).use { nativeOptions ->
      command { completion ->
        Status.check(
          MaplibreNativeC.mln_map_set_viewport_options(
            requireLiveHandle(),
            nativeOptions.options,
            completion,
          )
        )
      }
    }

  public actual fun setTileOptions(options: TileOptions): Deferred<CommandCompletion> =
    TileOptionsScope(options).use { nativeOptions ->
      command { completion ->
        Status.check(
          MaplibreNativeC.mln_map_set_tile_options(
            requireLiveHandle(),
            nativeOptions.options,
            completion,
          )
        )
      }
    }

  public actual fun setBounds(options: BoundOptions): Deferred<CommandCompletion> =
    BoundOptionsScope(options).use { nativeOptions ->
      command { completion ->
        Status.check(
          MaplibreNativeC.mln_map_set_bounds(requireLiveHandle(), nativeOptions.options, completion)
        )
      }
    }

  public actual fun setFreeCameraOptions(options: FreeCameraOptions): Deferred<CommandCompletion> =
    FreeCameraOptionsScope(options).use { nativeOptions ->
      command { completion ->
        Status.check(
          MaplibreNativeC.mln_map_set_free_camera_options(
            requireLiveHandle(),
            nativeOptions.options,
            completion,
          )
        )
      }
    }

  public actual fun resize(size: MapSize): Deferred<CommandCompletion> =
    MaplibreNativeC.mln_logical_extent().use { extent ->
      extent.width(size.width).height(size.height).scale_factor(size.scaleFactor)
      command { completion ->
        Status.check(MaplibreNativeC.mln_map_resize(requireLiveHandle(), extent, completion))
      }
    }

  public actual fun cameraSnapshot(): CameraSnapshot {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_camera_options_default().use { outCamera ->
      val outGeneration = longArrayOf(0L)
      Status.check(
        MaplibreNativeC.mln_map_camera_snapshot_get(requireLiveHandle(), outCamera, outGeneration)
      )
      return CameraSnapshot(outGeneration[0], cameraOptions(outCamera))
    }
  }

  public actual fun updateCamera(update: CameraUpdate): Deferred<CommandCompletion> =
    CameraOptionsScope(update.camera).use { nativeCamera ->
      AnimationOptionsScope(update.animation).use { nativeAnimation ->
        MaplibreNativeC.mln_camera_update_default().use { nativeUpdate ->
          nativeUpdate
            .mode(update.mode.nativeValue)
            .camera(nativeCamera.options)
            .animation(nativeAnimation.options)
            .gesture_phase(update.gesturePhase.nativeValue)
          command { completion ->
            Status.check(
              MaplibreNativeC.mln_map_update_camera(requireLiveHandle(), nativeUpdate, completion)
            )
          }
        }
      }
    }

  public actual fun applyCameraDelta(delta: CameraDelta): Deferred<CommandCompletion> =
    AnimationOptionsScope(delta.animation).use { nativeAnimation ->
      screenPoint(delta.offset).use { nativeOffset ->
        val nativeAnchor = delta.anchor?.let(::screenPoint)
        try {
          MaplibreNativeC.mln_camera_delta_default().use { nativeDelta ->
            nativeDelta
              .kind(delta.kind.nativeValue)
              .offset(nativeOffset)
              .amount(delta.amount)
              .has_anchor(nativeAnchor != null)
              .animation(nativeAnimation.options)
            nativeAnchor?.let(nativeDelta::anchor)
            command { completion ->
              Status.check(
                MaplibreNativeC.mln_map_apply_camera_delta(
                  requireLiveHandle(),
                  nativeDelta,
                  completion,
                )
              )
            }
          }
        } finally {
          nativeAnchor?.close()
        }
      }
    }

  public actual fun queryCamera(): Deferred<CameraSnapshot> =
    CompletionBridge.submit(
      { result ->
        val raw = MaplibreNativeC.mln_camera_query_result(result.value())
        CameraSnapshot(raw.generation(), cameraOptions(raw.camera()))
      },
      { completion -> MaplibreNativeC.mln_map_camera_query(requireLiveHandle(), completion) },
    )

  public actual fun attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachMetalOwnedTexture(this, descriptor, options)

  public actual fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachMetalBorrowedTexture(this, descriptor, options)

  public actual fun attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachVulkanOwnedTexture(this, descriptor, options)

  public actual fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachVulkanBorrowedTexture(this, descriptor, options)

  public actual fun attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachOpenGLOwnedTexture(this, descriptor, options)

  public actual fun attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachOpenGLBorrowedTexture(this, descriptor, options)

  public actual fun attachMetalSurface(
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment = RenderSessionHandle.attachMetalSurface(this, descriptor, options)

  public actual fun attachVulkanSurface(
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment = RenderSessionHandle.attachVulkanSurface(this, descriptor, options)

  public actual fun attachOpenGLSurface(
    descriptor: OpenGLSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment = RenderSessionHandle.attachOpenGLSurface(this, descriptor, options)

  public actual fun createProjection(): Deferred<MapProjectionHandle> =
    CompletionBridge.submit(
      { result ->
        val address = LongPointer(result.value()).get()
        require(address != 0L) { "mln_map_projection_create returned a null projection" }
        MapProjectionHandle(address)
      },
      { completion -> MaplibreNativeC.mln_map_projection_create(requireLiveHandle(), completion) },
    )

  public actual fun close() {
    if (!core.beginClose()) return
    try {
      Status.check(MaplibreNativeC.mln_map_release(handleId))
    } catch (error: Throwable) {
      core.abortClose()
      throw error
    }
    core.completeClose { runtime.unregisterMap(this) }
  }

  internal fun nativeHandleId(): Long = handleId

  private fun addTileSourceUrl(
    function:
      (
        Long,
        MaplibreNativeC.mln_buffer_view,
        MaplibreNativeC.mln_buffer_view,
        MaplibreNativeC.mln_style_tile_source_options,
        MaplibreNativeC.mln_completion,
      ) -> Int,
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
    completion: MaplibreNativeC.mln_completion,
  ) {
    StringViewScope(sourceId).use { nativeSourceId ->
      StringViewScope(url).use { nativeUrl ->
        TileSourceOptionsScope(options).use { nativeOptions ->
          Status.check(
            function(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeUrl.view,
              nativeOptions.options,
              completion,
            )
          )
        }
      }
    }
  }

  private fun addTileSourceTiles(
    function:
      (
        Long,
        MaplibreNativeC.mln_buffer_view,
        MaplibreNativeC.mln_buffer_view?,
        Long,
        MaplibreNativeC.mln_style_tile_source_options,
        MaplibreNativeC.mln_completion,
      ) -> Int,
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
    completion: MaplibreNativeC.mln_completion,
  ) {
    val tileSnapshot = tiles.toList()
    StringViewScope(sourceId).use { nativeSourceId ->
      StringViewArrayScope(tileSnapshot).use { nativeTiles ->
        TileSourceOptionsScope(options).use { nativeOptions ->
          Status.check(
            function(
              requireLiveHandle(),
              nativeSourceId.view,
              nativeTiles.views,
              nativeTiles.count,
              nativeOptions.options,
              completion,
            )
          )
        }
      }
    }
  }

  public actual companion object {
    public actual fun create(runtime: RuntimeHandle, options: MapOptions): Deferred<MapHandle> {
      NativeAccess.ensureLoaded()
      return MapOptionsScope(options).use { nativeOptions ->
        CompletionBridge.submit(
          { result ->
            val address = LongPointer(result.value()).get()
            require(address != 0L) { "mln_map_create returned a null map" }
            MapHandle(runtime, address, options.eventMask).also(runtime::registerMap)
          },
          { completion ->
            MaplibreNativeC.mln_map_create(
              runtime.nativeHandleId(),
              nativeOptions.options,
              completion,
            )
          },
        )
      }
    }
  }

  private fun requireLiveHandle(): Long {
    core.requireLive()
    return handleId
  }
}

private inline fun command(
  crossinline call: (MaplibreNativeC.mln_completion) -> Unit
): Deferred<CommandCompletion> = CompletionBridge.command { completion ->
  call(completion)
  MaplibreNativeC.MLN_STATUS_OK
}

private fun latLng(value: LatLng): MaplibreNativeC.mln_lat_lng =
  MaplibreNativeC.mln_lat_lng().latitude(value.latitude).longitude(value.longitude)

private fun latLng(value: MaplibreNativeC.mln_lat_lng): LatLng =
  LatLng(value.latitude(), value.longitude())

private fun latLngBounds(value: LatLngBounds): MaplibreNativeC.mln_lat_lng_bounds =
  MaplibreNativeC.mln_lat_lng_bounds()
    .southwest(latLng(value.southwest))
    .northeast(latLng(value.northeast))

private fun latLngBounds(value: MaplibreNativeC.mln_lat_lng_bounds): LatLngBounds =
  LatLngBounds(latLng(value.southwest()), latLng(value.northeast()))

private fun screenPoint(value: ScreenPoint): MaplibreNativeC.mln_screen_point =
  MaplibreNativeC.mln_screen_point().x(value.x).y(value.y)

private fun screenPoint(value: MaplibreNativeC.mln_screen_point): ScreenPoint =
  ScreenPoint(value.x(), value.y())

private const val IMAGE_SOURCE_COORDINATE_COUNT: Long = 4

private fun optionalCString(value: String): BytePointer {
  return JavaCppSupport.cString(value)
}

private fun stringView(view: MaplibreNativeC.mln_buffer_view): String {
  if (view.size() == 0L || view.data() == null || view.data().isNull) return ""
  val bytes = ByteArray(Math.toIntExact(view.size()))
  BytePointer(view.data()).get(bytes, 0, bytes.size)
  return String(bytes, java.nio.charset.StandardCharsets.UTF_8)
}

private fun bufferView(view: MaplibreNativeC.mln_buffer_view): ByteArray =
  JavaCppSupport.byteArray(view.data(), view.size())

private fun optionalBuffer(result: MaplibreNativeC.mln_completion_result): ByteArray? =
  if (result.value_count() == 0L) null
  else bufferView(MaplibreNativeC.mln_buffer_view(result.value()))

private fun requiredBuffer(result: MaplibreNativeC.mln_completion_result): ByteArray =
  requireNotNull(optionalBuffer(result)) { "native completion omitted its byte result" }

private fun stringList(result: MaplibreNativeC.mln_completion_result): List<String> {
  val count = Math.toIntExact(result.value_count())
  if (count == 0) return emptyList()
  val views = MaplibreNativeC.mln_buffer_view(result.value())
  return List(count) { index -> stringView(views.position(index.toLong())) }
}

private fun styleIdList(list: Long): List<String> =
  try {
    SizeTPointer(1).use { outCount ->
      Status.check(MaplibreNativeC.mln_style_id_list_count(list, outCount))
      List(Math.toIntExact(outCount.get())) { index ->
        MaplibreNativeC.mln_buffer_view().use { outId ->
          Status.check(MaplibreNativeC.mln_style_id_list_get(list, index.toLong(), outId))
          stringView(outId)
        }
      }
    }
  } finally {
    MaplibreNativeC.mln_style_id_list_destroy(list)
  }

private fun styleStringList(list: Long): List<String> =
  styleStringList(
    list,
    counter = MaplibreNativeC::mln_style_string_list_count,
    getter = MaplibreNativeC::mln_style_string_list_get,
    destroyer = MaplibreNativeC::mln_style_string_list_destroy,
  )

private fun styleStringList(
  list: Long,
  counter: (Long, SizeTPointer) -> Int,
  getter: (Long, Long, MaplibreNativeC.mln_buffer_view) -> Int,
  destroyer: (Long) -> Unit,
): List<String> =
  try {
    SizeTPointer(1).use { outCount ->
      Status.check(counter(list, outCount))
      List(Math.toIntExact(outCount.get())) { index ->
        MaplibreNativeC.mln_buffer_view().use { outValue ->
          Status.check(getter(list, index.toLong(), outValue))
          stringView(outValue)
        }
      }
    }
  } finally {
    destroyer(list)
  }

private const val STYLE_SOURCE_INFO_URL: Int = 1 shl 0
private const val STYLE_SOURCE_INFO_TILEJSON: Int = 1 shl 1
private const val STYLE_SOURCE_INFO_BOUNDS: Int = 1 shl 2
private const val STYLE_SOURCE_INFO_TILE_SIZE: Int = 1 shl 3
private const val STYLE_SOURCE_INFO_VECTOR_ENCODING: Int = 1 shl 4
private const val STYLE_SOURCE_INFO_RASTER_ENCODING: Int = 1 shl 5

private fun readStretches(
  array: MaplibreNativeC.mln_image_stretch?,
  count: Int,
): List<ImageStretch> =
  List(count) { index ->
    val element = array!!.position(index.toLong())
    ImageStretch(element.from(), element.to())
  }

private fun styleImageInfo(info: MaplibreNativeC.mln_style_image_info): StyleImageInfo =
  StyleImageInfo(
    info.width(),
    info.height(),
    info.stride(),
    checkedSizeT(info.byte_length(), "style image byte length"),
    info.pixel_ratio(),
    info.sdf(),
    checkedSizeT(info.stretch_x_count(), "style image stretch X count"),
    checkedSizeT(info.stretch_y_count(), "style image stretch Y count"),
    if (info.has_content()) {
      val content = info.content()
      ImageContent(content.left(), content.top(), content.right(), content.bottom())
    } else null,
    if (info.has_text_fit_width()) StyleImageTextFit.fromNative(info.text_fit_width()) else null,
    if (info.has_text_fit_height()) StyleImageTextFit.fromNative(info.text_fit_height()) else null,
  )

private fun checkedSizeT(value: Long, name: String): Long {
  require(value >= 0L) { "$name exceeds Long.MAX_VALUE" }
  return value
}

private fun debugOptionMask(options: Set<DebugOption>): Int =
  options.fold(0) { acc, option -> acc or option.nativeMask }

private fun debugOptions(mask: Int): Set<DebugOption> =
  DebugOption.entries.filterTo(mutableSetOf()) { option -> (mask and option.nativeMask) != 0 }

private fun edgeInsets(value: MaplibreNativeC.mln_edge_insets): EdgeInsets =
  EdgeInsets(value.top(), value.left(), value.bottom(), value.right())

private fun writeEdgeInsets(out: MaplibreNativeC.mln_edge_insets, value: EdgeInsets) {
  out.top(value.top).left(value.left).bottom(value.bottom).right(value.right)
}

private fun viewportOptions(value: MaplibreNativeC.mln_map_viewport_options): ViewportOptions {
  val fields = value.fields()
  return ViewportOptions().apply {
    if ((fields and MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0) {
      northOrientation = NorthOrientation.fromNative(value.north_orientation())
    }
    if ((fields and MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0) {
      constrainMode = ConstrainMode.fromNative(value.constrain_mode())
    }
    if ((fields and MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0) {
      viewportMode = ViewportMode.fromNative(value.viewport_mode())
    }
    if ((fields and MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0) {
      frustumOffset = edgeInsets(value.frustum_offset())
    }
  }
}

private fun tileOptions(value: MaplibreNativeC.mln_map_tile_options): TileOptions {
  val fields = value.fields()
  return TileOptions().apply {
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0) {
      prefetchZoomDelta = value.prefetch_zoom_delta()
    }
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0) {
      lodMinRadius = value.lod_min_radius()
    }
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_SCALE) != 0) {
      lodScale = value.lod_scale()
    }
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0) {
      lodPitchThreshold = value.lod_pitch_threshold()
    }
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0) {
      lodZoomShift = value.lod_zoom_shift()
    }
    if ((fields and MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MODE) != 0) {
      lodMode = TileLodMode.fromNative(value.lod_mode())
    }
  }
}

private fun projectionModeOptions(
  value: MaplibreNativeC.mln_projection_mode
): ProjectionModeOptions {
  val fields = value.fields()
  return ProjectionModeOptions().apply {
    if ((fields and MaplibreNativeC.MLN_PROJECTION_MODE_AXONOMETRIC) != 0) {
      axonometric = value.axonometric()
    }
    if ((fields and MaplibreNativeC.MLN_PROJECTION_MODE_X_SKEW) != 0) {
      xSkew = value.x_skew()
    }
    if ((fields and MaplibreNativeC.MLN_PROJECTION_MODE_Y_SKEW) != 0) {
      ySkew = value.y_skew()
    }
  }
}

private fun cameraOptions(value: MaplibreNativeC.mln_camera_options): CameraOptions {
  val fields = value.fields()
  return CameraOptions().apply {
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_CENTER) != 0) {
      center = LatLng(value.latitude(), value.longitude())
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0) {
      centerAltitude = value.center_altitude()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_PADDING) != 0) {
      padding = edgeInsets(value.padding())
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ANCHOR) != 0) {
      anchor = screenPoint(value.anchor())
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ZOOM) != 0) {
      zoom = value.zoom()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_BEARING) != 0) {
      bearing = value.bearing()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_PITCH) != 0) {
      pitch = value.pitch()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ROLL) != 0) {
      roll = value.roll()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_FOV) != 0) {
      fieldOfView = value.field_of_view()
    }
  }
}

private fun boundOptions(value: MaplibreNativeC.mln_bound_options): BoundOptions {
  val fields = value.fields()
  return BoundOptions().apply {
    if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_BOUNDS) != 0) {
      bounds = BoundsConstraint.Bounded(latLngBounds(value.bounds()))
    } else if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_UNBOUNDED) != 0) {
      bounds = BoundsConstraint.Unbounded
    }
    if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_MIN_ZOOM) != 0) {
      minZoom = value.min_zoom()
    }
    if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_MAX_ZOOM) != 0) {
      maxZoom = value.max_zoom()
    }
    if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_MIN_PITCH) != 0) {
      minPitch = value.min_pitch()
    }
    if ((fields and MaplibreNativeC.MLN_BOUND_OPTION_MAX_PITCH) != 0) {
      maxPitch = value.max_pitch()
    }
  }
}

private fun freeCameraOptions(value: MaplibreNativeC.mln_free_camera_options): FreeCameraOptions {
  val fields = value.fields()
  return FreeCameraOptions().apply {
    if ((fields and MaplibreNativeC.MLN_FREE_CAMERA_OPTION_POSITION) != 0) {
      position = Vec3(value._position().x(), value._position().y(), value._position().z())
    }
    if ((fields and MaplibreNativeC.MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0) {
      orientation =
        Quaternion(
          value.orientation().x(),
          value.orientation().y(),
          value.orientation().z(),
          value.orientation().w(),
        )
    }
  }
}

private class StringViewScope(value: String) : AutoCloseable {
  private val bytes: BytePointer
  val view: MaplibreNativeC.mln_buffer_view = MaplibreNativeC.mln_buffer_view()

  init {
    val utf8 = value.toByteArray(java.nio.charset.StandardCharsets.UTF_8)
    bytes = BytePointer(Math.max(utf8.size, 1).toLong())
    if (utf8.isNotEmpty()) bytes.put(utf8, 0, utf8.size)
    view.data(if (utf8.isEmpty()) null else bytes)
    view.size(utf8.size.toLong())
  }

  override fun close() {
    view.close()
    bytes.close()
  }
}

private class StringViewArrayScope(values: List<String>) : AutoCloseable {
  private val strings: List<StringViewScope> = values.map(::StringViewScope)
  val views: MaplibreNativeC.mln_buffer_view? =
    if (strings.isEmpty()) null else MaplibreNativeC.mln_buffer_view(strings.size.toLong())
  val count: Long = strings.size.toLong()

  init {
    strings.forEachIndexed { index, string ->
      views?.position(index.toLong())?.put<MaplibreNativeC.mln_buffer_view>(string.view)
    }
    views?.position(0)
  }

  override fun close() {
    views?.close()
    strings.asReversed().forEach(StringViewScope::close)
  }
}

private class LatLngArrayScope : AutoCloseable {
  val coordinates: MaplibreNativeC.mln_lat_lng
  val count: Long

  constructor(values: List<LatLng>) {
    val coordinateSnapshot = values.toList()
    coordinates = MaplibreNativeC.mln_lat_lng(coordinateSnapshot.size.toLong())
    count = coordinateSnapshot.size.toLong()
    coordinateSnapshot.forEachIndexed { index, coordinate ->
      coordinates
        .position(index.toLong())
        .latitude(coordinate.latitude)
        .longitude(coordinate.longitude)
    }
    coordinates.position(0)
  }

  constructor(count: Long) {
    coordinates = MaplibreNativeC.mln_lat_lng(count)
    this.count = count
  }

  fun toList(count: Int): List<LatLng> =
    List(count) { index ->
        val coordinate = coordinates.position(index.toLong())
        LatLng(coordinate.latitude(), coordinate.longitude())
      }
      .also { coordinates.position(0) }

  override fun close() {
    coordinates.close()
  }
}

private class CameraOptionsScope(value: CameraOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_camera_options = MaplibreNativeC.mln_camera_options_default()

  init {
    var fields = 0
    value.center?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_CENTER
      options.latitude(it.latitude).longitude(it.longitude)
    }
    value.centerAltitude?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE
      options.center_altitude(it)
    }
    value.padding?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_PADDING
      writeEdgeInsets(options.padding(), it)
    }
    value.anchor?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_ANCHOR
      options.anchor(screenPoint(it))
    }
    value.zoom?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_ZOOM
      options.zoom(it)
    }
    value.bearing?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_BEARING
      options.bearing(it)
    }
    value.pitch?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_PITCH
      options.pitch(it)
    }
    value.roll?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_ROLL
      options.roll(it)
    }
    value.fieldOfView?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_FOV
      options.field_of_view(it)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private class AnimationOptionsScope(value: AnimationOptions?) : AutoCloseable {
  val options: MaplibreNativeC.mln_animation_options? = value?.let {
    MaplibreNativeC.mln_animation_options_default()
  }

  init {
    if (value != null && options != null) {
      var fields = 0
      value.durationMs?.let {
        fields = fields or MaplibreNativeC.MLN_ANIMATION_OPTION_DURATION
        options.duration_ms(it)
      }
      value.velocity?.let {
        fields = fields or MaplibreNativeC.MLN_ANIMATION_OPTION_VELOCITY
        options.velocity(it)
      }
      value.minZoom?.let {
        fields = fields or MaplibreNativeC.MLN_ANIMATION_OPTION_MIN_ZOOM
        options.min_zoom(it)
      }
      value.easing?.let {
        fields = fields or MaplibreNativeC.MLN_ANIMATION_OPTION_EASING
        options.easing(MaplibreNativeC.mln_unit_bezier().x1(it.x1).y1(it.y1).x2(it.x2).y2(it.y2))
      }
      value.transitionId?.let {
        fields = fields or MaplibreNativeC.MLN_ANIMATION_OPTION_TRANSITION_ID
        options.transition_id(it)
      }
      options.fields(fields)
    }
  }

  override fun close() {
    options?.close()
  }
}

private class BoundOptionsScope(value: BoundOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_bound_options = MaplibreNativeC.mln_bound_options_default()

  init {
    var fields = 0
    when (val constraint = value.bounds) {
      is BoundsConstraint.Bounded -> {
        fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_BOUNDS
        options.bounds(latLngBounds(constraint.bounds))
      }
      BoundsConstraint.Unbounded -> {
        fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_UNBOUNDED
      }
      null -> {}
    }
    value.minZoom?.let {
      fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_MIN_ZOOM
      options.min_zoom(it)
    }
    value.maxZoom?.let {
      fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_MAX_ZOOM
      options.max_zoom(it)
    }
    value.minPitch?.let {
      fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_MIN_PITCH
      options.min_pitch(it)
    }
    value.maxPitch?.let {
      fields = fields or MaplibreNativeC.MLN_BOUND_OPTION_MAX_PITCH
      options.max_pitch(it)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private class FreeCameraOptionsScope(value: FreeCameraOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_free_camera_options =
    MaplibreNativeC.mln_free_camera_options_default()

  init {
    var fields = 0
    value.position?.let {
      fields = fields or MaplibreNativeC.MLN_FREE_CAMERA_OPTION_POSITION
      options._position(MaplibreNativeC.mln_vec3().x(it.x).y(it.y).z(it.z))
    }
    value.orientation?.let {
      fields = fields or MaplibreNativeC.MLN_FREE_CAMERA_OPTION_ORIENTATION
      options.orientation(MaplibreNativeC.mln_quaternion().x(it.x).y(it.y).z(it.z).w(it.w))
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private class CanonicalTileIdScope(value: CanonicalTileId) : AutoCloseable {
  val tileId: MaplibreNativeC.mln_canonical_tile_id =
    MaplibreNativeC.mln_canonical_tile_id().z(value.z).x(value.x.toInt()).y(value.y.toInt())

  override fun close() {
    tileId.close()
  }
}

/**
 * Owns map/style-scoped custom geometry source callback state.
 *
 * Native invokes [onReleased] after it stops referencing this state, which drops it from its map's
 * registry and closes it.
 */
internal class CustomGeometrySourceState(
  private val options: CustomGeometrySourceOptions,
  private val onReleased: () -> Unit,
) : AutoCloseable {
  private val token = TOKENS.getAndIncrement()
  private val gate = CallbackGate("custom geometry callbacks", ::closeNative)
  // JavaCPP passes null for a null void* and drops the upcall if the Kotlin
  // override rejects it, so every parameter stays nullable.
  private val fetchTile =
    object : MaplibreNativeC.mln_custom_geometry_source_tile_callback() {
      override fun call(userData: Pointer?, tileId: MaplibreNativeC.mln_canonical_tile_id?) {
        fetchTile(tileId)
      }
    }
  private val cancelTile =
    object : MaplibreNativeC.mln_custom_geometry_source_tile_callback() {
      override fun call(userData: Pointer?, tileId: MaplibreNativeC.mln_canonical_tile_id?) {
        cancelTile(tileId)
      }
    }
  val descriptor: MaplibreNativeC.mln_custom_geometry_source_options =
    MaplibreNativeC.mln_custom_geometry_source_options_default()

  init {
    descriptor.fetch_tile(fetchTile)
    descriptor.cancel_tile(cancelTile)
    descriptor.release_user_data(RELEASE_CALLBACK)
    descriptor.user_data(AddressPointer(token))
    STATES[token] = this
    var fields = 0
    options.minZoom?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
      descriptor.min_zoom(it)
    }
    options.maxZoom?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
      descriptor.max_zoom(it)
    }
    options.tolerance?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
      descriptor.tolerance(it)
    }
    options.tileSize?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
      descriptor.tile_size(it)
    }
    options.buffer?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
      descriptor.buffer(it)
    }
    options.clip?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
      descriptor.clip(it)
    }
    options.wrap?.let {
      fields = fields or MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
      descriptor.wrap(it)
    }
    descriptor.fields(fields)
  }

  override fun close() {
    gate.close()
  }

  internal fun fetchTileForTesting(tileId: CanonicalTileId) {
    val lease = gate.enter() ?: return
    try {
      options.callback.fetchTile(tileId)
    } catch (_: Throwable) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      lease.close()
    }
  }

  internal fun isClosedForTesting(): Boolean = gate.isClosedForTesting()

  private fun fetchTile(tileId: MaplibreNativeC.mln_canonical_tile_id?) {
    if (tileId == null) return
    fetchTileForTesting(canonicalTileId(tileId))
  }

  private fun cancelTile(tileId: MaplibreNativeC.mln_canonical_tile_id?) {
    if (tileId == null) return
    val lease = gate.enter() ?: return
    try {
      options.callback.cancelTile(canonicalTileId(tileId))
    } catch (_: Throwable) {
      // Native callbacks must not unwind through the C ABI.
    } finally {
      lease.close()
    }
  }

  private fun closeNative() {
    STATES.remove(token)
    descriptor.close()
    fetchTile.close()
    cancelTile.close()
  }

  private fun canonicalTileId(tileId: MaplibreNativeC.mln_canonical_tile_id): CanonicalTileId =
    CanonicalTileId(
      tileId.z(),
      Integer.toUnsignedLong(tileId.x()),
      Integer.toUnsignedLong(tileId.y()),
    )

  private companion object {
    private val TOKENS = AtomicLong(1)

    /** The live states by token, which is the `user_data` this binding hands to native. */
    private val STATES = ConcurrentHashMap<Long, CustomGeometrySourceState>()

    /**
     * One process-wide release callback, so releasing a state can close that state's own callbacks.
     * A per-state callback would be one of the callbacks it has to close.
     */
    private val RELEASE_CALLBACK =
      object : MaplibreNativeC.mln_custom_geometry_source_release_callback() {
        override fun call(userData: Pointer?) {
          try {
            STATES[userData?.address() ?: 0L]?.onReleased?.invoke()
          } catch (_: Throwable) {
            // Native callbacks must not unwind through the C ABI.
          }
        }
      }
  }
}

private class TileSourceOptionsScope(value: TileSourceOptions?) : AutoCloseable {
  private val attribution: StringViewScope? = value?.attribution?.let(::StringViewScope)
  val options: MaplibreNativeC.mln_style_tile_source_options =
    MaplibreNativeC.mln_style_tile_source_options_default()

  init {
    var fields = 0
    value?.minZoom?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
      options.min_zoom(it)
    }
    value?.maxZoom?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM
      options.max_zoom(it)
    }
    attribution?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
      options.attribution(it.view)
    }
    value?.scheme?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME
      options.scheme(it.nativeValue)
    }
    value?.bounds?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS
      options.bounds().southwest().latitude(it.southwest.latitude)
      options.bounds().southwest().longitude(it.southwest.longitude)
      options.bounds().northeast().latitude(it.northeast.latitude)
      options.bounds().northeast().longitude(it.northeast.longitude)
    }
    value?.tileSize?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
      options.tile_size(it)
    }
    value?.vectorEncoding?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
      options.vector_encoding(it.nativeValue)
    }
    value?.rasterDemEncoding?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
      options.raster_encoding(it.nativeValue)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
    attribution?.close()
  }
}

private class ViewportOptionsScope(value: ViewportOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_map_viewport_options =
    MaplibreNativeC.mln_map_viewport_options_default()

  init {
    var fields = 0
    value.northOrientation?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown north orientation cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
      options.north_orientation(it.nativeValue)
    }
    value.constrainMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown constrain mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
      options.constrain_mode(it.nativeValue)
    }
    value.viewportMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown viewport mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
      options.viewport_mode(it.nativeValue)
    }
    value.frustumOffset?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET
      writeEdgeInsets(options.frustum_offset(), it)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private class TileOptionsScope(value: TileOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_map_tile_options = MaplibreNativeC.mln_map_tile_options_default()

  init {
    var fields = 0
    value.prefetchZoomDelta?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
      options.prefetch_zoom_delta(it)
    }
    value.lodMinRadius?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS
      options.lod_min_radius(it)
    }
    value.lodScale?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_SCALE
      options.lod_scale(it)
    }
    value.lodPitchThreshold?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
      options.lod_pitch_threshold(it)
    }
    value.lodZoomShift?.let {
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT
      options.lod_zoom_shift(it)
    }
    value.lodMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown tile LOD mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MODE
      options.lod_mode(it.nativeValue)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private class ProjectionModeOptionsScope(value: ProjectionModeOptions) : AutoCloseable {
  val mode: MaplibreNativeC.mln_projection_mode = MaplibreNativeC.mln_projection_mode_default()

  init {
    var fields = 0
    value.axonometric?.let {
      fields = fields or MaplibreNativeC.MLN_PROJECTION_MODE_AXONOMETRIC
      mode.axonometric(it)
    }
    value.xSkew?.let {
      fields = fields or MaplibreNativeC.MLN_PROJECTION_MODE_X_SKEW
      mode.x_skew(it)
    }
    value.ySkew?.let {
      fields = fields or MaplibreNativeC.MLN_PROJECTION_MODE_Y_SKEW
      mode.y_skew(it)
    }
    mode.fields(fields)
  }

  override fun close() {
    mode.close()
  }
}

private class PremultipliedImageScope(value: PremultipliedRgba8Image) : AutoCloseable {
  private val pixels: BytePointer
  val image: MaplibreNativeC.mln_premultiplied_rgba8_image =
    MaplibreNativeC.mln_premultiplied_rgba8_image_default()

  init {
    val bytes = value.pixels
    pixels = BytePointer(bytes.size.toLong())
    pixels.put(bytes, 0, bytes.size)
    image.width(value.width)
    image.height(value.height)
    image.stride(value.stride)
    image.pixels(pixels)
    image.byte_length(bytes.size.toLong())
  }

  override fun close() {
    image.close()
    pixels.close()
  }
}

private class StyleImageOptionsScope(value: StyleImageOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_style_image_options =
    MaplibreNativeC.mln_style_image_options_default()

  // Native borrows the stretch arrays for the call, so this scope owns them.
  private val stretchX: MaplibreNativeC.mln_image_stretch? = allocStretches(value.stretchX)
  private val stretchY: MaplibreNativeC.mln_image_stretch? = allocStretches(value.stretchY)

  init {
    var fields = 0
    value.pixelRatio?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
      options.pixel_ratio(it)
    }
    value.sdf?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_SDF
      options.sdf(it)
    }
    value.stretchX?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_STRETCH_X
      options.stretch_x(stretchX)
      options.stretch_x_count(it.size.toLong())
    }
    value.stretchY?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_STRETCH_Y
      options.stretch_y(stretchY)
      options.stretch_y_count(it.size.toLong())
    }
    value.content?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_CONTENT
      options.content().left(it.left).top(it.top).right(it.right).bottom(it.bottom)
    }
    value.textFitWidth?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH
      options.text_fit_width(it.nativeValue)
    }
    value.textFitHeight?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT
      options.text_fit_height(it.nativeValue)
    }
    options.fields(fields)
  }

  override fun close() {
    stretchX?.close()
    stretchY?.close()
    options.close()
  }
}

private class StyleTransitionOptionsScope(value: StyleTransitionOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_style_transition_options =
    MaplibreNativeC.mln_style_transition_options_default()

  init {
    var fields = 0
    value.enablePlacementTransitions?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
      options.enable_placement_transitions(it)
    }
    value.durationMs?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_DURATION
      options.duration_ms(it)
    }
    value.delayMs?.let {
      fields = fields or MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_DELAY
      options.delay_ms(it)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private fun styleTransitionOptions(
  options: MaplibreNativeC.mln_style_transition_options
): StyleTransitionOptions =
  StyleTransitionOptions().apply {
    val fields = options.fields()
    durationMs =
      if (fields and MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_DURATION != 0) {
        options.duration_ms()
      } else {
        null
      }
    delayMs =
      if (fields and MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_DELAY != 0) {
        options.delay_ms()
      } else {
        null
      }
    enablePlacementTransitions =
      if (
        fields and MaplibreNativeC.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS != 0
      ) {
        options.enable_placement_transitions()
      } else {
        null
      }
  }

private fun allocStretches(stretches: List<ImageStretch>?): MaplibreNativeC.mln_image_stretch? {
  if (stretches.isNullOrEmpty()) return null
  val array = MaplibreNativeC.mln_image_stretch(stretches.size.toLong())
  stretches.forEachIndexed { index, stretch ->
    array.position(index.toLong()).from(stretch.from).to(stretch.to)
  }
  return array.position(0)
}

private class MapOptionsScope(value: MapOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_map_options = MaplibreNativeC.mln_map_options_default()

  init {
    value.width?.let { options.initial_extent().width(it) }
    value.height?.let { options.initial_extent().height(it) }
    value.scaleFactor?.let { options.initial_extent().scale_factor(it) }
    value.mapMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown map mode cannot be used as input: ${it.nativeValue}"
      }
      options.map_mode(it.nativeValue)
    }
    value.fastPforEnabled?.let { options.fast_pfor_enabled(it) }
    options.event_mask(value.eventMask.nativeValue)
  }

  override fun close() {
    options.close()
  }
}

private fun releaseCallbackRoot(root: AutoCloseable?) {
  HandleLeakCleaner.releaseNativeCallbackRoot(root)
  closeQuietly(root)
}

private fun closeQuietly(closeable: AutoCloseable?) {
  try {
    closeable?.close()
  } catch (_: Exception) {}
}

/** A `void*` built from a raw address, for backend-native pointers and user data. */
private class AddressPointer(address: Long) : Pointer(null as Pointer?) {
  init {
    this.address = address
  }
}

/** Direct test seam for the JavaCPP map and style descriptor adapter. */
internal object JavaCppMapStructs {
  fun stringViewRoundTrip(value: String): String =
    StringViewScope(value).use { stringView(it.view) }

  fun cameraOptionsRoundTrip(value: CameraOptions): Pair<Int, CameraOptions> =
    CameraOptionsScope(value).use { it.options.fields() to cameraOptions(it.options) }

  fun animationOptionsSnapshot(value: AnimationOptions): AnimationOptionsSnapshot =
    AnimationOptionsScope(value).use {
      val options = requireNotNull(it.options)
      AnimationOptionsSnapshot(
        options.fields(),
        options.duration_ms(),
        options.velocity(),
        options.transition_id(),
      )
    }

  fun viewportOptionsRoundTrip(value: ViewportOptions): ViewportOptions =
    ViewportOptionsScope(value).use { viewportOptions(it.options) }

  fun tileOptionsRoundTrip(value: TileOptions): TileOptions =
    TileOptionsScope(value).use { tileOptions(it.options) }

  fun projectionModeOptionsRoundTrip(value: ProjectionModeOptions): ProjectionModeOptions =
    ProjectionModeOptionsScope(value).use { projectionModeOptions(it.mode) }

  fun styleImageOptionsSnapshot(value: StyleImageOptions): StyleImageOptionsSnapshot =
    StyleImageOptionsScope(value).use {
      StyleImageOptionsSnapshot(it.options.fields(), it.options.pixel_ratio(), it.options.sdf())
    }

  fun styleImageInfoSnapshot(byteLength: Long): StyleImageInfo =
    MaplibreNativeC.mln_style_image_info().use {
      it.width(2).height(3).stride(8).byte_length(byteLength).pixel_ratio(2.0f).sdf(true)
      styleImageInfo(it)
    }

  fun sourceInfoSnapshot(type: Int, volatileSource: Boolean): SourceInfo =
    MaplibreNativeC.mln_style_source_info().use {
      it.type(type).is_volatile(volatileSource)
      SourceInfo(
        SourceType.fromNative(it.type()),
        it.is_volatile(),
        null,
        null,
        null,
        null,
        null,
        null,
      )
    }

  fun styleStringListCleanupAfterCopyFailure(): Int {
    var destroys = 0
    try {
      styleStringList(
        1L,
        counter = { _, outCount ->
          outCount.put(Long.MAX_VALUE)
          org.maplibre.nativeffi.error.MaplibreStatus.OK.nativeCode
        },
        getter = { _, _, _ -> org.maplibre.nativeffi.error.MaplibreStatus.OK.nativeCode },
        destroyer = { destroys++ },
      )
    } catch (_: ArithmeticException) {
      return destroys
    }
    error("style list conversion unexpectedly succeeded")
  }

  fun mapOptionsSnapshot(value: MapOptions): MapOptionsSnapshot =
    MapOptionsScope(value).use {
      val extent = it.options.initial_extent()
      MapOptionsSnapshot(
        extent.width(),
        extent.height(),
        extent.scale_factor(),
        it.options.map_mode(),
        it.options.fast_pfor_enabled(),
      )
    }

  data class StyleImageOptionsSnapshot(val fields: Int, val pixelRatio: Float, val sdf: Boolean)

  data class AnimationOptionsSnapshot(
    val fields: Int,
    val durationMs: Double,
    val velocity: Double,
    val transitionId: Long,
  )

  data class MapOptionsSnapshot(
    val width: Int,
    val height: Int,
    val scaleFactor: Double,
    val mapMode: Int,
    val fastPforEnabled: Boolean,
  )
}
