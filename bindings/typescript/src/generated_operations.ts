// Generated from the C headers by tools/bindgen. Do not edit.

import type {
  AmbientCacheOperation,
  BoundOptions,
  CameraDelta,
  CameraFitOptions,
  CameraOptions,
  CameraQueryResult,
  CameraTransitionHandler,
  CameraUpdate,
  CanonicalTileId,
  CustomGeometrySourceOptions,
  CustomVectorSourceOptions,
  FeatureStateSelector,
  FrameDemand,
  FreeCameraOptions,
  GeoJsonSourceOptions,
  GpuSync,
  GraphicsDeviceOptions,
  GraphicsDeviceTextureDescriptor,
  HttpHeaderTransform,
  LatLng,
  LatLngBounds,
  LogHandler,
  LogSeverityFlag,
  LogicalExtent,
  MapDebugFlag,
  MapOptions,
  MapSnapshot,
  MapTileOptions,
  MapView,
  MapViewportOptions,
  MetalBorrowedTextureDescriptor,
  MetalOwnedTextureDescriptor,
  MetalSurfaceDescriptor,
  MetalTextureFrame,
  NetworkStatus,
  OfflineRegionDefinition,
  OfflineRegionDownloadState,
  OfflineRegionInfo,
  OfflineRegionStatus,
  OpenGlBorrowedTextureDescriptor,
  OpenGlContextProviderFlag,
  OpenGlOwnedTextureDescriptor,
  OpenGlSurfaceDescriptor,
  OpenGlTextureFrame,
  PremultipliedRgba8Image,
  ProjectedMeters,
  ProjectionMode,
  RenderAbandonResult,
  RenderBackendFlag,
  RenderFrameResult,
  RenderSessionAttachOptions,
  RenderSessionCapabilities,
  RenderSessionSnapshot,
  RenderedFeature,
  RenderedFeatureQueryOptions,
  RenderedQueryGeometry,
  ResourceProvider,
  ResourceRequestCancelHandler,
  ResourceResponse,
  ResourceTransform,
  RuntimeEventFlag,
  RuntimeEventHandler,
  RuntimeOptions,
  ScreenPoint,
  SourceFeature,
  SourceFeatureQueryOptions,
  StyleExpressionInfo,
  StyleExpressionInputs,
  StyleExpressionOptions,
  StyleImageInfo,
  StyleImageOptions,
  StyleLayerInfo,
  StyleSourceInfo,
  StyleTransitionOptions,
  StyleVectorTileEncoding,
  TextureReadbackResult,
  VulkanBorrowedTextureDescriptor,
  VulkanOwnedTextureDescriptor,
  VulkanSurfaceDescriptor,
  VulkanTextureFrame,
  WebGpuBorrowedTextureDescriptor,
  WebGpuOwnedTextureDescriptor,
  WebGpuSurfaceDescriptor,
  WebGpuTextureFrame,
} from "./generated_values.ts";
import { native } from "./internal/addon.ts";
import {
  type CommandCompletion,
  attach,
  command,
  query,
  run,
  submitRun,
  threadStart,
} from "./internal/completions.ts";
import {
  ADOPT,
  type AdoptKey,
  Owner,
  ViewScope,
  adopt,
  answerSync,
  checkKey,
  closeAsync,
  closeSync,
  defineDisposers,
  rootsOf,
} from "./internal/owners.ts";
import {
  type Registrations,
  defineCallbacks,
  withRegistrations,
} from "./internal/realm.ts";

/**
 * Returns the API epoch of the library.
 *
 * Headers and a library with different epochs aren't compatible.
 *
 * See `mln_c_version` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export function cVersion(): number {
  return native.mln_c_version();
}

/**
 * Sends log messages to the platform's log again, instead of a callback.
 *
 * See `mln_log_clear_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export function clearLogCallback(): void {
  native.mln_log_clear_callback();
}

/**
 * Reads whether the library may use the network.
 *
 * See `mln_network_get_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export function getNetworkStatus(): NetworkStatus {
  return native.mln_network_get_status();
}

/**
 * Returns the function that registers a plugin's style layer types.
 *
 * Pass the function to the plugin's registration entry point before loading a
 * style that uses the plugin's layer types. Plugin callbacks run on library
 * threads and can keep running while the process exits, so they must not use
 * state that the process destroys at exit.
 *
 * See `mln_plugin_get_register_function_v1` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
 */
export function getPluginRegisterFunctionV1(): bigint {
  return native.mln_plugin_get_register_function_v1();
}

/**
 * Converts meters in the Web Mercator projection to a coordinate.
 *
 * See `mln_lat_lng_for_projected_meters` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
export function latLngForProjectedMeters(meters: ProjectedMeters = {}): LatLng {
  return native.mln_lat_lng_for_projected_meters(meters);
}

/**
 * Computes the size in pixels of a target with a logical size and scale factor.
 *
 * See `mln_logical_extent_physical_size` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export function logicalExtentPhysicalSize(extent: LogicalExtent = {}): {
  readonly width: number;
  readonly height: number;
} {
  return native.mln_logical_extent_physical_size(extent);
}

/**
 * Reports the OpenGL context platforms that this build of the library has.
 *
 * See `mln_open_gl_supported_context_provider_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export function openGlSupportedContextProviderMask(): OpenGlContextProviderFlag {
  return native.mln_open_gl_supported_context_provider_mask();
}

/**
 * Converts a coordinate to meters in the Web Mercator projection.
 *
 * See `mln_projected_meters_for_lat_lng` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
export function projectedMetersForLatLng(
  coordinate: LatLng = {},
): ProjectedMeters {
  return native.mln_projected_meters_for_lat_lng(coordinate);
}

/**
 * Selects the severities that the library logs on a background thread.
 *
 * The library logs other severities on the thread that logged them. `mask`
 * applies to every runtime in the process.
 *
 * See `mln_log_set_async_severity_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export function setLogAsyncSeverityMask(mask: LogSeverityFlag): void {
  native.mln_log_set_async_severity_mask(mask);
}

/**
 * Sends the log messages of every runtime in the process to a callback.
 *
 * The callback replaces the one set before.
 *
 * The callback runs asynchronously on the JavaScript thread that registered it,
 * after native queues the call.
 *
 * See `mln_log_set_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export function setLogCallback(handler: LogHandler): void {
  return withRegistrations((registrations) => {
    native.mln_log_set_callback(handler);
    registrations.commit();
  });
}

/**
 * Tells the library whether it may use the network.
 *
 * While the status is `NetworkStatus.offline`, the library starts no network
 * requests. The requests that wait start when the status returns to
 * `NetworkStatus.online`. The status starts as online and applies to every
 * runtime in the process.
 *
 * See `mln_network_set_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export function setNetworkStatus(status: NetworkStatus): void {
  native.mln_network_set_status(status);
}

/**
 * Returns the render backends that this build of the library draws with.
 *
 * See `mln_supported_render_backend_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export function supportedRenderBackendMask(): RenderBackendFlag {
  return native.mln_supported_render_backend_mask();
}

/**
 * A rendered frame that the caller holds to read or show.
 *
 * See `mln_acquired_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class AcquiredFrameHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(
    key: AdoptKey,
    handle: bigint,
    parent: RenderSessionHandle,
    registrations?: Registrations,
  ) {
    super(key, handle, "AcquiredFrameHandle", { parent, registrations });
  }

  /**
   * Gives up an acquired frame without saying when your GPU stops reading it.
   *
   * Because your GPU may still read the frame's texture, the session never
   * renders into that texture again.
   *
   * See `mln_acquired_frame_discard` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  discard(): void {
    closeSync(this, (handle, token) =>
      native.mln_acquired_frame_discard(handle, token),
    );
  }

  /**
   * Reads the frame result of an acquired frame.
   *
   * See `mln_acquired_frame_get_result` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session lost its target.
   */
  getResult(): RenderFrameResult {
    return native.mln_acquired_frame_get_result(this.id);
  }

  /**
   * Gives an acquired frame back to the session once you're done reading it.
   *
   * `consumerCompletion` says when your GPU work that reads the texture
   * finishes. Null means that the work has already finished. The session waits
   * for that point before it renders into the texture again.
   *
   * See `mln_acquired_frame_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `RenderSessionCapabilities.consumerSyncKinds`
   *   lacks the kind of `consumerCompletion`.
   */
  release(consumerCompletion?: GpuSync): void {
    closeSync(this, (handle, token) =>
      native.mln_acquired_frame_release(handle, token, consumerCompletion),
    );
  }

  /**
   * Gets the Metal texture of an acquired frame.
   *
   * See `mln_acquired_frame_get_metal_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session lost its target.
   */
  withMetalTexture<T>(use: (view: ScopedMetalTextureFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedMetalTextureFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_metal_texture(this.id),
        ),
      ),
    );
  }

  /**
   * Gets the OpenGL texture of an acquired frame.
   *
   * See `mln_acquired_frame_get_open_gl_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session lost its target.
   */
  withOpenGlTexture<T>(use: (view: ScopedOpenGlTextureFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedOpenGlTextureFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_open_gl_texture(this.id),
        ),
      ),
    );
  }

  /**
   * Reads what to wait for before you read an acquired frame's texture.
   *
   * See `mln_acquired_frame_get_producer_sync` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session lost its target.
   */
  withProducerSync<T>(use: (view: ScopedGpuSync) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedGpuSync(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_producer_sync(this.id),
        ),
      ),
    );
  }

  /**
   * Gets the Vulkan image of an acquired frame.
   *
   * See `mln_acquired_frame_get_vulkan_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session lost its target.
   */
  withVulkanTexture<T>(use: (view: ScopedVulkanTextureFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedVulkanTextureFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_vulkan_texture(this.id),
        ),
      ),
    );
  }

  /**
   * Gets the WebGPU texture of an acquired frame.
   *
   * See `mln_acquired_frame_get_web_gpu_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session lost its target.
   */
  withWebGpuTexture<T>(use: (view: ScopedWebGpuTextureFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedWebGpuTextureFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_web_gpu_texture(this.id),
        ),
      ),
    );
  }
}
defineDisposers(AcquiredFrameHandle, undefined, undefined);

/**
 * GeoJSON data, parsed and ready to add to a GeoJSON source.
 *
 * See `mln_geo_json_source_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class GeoJsonSourceDataHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "GeoJsonSourceDataHandle", { registrations });
  }
  /**
   * Calls `close()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `close()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Releases prepared GeoJSON data.
   *
   * Sources that use the data keep it.
   *
   * See `mln_geo_json_source_data_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  close(): void {
    closeSync(this, (handle, token) =>
      native.mln_geo_json_source_data_release(handle, token),
    );
  }

  /**
   * Prepares GeoJSON data for GeoJSON sources.
   *
   * `data` is a GeoJSON feature collection, feature, or geometry. Preparing it
   * can be slow, but adding the result to a source is quick, and several
   * sources can share it.
   *
   * See `mln_geo_json_source_data_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when `data` isn't valid GeoJSON, or when
   *   `options` turns on clustering and `data` isn't a feature collection of
   *   points.
   */
  static create(
    data: unknown,
    options?: GeoJsonSourceOptions,
  ): GeoJsonSourceDataHandle {
    const handle = native.mln_geo_json_source_data_create(data, options);
    return adopt(
      handle,
      () => new GeoJsonSourceDataHandle(ADOPT, handle),
      (disposed) =>
        native.mln_geo_json_source_data_release(disposed, undefined),
    );
  }
}
defineDisposers(GeoJsonSourceDataHandle, undefined, "close");

/**
 * A GPU device that the library creates to render maps into textures.
 *
 * See `mln_graphics_device` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class GraphicsDeviceHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "GraphicsDeviceHandle", { registrations });
  }
  /**
   * Calls `close()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `close()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Releases a graphics device.
   *
   * Render sessions on the device keep working until they detach.
   *
   * See `mln_graphics_device_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
   */
  close(): void {
    closeSync(this, (handle, token) =>
      native.mln_graphics_device_release(handle, token),
    );
  }

  /**
   * Creates a GPU device for rendering maps into textures.
   *
   * Render sessions on several maps can share one device.
   *
   * See `mln_graphics_device_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when this build's render backend can't create a
   *   device.
   * - `Status.nativeError` when `adapter_index` names no GPU, or the platform
   *   can't create the device.
   */
  static create(options?: GraphicsDeviceOptions): GraphicsDeviceHandle {
    const handle = native.mln_graphics_device_create(options);
    return adopt(
      handle,
      () => new GraphicsDeviceHandle(ADOPT, handle),
      (disposed) => native.mln_graphics_device_release(disposed, undefined),
    );
  }
}
defineDisposers(GraphicsDeviceHandle, undefined, "close");

/**
 * A map, with its style and camera.
 *
 * See `mln_map` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class MapHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(
    key: AdoptKey,
    handle: bigint,
    parent: RuntimeHandle,
    registrations?: Registrations,
  ) {
    super(key, handle, "MapHandle", { parent, registrations });
  }

  /**
   * The native handle value, which runtime events report as their
   * source. It stays readable after close, so events drained later
   * still match this handle.
   */
  declare readonly id: bigint;

  /** The commands of this owner that discard their outcomes. */
  get discarding(): MapHandleDiscarding {
    return new MapHandleDiscarding(ADOPT, this);
  }
  /**
   * Calls `close()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `dispose()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Adds a source whose GeoJSON tiles come from callbacks.
   *
   * See `mln_map_add_custom_geometry_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the style already has a source named
   *   `sourceId`.
   */
  addCustomGeometrySource(
    sourceId: string,
    options: CustomGeometrySourceOptions,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_add_custom_geometry_source(
          this.id,
          sourceId,
          options,
          completion,
        );
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Adds a source whose vector tiles come from callbacks.
   *
   * See `mln_map_add_custom_vector_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the style already has a source named
   *   `sourceId`.
   */
  addCustomVectorSource(
    sourceId: string,
    options: CustomVectorSourceOptions,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_add_custom_vector_source(
          this.id,
          sourceId,
          options,
          completion,
        );
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Adds a GeoJSON source with prepared data.
   *
   * The source uses the options that `data` was prepared with. `data` can be
   * released once this function returns.
   *
   * See `mln_map_add_geo_json_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the style already has a source named
   *   `sourceId`.
   */
  addGeoJsonSourceData(
    sourceId: string,
    data: GeoJsonSourceDataHandle,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_geo_json_source_data(
        this.id,
        sourceId,
        data.id,
        completion,
      ),
    );
  }

  /**
   * Adds a GeoJSON source that loads its data from `url`.
   *
   * See `mln_map_add_geo_json_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the style already has a source named
   *   `sourceId`.
   */
  addGeoJsonSourceUrl(
    sourceId: string,
    url: string,
    options?: GeoJsonSourceOptions,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_geo_json_source_url(
        this.id,
        sourceId,
        url,
        options,
        completion,
      ),
    );
  }

  /**
   * Adds an image source that shows `image`.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_add_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the style already has a source named
   *   `sourceId`.
   */
  addImageSourceImage(
    sourceId: string,
    coordinates: readonly LatLng[],
    image: PremultipliedRgba8Image,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_image_source_image(
        this.id,
        sourceId,
        coordinates,
        image,
        completion,
      ),
    );
  }

  /**
   * Adds an image source that loads its image from `url`.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_add_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the style already has a source named
   *   `sourceId`.
   */
  addImageSourceUrl(
    sourceId: string,
    coordinates: readonly LatLng[],
    url: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_image_source_url(
        this.id,
        sourceId,
        coordinates,
        url,
        completion,
      ),
    );
  }

  /**
   * Adds a layer to the map's style.
   *
   * `layerJson` is a
   * &#91;layer&#93;(https://maplibre.org/maplibre-style-spec/layers/) object.
   * The layer goes below the layer named `beforeLayerId`, or above every layer
   * when `beforeLayerId` is empty.
   *
   * See `mln_map_add_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no layer named `beforeLayerId`, or
   *   no source with the ID that the layer uses.
   * - `Status.invalidArgument` when the style already has a layer with the new
   *   layer's ID.
   */
  addStyleLayerJson(
    layerJson: unknown,
    beforeLayerId: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_style_layer_json(
        this.id,
        layerJson,
        beforeLayerId,
        completion,
      ),
    );
  }

  /**
   * Adds a source to the map's style.
   *
   * `sourceJson` is a
   * &#91;source&#93;(https://maplibre.org/maplibre-style-spec/sources/) object.
   *
   * See `mln_map_add_style_source_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the style already has a source named
   *   `sourceId`.
   */
  addStyleSourceJson(
    sourceId: string,
    sourceJson: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_style_source_json(
        this.id,
        sourceId,
        sourceJson,
        completion,
      ),
    );
  }

  /**
   * Moves the camera relative to where it is, as a gesture does.
   *
   * `delta` can pan the map's content by an offset on the screen, scale the
   * zoom, and add to the bearing and pitch. An anchor keeps one screen point in
   * place while the scale, bearing, or pitch changes. An anchor needs one of
   * those changes, and combines with an offset only when the delta has no
   * animation. The function finishes when the map applies the delta, before an
   * animation ends.
   *
   * See `mln_map_apply_camera_delta` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  applyCameraDelta(delta: CameraDelta = {}): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_apply_camera_delta(this.id, delta, completion);
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Attaches a render session that draws the map into a texture on `device`.
   *
   * `options` must select `RenderDriverKind.coreWorker`. Read the frames back
   * with `RenderSessionHandle.readTexture()`.
   *
   * See `mln_map_attach_graphics_device_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` when the map already has a render session.
   */
  attachGraphicsDeviceTexture(
    device: GraphicsDeviceHandle,
    descriptor: GraphicsDeviceTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_graphics_device_texture(
            this.id,
            device.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map into a ring of your Metal textures.
   *
   * See `mln_map_attach_metal_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.metal`, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_metal_borrowed_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map into a ring of new Metal textures.
   *
   * See `mln_map_attach_metal_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.metal`, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_metal_owned_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map to a Metal layer.
   *
   * See `mln_map_attach_metal_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.metal`.
   */
  attachMetalSurface(
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_metal_surface(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map into a ring of your OpenGL textures.
   *
   * The session needs `RenderDriverKind.callerGraphicsThread`.
   *
   * See `mln_map_attach_open_gl_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `openGlSupportedContextProviderMask()` lacks
   *   the context's platform, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachOpenGlBorrowedTexture(
    descriptor: OpenGlBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_open_gl_borrowed_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map into a ring of new OpenGL textures.
   *
   * With `OpenGlContextOwnership.shared`, the session needs
   * `RenderDriverKind.callerGraphicsThread`, and the textures are in your
   * context's share group. With `OpenGlContextOwnership.dedicated`, the session
   * needs `RenderDriverKind.coreWorker` and renders in a context of its own.
   * You can't acquire its frames, but `RenderSessionHandle.readTexture()` reads
   * them.
   *
   * See `mln_map_attach_open_gl_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `openGlSupportedContextProviderMask()` lacks
   *   the context's platform, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachOpenGlOwnedTexture(
    descriptor: OpenGlOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_open_gl_owned_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map to an OpenGL surface.
   *
   * The session needs `RenderDriverKind.coreWorker` for a transferred WebGL
   * canvas, and `RenderDriverKind.callerGraphicsThread` for any other context.
   *
   * See `mln_map_attach_open_gl_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `openGlSupportedContextProviderMask()` lacks
   *   the context's platform.
   */
  attachOpenGlSurface(
    descriptor: OpenGlSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_open_gl_surface(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map into a ring of your Vulkan images.
   *
   * See `mln_map_attach_vulkan_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.vulkan`, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_vulkan_borrowed_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map into a ring of new Vulkan images.
   *
   * See `mln_map_attach_vulkan_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.vulkan`, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_vulkan_owned_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map to a Vulkan surface.
   *
   * See `mln_map_attach_vulkan_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.vulkan`.
   */
  attachVulkanSurface(
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_vulkan_surface(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map into a ring of your WebGPU textures.
   *
   * The session needs `RenderDriverKind.callerGraphicsThread`.
   *
   * See `mln_map_attach_web_gpu_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.webGpu`, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachWebGpuBorrowedTexture(
    descriptor: WebGpuBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_web_gpu_borrowed_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map into a ring of new WebGPU textures.
   *
   * The session needs `RenderDriverKind.callerGraphicsThread`.
   *
   * See `mln_map_attach_web_gpu_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.webGpu`, or when the target supports none of the
   *   `producer_sync_kinds` in `options`.
   */
  attachWebGpuOwnedTexture(
    descriptor: WebGpuOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_web_gpu_owned_texture(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Creates a session that renders the map to a WebGPU surface.
   *
   * The session needs `RenderDriverKind.callerGraphicsThread`.
   *
   * See `mln_map_attach_web_gpu_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when `supportedRenderBackendMask()` lacks
   *   `RenderBackendFlag.webGpu`.
   */
  attachWebGpuSurface(
    descriptor: WebGpuSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): NewRenderSession {
    return withRegistrations((registrations) => {
      const [session, ready] = attach(
        (completion) =>
          native.mln_map_attach_web_gpu_surface(
            this.id,
            descriptor,
            options,
            completion,
          ),
        (handle) =>
          adopt(
            handle,
            () => new RenderSessionHandle(ADOPT, handle, this, registrations),
            (disposed) =>
              native.mln_render_session_dispose(disposed, undefined, undefined),
          ),
      );
      return { session, ready };
    });
  }

  /**
   * Holds back the map's changes from rendering until
   * `MapHandle.endCommandGroup()`, so they show in the same frame.
   *
   * Groups nest, and the changes render when the outermost group ends.
   *
   * See `mln_map_begin_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  beginCommandGroup(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_begin_command_group(this.id, completion),
    );
  }

  /**
   * Computes a camera that fits a GeoJSON geometry in the map's view.
   *
   * `geometry` is a GeoJSON geometry object with at least one coordinate, not a
   * feature. The camera keeps the map's bearing and pitch unless `fitOptions`
   * sets them, and its zoom stays within the map's zoom limits.
   *
   * See `mln_map_camera_for_geometry` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cameraForGeometry(
    geometry: unknown,
    fitOptions?: CameraFitOptions,
  ): Promise<CameraOptions> {
    return query<CameraOptions>((completion) =>
      native.mln_map_camera_for_geometry(
        this.id,
        geometry,
        fitOptions,
        completion,
      ),
    );
  }

  /**
   * Computes a camera that fits `bounds` in the map's view.
   *
   * The camera keeps the map's bearing and pitch unless `fitOptions` sets them,
   * and its zoom stays within the map's zoom limits.
   *
   * See `mln_map_camera_for_lat_lng_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cameraForLatLngBounds(
    bounds: LatLngBounds = {},
    fitOptions?: CameraFitOptions,
  ): Promise<CameraOptions> {
    return query<CameraOptions>((completion) =>
      native.mln_map_camera_for_lat_lng_bounds(
        this.id,
        bounds,
        fitOptions,
        completion,
      ),
    );
  }

  /**
   * Computes a camera that fits `coordinates` in the map's view.
   *
   * `coordinates` must not be empty. The camera keeps the map's bearing and
   * pitch unless `fitOptions` sets them, and its zoom stays within the map's
   * zoom limits.
   *
   * See `mln_map_camera_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cameraForLatLngs(
    coordinates: readonly LatLng[],
    fitOptions?: CameraFitOptions,
  ): Promise<CameraOptions> {
    return query<CameraOptions>((completion) =>
      native.mln_map_camera_for_lat_lngs(
        this.id,
        coordinates,
        fitOptions,
        completion,
      ),
    );
  }

  /**
   * Stops the running camera animations whose animation options set
   * `transitionId`.
   *
   * Other animations keep running. The end handler of each stopped animation
   * reports `CameraTransitionOutcome.cancelled`.
   *
   * See `mln_map_cancel_camera_transition` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cancelCameraTransition(transitionId: bigint): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_cancel_camera_transition(
        this.id,
        transitionId,
        completion,
      ),
    );
  }

  /**
   * Stops every running camera animation where it is.
   *
   * The end handler of each stopped animation reports
   * `CameraTransitionOutcome.cancelled`.
   *
   * See `mln_map_cancel_transitions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cancelTransitions(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_cancel_transitions(this.id, completion),
    );
  }

  /**
   * Releases the map.
   *
   * Detach the map's render session first.
   *
   * See `mln_map_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  close(): Promise<void> {
    return closeAsync(this, (handle, token) =>
      submitRun((completion) =>
        native.mln_map_release(handle, token, completion),
      ),
    );
  }

  /**
   * Releases the map in any state, and finishes in the background.
   *
   * Use it from destructors and finalizers. Otherwise, use `MapHandle.close()`.
   *
   * See `mln_map_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  dispose(): void {
    closeSync(this, (handle, token) => native.mln_map_dispose(handle, token));
  }

  /**
   * Logs debugging information about the map's style and sources.
   *
   * See `mln_map_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  dumpDebugLogs(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_dump_debug_logs(this.id, completion),
    );
  }

  /**
   * Ends the group that `MapHandle.beginCommandGroup()` began.
   *
   * See `mln_map_end_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  endCommandGroup(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_end_command_group(this.id, completion),
    );
  }

  /**
   * Reads the map's camera.
   *
   * The query reads the camera after the map applies the commands sent before
   * it.
   *
   * See `mln_map_get_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  getCamera(): Promise<CameraQueryResult> {
    return query<CameraQueryResult>((completion) =>
      native.mln_map_get_camera(this.id, completion),
    );
  }

  /**
   * Reads the state of a feature, as a JSON object.
   *
   * The object is empty when the feature has no state.
   *
   * See `mln_map_get_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getFeatureState(selector: FeatureStateSelector): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_feature_state(this.id, selector, completion),
    );
  }

  /**
   * Reads the global state of the map's style as a JSON object.
   *
   * See `mln_map_get_global_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getGlobalState(): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_global_state(this.id, completion),
    );
  }

  /**
   * Reads the corners of an image source's image, clockwise from the top left.
   *
   * See `mln_map_get_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't an image
   *   source.
   */
  getImageSourceCoordinates(sourceId: string): Promise<readonly LatLng[]> {
    return query<readonly LatLng[]>((completion) =>
      native.mln_map_get_image_source_coordinates(
        this.id,
        sourceId,
        completion,
      ),
    );
  }

  /**
   * Reads the JSON of the style that the map loaded.
   *
   * The JSON doesn't include the changes made to the style since it loaded.
   *
   * See `mln_map_get_loaded_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getLoadedStyleJson(): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_loaded_style_json(this.id, completion),
    );
  }

  /**
   * Copies the map's state.
   *
   * The copy doesn't wait for pending commands. To read the camera after they
   * apply, use `MapHandle.getCamera()`.
   *
   * See `mln_map_get_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getSnapshot(): MapSnapshot {
    return native.mln_map_get_snapshot(this.id);
  }

  /**
   * Reads an image of the map's style.
   *
   * Completes with no value when the style has no image named `imageId`.
   *
   * See `mln_map_get_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleImage(imageId: string): Promise<StyleImageInfo | undefined> {
    return query<StyleImageInfo | undefined>((completion) =>
      native.mln_map_get_style_image(this.id, imageId, completion),
    );
  }

  /**
   * Reads a layer of the map's style.
   *
   * Completes with no value when the style has no layer named `layerId`.
   *
   * See `mln_map_get_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleLayer(layerId: string): Promise<StyleLayerInfo | undefined> {
    return query<StyleLayerInfo | undefined>((completion) =>
      native.mln_map_get_style_layer(this.id, layerId, completion),
    );
  }

  /**
   * Reads a layer of the map's style as a
   * &#91;layer&#93;(https://maplibre.org/maplibre-style-spec/layers/) object.
   *
   * Completes with no value when the style has no layer named `layerId`.
   *
   * See `mln_map_get_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleLayerJson(layerId: string): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_style_layer_json(this.id, layerId, completion),
    );
  }

  /**
   * Reads a property of a layer in the map's style.
   *
   * Completes with no value when the layer doesn't set the property.
   *
   * See `mln_map_get_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no layer named `layerId`.
   */
  getStyleLayerProperty(
    layerId: string,
    propertyName: string,
  ): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_style_layer_property(
        this.id,
        layerId,
        propertyName,
        completion,
      ),
    );
  }

  /**
   * Reads a property of the light of the map's style.
   *
   * Completes with no value when the light doesn't set the property.
   *
   * See `mln_map_get_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleLightProperty(propertyName: string): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_map_get_style_light_property(
        this.id,
        propertyName,
        completion,
      ),
    );
  }

  /**
   * Reads a source of the map's style.
   *
   * Completes with no value when the style has no source named `sourceId`.
   *
   * See `mln_map_get_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleSource(sourceId: string): Promise<StyleSourceInfo | undefined> {
    return query<StyleSourceInfo | undefined>((completion) =>
      native.mln_map_get_style_source(this.id, sourceId, completion),
    );
  }

  /**
   * Reads the default transition of the map's style.
   *
   * See `mln_map_get_style_transition_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleTransitionOptions(): Promise<StyleTransitionOptions> {
    return query<StyleTransitionOptions>((completion) =>
      native.mln_map_get_style_transition_options(this.id, completion),
    );
  }

  /**
   * Reads the URL of the map's style.
   *
   * The URL is empty when the style came from `MapHandle.setStyleJson()`.
   *
   * See `mln_map_get_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getStyleUrl(): Promise<string> {
    return query<string>((completion) =>
      native.mln_map_get_style_url(this.id, completion),
    );
  }

  /**
   * Makes a custom geometry source ask again for the tiles in `bounds` that the
   * map still needs.
   *
   * See `mln_map_invalidate_custom_geometry_source_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   geometry source.
   */
  invalidateCustomGeometrySourceRegion(
    sourceId: string,
    bounds: LatLngBounds = {},
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_invalidate_custom_geometry_source_region(
        this.id,
        sourceId,
        bounds,
        completion,
      ),
    );
  }

  /**
   * Makes a custom geometry source ask for a tile again when the map needs it.
   *
   * See `mln_map_invalidate_custom_geometry_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   geometry source.
   */
  invalidateCustomGeometrySourceTile(
    sourceId: string,
    tileId: CanonicalTileId = {},
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_invalidate_custom_geometry_source_tile(
        this.id,
        sourceId,
        tileId,
        completion,
      ),
    );
  }

  /**
   * Makes a custom vector source ask for a tile again when the map needs it.
   *
   * See `mln_map_invalidate_custom_vector_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   vector source.
   */
  invalidateCustomVectorSourceTile(
    sourceId: string,
    tileId: CanonicalTileId = {},
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_invalidate_custom_vector_source_tile(
        this.id,
        sourceId,
        tileId,
        completion,
      ),
    );
  }

  /**
   * Computes the area that the map would show with `camera`.
   *
   * Camera fields that `camera` doesn't set come from the map's camera. The
   * longitudes stay between -180 and 180.
   *
   * See `mln_map_lat_lng_bounds_for_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  latLngBoundsForCamera(camera: CameraOptions = {}): Promise<LatLngBounds> {
    return query<LatLngBounds>((completion) =>
      native.mln_map_lat_lng_bounds_for_camera(this.id, camera, completion),
    );
  }

  /**
   * Computes the area that the map would show with `camera`, without wrapping
   * longitudes.
   *
   * Camera fields that `camera` doesn't set come from the map's camera. The
   * longitudes can fall outside -180 to 180, so a view across the antimeridian
   * gives one continuous range.
   *
   * See `mln_map_lat_lng_bounds_for_camera_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  latLngBoundsForCameraUnwrapped(
    camera: CameraOptions = {},
  ): Promise<LatLngBounds> {
    return query<LatLngBounds>((completion) =>
      native.mln_map_lat_lng_bounds_for_camera_unwrapped(
        this.id,
        camera,
        completion,
      ),
    );
  }

  /**
   * Lists the layers of the map's style, from bottom to top.
   *
   * See `mln_map_list_style_layers` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  listStyleLayers(): Promise<readonly StyleLayerInfo[]> {
    return query<readonly StyleLayerInfo[]>((completion) =>
      native.mln_map_list_style_layers(this.id, completion),
    );
  }

  /**
   * Lists the sources of the map's style.
   *
   * See `mln_map_list_style_sources` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  listStyleSources(): Promise<readonly StyleSourceInfo[]> {
    return query<readonly StyleSourceInfo[]>((completion) =>
      native.mln_map_list_style_sources(this.id, completion),
    );
  }

  /**
   * Moves a layer of the map's style below the layer named `beforeLayerId`, or
   * above every layer when `beforeLayerId` is empty.
   *
   * See `mln_map_move_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no layer named `layerId` or
   *   `beforeLayerId`.
   */
  moveStyleLayer(
    layerId: string,
    beforeLayerId: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_move_style_layer(
        this.id,
        layerId,
        beforeLayerId,
        completion,
      ),
    );
  }

  /**
   * Removes the feature state that `selector` selects.
   *
   * See `mln_map_remove_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  removeFeatureState(
    selector: FeatureStateSelector,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_remove_feature_state(this.id, selector, completion),
    );
  }

  /**
   * Removes an image from the map's style.
   *
   * See `mln_map_remove_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no image named `imageId`.
   */
  removeStyleImage(imageId: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_remove_style_image(this.id, imageId, completion),
    );
  }

  /**
   * Removes a layer from the map's style.
   *
   * See `mln_map_remove_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no layer named `layerId`.
   */
  removeStyleLayer(layerId: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_remove_style_layer(this.id, layerId, completion),
    );
  }

  /**
   * Removes a source from the map's style.
   *
   * See `mln_map_remove_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidState` when a layer still uses the source.
   */
  removeStyleSource(sourceId: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_remove_style_source(this.id, sourceId, completion),
    );
  }

  /**
   * Renders a new frame of a continuous map, even when nothing changed.
   *
   * See `mln_map_request_repaint` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  requestRepaint(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_request_repaint(this.id, completion),
    );
  }

  /**
   * Renders a still image of a static or tile map.
   *
   * The map's render session draws the image once every resource that the image
   * needs has loaded.
   *
   * See `mln_map_request_still_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.cancelled` when the map is released before the image is done.
   * - `Status.nativeError` when the style or a resource that the image needs
   *   fails to load.
   */
  requestStillImage(): Promise<void> {
    return run((completion) =>
      native.mln_map_request_still_image(this.id, completion),
    );
  }

  /**
   * Sets the map's size.
   *
   * `extent` must keep the scale factor that the map was created with. A map
   * with a render session takes its size from the session, so resize the
   * session with `RenderSessionHandle.resize()` instead.
   *
   * See `mln_map_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  resize(extent: LogicalExtent = {}): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_resize(this.id, extent, completion),
    );
  }

  /**
   * Limits the camera's center to an area, and its zoom and pitch to ranges.
   *
   * Limits that `options` doesn't set keep their current values. When the
   * camera is outside the new limits, it moves inside them.
   *
   * See `mln_map_set_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setBounds(options: BoundOptions = {}): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_bounds(this.id, options, completion),
    );
  }

  /**
   * Sets the data of a tile in a custom geometry source.
   *
   * `data` is a GeoJSON feature collection. The source ignores data for a tile
   * that it hasn't asked for, or has since cancelled.
   *
   * See `mln_map_set_custom_geometry_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   geometry source.
   */
  setCustomGeometrySourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    data: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_custom_geometry_source_tile_data(
        this.id,
        sourceId,
        tileId,
        data,
        completion,
      ),
    );
  }

  /**
   * Reports that a tile of a custom geometry source failed to load.
   *
   * See `mln_map_set_custom_geometry_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   geometry source.
   */
  setCustomGeometrySourceTileError(
    sourceId: string,
    tileId: CanonicalTileId,
    message: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_custom_geometry_source_tile_error(
        this.id,
        sourceId,
        tileId,
        message,
        completion,
      ),
    );
  }

  /**
   * Sets the data of a tile in a custom vector source.
   *
   * `data` is the tile, encoded as `encoding`. The source ignores data for a
   * tile that it hasn't asked for, or has since cancelled.
   *
   * See `mln_map_set_custom_vector_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   vector source.
   */
  setCustomVectorSourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    encoding: StyleVectorTileEncoding,
    data: Uint8Array,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_custom_vector_source_tile_data(
        this.id,
        sourceId,
        tileId,
        encoding,
        data,
        completion,
      ),
    );
  }

  /**
   * Reports that a tile of a custom vector source failed to load.
   *
   * See `mln_map_set_custom_vector_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a custom
   *   vector source.
   */
  setCustomVectorSourceTileError(
    sourceId: string,
    tileId: CanonicalTileId,
    message: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_custom_vector_source_tile_error(
        this.id,
        sourceId,
        tileId,
        message,
        completion,
      ),
    );
  }

  /**
   * Draws debugging overlays, such as tile borders, on the map.
   *
   * `options` replaces the overlays set before.
   *
   * See `mln_map_set_debug_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setDebugOptions(options: MapDebugFlag): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_debug_options(this.id, options, completion),
    );
  }

  /**
   * Selects the events that the map reports.
   *
   * See `mln_map_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setEventMask(mask: RuntimeEventFlag): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_event_mask(this.id, mask, completion),
    );
  }

  /**
   * Merges `state` into the state of a feature.
   *
   * `state` is a JSON object. Its keys replace the same keys in the feature's
   * state, and the other keys stay.
   *
   * See `mln_map_set_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setFeatureState(
    selector: FeatureStateSelector,
    state: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_feature_state(this.id, selector, state, completion),
    );
  }

  /**
   * Places the camera at a position and orientation in space.
   *
   * The map adjusts them to a camera that it can show, and stops running camera
   * animations.
   *
   * See `mln_map_set_free_camera_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setFreeCameraOptions(
    options: FreeCameraOptions = {},
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_free_camera_options(this.id, options, completion),
    );
  }

  /**
   * Replaces the data of a GeoJSON source.
   *
   * `data` must have been prepared with the same options as the source. It can
   * be released once this function returns.
   *
   * See `mln_map_set_geo_json_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a GeoJSON
   *   source, or `data` was prepared with other options.
   */
  setGeoJsonSourceData(
    sourceId: string,
    data: GeoJsonSourceDataHandle,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_geo_json_source_data(
        this.id,
        sourceId,
        data.id,
        completion,
      ),
    );
  }

  /**
   * Sets whether a GeoJSON source builds its tiles synchronously.
   *
   * This works like the synchronous tiling option of `GeoJsonSourceOptions`,
   * but `enabled` can't turn off synchronous tiling that the source's options
   * turned on.
   *
   * See `mln_map_set_geo_json_source_synchronous_tiling` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a GeoJSON
   *   source.
   */
  setGeoJsonSourceSynchronousTiling(
    sourceId: string,
    enabled: boolean,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_geo_json_source_synchronous_tiling(
        this.id,
        sourceId,
        enabled,
        completion,
      ),
    );
  }

  /**
   * Makes a GeoJSON source load its data from `url`.
   *
   * See `mln_map_set_geo_json_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't a GeoJSON
   *   source.
   */
  setGeoJsonSourceUrl(
    sourceId: string,
    url: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_geo_json_source_url(
        this.id,
        sourceId,
        url,
        completion,
      ),
    );
  }

  /**
   * Sets a value that the style's `global-state`
   * &#91;expressions&#93;(https://maplibre.org/maplibre-style-spec/expressions/)
   * read.
   *
   * See `mln_map_set_global_state_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` when the style or one of its sources or sprites is
   *   still loading.
   */
  setGlobalStateProperty(
    propertyName: string,
    value: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_global_state_property(
        this.id,
        propertyName,
        value,
        completion,
      ),
    );
  }

  /**
   * Moves the image of an image source.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_set_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't an image
   *   source.
   */
  setImageSourceCoordinates(
    sourceId: string,
    coordinates: readonly LatLng[],
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_image_source_coordinates(
        this.id,
        sourceId,
        coordinates,
        completion,
      ),
    );
  }

  /**
   * Replaces the image of an image source.
   *
   * See `mln_map_set_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't an image
   *   source.
   */
  setImageSourceImage(
    sourceId: string,
    image: PremultipliedRgba8Image,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_image_source_image(
        this.id,
        sourceId,
        image,
        completion,
      ),
    );
  }

  /**
   * Makes an image source load its image from `url`.
   *
   * See `mln_map_set_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   * - `Status.invalidArgument` when the source named `sourceId` isn't an image
   *   source.
   */
  setImageSourceUrl(sourceId: string, url: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_image_source_url(this.id, sourceId, url, completion),
    );
  }

  /**
   * Sets whether the map draws in perspective or axonometric projection, and
   * how much it skews the view.
   *
   * Settings that `mode` doesn't set keep their current values.
   *
   * See `mln_map_set_projection_mode` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setProjectionMode(mode: ProjectionMode = {}): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_projection_mode(this.id, mode, completion),
    );
  }

  /**
   * Shows or hides an overlay of rendering statistics on the map.
   *
   * See `mln_map_set_rendering_stats_view_enabled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setRenderingStatsViewEnabled(enabled: boolean): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_rendering_stats_view_enabled(
        this.id,
        enabled,
        completion,
      ),
    );
  }

  /**
   * Adds an image to the map's style, or replaces the image named `imageId`.
   *
   * See `mln_map_set_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleImage(
    imageId: string,
    image: PremultipliedRgba8Image,
    options?: StyleImageOptions,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_image(
        this.id,
        imageId,
        image,
        options,
        completion,
      ),
    );
  }

  /**
   * Loads the map's style from `json`.
   *
   * The style's sources, sprites, and glyphs load in the background.
   *
   * See `mln_map_set_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.nativeError` when `json` isn't a valid style.
   */
  setStyleJson(json: unknown): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_json(this.id, json, completion),
    );
  }

  /**
   * Sets a property of a layer in the map's style.
   *
   * `propertyName` is one of the layer's layout or paint
   * &#91;properties&#93;(https://maplibre.org/maplibre-style-spec/layers/), or
   * `filter`, `minzoom`, `maxzoom`, `source`, or `source-layer`. A null `value`
   * resets any of these but `source` and `source-layer` to its default.
   *
   * See `mln_map_set_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no layer named `layerId`.
   */
  setStyleLayerProperty(
    layerId: string,
    propertyName: string,
    value: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_layer_property(
        this.id,
        layerId,
        propertyName,
        value,
        completion,
      ),
    );
  }

  /**
   * Replaces the
   * &#91;light&#93;(https://maplibre.org/maplibre-style-spec/light/) of the
   * map's style.
   *
   * See `mln_map_set_style_light_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLightJson(lightJson: unknown): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_light_json(this.id, lightJson, completion),
    );
  }

  /**
   * Sets a property of the light of the map's style.
   *
   * See `mln_map_set_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLightProperty(
    propertyName: string,
    value: unknown,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_light_property(
        this.id,
        propertyName,
        value,
        completion,
      ),
    );
  }

  /**
   * Sets whether a source is volatile.
   *
   * The map doesn't store the tiles of a volatile source in the persistent
   * cache.
   *
   * See `mln_map_set_style_source_volatile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the style has no source named `sourceId`.
   */
  setStyleSourceVolatile(
    sourceId: string,
    isVolatile: boolean,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_source_volatile(
        this.id,
        sourceId,
        isVolatile,
        completion,
      ),
    );
  }

  /**
   * Sets the default transition of the map's style.
   *
   * See `mln_map_set_style_transition_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleTransitionOptions(
    options: StyleTransitionOptions = {},
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_transition_options(this.id, options, completion),
    );
  }

  /**
   * Loads the map's style from `url`.
   *
   * The style loads in the background, and `RuntimeEventType.mapStyleLoaded` or
   * `RuntimeEventType.mapLoadingFailed` reports how the load ends. Sources,
   * layers, and images added before the style loads are lost when it loads.
   *
   * See `mln_map_set_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setStyleUrl(url: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_url(this.id, url, completion),
    );
  }

  /**
   * Sets how the map chooses which tiles to load, and at what detail.
   *
   * Settings that `options` doesn't set keep their current values.
   *
   * See `mln_map_set_tile_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setTileOptions(options: MapTileOptions = {}): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_tile_options(this.id, options, completion),
    );
  }

  /**
   * Sets how the map orients and constrains its view.
   *
   * Settings that `options` doesn't set keep their current values.
   *
   * See `mln_map_set_viewport_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setViewportOptions(
    options: MapViewportOptions = {},
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_viewport_options(this.id, options, completion),
    );
  }

  /**
   * Moves the camera, at once or with an animation.
   *
   * Camera fields that `update` doesn't set keep their values, and the update
   * stops any running animation of the fields it sets. The function finishes
   * when the map applies the update, before an animation ends. To learn when an
   * animation ends, set an `CameraTransitionHandler` in its animation options.
   *
   * See `mln_map_update_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  updateCamera(update: CameraUpdate = {}): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_update_camera(this.id, update, completion);
        registrations.commit(rootsOf(this));
      }),
    );
  }
}
defineDisposers(MapHandle, "close", "dispose");

/**
 * The commands of a {@link MapHandle} that discard their outcomes:
 * each returns once native code accepts it and allocates no completion.
 * A later completion on the same map fences them.
 */
export class MapHandleDiscarding {
  readonly #owner: MapHandle;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, owner: MapHandle) {
    checkKey(key);
    this.#owner = owner;
  }

  /**
   * Adds a source whose GeoJSON tiles come from callbacks.
   *
   * See `mln_map_add_custom_geometry_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addCustomGeometrySource(
    sourceId: string,
    options: CustomGeometrySourceOptions,
  ): void {
    withRegistrations((registrations) => {
      native.mln_map_add_custom_geometry_source(
        this.#owner.id,
        sourceId,
        options,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Adds a source whose vector tiles come from callbacks.
   *
   * See `mln_map_add_custom_vector_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addCustomVectorSource(
    sourceId: string,
    options: CustomVectorSourceOptions,
  ): void {
    withRegistrations((registrations) => {
      native.mln_map_add_custom_vector_source(
        this.#owner.id,
        sourceId,
        options,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Adds a GeoJSON source with prepared data.
   *
   * The source uses the options that `data` was prepared with. `data` can be
   * released once this function returns.
   *
   * See `mln_map_add_geo_json_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addGeoJsonSourceData(sourceId: string, data: GeoJsonSourceDataHandle): void {
    native.mln_map_add_geo_json_source_data(
      this.#owner.id,
      sourceId,
      data.id,
      undefined,
    );
  }

  /**
   * Adds a GeoJSON source that loads its data from `url`.
   *
   * See `mln_map_add_geo_json_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addGeoJsonSourceUrl(
    sourceId: string,
    url: string,
    options?: GeoJsonSourceOptions,
  ): void {
    native.mln_map_add_geo_json_source_url(
      this.#owner.id,
      sourceId,
      url,
      options,
      undefined,
    );
  }

  /**
   * Adds an image source that shows `image`.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_add_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addImageSourceImage(
    sourceId: string,
    coordinates: readonly LatLng[],
    image: PremultipliedRgba8Image,
  ): void {
    native.mln_map_add_image_source_image(
      this.#owner.id,
      sourceId,
      coordinates,
      image,
      undefined,
    );
  }

  /**
   * Adds an image source that loads its image from `url`.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_add_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addImageSourceUrl(
    sourceId: string,
    coordinates: readonly LatLng[],
    url: string,
  ): void {
    native.mln_map_add_image_source_url(
      this.#owner.id,
      sourceId,
      coordinates,
      url,
      undefined,
    );
  }

  /**
   * Adds a layer to the map's style.
   *
   * `layerJson` is a
   * &#91;layer&#93;(https://maplibre.org/maplibre-style-spec/layers/) object.
   * The layer goes below the layer named `beforeLayerId`, or above every layer
   * when `beforeLayerId` is empty.
   *
   * See `mln_map_add_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addStyleLayerJson(layerJson: unknown, beforeLayerId: string): void {
    native.mln_map_add_style_layer_json(
      this.#owner.id,
      layerJson,
      beforeLayerId,
      undefined,
    );
  }

  /**
   * Adds a source to the map's style.
   *
   * `sourceJson` is a
   * &#91;source&#93;(https://maplibre.org/maplibre-style-spec/sources/) object.
   *
   * See `mln_map_add_style_source_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addStyleSourceJson(sourceId: string, sourceJson: unknown): void {
    native.mln_map_add_style_source_json(
      this.#owner.id,
      sourceId,
      sourceJson,
      undefined,
    );
  }

  /**
   * Moves the camera relative to where it is, as a gesture does.
   *
   * `delta` can pan the map's content by an offset on the screen, scale the
   * zoom, and add to the bearing and pitch. An anchor keeps one screen point in
   * place while the scale, bearing, or pitch changes. An anchor needs one of
   * those changes, and combines with an offset only when the delta has no
   * animation. The function finishes when the map applies the delta, before an
   * animation ends.
   *
   * See `mln_map_apply_camera_delta` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  applyCameraDelta(delta: CameraDelta = {}): void {
    withRegistrations((registrations) => {
      native.mln_map_apply_camera_delta(this.#owner.id, delta, undefined);
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Holds back the map's changes from rendering until
   * `MapHandle.endCommandGroup()`, so they show in the same frame.
   *
   * Groups nest, and the changes render when the outermost group ends.
   *
   * See `mln_map_begin_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  beginCommandGroup(): void {
    native.mln_map_begin_command_group(this.#owner.id, undefined);
  }

  /**
   * Stops the running camera animations whose animation options set
   * `transitionId`.
   *
   * Other animations keep running. The end handler of each stopped animation
   * reports `CameraTransitionOutcome.cancelled`.
   *
   * See `mln_map_cancel_camera_transition` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cancelCameraTransition(transitionId: bigint): void {
    native.mln_map_cancel_camera_transition(
      this.#owner.id,
      transitionId,
      undefined,
    );
  }

  /**
   * Stops every running camera animation where it is.
   *
   * The end handler of each stopped animation reports
   * `CameraTransitionOutcome.cancelled`.
   *
   * See `mln_map_cancel_transitions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cancelTransitions(): void {
    native.mln_map_cancel_transitions(this.#owner.id, undefined);
  }

  /**
   * Logs debugging information about the map's style and sources.
   *
   * See `mln_map_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  dumpDebugLogs(): void {
    native.mln_map_dump_debug_logs(this.#owner.id, undefined);
  }

  /**
   * Ends the group that `MapHandle.beginCommandGroup()` began.
   *
   * See `mln_map_end_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  endCommandGroup(): void {
    native.mln_map_end_command_group(this.#owner.id, undefined);
  }

  /**
   * Makes a custom geometry source ask again for the tiles in `bounds` that the
   * map still needs.
   *
   * See `mln_map_invalidate_custom_geometry_source_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  invalidateCustomGeometrySourceRegion(
    sourceId: string,
    bounds: LatLngBounds = {},
  ): void {
    native.mln_map_invalidate_custom_geometry_source_region(
      this.#owner.id,
      sourceId,
      bounds,
      undefined,
    );
  }

  /**
   * Makes a custom geometry source ask for a tile again when the map needs it.
   *
   * See `mln_map_invalidate_custom_geometry_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  invalidateCustomGeometrySourceTile(
    sourceId: string,
    tileId: CanonicalTileId = {},
  ): void {
    native.mln_map_invalidate_custom_geometry_source_tile(
      this.#owner.id,
      sourceId,
      tileId,
      undefined,
    );
  }

  /**
   * Makes a custom vector source ask for a tile again when the map needs it.
   *
   * See `mln_map_invalidate_custom_vector_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  invalidateCustomVectorSourceTile(
    sourceId: string,
    tileId: CanonicalTileId = {},
  ): void {
    native.mln_map_invalidate_custom_vector_source_tile(
      this.#owner.id,
      sourceId,
      tileId,
      undefined,
    );
  }

  /**
   * Moves a layer of the map's style below the layer named `beforeLayerId`, or
   * above every layer when `beforeLayerId` is empty.
   *
   * See `mln_map_move_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  moveStyleLayer(layerId: string, beforeLayerId: string): void {
    native.mln_map_move_style_layer(
      this.#owner.id,
      layerId,
      beforeLayerId,
      undefined,
    );
  }

  /**
   * Removes the feature state that `selector` selects.
   *
   * See `mln_map_remove_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  removeFeatureState(selector: FeatureStateSelector): void {
    native.mln_map_remove_feature_state(this.#owner.id, selector, undefined);
  }

  /**
   * Removes an image from the map's style.
   *
   * See `mln_map_remove_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleImage(imageId: string): void {
    native.mln_map_remove_style_image(this.#owner.id, imageId, undefined);
  }

  /**
   * Removes a layer from the map's style.
   *
   * See `mln_map_remove_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleLayer(layerId: string): void {
    native.mln_map_remove_style_layer(this.#owner.id, layerId, undefined);
  }

  /**
   * Removes a source from the map's style.
   *
   * See `mln_map_remove_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleSource(sourceId: string): void {
    native.mln_map_remove_style_source(this.#owner.id, sourceId, undefined);
  }

  /**
   * Renders a new frame of a continuous map, even when nothing changed.
   *
   * See `mln_map_request_repaint` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  requestRepaint(): void {
    native.mln_map_request_repaint(this.#owner.id, undefined);
  }

  /**
   * Sets the map's size.
   *
   * `extent` must keep the scale factor that the map was created with. A map
   * with a render session takes its size from the session, so resize the
   * session with `RenderSessionHandle.resize()` instead.
   *
   * See `mln_map_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  resize(extent: LogicalExtent = {}): void {
    native.mln_map_resize(this.#owner.id, extent, undefined);
  }

  /**
   * Limits the camera's center to an area, and its zoom and pitch to ranges.
   *
   * Limits that `options` doesn't set keep their current values. When the
   * camera is outside the new limits, it moves inside them.
   *
   * See `mln_map_set_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setBounds(options: BoundOptions = {}): void {
    native.mln_map_set_bounds(this.#owner.id, options, undefined);
  }

  /**
   * Sets the data of a tile in a custom geometry source.
   *
   * `data` is a GeoJSON feature collection. The source ignores data for a tile
   * that it hasn't asked for, or has since cancelled.
   *
   * See `mln_map_set_custom_geometry_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomGeometrySourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    data: unknown,
  ): void {
    native.mln_map_set_custom_geometry_source_tile_data(
      this.#owner.id,
      sourceId,
      tileId,
      data,
      undefined,
    );
  }

  /**
   * Reports that a tile of a custom geometry source failed to load.
   *
   * See `mln_map_set_custom_geometry_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomGeometrySourceTileError(
    sourceId: string,
    tileId: CanonicalTileId,
    message: string,
  ): void {
    native.mln_map_set_custom_geometry_source_tile_error(
      this.#owner.id,
      sourceId,
      tileId,
      message,
      undefined,
    );
  }

  /**
   * Sets the data of a tile in a custom vector source.
   *
   * `data` is the tile, encoded as `encoding`. The source ignores data for a
   * tile that it hasn't asked for, or has since cancelled.
   *
   * See `mln_map_set_custom_vector_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomVectorSourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    encoding: StyleVectorTileEncoding,
    data: Uint8Array,
  ): void {
    native.mln_map_set_custom_vector_source_tile_data(
      this.#owner.id,
      sourceId,
      tileId,
      encoding,
      data,
      undefined,
    );
  }

  /**
   * Reports that a tile of a custom vector source failed to load.
   *
   * See `mln_map_set_custom_vector_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomVectorSourceTileError(
    sourceId: string,
    tileId: CanonicalTileId,
    message: string,
  ): void {
    native.mln_map_set_custom_vector_source_tile_error(
      this.#owner.id,
      sourceId,
      tileId,
      message,
      undefined,
    );
  }

  /**
   * Draws debugging overlays, such as tile borders, on the map.
   *
   * `options` replaces the overlays set before.
   *
   * See `mln_map_set_debug_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setDebugOptions(options: MapDebugFlag): void {
    native.mln_map_set_debug_options(this.#owner.id, options, undefined);
  }

  /**
   * Selects the events that the map reports.
   *
   * See `mln_map_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setEventMask(mask: RuntimeEventFlag): void {
    native.mln_map_set_event_mask(this.#owner.id, mask, undefined);
  }

  /**
   * Merges `state` into the state of a feature.
   *
   * `state` is a JSON object. Its keys replace the same keys in the feature's
   * state, and the other keys stay.
   *
   * See `mln_map_set_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setFeatureState(selector: FeatureStateSelector, state: unknown): void {
    native.mln_map_set_feature_state(
      this.#owner.id,
      selector,
      state,
      undefined,
    );
  }

  /**
   * Places the camera at a position and orientation in space.
   *
   * The map adjusts them to a camera that it can show, and stops running camera
   * animations.
   *
   * See `mln_map_set_free_camera_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setFreeCameraOptions(options: FreeCameraOptions = {}): void {
    native.mln_map_set_free_camera_options(this.#owner.id, options, undefined);
  }

  /**
   * Replaces the data of a GeoJSON source.
   *
   * `data` must have been prepared with the same options as the source. It can
   * be released once this function returns.
   *
   * See `mln_map_set_geo_json_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeoJsonSourceData(sourceId: string, data: GeoJsonSourceDataHandle): void {
    native.mln_map_set_geo_json_source_data(
      this.#owner.id,
      sourceId,
      data.id,
      undefined,
    );
  }

  /**
   * Sets whether a GeoJSON source builds its tiles synchronously.
   *
   * This works like the synchronous tiling option of `GeoJsonSourceOptions`,
   * but `enabled` can't turn off synchronous tiling that the source's options
   * turned on.
   *
   * See `mln_map_set_geo_json_source_synchronous_tiling` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeoJsonSourceSynchronousTiling(sourceId: string, enabled: boolean): void {
    native.mln_map_set_geo_json_source_synchronous_tiling(
      this.#owner.id,
      sourceId,
      enabled,
      undefined,
    );
  }

  /**
   * Makes a GeoJSON source load its data from `url`.
   *
   * See `mln_map_set_geo_json_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeoJsonSourceUrl(sourceId: string, url: string): void {
    native.mln_map_set_geo_json_source_url(
      this.#owner.id,
      sourceId,
      url,
      undefined,
    );
  }

  /**
   * Sets a value that the style's `global-state`
   * &#91;expressions&#93;(https://maplibre.org/maplibre-style-spec/expressions/)
   * read.
   *
   * See `mln_map_set_global_state_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGlobalStateProperty(propertyName: string, value: unknown): void {
    native.mln_map_set_global_state_property(
      this.#owner.id,
      propertyName,
      value,
      undefined,
    );
  }

  /**
   * Moves the image of an image source.
   *
   * `coordinates` are the image's four corners, clockwise from the top left.
   *
   * See `mln_map_set_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setImageSourceCoordinates(
    sourceId: string,
    coordinates: readonly LatLng[],
  ): void {
    native.mln_map_set_image_source_coordinates(
      this.#owner.id,
      sourceId,
      coordinates,
      undefined,
    );
  }

  /**
   * Replaces the image of an image source.
   *
   * See `mln_map_set_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setImageSourceImage(sourceId: string, image: PremultipliedRgba8Image): void {
    native.mln_map_set_image_source_image(
      this.#owner.id,
      sourceId,
      image,
      undefined,
    );
  }

  /**
   * Makes an image source load its image from `url`.
   *
   * See `mln_map_set_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setImageSourceUrl(sourceId: string, url: string): void {
    native.mln_map_set_image_source_url(
      this.#owner.id,
      sourceId,
      url,
      undefined,
    );
  }

  /**
   * Sets whether the map draws in perspective or axonometric projection, and
   * how much it skews the view.
   *
   * Settings that `mode` doesn't set keep their current values.
   *
   * See `mln_map_set_projection_mode` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setProjectionMode(mode: ProjectionMode = {}): void {
    native.mln_map_set_projection_mode(this.#owner.id, mode, undefined);
  }

  /**
   * Shows or hides an overlay of rendering statistics on the map.
   *
   * See `mln_map_set_rendering_stats_view_enabled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setRenderingStatsViewEnabled(enabled: boolean): void {
    native.mln_map_set_rendering_stats_view_enabled(
      this.#owner.id,
      enabled,
      undefined,
    );
  }

  /**
   * Adds an image to the map's style, or replaces the image named `imageId`.
   *
   * See `mln_map_set_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleImage(
    imageId: string,
    image: PremultipliedRgba8Image,
    options?: StyleImageOptions,
  ): void {
    native.mln_map_set_style_image(
      this.#owner.id,
      imageId,
      image,
      options,
      undefined,
    );
  }

  /**
   * Loads the map's style from `json`.
   *
   * The style's sources, sprites, and glyphs load in the background.
   *
   * See `mln_map_set_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setStyleJson(json: unknown): void {
    native.mln_map_set_style_json(this.#owner.id, json, undefined);
  }

  /**
   * Sets a property of a layer in the map's style.
   *
   * `propertyName` is one of the layer's layout or paint
   * &#91;properties&#93;(https://maplibre.org/maplibre-style-spec/layers/), or
   * `filter`, `minzoom`, `maxzoom`, `source`, or `source-layer`. A null `value`
   * resets any of these but `source` and `source-layer` to its default.
   *
   * See `mln_map_set_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLayerProperty(
    layerId: string,
    propertyName: string,
    value: unknown,
  ): void {
    native.mln_map_set_style_layer_property(
      this.#owner.id,
      layerId,
      propertyName,
      value,
      undefined,
    );
  }

  /**
   * Replaces the
   * &#91;light&#93;(https://maplibre.org/maplibre-style-spec/light/) of the
   * map's style.
   *
   * See `mln_map_set_style_light_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLightJson(lightJson: unknown): void {
    native.mln_map_set_style_light_json(this.#owner.id, lightJson, undefined);
  }

  /**
   * Sets a property of the light of the map's style.
   *
   * See `mln_map_set_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLightProperty(propertyName: string, value: unknown): void {
    native.mln_map_set_style_light_property(
      this.#owner.id,
      propertyName,
      value,
      undefined,
    );
  }

  /**
   * Sets whether a source is volatile.
   *
   * The map doesn't store the tiles of a volatile source in the persistent
   * cache.
   *
   * See `mln_map_set_style_source_volatile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleSourceVolatile(sourceId: string, isVolatile: boolean): void {
    native.mln_map_set_style_source_volatile(
      this.#owner.id,
      sourceId,
      isVolatile,
      undefined,
    );
  }

  /**
   * Sets the default transition of the map's style.
   *
   * See `mln_map_set_style_transition_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleTransitionOptions(options: StyleTransitionOptions = {}): void {
    native.mln_map_set_style_transition_options(
      this.#owner.id,
      options,
      undefined,
    );
  }

  /**
   * Loads the map's style from `url`.
   *
   * The style loads in the background, and `RuntimeEventType.mapStyleLoaded` or
   * `RuntimeEventType.mapLoadingFailed` reports how the load ends. Sources,
   * layers, and images added before the style loads are lost when it loads.
   *
   * See `mln_map_set_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setStyleUrl(url: string): void {
    native.mln_map_set_style_url(this.#owner.id, url, undefined);
  }

  /**
   * Sets how the map chooses which tiles to load, and at what detail.
   *
   * Settings that `options` doesn't set keep their current values.
   *
   * See `mln_map_set_tile_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setTileOptions(options: MapTileOptions = {}): void {
    native.mln_map_set_tile_options(this.#owner.id, options, undefined);
  }

  /**
   * Sets how the map orients and constrains its view.
   *
   * Settings that `options` doesn't set keep their current values.
   *
   * See `mln_map_set_viewport_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setViewportOptions(options: MapViewportOptions = {}): void {
    native.mln_map_set_viewport_options(this.#owner.id, options, undefined);
  }

  /**
   * Moves the camera, at once or with an animation.
   *
   * Camera fields that `update` doesn't set keep their values, and the update
   * stops any running animation of the fields it sets. The function finishes
   * when the map applies the update, before an animation ends. To learn when an
   * animation ends, set an `CameraTransitionHandler` in its animation options.
   *
   * See `mln_map_update_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  updateCamera(update: CameraUpdate = {}): void {
    withRegistrations((registrations) => {
      native.mln_map_update_camera(this.#owner.id, update, undefined);
      registrations.commit(rootsOf(this.#owner));
    });
  }
}

/**
 * A camera view for converting between screen points and coordinates.
 *
 * See `mln_map_projection` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class MapProjectionHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "MapProjectionHandle", { registrations });
  }
  /**
   * Calls `close()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `close()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Releases a projection.
   *
   * See `mln_map_projection_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  close(): void {
    closeSync(this, (handle, token) =>
      native.mln_map_projection_release(handle, token),
    );
  }

  /**
   * Creates a projection that converts coordinates for one view of a map.
   *
   * Create it from the `view` of an `MapSnapshot` or an `RenderFrameResult`.
   * The projection doesn't follow the map, so create a new one after the map's
   * camera or size changes.
   *
   * See `mln_map_projection_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  static create(view: MapView = {}): MapProjectionHandle {
    const handle = native.mln_map_projection_create(view);
    return adopt(
      handle,
      () => new MapProjectionHandle(ADOPT, handle),
      (disposed) => native.mln_map_projection_release(disposed, undefined),
    );
  }

  /**
   * Reads the projection's camera.
   *
   * See `mln_map_projection_get_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  getCamera(): CameraOptions {
    return native.mln_map_projection_get_camera(this.id);
  }

  /**
   * Finds the coordinate at a screen point.
   *
   * The longitude stays between -180 and 180.
   *
   * See `mln_map_projection_lat_lng_for_pixel` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  latLngForPixel(point: ScreenPoint = {}): LatLng {
    return native.mln_map_projection_lat_lng_for_pixel(this.id, point);
  }

  /**
   * Finds the coordinate at a screen point, without wrapping its longitude.
   *
   * The longitude can fall outside -180 to 180, to show which copy of the world
   * the point is in.
   *
   * See `mln_map_projection_lat_lng_for_pixel_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  latLngForPixelUnwrapped(point: ScreenPoint = {}): LatLng {
    return native.mln_map_projection_lat_lng_for_pixel_unwrapped(
      this.id,
      point,
    );
  }

  /**
   * Finds the coordinates at several screen points, as
   * `MapProjectionHandle.latLngForPixel()` does.
   *
   * See `mln_map_projection_lat_lngs_for_pixels` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  latLngsForPixels(points: readonly ScreenPoint[]): readonly LatLng[] {
    return native.mln_map_projection_lat_lngs_for_pixels(this.id, points);
  }

  /**
   * Finds the coordinates at several screen points, as
   * `MapProjectionHandle.latLngForPixelUnwrapped()` does.
   *
   * See `mln_map_projection_lat_lngs_for_pixels_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  latLngsForPixelsUnwrapped(points: readonly ScreenPoint[]): readonly LatLng[] {
    return native.mln_map_projection_lat_lngs_for_pixels_unwrapped(
      this.id,
      points,
    );
  }

  /**
   * Computes how many meters one screen pixel covers at `latitude`, at the
   * projection's zoom.
   *
   * The result ignores the camera's pitch.
   *
   * See `mln_map_projection_meters_per_pixel_at_latitude` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  metersPerPixelAtLatitude(latitude: number): number {
    return native.mln_map_projection_meters_per_pixel_at_latitude(
      this.id,
      latitude,
    );
  }

  /**
   * Finds the screen point of a coordinate.
   *
   * The point is in the copy of the world nearest the camera's center.
   *
   * See `mln_map_projection_pixel_for_lat_lng` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  pixelForLatLng(coordinate: LatLng = {}): ScreenPoint {
    return native.mln_map_projection_pixel_for_lat_lng(this.id, coordinate);
  }

  /**
   * Finds the screen points of several coordinates, as
   * `MapProjectionHandle.pixelForLatLng()` does.
   *
   * See `mln_map_projection_pixels_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  pixelsForLatLngs(coordinates: readonly LatLng[]): readonly ScreenPoint[] {
    return native.mln_map_projection_pixels_for_lat_lngs(this.id, coordinates);
  }

  /**
   * Moves the projection's camera, without changing the map.
   *
   * Camera fields that `camera` doesn't set keep their values.
   *
   * See `mln_map_projection_set_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  setCamera(camera: CameraOptions = {}): void {
    native.mln_map_projection_set_camera(this.id, camera);
  }
}
defineDisposers(MapProjectionHandle, undefined, "close");

/**
 * A render session draws a map into a surface or a texture.
 *
 * See `mln_render_session` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class RenderSessionHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(
    key: AdoptKey,
    handle: bigint,
    parent: MapHandle,
    registrations?: Registrations,
  ) {
    super(key, handle, "RenderSessionHandle", {
      parent,
      registrations,
      session: true,
    });
  }

  /** The commands of this owner that discard their outcomes. */
  get discarding(): RenderSessionHandleDiscarding {
    return new RenderSessionHandleDiscarding(ADOPT, this);
  }
  /**
   * Calls `close()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `dispose()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Stops the session right away, without running its pending graphics work.
   *
   * Use it when the target's graphics objects are going away and a detach can't
   * finish first. Pending work completes with `Status.targetLost`, and pending
   * frame requests finish with `RenderResult.targetNotReady`. Once the call
   * returns, the library no longer uses your graphics objects, and you can
   * attach another session to the map. Dispose the session afterward.
   *
   * See `mln_render_session_abandon` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  abandon(): RenderAbandonResult {
    return native.mln_render_session_abandon(this.id);
  }

  /**
   * Takes a rendered frame from the session's texture ring, if one is ready.
   *
   * It takes the oldest frame that you haven't acquired. The session can render
   * over a frame before you acquire it. While you hold a frame in every texture
   * of the ring, new frames wait until you release one.
   *
   * See `mln_render_session_acquire_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the session lacks
   *   `RenderSessionCapabilityFlag.frameAcquisition`.
   * - `Status.invalidState` when the session has failed.
   */
  acquireFrame(): AcquiredFrameHandle | undefined {
    const handle = native.mln_render_session_acquire_frame(this.id);
    return handle === undefined
      ? undefined
      : adopt(
          handle,
          () => new AcquiredFrameHandle(ADOPT, handle, this),
          (disposed) => native.mln_acquired_frame_discard(disposed, undefined),
        );
  }

  /**
   * Ends the session's pending frame requests and waits for its earlier work.
   *
   * Frame requests that haven't started, including those that wait for a map
   * update, finish with `RenderResult.noUpdate`.
   *
   * See `mln_render_session_barrier` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session loses its target before the barrier
   *   runs.
   */
  barrier(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_barrier(this.id, completion),
    );
  }

  /**
   * Clears the data that the session's renderer holds, such as loaded tiles,
   * and draws the map again.
   *
   * See `mln_render_session_clear_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session loses its target before the work
   *   runs.
   */
  clearData(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_clear_data(this.id, completion),
    );
  }

  /**
   * Releases the session in any state.
   *
   * An attached session driven by `RenderDriverKind.coreWorker` detaches first
   * unless you hold one of its frames. Any other session is abandoned as
   * `RenderSessionHandle.abandon()` does. To free the graphics objects of a
   * session driven by `RenderDriverKind.callerGraphicsThread`, detach it before
   * you dispose it. Once the disposal completes, the library no longer uses
   * your graphics objects.
   *
   * See `mln_render_session_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  close(): Promise<void> {
    return closeAsync(this, (handle, token) =>
      submitRun((completion) =>
        native.mln_render_session_dispose(handle, token, completion),
      ),
    );
  }

  /**
   * Detaches the session from its map and frees its graphics objects.
   *
   * Release every acquired frame first. Pending frame requests finish with
   * `RenderResult.targetNotReady`. Once the detach completes, the library no
   * longer uses your graphics objects, and you can attach another session to
   * the map. Dispose the session afterward.
   *
   * See `mln_render_session_detach` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session loses its target before the detach
   *   runs.
   */
  detach(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_detach(this.id, completion),
    );
  }

  /**
   * Releases the session in any state.
   *
   * An attached session driven by `RenderDriverKind.coreWorker` detaches first
   * unless you hold one of its frames. Any other session is abandoned as
   * `RenderSessionHandle.abandon()` does. To free the graphics objects of a
   * session driven by `RenderDriverKind.callerGraphicsThread`, detach it before
   * you dispose it. Once the disposal completes, the library no longer uses
   * your graphics objects.
   *
   * See `mln_render_session_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  dispose(): void {
    closeSync(this, (handle, token) =>
      native.mln_render_session_dispose(handle, token, undefined),
    );
  }

  /**
   * Takes the frame results that the session has queued.
   *
   * See `mln_render_session_drain_frame_results` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  drainFrameResults(): readonly RenderFrameResult[] {
    return native.mln_render_session_drain_frame_results(this.id) ?? [];
  }

  /**
   * Logs debugging information about the session's renderer.
   *
   * See `mln_render_session_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session loses its target before the work
   *   runs.
   */
  dumpDebugLogs(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_dump_debug_logs(this.id, completion),
    );
  }

  /**
   * Reads what the session supports.
   *
   * See `mln_render_session_get_capabilities` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  getCapabilities(): RenderSessionCapabilities {
    return native.mln_render_session_get_capabilities(this.id);
  }

  /**
   * Reports the error that made the session fail.
   *
   * The call fails with that error, and succeeds while the session hasn't
   * failed.
   *
   * See `mln_render_session_get_failure` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session failed because it lost its target.
   * - `Status.nativeError` when the session failed for another reason.
   */
  getFailure(): void {
    native.mln_render_session_get_failure(this.id);
  }

  /**
   * Reads the session's current state.
   *
   * See `mln_render_session_get_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  getSnapshot(): RenderSessionSnapshot {
    return native.mln_render_session_get_snapshot(this.id);
  }

  /**
   * Reads a value that a source computes for one of its features.
   *
   * A clustered GeoJSON source answers the `supercluster` extension for a
   * cluster feature. Its `extensionField` is `children` for the cluster's child
   * features, `leaves` for the points in the cluster, or `expansion-zoom` for
   * the zoom level where the cluster splits. For `leaves`, `argumentsValue` can
   * set `limit` and `offset`. Without `limit`, the result has up to 10 points
   * and ignores `offset`. The result is JSON: a GeoJSON feature collection, a
   * number, or null when the source has no answer.
   *
   * See `mln_render_session_query_feature_extensions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  queryFeatureExtensions(
    sourceId: string,
    feature: unknown,
    extension: string,
    extensionField: string,
    argumentsValue: unknown,
  ): Promise<unknown> {
    return query<unknown>((completion) =>
      native.mln_render_session_query_feature_extensions(
        this.id,
        sourceId,
        feature,
        extension,
        extensionField,
        argumentsValue,
        completion,
      ),
    );
  }

  /**
   * Finds the features that the session drew in an area of the screen.
   *
   * The query reads the last frame that the session rendered. A feature that
   * several layers draw appears once for each layer.
   *
   * See `mln_render_session_query_rendered_features` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options?: RenderedFeatureQueryOptions,
  ): Promise<readonly RenderedFeature[]> {
    return query<readonly RenderedFeature[]>((completion) =>
      native.mln_render_session_query_rendered_features(
        this.id,
        geometry,
        options,
        completion,
      ),
    );
  }

  /**
   * Finds the features of the source named `sourceId`.
   *
   * The query covers only the tiles that the source has loaded for the current
   * view. A source loads tiles only while a visible layer uses it.
   *
   * See `mln_render_session_query_source_features` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  querySourceFeatures(
    sourceId: string,
    options?: SourceFeatureQueryOptions,
  ): Promise<readonly SourceFeature[]> {
    return query<readonly SourceFeature[]>((completion) =>
      native.mln_render_session_query_source_features(
        this.id,
        sourceId,
        options,
        completion,
      ),
    );
  }

  /**
   * Reads the pixels of the last frame that the session rendered.
   *
   * See `mln_render_session_read_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the session's capabilities lack
   *   `RenderSessionCapabilityFlag.readback`.
   * - `Status.invalidState` when the session hasn't rendered a frame since it
   *   attached, resized, or changed its target.
   */
  readTexture(): Promise<TextureReadbackResult> {
    return query<TextureReadbackResult>((completion) =>
      native.mln_render_session_read_texture(this.id, completion),
    );
  }

  /**
   * Frees memory that the session's renderer can rebuild, such as caches.
   *
   * See `mln_render_session_reduce_memory_use` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.targetLost` when the session loses its target before the work
   *   runs.
   */
  reduceMemoryUse(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_reduce_memory_use(this.id, completion),
    );
  }

  /**
   * Lets go of the session's surface or textures, and keeps the rest of the
   * session.
   *
   * Use it when the platform takes a surface away, such as when your app goes
   * to the background. Release every acquired frame first. Until you give the
   * session a new target, frame requests finish with
   * `RenderResult.targetNotReady`. Once the release completes, you can destroy
   * the old target.
   *
   * See `mln_render_session_release_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the session renders into a ring that it
   *   created.
   * - `Status.unsupported` when the target is a WebGL canvas.
   * - `Status.targetLost` when the session loses its target before the release
   *   runs.
   */
  releaseTarget(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_release_target(this.id, completion),
    );
  }

  /**
   * Asks the session to render a frame.
   *
   * Call it once the attach completes. Every request finishes with one frame
   * result that carries its token, and
   * `RenderSessionHandle.drainFrameResults()` takes it. A request without
   * `FrameDemandFlag.force` replaces each pending request with the same flags.
   *
   * See `mln_render_session_request_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` when the session has failed.
   */
  requestFrame(demand: FrameDemand = {}): void {
    native.mln_render_session_request_frame(this.id, demand);
  }

  /**
   * Resizes the session's target and its map.
   *
   * `extent` must keep the session's scale factor. To change the scale factor,
   * give the session a new target or attach a new session. Release every
   * acquired frame first.
   *
   * See `mln_render_session_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the session renders into a borrowed ring. Give
   *   it new textures of the new size instead.
   * - `Status.targetLost` when the session loses its target before the resize
   *   applies.
   */
  resize(extent: LogicalExtent = {}): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_render_session_resize(this.id, extent, completion),
    );
  }

  /**
   * Runs the session's ready graphics work on your graphics thread.
   *
   * Use it with `RenderDriverKind.callerGraphicsThread`, and call it from the
   * same thread every time. Call it each time
   * `RenderSessionAttachOptions.driverWorkWake` is called, even while you
   * request no frames, and until a detach completes. It returns once no work is
   * ready.
   *
   * See `mln_render_session_service_driver_work` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  serviceDriverWork(): void {
    native.mln_render_session_service_driver_work(this.id);
  }

  /**
   * Replaces the Metal textures that a borrowed ring renders into.
   *
   * Use it to resize the ring, or to give the session textures again after
   * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first.
   * The new ring must have as many textures as the old one, and the same device
   * and command queue. Once the replacement completes, the session no longer
   * uses the old textures.
   *
   * See `mln_render_session_set_metal_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the new textures have a different pixel format
   *   than the old ones. Attach a new session to change it.
   */
  setMetalBorrowedTextureTarget(
    descriptor: MetalBorrowedTextureDescriptor,
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_metal_borrowed_texture_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Replaces the Metal layer that the session renders to.
   *
   * Use it to give the session a layer again after
   * `RenderSessionHandle.releaseTarget()`, or to move it to another layer. The
   * new layer must use the same device. Once the replacement completes, the
   * session no longer uses the old layer.
   *
   * See `mln_render_session_set_metal_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_metal_surface_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Replaces the OpenGL textures that a borrowed ring renders into.
   *
   * Use it to resize the ring, or to give the session textures again after
   * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first.
   * The new ring must have as many textures as the old one, and the same
   * context. Once the replacement completes, the session no longer uses the old
   * textures.
   *
   * See `mln_render_session_set_open_gl_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  setOpenGlBorrowedTextureTarget(
    descriptor: OpenGlBorrowedTextureDescriptor,
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_open_gl_borrowed_texture_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Replaces the OpenGL surface that the session presents to.
   *
   * Use it to give the session a surface again after
   * `RenderSessionHandle.releaseTarget()`, or to move it to another surface.
   * The new surface must use the same context. Once the replacement completes,
   * the session no longer uses the old surface.
   *
   * See `mln_render_session_set_open_gl_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  setOpenGlSurfaceTarget(descriptor: OpenGlSurfaceDescriptor): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_open_gl_surface_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Replaces the Vulkan images that a borrowed ring renders into.
   *
   * Use it to resize the ring, or to give the session images again after
   * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first.
   * The new ring must have as many images as the old one, and the same context.
   * Once the replacement completes, the session no longer uses the old images.
   *
   * See `mln_render_session_set_vulkan_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the new ring has a different `format`,
   *   `final_layout`, or `owner` than the old one. Attach a new session to
   *   change them.
   */
  setVulkanBorrowedTextureTarget(
    descriptor: VulkanBorrowedTextureDescriptor,
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_vulkan_borrowed_texture_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Replaces the Vulkan surface that the session presents to.
   *
   * Use it to give the session a surface again after
   * `RenderSessionHandle.releaseTarget()`, or to move it to another surface.
   * The new surface must use the same context. Once the replacement completes,
   * the session no longer uses the old surface.
   *
   * See `mln_render_session_set_vulkan_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the new surface needs a different color format
   *   or pre-rotation support than the old one. Attach a new session instead.
   */
  setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_vulkan_surface_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Replaces the WebGPU textures that a borrowed ring renders into.
   *
   * Use it to resize the ring, or to give the session textures again after
   * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first.
   * The new ring must have as many textures as the old one, and the same
   * context. Once the replacement completes, the session no longer uses the old
   * textures.
   *
   * See `mln_render_session_set_web_gpu_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the new ring has a different `format` than the
   *   old one. Attach a new session to change it.
   */
  setWebGpuBorrowedTextureTarget(
    descriptor: WebGpuBorrowedTextureDescriptor,
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_web_gpu_borrowed_texture_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Replaces the WebGPU surface that the session presents to.
   *
   * Use it to give the session a surface again after
   * `RenderSessionHandle.releaseTarget()`, or to move it to another surface.
   * The new surface must use the same context. Once the replacement completes,
   * the session no longer uses the old surface.
   *
   * See `mln_render_session_set_web_gpu_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the new `format` differs from the old one.
   *   Attach a new session to change it.
   */
  setWebGpuSurfaceTarget(descriptor: WebGpuSurfaceDescriptor): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_web_gpu_surface_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }
}
defineDisposers(RenderSessionHandle, "close", "dispose");

/**
 * The commands of a {@link RenderSessionHandle} that discard their outcomes:
 * each returns once native code accepts it and allocates no completion.
 * A later completion on the same map fences them.
 */
export class RenderSessionHandleDiscarding {
  readonly #owner: RenderSessionHandle;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, owner: RenderSessionHandle) {
    checkKey(key);
    this.#owner = owner;
  }

  /**
   * Resizes the session's target and its map.
   *
   * `extent` must keep the session's scale factor. To change the scale factor,
   * give the session a new target or attach a new session. Release every
   * acquired frame first.
   *
   * See `mln_render_session_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the session renders into a borrowed ring. Give
   *   it new textures of the new size instead.
   */
  resize(extent: LogicalExtent = {}): void {
    native.mln_render_session_resize(this.#owner.id, extent, undefined);
  }
}

/**
 * A resource load that a resource provider answers.
 *
 * See `mln_resource_request` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class ResourceRequestHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "ResourceRequestHandle", { registrations });
  }
  /**
   * Calls `close()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `close()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Releases a resource request.
   *
   * See `mln_resource_request_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  close(): void {
    closeSync(this, (handle, token) =>
      native.mln_resource_request_release(handle, token),
    );
  }

  /**
   * Answers a resource request.
   *
   * See `mln_resource_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` when the library cancelled the request.
   */
  complete(response: ResourceResponse = {}): void {
    answerSync(this, (handle, token) =>
      native.mln_resource_request_complete(handle, token, response),
    );
  }

  /**
   * Reads whether the library cancelled a resource request.
   *
   * See `mln_resource_request_is_cancelled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  isCancelled(): boolean {
    return native.mln_resource_request_is_cancelled(this.id);
  }

  /**
   * Hands a resource request back to the library to load as usual.
   *
   * See `mln_resource_request_pass_through` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidState` when the library cancelled the request.
   */
  passThrough(): void {
    answerSync(this, (handle, token) =>
      native.mln_resource_request_pass_through(handle, token),
    );
  }

  /**
   * Calls a callback when the library cancels a resource request.
   *
   * The callback never runs after the request is answered. A request takes one
   * cancel callback. When the library has already cancelled the request, the
   * call reports that instead and the callback never runs.
   *
   * See `mln_resource_request_set_cancel_callback` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setCancelCallback(handler: ResourceRequestCancelHandler): boolean {
    return withRegistrations((registrations) => {
      const result = native.mln_resource_request_set_cancel_callback(
        this.id,
        handler,
      );
      if (!result) registrations.commit(rootsOf(this));
      return result;
    });
  }
}
defineDisposers(ResourceRequestHandle, undefined, "close");

/**
 * A URL rewrite that a resource transform answers with the URL to load.
 *
 * See `mln_resource_transform_request` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class ResourceTransformRequestHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "ResourceTransformRequestHandle", { registrations });
  }
  /**
   * Calls `close()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `close()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Releases a resource transform request.
   *
   * See `mln_resource_transform_request_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  close(): void {
    closeSync(this, (handle, token) =>
      native.mln_resource_transform_request_release(handle, token),
    );
  }

  /**
   * Answers a resource transform request with the URL to load instead.
   *
   * `url` must not be empty.
   *
   * See `mln_resource_transform_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  complete(url: string): void {
    answerSync(this, (handle, token) =>
      native.mln_resource_transform_request_complete(handle, token, url),
    );
  }
}
defineDisposers(ResourceTransformRequestHandle, undefined, "close");

/**
 * A runtime owns maps and the network and cache settings that they share.
 *
 * See `mln_runtime` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class RuntimeHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "RuntimeHandle", { registrations, root: true });
  }

  /**
   * The native handle value, which runtime events report as their
   * source. It stays readable after close, so events drained later
   * still match this handle.
   */
  declare readonly id: bigint;

  /** The commands of this owner that discard their outcomes. */
  get discarding(): RuntimeHandleDiscarding {
    return new RuntimeHandleDiscarding(ADOPT, this);
  }
  /**
   * Calls `close()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `dispose()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Stops sending the events of the runtime and its maps to a callback.
   *
   * See `mln_runtime_clear_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearEventHandler(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_runtime_clear_event_handler(this.id, completion),
    );
  }

  /**
   * Stops adding headers to the runtime's HTTP requests.
   *
   * See `mln_runtime_clear_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearHttpHeaderTransform(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_runtime_clear_http_header_transform(this.id, completion),
    );
  }

  /**
   * Removes the runtime's resource provider, so the library loads every request
   * itself.
   *
   * See `mln_runtime_clear_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearResourceProvider(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_runtime_clear_resource_provider(this.id, completion),
    );
  }

  /**
   * Stops rewriting the URLs of the runtime's network requests.
   *
   * See `mln_runtime_clear_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearResourceTransform(): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_runtime_clear_resource_transform(this.id, completion),
    );
  }

  /**
   * Releases a runtime.
   *
   * Release the runtime's maps first.
   *
   * See `mln_runtime_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  close(): Promise<void> {
    return closeAsync(this, (handle, token) =>
      submitRun((completion) =>
        native.mln_runtime_release(handle, token, completion),
      ),
    );
  }

  /**
   * Creates a runtime.
   *
   * A runtime holds maps and the resource loading, cache, and offline regions
   * that they share. In a browser, create runtimes on a Web Worker, not on the
   * main thread.
   *
   * Resolves once the call completes, so a browser page, which cannot block,
   * can call it.
   *
   * See `mln_runtime_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  static create(options: RuntimeOptions = {}): Promise<RuntimeHandle> {
    return threadStart(
      (start) => native.mln_runtime_create(options, start),
      (handle) =>
        adopt(
          handle,
          () => new RuntimeHandle(ADOPT, handle),
          (disposed) => native.mln_runtime_dispose(disposed, undefined),
        ),
    );
  }

  /**
   * Creates a map on `runtime`.
   *
   * See `mln_runtime_create_map` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  createMap(options: MapOptions): NewMap {
    const [map, ready] = attach(
      (completion) =>
        native.mln_runtime_create_map(this.id, options, completion),
      (handle) =>
        adopt(
          handle,
          () => new MapHandle(ADOPT, handle, this),
          (disposed) => native.mln_map_dispose(disposed, undefined),
        ),
    );
    return { map, ready };
  }

  /**
   * Creates an offline region in the runtime's cache file.
   *
   * `metadata` holds any bytes to store with the region, such as its name. The
   * region doesn't download until
   * `RuntimeHandle.setOfflineRegionDownloadState()` makes its download active.
   *
   * See `mln_runtime_create_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  createOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: Uint8Array,
  ): Promise<OfflineRegionInfo> {
    return query<OfflineRegionInfo>((completion) =>
      native.mln_runtime_create_offline_region(
        this.id,
        definition,
        metadata,
        completion,
      ),
    );
  }

  /**
   * Deletes the offline region `regionId` from the runtime's cache file.
   *
   * See `mln_runtime_delete_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  deleteOfflineRegion(regionId: bigint): Promise<void> {
    return run((completion) =>
      native.mln_runtime_delete_offline_region(this.id, regionId, completion),
    );
  }

  /**
   * Releases a runtime in any state, without waiting.
   *
   * The runtime finishes its teardown after its maps are released.
   *
   * See `mln_runtime_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  dispose(): void {
    closeSync(this, (handle, token) =>
      native.mln_runtime_dispose(handle, token),
    );
  }

  /**
   * Reads the runtime's event mask.
   *
   * See `mln_runtime_get_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  getEventMask(): RuntimeEventFlag {
    return native.mln_runtime_get_event_mask(this.id);
  }

  /**
   * Reads the offline region `regionId`.
   *
   * The result is null when the cache file has no region with that ID.
   *
   * See `mln_runtime_get_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getOfflineRegion(regionId: bigint): Promise<OfflineRegionInfo | undefined> {
    return query<OfflineRegionInfo | undefined>((completion) =>
      native.mln_runtime_get_offline_region(this.id, regionId, completion),
    );
  }

  /**
   * Reads the download progress of the offline region `regionId`.
   *
   * See `mln_runtime_get_offline_region_status` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  getOfflineRegionStatus(regionId: bigint): Promise<OfflineRegionStatus> {
    return query<OfflineRegionStatus>((completion) =>
      native.mln_runtime_get_offline_region_status(
        this.id,
        regionId,
        completion,
      ),
    );
  }

  /**
   * Makes maps check each resource of the offline region `regionId` with the
   * server before they use it.
   *
   * The region keeps its resources.
   *
   * See `mln_runtime_invalidate_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  invalidateOfflineRegion(regionId: bigint): Promise<void> {
    return run((completion) =>
      native.mln_runtime_invalidate_offline_region(
        this.id,
        regionId,
        completion,
      ),
    );
  }

  /**
   * Lists the offline regions in the runtime's cache file.
   *
   * See `mln_runtime_list_offline_regions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  listOfflineRegions(): Promise<readonly OfflineRegionInfo[]> {
    return query<readonly OfflineRegionInfo[]>((completion) =>
      native.mln_runtime_list_offline_regions(this.id, completion),
    );
  }

  /**
   * Copies the offline regions of another cache file into the runtime's cache
   * file.
   *
   * A region whose definition and metadata match one that the runtime's file
   * already has isn't copied again. The result lists the runtime's copy of each
   * region in the other file.
   *
   * See `mln_runtime_merge_offline_regions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when `sideDatabasePath` isn't a readable
   *   database file.
   */
  mergeOfflineRegions(
    sideDatabasePath: string,
  ): Promise<readonly OfflineRegionInfo[]> {
    return query<readonly OfflineRegionInfo[]>((completion) =>
      native.mln_runtime_merge_offline_regions(
        this.id,
        sideDatabasePath,
        completion,
      ),
    );
  }

  /**
   * Runs an action on the runtime's ambient cache.
   *
   * See `mln_runtime_run_ambient_cache_operation` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  runAmbientCacheOperation(operation: AmbientCacheOperation): Promise<void> {
    return run((completion) =>
      native.mln_runtime_run_ambient_cache_operation(
        this.id,
        operation,
        completion,
      ),
    );
  }

  /**
   * Sends the events of the runtime and its maps to a callback.
   *
   * The callback replaces the one set before.
   *
   * See `mln_runtime_set_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setEventHandler(handler: RuntimeEventHandler): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_runtime_set_event_handler(this.id, handler, completion);
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Selects the runtime's own events that reach the event callback.
   *
   * Each map selects its events with `MapHandle.setEventMask()` instead. A
   * runtime starts with every event selected.
   *
   * See `mln_runtime_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setEventMask(mask: RuntimeEventFlag): void {
    native.mln_runtime_set_event_mask(this.id, mask);
  }

  /**
   * Adds headers to the runtime's HTTP requests.
   *
   * The transform replaces the one set before. In a browser or on OpenHarmony,
   * serve the requests with `RuntimeHandle.setResourceProvider()` instead.
   *
   * The callback runs asynchronously on the JavaScript thread that registered
   * it, after native queues the call.
   *
   * See `mln_runtime_set_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the library runs in a browser or on
   *   OpenHarmony.
   */
  setHttpHeaderTransform(
    transform: HttpHeaderTransform,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_runtime_set_http_header_transform(
          this.id,
          transform,
          completion,
        );
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Limits the size of the runtime's ambient cache.
   *
   * `size` is in bytes. Offline regions count toward it, but the library evicts
   * only ambient cache resources to stay within it.
   *
   * See `mln_runtime_set_maximum_ambient_cache_size` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setMaximumAmbientCacheSize(size: bigint): Promise<void> {
    return run((completion) =>
      native.mln_runtime_set_maximum_ambient_cache_size(
        this.id,
        size,
        completion,
      ),
    );
  }

  /**
   * Starts or pauses the download of the offline region `regionId`.
   *
   * A download stops when the runtime closes.
   *
   * See `mln_runtime_set_offline_region_download_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  setOfflineRegionDownloadState(
    regionId: bigint,
    state: OfflineRegionDownloadState,
  ): Promise<void> {
    return run((completion) =>
      native.mln_runtime_set_offline_region_download_state(
        this.id,
        regionId,
        state,
        completion,
      ),
    );
  }

  /**
   * Sets whether the runtime reports the download progress of the offline
   * region `regionId` as events.
   *
   * See `mln_runtime_set_offline_region_observed` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  setOfflineRegionObserved(regionId: bigint, observed: boolean): Promise<void> {
    return run((completion) =>
      native.mln_runtime_set_offline_region_observed(
        this.id,
        regionId,
        observed,
        completion,
      ),
    );
  }

  /**
   * Serves the runtime's network requests through a callback.
   *
   * The provider receives the network requests that its routes match, and the
   * library loads the rest as usual. A provider can serve URL schemes that the
   * library can't load itself. The provider replaces the one set before.
   *
   * See `mln_runtime_set_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setResourceProvider(provider: ResourceProvider): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_runtime_set_resource_provider(this.id, provider, completion);
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Rewrites the URLs of the runtime's network requests through a callback.
   *
   * The transform replaces the one set before.
   *
   * See `mln_runtime_set_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setResourceTransform(
    transform: ResourceTransform,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_runtime_set_resource_transform(
          this.id,
          transform,
          completion,
        );
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Replaces the metadata of the offline region `regionId`.
   *
   * See `mln_runtime_update_offline_region_metadata` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   *
   * Errors:
   *
   * - `Status.notFound` when the cache file has no region `regionId`.
   */
  updateOfflineRegionMetadata(
    regionId: bigint,
    metadata: Uint8Array,
  ): Promise<OfflineRegionInfo> {
    return query<OfflineRegionInfo>((completion) =>
      native.mln_runtime_update_offline_region_metadata(
        this.id,
        regionId,
        metadata,
        completion,
      ),
    );
  }
}
defineDisposers(RuntimeHandle, "close", "dispose");

/**
 * The commands of a {@link RuntimeHandle} that discard their outcomes:
 * each returns once native code accepts it and allocates no completion.
 * A later completion on the same map fences them.
 */
export class RuntimeHandleDiscarding {
  readonly #owner: RuntimeHandle;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, owner: RuntimeHandle) {
    checkKey(key);
    this.#owner = owner;
  }

  /**
   * Stops sending the events of the runtime and its maps to a callback.
   *
   * See `mln_runtime_clear_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearEventHandler(): void {
    native.mln_runtime_clear_event_handler(this.#owner.id, undefined);
  }

  /**
   * Stops adding headers to the runtime's HTTP requests.
   *
   * See `mln_runtime_clear_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearHttpHeaderTransform(): void {
    native.mln_runtime_clear_http_header_transform(this.#owner.id, undefined);
  }

  /**
   * Removes the runtime's resource provider, so the library loads every request
   * itself.
   *
   * See `mln_runtime_clear_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearResourceProvider(): void {
    native.mln_runtime_clear_resource_provider(this.#owner.id, undefined);
  }

  /**
   * Stops rewriting the URLs of the runtime's network requests.
   *
   * See `mln_runtime_clear_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearResourceTransform(): void {
    native.mln_runtime_clear_resource_transform(this.#owner.id, undefined);
  }

  /**
   * Sends the events of the runtime and its maps to a callback.
   *
   * The callback replaces the one set before.
   *
   * See `mln_runtime_set_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setEventHandler(handler: RuntimeEventHandler): void {
    withRegistrations((registrations) => {
      native.mln_runtime_set_event_handler(this.#owner.id, handler, undefined);
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Adds headers to the runtime's HTTP requests.
   *
   * The transform replaces the one set before. In a browser or on OpenHarmony,
   * serve the requests with `RuntimeHandle.setResourceProvider()` instead.
   *
   * The callback runs asynchronously on the JavaScript thread that registered
   * it, after native queues the call.
   *
   * See `mln_runtime_set_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   *
   * Errors:
   *
   * - `Status.unsupported` when the library runs in a browser or on
   *   OpenHarmony.
   */
  setHttpHeaderTransform(transform: HttpHeaderTransform): void {
    withRegistrations((registrations) => {
      native.mln_runtime_set_http_header_transform(
        this.#owner.id,
        transform,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Serves the runtime's network requests through a callback.
   *
   * The provider receives the network requests that its routes match, and the
   * library loads the rest as usual. A provider can serve URL schemes that the
   * library can't load itself. The provider replaces the one set before.
   *
   * See `mln_runtime_set_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setResourceProvider(provider: ResourceProvider): void {
    withRegistrations((registrations) => {
      native.mln_runtime_set_resource_provider(
        this.#owner.id,
        provider,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Rewrites the URLs of the runtime's network requests through a callback.
   *
   * The transform replaces the one set before.
   *
   * See `mln_runtime_set_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setResourceTransform(transform: ResourceTransform): void {
    withRegistrations((registrations) => {
      native.mln_runtime_set_resource_transform(
        this.#owner.id,
        transform,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }
}

/**
 * A parsed style expression.
 *
 * See `mln_style_expression` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class StyleExpressionHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "StyleExpressionHandle", { registrations });
  }
  /**
   * Calls `close()`, for `await using`.
   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.asyncDispose]: () => Promise<void>;
  /**
   * Calls `close()`, for `using`.
   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.
   */
  declare [Symbol.dispose]: () => void;

  /**
   * Releases a parsed style expression.
   *
   * See `mln_style_expression_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  close(): void {
    closeSync(this, (handle, token) =>
      native.mln_style_expression_release(handle, token),
    );
  }

  /**
   * Parses a style
   * &#91;expression&#93;(https://maplibre.org/maplibre-style-spec/expressions/)
   * to evaluate without a map.
   *
   * See `mln_style_expression_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when `expression` isn't a valid expression, or
   *   its result can't have the expected type.
   */
  static create(
    expression: unknown,
    options?: StyleExpressionOptions,
  ): StyleExpressionHandle {
    const handle = native.mln_style_expression_create(expression, options);
    return adopt(
      handle,
      () => new StyleExpressionHandle(ADOPT, handle),
      (disposed) => native.mln_style_expression_release(disposed, undefined),
    );
  }

  /**
   * Evaluates a style expression and writes its result as JSON.
   *
   * A number in the result that isn't finite becomes null.
   *
   * See `mln_style_expression_evaluate` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   *
   * Errors:
   *
   * - `Status.invalidArgument` when the expression fails to evaluate, such as
   *   when it reads an input that `inputs` doesn't set.
   */
  evaluate(inputs?: StyleExpressionInputs): unknown {
    return native.mln_style_expression_evaluate(this.id, inputs);
  }

  /**
   * Reads what a style expression's result depends on.
   *
   * See `mln_style_expression_get_info` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  getInfo(): StyleExpressionInfo {
    return native.mln_style_expression_get_info(this.id);
  }
}
defineDisposers(StyleExpressionHandle, undefined, "close");

/** A new map and the completion that reports it ready. */
export interface NewMap {
  /** The map, which the caller owns before it is ready. */
  readonly map: MapHandle;
  /**
   * Settles once the map is ready. It rejects with a `MaplibreError`
   * when it fails to become ready, so await it or attach a rejection
   * handler: an unhandled rejection ends a Node process by default.
   */
  readonly ready: Promise<void>;
}

/** A new session and the completion that reports it ready. */
export interface NewRenderSession {
  /** The session, which the caller owns before it is ready. */
  readonly session: RenderSessionHandle;
  /**
   * Settles once the session is ready. It rejects with a `MaplibreError`
   * when it fails to become ready, so await it or attach a rejection
   * handler: an unhandled rejection ends a Node process by default.
   */
  readonly ready: Promise<void>;
}

/**
 * A `GpuSync` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withProducerSync} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedGpuSync {
  readonly #value: GpuSync;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: GpuSync) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get kind(): GpuSync["kind"] {
    return this.#value.kind;
  }

  get handle(): GpuSync["handle"] {
    return this.#scope.active(this.#value).handle;
  }

  get value(): GpuSync["value"] {
    return this.#value.value;
  }
}

/**
 * A `MetalTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withMetalTexture} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedMetalTextureFrame {
  readonly #value: MetalTextureFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: MetalTextureFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get generation(): MetalTextureFrame["generation"] {
    return this.#value.generation;
  }

  get width(): MetalTextureFrame["width"] {
    return this.#value.width;
  }

  get height(): MetalTextureFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): MetalTextureFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get frameId(): MetalTextureFrame["frameId"] {
    return this.#value.frameId;
  }

  get slot(): MetalTextureFrame["slot"] {
    return this.#value.slot;
  }

  get texture(): MetalTextureFrame["texture"] {
    return this.#scope.active(this.#value).texture;
  }

  get device(): MetalTextureFrame["device"] {
    return this.#scope.active(this.#value).device;
  }

  get pixelFormat(): MetalTextureFrame["pixelFormat"] {
    return this.#value.pixelFormat;
  }
}

/**
 * A `OpenGlTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withOpenGlTexture} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedOpenGlTextureFrame {
  readonly #value: OpenGlTextureFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: OpenGlTextureFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get generation(): OpenGlTextureFrame["generation"] {
    return this.#value.generation;
  }

  get width(): OpenGlTextureFrame["width"] {
    return this.#value.width;
  }

  get height(): OpenGlTextureFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): OpenGlTextureFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get frameId(): OpenGlTextureFrame["frameId"] {
    return this.#value.frameId;
  }

  get slot(): OpenGlTextureFrame["slot"] {
    return this.#value.slot;
  }

  get texture(): OpenGlTextureFrame["texture"] {
    return this.#scope.active(this.#value).texture;
  }

  get target(): OpenGlTextureFrame["target"] {
    return this.#value.target;
  }

  get internalFormat(): OpenGlTextureFrame["internalFormat"] {
    return this.#value.internalFormat;
  }

  get format(): OpenGlTextureFrame["format"] {
    return this.#value.format;
  }

  get type(): OpenGlTextureFrame["type"] {
    return this.#value.type;
  }
}

/**
 * A `VulkanTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withVulkanTexture} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedVulkanTextureFrame {
  readonly #value: VulkanTextureFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: VulkanTextureFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get generation(): VulkanTextureFrame["generation"] {
    return this.#value.generation;
  }

  get width(): VulkanTextureFrame["width"] {
    return this.#value.width;
  }

  get height(): VulkanTextureFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): VulkanTextureFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get frameId(): VulkanTextureFrame["frameId"] {
    return this.#value.frameId;
  }

  get slot(): VulkanTextureFrame["slot"] {
    return this.#value.slot;
  }

  get image(): VulkanTextureFrame["image"] {
    return this.#scope.active(this.#value).image;
  }

  get imageView(): VulkanTextureFrame["imageView"] {
    return this.#scope.active(this.#value).imageView;
  }

  get device(): VulkanTextureFrame["device"] {
    return this.#scope.active(this.#value).device;
  }

  get format(): VulkanTextureFrame["format"] {
    return this.#value.format;
  }

  get layout(): VulkanTextureFrame["layout"] {
    return this.#value.layout;
  }
}

/**
 * A `WebGpuTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withWebGpuTexture} runs. Its other members stay
 * readable after the callback returns.
 */
export class ScopedWebGpuTextureFrame {
  readonly #value: WebGpuTextureFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: WebGpuTextureFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get generation(): WebGpuTextureFrame["generation"] {
    return this.#value.generation;
  }

  get width(): WebGpuTextureFrame["width"] {
    return this.#value.width;
  }

  get height(): WebGpuTextureFrame["height"] {
    return this.#value.height;
  }

  get scaleFactor(): WebGpuTextureFrame["scaleFactor"] {
    return this.#value.scaleFactor;
  }

  get frameId(): WebGpuTextureFrame["frameId"] {
    return this.#value.frameId;
  }

  get slot(): WebGpuTextureFrame["slot"] {
    return this.#value.slot;
  }

  get texture(): WebGpuTextureFrame["texture"] {
    return this.#scope.active(this.#value).texture;
  }

  get textureView(): WebGpuTextureFrame["textureView"] {
    return this.#scope.active(this.#value).textureView;
  }

  get device(): WebGpuTextureFrame["device"] {
    return this.#scope.active(this.#value).device;
  }

  get format(): WebGpuTextureFrame["format"] {
    return this.#value.format;
  }
}

// How a dispatched call reaches the host value that its registration
// rooted, by the callback type that the addon's trampoline numbers.
defineCallbacks([
  // mln_camera_transition_end_callback
  (target, args) => {
    const callback = target as CameraTransitionHandler;
    const [end] = args as Parameters<CameraTransitionHandler>;
    callback(end);
  },
  // mln_custom_source_cancel_tile_callback
  (target, args) => {
    const callback = (target as CustomGeometrySourceOptions)
      .cancelTile as NonNullable<CustomGeometrySourceOptions["cancelTile"]>;
    const [tileId] = args as Parameters<
      NonNullable<CustomGeometrySourceOptions["cancelTile"]>
    >;
    callback(tileId);
  },
  // mln_custom_source_fetch_tile_callback
  (target, args) => {
    const callback = (target as CustomGeometrySourceOptions).fetchTile;
    const [tileId] = args as Parameters<
      NonNullable<CustomGeometrySourceOptions["fetchTile"]>
    >;
    callback(tileId);
  },
  // mln_log_callback
  (target, args) => {
    const callback = target as LogHandler;
    const [severity, event, code, message] = args as Parameters<LogHandler>;
    callback(severity, event, code, message);
  },
  // mln_resource_provider_callback
  (target, args) => {
    const [request, handleValue] = args as Parameters<
      NonNullable<ResourceProvider["callback"]>
    >;
    const owner = adopt(
      handleValue as unknown as bigint,
      () => new ResourceRequestHandle(ADOPT, handleValue as unknown as bigint),
      (disposed) => native.mln_resource_request_release(disposed, undefined),
    );
    try {
      const callback = (target as ResourceProvider).callback;
      callback(request, owner);
    } catch (error) {
      owner.close();
      throw error;
    }
  },
  // mln_resource_request_cancel_callback
  (target, _args) => {
    const callback = target as ResourceRequestCancelHandler;
    callback();
  },
  // mln_resource_transform_callback
  (target, args) => {
    const [kind, url, request] = args as Parameters<ResourceTransform>;
    const owner = adopt(
      request as unknown as bigint,
      () =>
        new ResourceTransformRequestHandle(ADOPT, request as unknown as bigint),
      (disposed) =>
        native.mln_resource_transform_request_release(disposed, undefined),
    );
    try {
      const callback = target as ResourceTransform;
      callback(kind, url, owner);
    } catch (error) {
      owner.close();
      throw error;
    }
  },
  // mln_runtime_event_callback
  (target, args) => {
    const callback = target as RuntimeEventHandler;
    const [events] = args as Parameters<RuntimeEventHandler>;
    callback(events);
  },
  // mln_wake_callback
  (target, _args) => {
    const callback = target as () => void;
    callback();
  },
]);
