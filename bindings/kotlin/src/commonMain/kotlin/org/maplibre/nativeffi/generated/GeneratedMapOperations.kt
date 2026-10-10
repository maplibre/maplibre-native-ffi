// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*
import org.maplibre.nativeffi.runtime.CommandCompletion

public abstract class GeneratedMapOperations internal constructor() {
  internal abstract val binding: HandleStateCore
  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  /**
   * Adds a color-relief layer for a raster DEM source.
   *
   * See `mln_map_add_color_relief_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_color_relief_layer") {
      check(
        C.mln_map_add_color_relief_layer(
          handle,
          view(layerId),
          view(sourceId),
          view(beforeLayerId ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a custom geometry source.
   *
   * See `mln_map_add_custom_geometry_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_custom_geometry_source", bindingCallbacks) {
      check(
        C.mln_map_add_custom_geometry_source(
          handle,
          view(sourceId),
          writeCustomGeometrySourceOptions(options),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a custom MVT vector source.
   *
   * See `mln_map_add_custom_mvt_vector_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addCustomMvtVectorSource(
    sourceId: String,
    options: CustomMvtVectorSourceOptions,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_custom_mvt_vector_source", bindingCallbacks) {
      check(
        C.mln_map_add_custom_mvt_vector_source(
          handle,
          view(sourceId),
          writeCustomMvtVectorSourceOptions(options),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a GeoJSON source with prepared inline data.
   *
   * See `mln_map_add_geojson_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addGeojsonSourceData(
    sourceId: String,
    data: GeojsonSourceDataHandle,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_geojson_source_data") {
      check(
        C.mln_map_add_geojson_source_data(
          handle,
          view(sourceId),
          data.binding.handle(),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a GeoJSON source with URL data.
   *
   * See `mln_map_add_geojson_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addGeojsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeojsonSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_geojson_source_url") {
      check(
        C.mln_map_add_geojson_source_url(
          handle,
          view(sourceId),
          view(url),
          options?.let { writeGeojsonSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a hillshade layer for a raster DEM source.
   *
   * See `mln_map_add_hillshade_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_hillshade_layer") {
      check(
        C.mln_map_add_hillshade_layer(
          handle,
          view(layerId),
          view(sourceId),
          view(beforeLayerId ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds an image source with inline image pixels.
   *
   * See `mln_map_add_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_image_source_image") {
      check(
        C.mln_map_add_image_source_image(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          writePremultipliedRgba8Image(image),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds an image source that loads its image from a URL.
   *
   * See `mln_map_add_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_image_source_url") {
      check(
        C.mln_map_add_image_source_url(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          view(url),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a source-free location indicator layer.
   *
   * See `mln_map_add_location_indicator_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_location_indicator_layer") {
      check(
        C.mln_map_add_location_indicator_layer(
          handle,
          view(layerId),
          view(beforeLayerId ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a raster DEM source with inline tile URLs.
   *
   * See `mln_map_add_raster_dem_source_tiles` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_raster_dem_source_tiles") {
      check(
        C.mln_map_add_raster_dem_source_tiles(
          handle,
          view(sourceId),
          array(tiles, 2 * NativeMemory.addressSize, NativeMemory.addressSize) { at, item ->
            putView(at, item)
          },
          tiles.size.toLong(),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a raster DEM source with a TileJSON URL.
   *
   * See `mln_map_add_raster_dem_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_raster_dem_source_url") {
      check(
        C.mln_map_add_raster_dem_source_url(
          handle,
          view(sourceId),
          view(url),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a raster source with inline tile URLs.
   *
   * See `mln_map_add_raster_source_tiles` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_raster_source_tiles") {
      check(
        C.mln_map_add_raster_source_tiles(
          handle,
          view(sourceId),
          array(tiles, 2 * NativeMemory.addressSize, NativeMemory.addressSize) { at, item ->
            putView(at, item)
          },
          tiles.size.toLong(),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a raster source with a TileJSON URL.
   *
   * See `mln_map_add_raster_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addRasterSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_raster_source_url") {
      check(
        C.mln_map_add_raster_source_url(
          handle,
          view(sourceId),
          view(url),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds one style layer from a full style-spec layer JSON object.
   *
   * See `mln_map_add_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addStyleLayerJson(
    layerJson: ByteArray,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_style_layer_json") {
      check(
        C.mln_map_add_style_layer_json(
          handle,
          view(layerJson),
          view(beforeLayerId ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds one style source from a style-spec source JSON object.
   *
   * See `mln_map_add_style_source_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addStyleSourceJson(
    sourceId: String,
    sourceJson: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_style_source_json") {
      check(
        C.mln_map_add_style_source_json(
          handle,
          view(sourceId),
          view(sourceJson),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a vector source with inline tile URLs.
   *
   * See `mln_map_add_vector_source_tiles` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_vector_source_tiles") {
      check(
        C.mln_map_add_vector_source_tiles(
          handle,
          view(sourceId),
          array(tiles, 2 * NativeMemory.addressSize, NativeMemory.addressSize) { at, item ->
            putView(at, item)
          },
          tiles.size.toLong(),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Adds a vector source with a TileJSON URL.
   *
   * See `mln_map_add_vector_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun addVectorSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_vector_source_url") {
      check(
        C.mln_map_add_vector_source_url(
          handle,
          view(sourceId),
          view(url),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Submits one copied relative camera update.
   *
   * See `mln_map_apply_camera_delta` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun applyCameraDelta(delta: CameraDelta): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_apply_camera_delta") {
      check(C.mln_map_apply_camera_delta(handle, writeCameraDelta(delta), completion, diagnostic))
    }

  /**
   * Starts attachment of a caller-owned Metal texture target.
   *
   * See `mln_map_attach_metal_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_metal_borrowed_texture") {
      attach(
        { out, completion ->
          C.mln_map_attach_metal_borrowed_texture(
            handle,
            writeMetalBorrowedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Starts attachment of a session-owned Metal texture ring.
   *
   * See `mln_map_attach_metal_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_metal_owned_texture") {
      attach(
        { out, completion ->
          C.mln_map_attach_metal_owned_texture(
            handle,
            writeMetalOwnedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Starts attachment of a Metal surface target.
   *
   * See `mln_map_attach_metal_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun attachMetalSurface(
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_metal_surface") {
      attach(
        { out, completion ->
          C.mln_map_attach_metal_surface(
            handle,
            writeMetalSurfaceDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Starts attachment of a caller-owned OpenGL texture target.
   *
   * See `mln_map_attach_opengl_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun attachOpenglBorrowedTexture(
    descriptor: OpenglBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_opengl_borrowed_texture") {
      attach(
        { out, completion ->
          C.mln_map_attach_opengl_borrowed_texture(
            handle,
            writeOpenglBorrowedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Starts attachment of a session-owned OpenGL texture ring.
   *
   * See `mln_map_attach_opengl_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun attachOpenglOwnedTexture(
    descriptor: OpenglOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_opengl_owned_texture") {
      attach(
        { out, completion ->
          C.mln_map_attach_opengl_owned_texture(
            handle,
            writeOpenglOwnedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Starts attachment of an OpenGL surface target.
   *
   * See `mln_map_attach_opengl_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun attachOpenglSurface(
    descriptor: OpenglSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_opengl_surface") {
      attach(
        { out, completion ->
          C.mln_map_attach_opengl_surface(
            handle,
            writeOpenglSurfaceDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Starts attachment of a caller-owned Vulkan texture target.
   *
   * See `mln_map_attach_vulkan_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_vulkan_borrowed_texture") {
      attach(
        { out, completion ->
          C.mln_map_attach_vulkan_borrowed_texture(
            handle,
            writeVulkanBorrowedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Starts attachment of a session-owned Vulkan texture ring.
   *
   * See `mln_map_attach_vulkan_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_vulkan_owned_texture") {
      attach(
        { out, completion ->
          C.mln_map_attach_vulkan_owned_texture(
            handle,
            writeVulkanOwnedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Starts attachment of a Vulkan surface target.
   *
   * See `mln_map_attach_vulkan_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun attachVulkanSurface(
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_vulkan_surface") {
      attach(
        { out, completion ->
          C.mln_map_attach_vulkan_surface(
            handle,
            writeVulkanSurfaceDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Starts attachment of a caller-owned WebGPU texture target.
   *
   * See `mln_map_attach_webgpu_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun attachWebgpuBorrowedTexture(
    descriptor: WebgpuBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_webgpu_borrowed_texture") {
      attach(
        { out, completion ->
          C.mln_map_attach_webgpu_borrowed_texture(
            handle,
            writeWebgpuBorrowedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Starts attachment of a session-owned WebGPU texture ring.
   *
   * See `mln_map_attach_webgpu_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun attachWebgpuOwnedTexture(
    descriptor: WebgpuOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_webgpu_owned_texture") {
      attach(
        { out, completion ->
          C.mln_map_attach_webgpu_owned_texture(
            handle,
            writeWebgpuOwnedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Starts attachment of a WebGPU surface target.
   *
   * See `mln_map_attach_webgpu_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun attachWebgpuSurface(
    descriptor: WebgpuSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_map_attach_webgpu_surface") {
      attach(
        { out, completion ->
          C.mln_map_attach_webgpu_surface(
            handle,
            writeWebgpuSurfaceDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        },
        GeneratedOwnerDisposal::renderSession,
        { RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle) },
        { it.bindingCallbacks },
        { it.dispose() },
        ::RenderSessionAttachment,
      )
    }

  /**
   * Begins a command group, which holds this map's render updates until the group ends.
   *
   * See `mln_map_begin_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun beginCommandGroup(): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_begin_command_group") {
      check(C.mln_map_begin_command_group(handle, completion, diagnostic))
    }

  /**
   * Starts an ordered query for a camera that fits a GeoJSON geometry.
   *
   * See `mln_map_camera_for_geometry` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cameraForGeometry(
    geometry: ByteArray,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions> =
    nativeSubmit(
      this,
      binding,
      "mln_map_camera_for_geometry",
      { result -> readCameraOptions(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_camera_for_geometry(
          handle,
          view(geometry),
          fitOptions?.let { writeCameraFitOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered query for a camera that fits geographic bounds.
   *
   * See `mln_map_camera_for_lat_lng_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions> =
    nativeSubmit(
      this,
      binding,
      "mln_map_camera_for_lat_lng_bounds",
      { result -> readCameraOptions(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_camera_for_lat_lng_bounds(
          handle,
          writeLatLngBounds(bounds),
          fitOptions?.let { writeCameraFitOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered query for a camera that fits geographic coordinates.
   *
   * See `mln_map_camera_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions> =
    nativeSubmit(
      this,
      binding,
      "mln_map_camera_for_lat_lngs",
      { result -> readCameraOptions(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_camera_for_lat_lngs(
          handle,
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          fitOptions?.let { writeCameraFitOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Cancels the camera transitions running when this command commits.
   *
   * See `mln_map_cancel_transitions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cancelTransitions(): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_cancel_transitions") {
      check(C.mln_map_cancel_transitions(handle, completion, diagnostic))
    }

  /**
   * Starts creation of a standalone projection from the map's ordered transform state.
   *
   * See `mln_map_create_projection` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun createProjection(): Deferred<MapProjectionHandle> =
    nativeSubmitOwned(
      this,
      binding,
      "mln_map_create_projection",
      { MapProjectionHandle(it) },
      GeneratedOwnerDisposal::mapProjection,
      { it.close() },
    ) {
      check(C.mln_map_create_projection(handle, completion, diagnostic))
    }

  /**
   * Consumes a map handle without observing its asynchronous retirement.
   *
   * See `mln_map_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun dispose(): Unit =
    nativeClose(this, binding, "mln_map_dispose") { check(C.mln_map_dispose(handle, diagnostic)) }

  /**
   * Submits an ordered debug-log command.
   *
   * See `mln_map_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun dumpDebugLogs(): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_dump_debug_logs") {
      check(C.mln_map_dump_debug_logs(handle, completion, diagnostic))
    }

  /**
   * Ends the innermost command group that `mln_map_begin_command_group()` began.
   *
   * See `mln_map_end_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun endCommandGroup(): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_end_command_group") {
      check(C.mln_map_end_command_group(handle, completion, diagnostic))
    }

  /**
   * Starts an ordered camera read.
   *
   * See `mln_map_get_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun getCamera(): Deferred<CameraQueryResult> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_camera",
      { result -> readCameraQueryResult(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_get_camera(handle, completion, diagnostic))
    }

  /**
   * Copies the camera from the latest immutable map snapshot.
   *
   * See `mln_map_get_camera_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun getCameraSnapshot(): MapGetCameraSnapshotResult =
    nativeCall(this, binding, "mln_map_get_camera_snapshot") {
      val out0 = sized(120, 8)
      val out1 = allocate(8, 8)
      check(C.mln_map_get_camera_snapshot(handle, out0, out1, diagnostic))
      MapGetCameraSnapshotResult(camera = readCameraOptions(out0), generation = readU64(out1))
    }

  /**
   * Starts an ordered read of per-feature state from this map.
   *
   * See `mln_map_get_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getFeatureState(selector: FeatureStateSelector): Deferred<ByteArray> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_feature_state",
      { result -> readView(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_get_feature_state(
          handle,
          writeFeatureStateSelector(selector),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Queries the global-state JSON object, including style defaults. Completion borrows one
   * `mln_buffer_view` for the duration of the callback.
   *
   * See `mln_map_get_global_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getGlobalState(): Deferred<ByteArray> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_global_state",
      { result -> readView(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_get_global_state(handle, completion, diagnostic))
    }

  /**
   * Copies image source coordinates.
   *
   * See `mln_map_get_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getImageSourceCoordinates(sourceId: String): Deferred<List<LatLng>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_image_source_coordinates",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          16.toLong(),
        ) {
          readLatLng(it)
        }
      },
    ) {
      check(C.mln_map_get_image_source_coordinates(handle, view(sourceId), completion, diagnostic))
    }

  /**
   * Starts an ordered copy of the last successfully parsed style document.
   *
   * See `mln_map_get_loaded_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getLoadedStyleJson(): Deferred<ByteArray> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_loaded_style_json",
      { result -> readView(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_get_loaded_style_json(handle, completion, diagnostic))
    }

  /**
   * Copies the latest immutable state published by the map worker.
   *
   * See `mln_map_get_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getSnapshot(): MapSnapshot =
    nativeCall(this, binding, "mln_map_get_snapshot") {
      val out = sized(456, 8)
      check(C.mln_map_get_snapshot(handle, out, diagnostic))
      readMapSnapshot(out)
    }

  /**
   * Copies one complete runtime style image.
   *
   * See `mln_map_get_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleImage(imageId: String): Deferred<StyleImageInfo?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_image",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readStyleImageInfo(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_image(handle, view(imageId), completion, diagnostic))
    }

  /**
   * Copies the complete metadata of one style layer.
   *
   * See `mln_map_get_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleLayer(layerId: String): Deferred<StyleLayerInfo?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_layer",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readStyleLayerInfo(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_layer(handle, view(layerId), completion, diagnostic))
    }

  /**
   * Serializes one layer filter as a style-spec JSON value.
   *
   * See `mln_map_get_style_layer_filter` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleLayerFilter(layerId: String): Deferred<ByteArray?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_layer_filter",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readView(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_layer_filter(handle, view(layerId), completion, diagnostic))
    }

  /**
   * Serializes one style layer as a full style-spec layer JSON object.
   *
   * See `mln_map_get_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleLayerJson(layerId: String): Deferred<ByteArray?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_layer_json",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readView(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_layer_json(handle, view(layerId), completion, diagnostic))
    }

  /**
   * Serializes one layer property as a style-spec JSON value.
   *
   * See `mln_map_get_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleLayerProperty(layerId: String, propertyName: String): Deferred<ByteArray?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_layer_property",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readView(CompletionBridge.value(result))
      },
    ) {
      check(
        C.mln_map_get_style_layer_property(
          handle,
          view(layerId),
          view(propertyName),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Serializes one style light property as a style-spec JSON value.
   *
   * See `mln_map_get_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleLightProperty(propertyName: String): Deferred<ByteArray?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_light_property",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readView(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_light_property(handle, view(propertyName), completion, diagnostic))
    }

  /**
   * Copies the complete metadata of one style source.
   *
   * See `mln_map_get_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleSource(sourceId: String): Deferred<StyleSourceInfo?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_source",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readStyleSourceInfo(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_source(handle, view(sourceId), completion, diagnostic))
    }

  /**
   * Reads the style's global transition options.
   *
   * See `mln_map_get_style_transition_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun getStyleTransitionOptions(): Deferred<StyleTransitionOptions> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_transition_options",
      { result -> readStyleTransitionOptions(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_get_style_transition_options(handle, completion, diagnostic))
    }

  /**
   * Starts an ordered copy of the last requested style URL.
   *
   * See `mln_map_get_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun getStyleUrl(): Deferred<String> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_url",
      { result -> readViewString(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_get_style_url(handle, completion, diagnostic))
    }

  /**
   * Invalidates custom geometry source data inside one geographic region.
   *
   * See `mln_map_invalidate_custom_geometry_source_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_invalidate_custom_geometry_source_region") {
      check(
        C.mln_map_invalidate_custom_geometry_source_region(
          handle,
          view(sourceId),
          writeLatLngBounds(bounds),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Invalidates custom geometry source data for one canonical tile.
   *
   * See `mln_map_invalidate_custom_geometry_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_invalidate_custom_geometry_source_tile") {
      check(
        C.mln_map_invalidate_custom_geometry_source_tile(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Invalidates custom MVT vector source data for one canonical tile.
   *
   * See `mln_map_invalidate_custom_mvt_vector_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun invalidateCustomMvtVectorSourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_invalidate_custom_mvt_vector_source_tile") {
      check(
        C.mln_map_invalidate_custom_mvt_vector_source_tile(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered wrapped-bounds query for a copied camera.
   *
   * See `mln_map_lat_lng_bounds_for_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun latLngBoundsForCamera(camera: CameraOptions): Deferred<LatLngBounds> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lng_bounds_for_camera",
      { result -> readLatLngBounds(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_lat_lng_bounds_for_camera(
          handle,
          writeCameraOptions(camera),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered unwrapped-bounds query for a copied camera.
   *
   * See `mln_map_lat_lng_bounds_for_camera_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): Deferred<LatLngBounds> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lng_bounds_for_camera_unwrapped",
      { result -> readLatLngBounds(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_lat_lng_bounds_for_camera_unwrapped(
          handle,
          writeCameraOptions(camera),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered conversion from a screen point to a geographic coordinate.
   *
   * See `mln_map_lat_lng_for_pixel` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun latLngForPixel(point: ScreenPoint): Deferred<LatLng> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lng_for_pixel",
      { result -> readLatLng(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_lat_lng_for_pixel(handle, writeScreenPoint(point), completion, diagnostic))
    }

  /**
   * Starts an ordered conversion from a screen point to an unwrapped geographic coordinate.
   *
   * See `mln_map_lat_lng_for_pixel_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun latLngForPixelUnwrapped(point: ScreenPoint): Deferred<LatLng> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lng_for_pixel_unwrapped",
      { result -> readLatLng(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_lat_lng_for_pixel_unwrapped(
          handle,
          writeScreenPoint(point),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered conversion of copied screen points to coordinates.
   *
   * See `mln_map_lat_lngs_for_pixels` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun latLngsForPixels(points: List<ScreenPoint>): Deferred<List<LatLng>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lngs_for_pixels",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          16.toLong(),
        ) {
          readLatLng(it)
        }
      },
    ) {
      check(
        C.mln_map_lat_lngs_for_pixels(
          handle,
          array(points, 16, 8) { at, item -> putScreenPoint(at, item) },
          points.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered conversion of copied screen points to unwrapped coordinates.
   *
   * See `mln_map_lat_lngs_for_pixels_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun latLngsForPixelsUnwrapped(points: List<ScreenPoint>): Deferred<List<LatLng>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lngs_for_pixels_unwrapped",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          16.toLong(),
        ) {
          readLatLng(it)
        }
      },
    ) {
      check(
        C.mln_map_lat_lngs_for_pixels_unwrapped(
          handle,
          array(points, 16, 8) { at, item -> putScreenPoint(at, item) },
          points.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered query of every style layer in style order.
   *
   * See `mln_map_list_style_layers` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun listStyleLayers(): Deferred<List<StyleLayerInfo>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_list_style_layers",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          w(64, 96).toLong(),
        ) {
          readStyleLayerInfo(it)
        }
      },
    ) {
      check(C.mln_map_list_style_layers(handle, completion, diagnostic))
    }

  /**
   * Lists every style source in style order.
   *
   * See `mln_map_list_style_sources` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun listStyleSources(): Deferred<List<StyleSourceInfo>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_list_style_sources",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          w(120, 160).toLong(),
        ) {
          readStyleSourceInfo(it)
        }
      },
    ) {
      check(C.mln_map_list_style_sources(handle, completion, diagnostic))
    }

  /**
   * Starts an ordered query of meters per logical pixel at a latitude and the current map zoom. The
   * completion borrows one double.
   *
   * See `mln_map_meters_per_pixel_at_latitude` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun metersPerPixelAtLatitude(latitude: Double): Deferred<Double> =
    nativeSubmit(
      this,
      binding,
      "mln_map_meters_per_pixel_at_latitude",
      { result -> readF64(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_meters_per_pixel_at_latitude(handle, latitude, completion, diagnostic))
    }

  /**
   * Moves one style layer before another layer or to the top.
   *
   * See `mln_map_move_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun moveStyleLayer(
    layerId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_move_style_layer") {
      check(
        C.mln_map_move_style_layer(
          handle,
          view(layerId),
          view(beforeLayerId ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Starts an ordered conversion from a geographic coordinate to a screen point.
   *
   * See `mln_map_pixel_for_lat_lng` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun pixelForLatLng(coordinate: LatLng): Deferred<ScreenPoint> =
    nativeSubmit(
      this,
      binding,
      "mln_map_pixel_for_lat_lng",
      { result -> readScreenPoint(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_pixel_for_lat_lng(handle, writeLatLng(coordinate), completion, diagnostic))
    }

  /**
   * Starts an ordered conversion of copied coordinates to screen points.
   *
   * See `mln_map_pixels_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun pixelsForLatLngs(coordinates: List<LatLng>): Deferred<List<ScreenPoint>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_pixels_for_lat_lngs",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          16.toLong(),
        ) {
          readScreenPoint(it)
        }
      },
    ) {
      check(
        C.mln_map_pixels_for_lat_lngs(
          handle,
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Releases a map after synchronous state preflight.
   *
   * See `mln_map_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun release(): Deferred<Unit> =
    nativeRetire(this, binding, "mln_map_release") {
      check(C.mln_map_release(handle, completion, diagnostic))
    }

  /**
   * Removes per-feature state from this map.
   *
   * See `mln_map_remove_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun removeFeatureState(selector: FeatureStateSelector): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_remove_feature_state") {
      check(
        C.mln_map_remove_feature_state(
          handle,
          writeFeatureStateSelector(selector),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Removes one runtime style image by ID.
   *
   * See `mln_map_remove_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun removeStyleImage(imageId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_remove_style_image") {
      check(C.mln_map_remove_style_image(handle, view(imageId), completion, diagnostic))
    }

  /**
   * Removes one style layer by ID.
   *
   * See `mln_map_remove_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun removeStyleLayer(layerId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_remove_style_layer") {
      check(C.mln_map_remove_style_layer(handle, view(layerId), completion, diagnostic))
    }

  /**
   * Removes one style source by ID.
   *
   * See `mln_map_remove_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun removeStyleSource(sourceId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_remove_style_source") {
      check(C.mln_map_remove_style_source(handle, view(sourceId), completion, diagnostic))
    }

  /**
   * Requests a repaint for a continuous map.
   *
   * See `mln_map_request_repaint` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun requestRepaint(): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_request_repaint") {
      check(C.mln_map_request_repaint(handle, completion, diagnostic))
    }

  /**
   * Requests one still image for a static or tile map.
   *
   * See `mln_map_request_still_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun requestStillImage(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_map_request_still_image") {
      check(C.mln_map_request_still_image(handle, completion, diagnostic))
    }

  /**
   * Submits the sole post-creation logical extent update.
   *
   * See `mln_map_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun resize(extent: LogicalExtent): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_resize") {
      check(C.mln_map_resize(handle, writeLogicalExtent(extent), completion, diagnostic))
    }

  /**
   * Submits a copied camera-constraint command.
   *
   * See `mln_map_set_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setBounds(options: BoundOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_bounds") {
      check(C.mln_map_set_bounds(handle, writeBoundOptions(options), completion, diagnostic))
    }

  /**
   * Sets custom geometry source data for one canonical tile.
   *
   * See `mln_map_set_custom_geometry_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_custom_geometry_source_tile_data") {
      check(
        C.mln_map_set_custom_geometry_source_tile_data(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(data),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Sets custom MVT vector source data for one canonical tile.
   *
   * See `mln_map_set_custom_mvt_vector_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setCustomMvtVectorSourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_custom_mvt_vector_source_tile_data") {
      check(
        C.mln_map_set_custom_mvt_vector_source_tile_data(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(data),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Reports a custom MVT vector source error for one canonical tile.
   *
   * See `mln_map_set_custom_mvt_vector_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setCustomMvtVectorSourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_custom_mvt_vector_source_tile_error") {
      check(
        C.mln_map_set_custom_mvt_vector_source_tile_error(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(message),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Submits a debug-overlay command.
   *
   * See `mln_map_set_debug_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setDebugOptions(options: MapDebugOption): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_debug_options") {
      check(C.mln_map_set_debug_options(handle, options.rawValue.toInt(), completion, diagnostic))
    }

  /**
   * Selects which map-originated event types this map queues.
   *
   * See `mln_map_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setEventMask(mask: RuntimeEventMask): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_event_mask") {
      check(C.mln_map_set_event_mask(handle, mask.rawValue.toLong(), completion, diagnostic))
    }

  /**
   * Submits a copied per-feature-state command.
   *
   * See `mln_map_set_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setFeatureState(
    selector: FeatureStateSelector,
    state: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_feature_state") {
      check(
        C.mln_map_set_feature_state(
          handle,
          writeFeatureStateSelector(selector),
          view(state),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Submits a copied free-camera command.
   *
   * See `mln_map_set_free_camera_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setFreeCameraOptions(options: FreeCameraOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_free_camera_options") {
      check(
        C.mln_map_set_free_camera_options(
          handle,
          writeFreeCameraOptions(options),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Updates one GeoJSON source with prepared inline data.
   *
   * See `mln_map_set_geojson_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setGeojsonSourceData(
    sourceId: String,
    data: GeojsonSourceDataHandle,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_geojson_source_data") {
      check(
        C.mln_map_set_geojson_source_data(
          handle,
          view(sourceId),
          data.binding.handle(),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Overrides one GeoJSON source's synchronous tiling at runtime.
   *
   * See `mln_map_set_geojson_source_synchronous_tiling` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setGeojsonSourceSynchronousTiling(
    sourceId: String,
    enabled: Boolean,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_geojson_source_synchronous_tiling") {
      check(
        C.mln_map_set_geojson_source_synchronous_tiling(
          handle,
          view(sourceId),
          enabled,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Updates one GeoJSON source to load data from a URL.
   *
   * See `mln_map_set_geojson_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setGeojsonSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_geojson_source_url") {
      check(
        C.mln_map_set_geojson_source_url(handle, view(sourceId), view(url), completion, diagnostic)
      )
    }

  /**
   * Submits a global-state JSON value. JSON null restores the style default. Input is copied before
   * return.
   *
   * See `mln_map_set_global_state_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setGlobalStateProperty(
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_global_state_property") {
      check(
        C.mln_map_set_global_state_property(
          handle,
          view(propertyName),
          view(valueValue),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Updates image source coordinates.
   *
   * See `mln_map_set_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setImageSourceCoordinates(
    sourceId: String,
    coordinates: List<LatLng>,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_image_source_coordinates") {
      check(
        C.mln_map_set_image_source_coordinates(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Updates an image source with inline image pixels.
   *
   * See `mln_map_set_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setImageSourceImage(
    sourceId: String,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_image_source_image") {
      check(
        C.mln_map_set_image_source_image(
          handle,
          view(sourceId),
          writePremultipliedRgba8Image(image),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Updates an image source to load its image from a URL.
   *
   * See `mln_map_set_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setImageSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_image_source_url") {
      check(
        C.mln_map_set_image_source_url(handle, view(sourceId), view(url), completion, diagnostic)
      )
    }

  /**
   * Sets a location indicator layer accuracy radius in meters.
   *
   * See `mln_map_set_location_indicator_accuracy_radius` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_location_indicator_accuracy_radius") {
      check(
        C.mln_map_set_location_indicator_accuracy_radius(
          handle,
          view(layerId),
          radius,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Sets a location indicator layer bearing in degrees.
   *
   * See `mln_map_set_location_indicator_bearing` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setLocationIndicatorBearing(
    layerId: String,
    bearing: Double,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_location_indicator_bearing") {
      check(
        C.mln_map_set_location_indicator_bearing(
          handle,
          view(layerId),
          bearing,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Sets one location indicator image-name property.
   *
   * See `mln_map_set_location_indicator_image_name` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_location_indicator_image_name") {
      check(
        C.mln_map_set_location_indicator_image_name(
          handle,
          view(layerId),
          imageKind.rawValue.toInt(),
          view(imageId),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Sets a location indicator layer location.
   *
   * See `mln_map_set_location_indicator_location` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_location_indicator_location") {
      check(
        C.mln_map_set_location_indicator_location(
          handle,
          view(layerId),
          writeLatLng(coordinate),
          altitude,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Submits copied axonometric rendering option fields.
   *
   * See `mln_map_set_projection_mode` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setProjectionMode(mode: ProjectionMode): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_projection_mode") {
      check(
        C.mln_map_set_projection_mode(handle, writeProjectionMode(mode), completion, diagnostic)
      )
    }

  /**
   * Submits a rendering-stats visibility command.
   *
   * See `mln_map_set_rendering_stats_view_enabled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setRenderingStatsViewEnabled(enabled: Boolean): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_rendering_stats_view_enabled") {
      check(C.mln_map_set_rendering_stats_view_enabled(handle, enabled, completion, diagnostic))
    }

  /**
   * Sets one runtime style image.
   *
   * See `mln_map_set_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_image") {
      check(
        C.mln_map_set_style_image(
          handle,
          view(imageId),
          writePremultipliedRgba8Image(image),
          options?.let { writeStyleImageOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Queues an inline style JSON command.
   *
   * See `mln_map_set_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setStyleJson(json: ByteArray): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_json") {
      check(C.mln_map_set_style_json(handle, view(json), completion, diagnostic))
    }

  /**
   * Sets or clears one layer filter.
   *
   * See `mln_map_set_style_layer_filter` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLayerFilter(
    layerId: String,
    filter: ByteArray? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_layer_filter") {
      check(
        C.mln_map_set_style_layer_filter(
          handle,
          view(layerId),
          filter?.let { view(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Sets the highest zoom at which one layer draws.
   *
   * See `mln_map_set_style_layer_max_zoom` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLayerMaxZoom(layerId: String, maxZoom: Double): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_layer_max_zoom") {
      check(
        C.mln_map_set_style_layer_max_zoom(handle, view(layerId), maxZoom, completion, diagnostic)
      )
    }

  /**
   * Sets the lowest zoom at which one layer draws.
   *
   * See `mln_map_set_style_layer_min_zoom` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLayerMinZoom(layerId: String, minZoom: Double): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_layer_min_zoom") {
      check(
        C.mln_map_set_style_layer_min_zoom(handle, view(layerId), minZoom, completion, diagnostic)
      )
    }

  /**
   * Sets one layer property using its MapLibre style-spec property name.
   *
   * See `mln_map_set_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLayerProperty(
    layerId: String,
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_layer_property") {
      check(
        C.mln_map_set_style_layer_property(
          handle,
          view(layerId),
          view(propertyName),
          view(valueValue),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Sets one layer's source ID.
   *
   * See `mln_map_set_style_layer_source_id` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLayerSourceId(layerId: String, sourceId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_layer_source_id") {
      check(
        C.mln_map_set_style_layer_source_id(
          handle,
          view(layerId),
          view(sourceId),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Sets one layer's source-layer ID.
   *
   * See `mln_map_set_style_layer_source_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLayerSourceLayer(
    layerId: String,
    sourceLayer: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_layer_source_layer") {
      check(
        C.mln_map_set_style_layer_source_layer(
          handle,
          view(layerId),
          view(sourceLayer ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Sets whether one layer draws.
   *
   * See `mln_map_set_style_layer_visibility` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_layer_visibility") {
      check(
        C.mln_map_set_style_layer_visibility(
          handle,
          view(layerId),
          visibility.rawValue.toInt(),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Sets the style light from a style-spec light JSON object.
   *
   * See `mln_map_set_style_light_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLightJson(lightJson: ByteArray): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_light_json") {
      check(C.mln_map_set_style_light_json(handle, view(lightJson), completion, diagnostic))
    }

  /**
   * Sets one style light property using its MapLibre style-spec property name.
   *
   * See `mln_map_set_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleLightProperty(
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_light_property") {
      check(
        C.mln_map_set_style_light_property(
          handle,
          view(propertyName),
          view(valueValue),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Sets whether one style source stores fetched tiles in the persistent cache.
   *
   * See `mln_map_set_style_source_volatile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleSourceVolatile(
    sourceId: String,
    isVolatile: Boolean,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_source_volatile") {
      check(
        C.mln_map_set_style_source_volatile(
          handle,
          view(sourceId),
          isVolatile,
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Sets the style's global transition options.
   *
   * See `mln_map_set_style_transition_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun setStyleTransitionOptions(
    options: StyleTransitionOptions
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_transition_options") {
      check(
        C.mln_map_set_style_transition_options(
          handle,
          writeStyleTransitionOptions(options),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Queues a style URL command.
   *
   * See `mln_map_set_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun setStyleUrl(url: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_url") {
      check(C.mln_map_set_style_url(handle, cString(url), completion, diagnostic))
    }

  /**
   * Submits a copied tile-options command.
   *
   * See `mln_map_set_tile_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setTileOptions(options: MapTileOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_tile_options") {
      check(
        C.mln_map_set_tile_options(handle, writeMapTileOptions(options), completion, diagnostic)
      )
    }

  /**
   * Submits a copied viewport-options command.
   *
   * See `mln_map_set_viewport_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun setViewportOptions(options: MapViewportOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_viewport_options") {
      check(
        C.mln_map_set_viewport_options(
          handle,
          writeMapViewportOptions(options),
          completion,
          diagnostic,
        )
      )
    }

  /**
   * Submits one atomic camera update.
   *
   * See `mln_map_update_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun updateCamera(update: CameraUpdate): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_update_camera") {
      check(C.mln_map_update_camera(handle, writeCameraUpdate(update), completion, diagnostic))
    }
}
