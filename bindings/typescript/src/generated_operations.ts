// Generated from the C headers by tools/bindgen. Do not edit.

import { Decoder } from "./generated_native.ts";
import type {
  AmbientCacheOperation,
  AnimationOptions,
  BoundOptions,
  CameraDelta,
  CameraFitOptions,
  CameraOptions,
  CameraQueryResult,
  CameraTransitionHandler,
  CameraUpdate,
  CanonicalTileId,
  CustomGeometrySourceOptions,
  CustomMvtVectorSourceOptions,
  FeatureStateSelector,
  FrameDemand,
  FreeCameraOptions,
  GeojsonSourceOptions,
  GpuSync,
  HttpHeaderRule,
  HttpHeaderTransform,
  LatLng,
  LatLngBounds,
  LogHandler,
  LogSeverityMask,
  LogicalExtent,
  MapDebugOption,
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
  OpenglBorrowedTextureDescriptor,
  OpenglContextProviderFlag,
  OpenglOwnedTextureDescriptor,
  OpenglSurfaceDescriptor,
  OpenglTextureFrame,
  PremultipliedRgba8Image,
  ProjectedMeters,
  ProjectionMode,
  RenderAbandonResult,
  RenderBackendFlag,
  RenderFrameBatchView,
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
  ResourceRoute,
  ResourceTransform,
  RuntimeEventHandler,
  RuntimeEventMask,
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
  TextureReadbackResult,
  VulkanBorrowedTextureDescriptor,
  VulkanOwnedTextureDescriptor,
  VulkanSurfaceDescriptor,
  VulkanTextureFrame,
  WebgpuBorrowedTextureDescriptor,
  WebgpuOwnedTextureDescriptor,
  WebgpuSurfaceDescriptor,
  WebgpuTextureFrame,
} from "./generated_values.ts";
import { native } from "./internal/addon.ts";
import {
  type CommandCompletion,
  attach,
  command,
  query,
  queryOwned,
  run,
  submitRun,
} from "./internal/completions.ts";
import {
  ADOPT,
  type AdoptKey,
  Owner,
  ViewScope,
  adopt,
  checkKey,
  closeAsync,
  closeSync,
  defineDisposers,
  rootsOf,
  takeDelivered,
} from "./internal/owners.ts";
import {
  type Registrations,
  defineDeliveries,
  withRegistrations,
} from "./internal/realm.ts";

/**
 * Initializes Android platform services.
 *
 * See `mln_android_init` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
 */
export function androidInit(jniEnv: bigint, context: bigint): void {
  native.mln_android_init(jniEnv, context);
}

/**
 * Returns empty animation options initialized for this C API version.
 *
 * See `mln_animation_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
export function animationOptionsDefault(): AnimationOptions {
  return native.mln_animation_options_default();
}

/**
 * Returns empty map bound options initialized for this C API version.
 *
 * See `mln_bound_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
export function boundOptionsDefault(): BoundOptions {
  return native.mln_bound_options_default();
}

/**
 * Reports the C ABI contract version. The value is 0 while the ABI is unstable,
 * and will increment on each SemVer major release.
 *
 * See `mln_c_version` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export function cVersion(): number {
  return native.mln_c_version();
}

/**
 * Returns an empty relative camera update initialized for this API version.
 *
 * See `mln_camera_delta_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
export function cameraDeltaDefault(): CameraDelta {
  return native.mln_camera_delta_default();
}

/**
 * Returns empty camera fitting options initialized for this C API version.
 *
 * See `mln_camera_fit_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
export function cameraFitOptionsDefault(): CameraFitOptions {
  return native.mln_camera_fit_options_default();
}

/**
 * Returns empty camera options initialized for this C API version.
 *
 * See `mln_camera_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
export function cameraOptionsDefault(): CameraOptions {
  return native.mln_camera_options_default();
}

/**
 * Returns an empty atomic camera update initialized for this API version.
 *
 * See `mln_camera_update_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
export function cameraUpdateDefault(): CameraUpdate {
  return native.mln_camera_update_default();
}

/**
 * Returns default custom geometry source options.
 *
 * See `mln_custom_geometry_source_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export function customGeometrySourceOptionsDefault(): CustomGeometrySourceOptions {
  return native.mln_custom_geometry_source_options_default();
}

/**
 * Returns default custom MVT vector source options.
 *
 * See `mln_custom_mvt_vector_source_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export function customMvtVectorSourceOptionsDefault(): CustomMvtVectorSourceOptions {
  return native.mln_custom_mvt_vector_source_options_default();
}

/**
 * Returns a zero-token, render-if-needed, nonpresenting frame demand.
 *
 * See `mln_frame_demand_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export function frameDemandDefault(): FrameDemand {
  return native.mln_frame_demand_default();
}

/**
 * Returns empty free camera options initialized for this C API version.
 *
 * See `mln_free_camera_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
export function freeCameraOptionsDefault(): FreeCameraOptions {
  return native.mln_free_camera_options_default();
}

/**
 * Prepares GeoJSON source data for installation on a map.
 *
 * See `mln_geojson_source_data_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export function geojsonSourceDataCreate(
  data: Uint8Array,
  options?: GeojsonSourceOptions,
): GeojsonSourceDataHandle {
  const handle = native.mln_geojson_source_data_create(data, options);
  return adopt(
    handle,
    () => new GeojsonSourceDataHandle(ADOPT, handle),
    (disposed) => native.mln_geojson_source_data_release(disposed, undefined),
  );
}

/**
 * Returns default GeoJSON source options.
 *
 * See `mln_geojson_source_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export function geojsonSourceOptionsDefault(): GeojsonSourceOptions {
  return native.mln_geojson_source_options_default();
}

/**
 * Returns CPU-complete synchronization for this C API version.
 *
 * See `mln_gpu_sync_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export function gpuSyncDefault(): GpuSync {
  return native.mln_gpu_sync_default();
}

/**
 * Returns an exact-match header rule for every resource kind, with no headers.
 *
 * See `mln_http_header_rule_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export function httpHeaderRuleDefault(): HttpHeaderRule {
  return native.mln_http_header_rule_default();
}

/**
 * Converts spherical Mercator projected meters to a geographic coordinate.
 *
 * See `mln_lat_lng_for_projected_meters` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
export function latLngForProjectedMeters(meters: ProjectedMeters = {}): LatLng {
  return native.mln_lat_lng_for_projected_meters(meters);
}

/**
 * Clears the process-global log callback.
 *
 * See `mln_log_clear_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export function logClearCallback(): void {
  native.mln_log_clear_callback();
}

/**
 * Controls which log severities MapLibre Native may dispatch asynchronously.
 *
 * See `mln_log_set_async_severity_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export function logSetAsyncSeverityMask(mask: LogSeverityMask): void {
  native.mln_log_set_async_severity_mask(mask);
}

/**
 * Installs a process-global MapLibre Native log callback.
 *
 * See `mln_log_set_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export function logSetCallback(handler: LogHandler): void {
  return withRegistrations((registrations) => {
    native.mln_log_set_callback(handler);
    registrations.commit();
  });
}

/**
 * Returns a 256 by 256 logical extent with one device pixel per UI pixel.
 *
 * See `mln_logical_extent_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export function logicalExtentDefault(): LogicalExtent {
  return native.mln_logical_extent_default();
}

/**
 * Computes the physical device-pixel size of a logical extent.
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
 * Returns map options initialized for this C API version.
 *
 * See `mln_map_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export function mapOptionsDefault(): MapOptions {
  return native.mln_map_options_default();
}

/**
 * Creates a standalone projection of a view, such as the view of a rendered
 * frame or of a map snapshot.
 *
 * See `mln_map_projection_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
export function mapProjectionCreate(view: MapView = {}): MapProjectionHandle {
  const handle = native.mln_map_projection_create(view);
  return adopt(
    handle,
    () => new MapProjectionHandle(ADOPT, handle),
    (disposed) => native.mln_map_projection_release(disposed, undefined),
  );
}

/**
 * Returns empty tile tuning options initialized for this C API version.
 *
 * See `mln_map_tile_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
export function mapTileOptionsDefault(): MapTileOptions {
  return native.mln_map_tile_options_default();
}

/**
 * Returns empty viewport options initialized for this C API version.
 *
 * See `mln_map_viewport_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
export function mapViewportOptionsDefault(): MapViewportOptions {
  return native.mln_map_viewport_options_default();
}

/**
 * Returns Metal borrowed-texture descriptor defaults for this C API version.
 *
 * See `mln_metal_borrowed_texture_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export function metalBorrowedTextureDescriptorDefault(): MetalBorrowedTextureDescriptor {
  return native.mln_metal_borrowed_texture_descriptor_default();
}

/**
 * Returns Metal owned-texture descriptor defaults for this C API version.
 *
 * See `mln_metal_owned_texture_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export function metalOwnedTextureDescriptorDefault(): MetalOwnedTextureDescriptor {
  return native.mln_metal_owned_texture_descriptor_default();
}

/**
 * Returns Metal surface descriptor defaults for this C API version.
 *
 * See `mln_metal_surface_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export function metalSurfaceDescriptorDefault(): MetalSurfaceDescriptor {
  return native.mln_metal_surface_descriptor_default();
}

/**
 * Reads MapLibre Native's process-global network status.
 *
 * See `mln_network_get_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export function networkGetStatus(): NetworkStatus {
  return native.mln_network_get_status();
}

/**
 * Sets MapLibre Native's process-global network status.
 *
 * See `mln_network_set_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export function networkSetStatus(status: NetworkStatus): void {
  native.mln_network_set_status(status);
}

/**
 * Returns OpenGL borrowed-texture descriptor defaults for this C API version.
 *
 * See `mln_opengl_borrowed_texture_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export function openglBorrowedTextureDescriptorDefault(): OpenglBorrowedTextureDescriptor {
  return native.mln_opengl_borrowed_texture_descriptor_default();
}

/**
 * Returns OpenGL owned-texture descriptor defaults for this C API version.
 *
 * See `mln_opengl_owned_texture_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export function openglOwnedTextureDescriptorDefault(): OpenglOwnedTextureDescriptor {
  return native.mln_opengl_owned_texture_descriptor_default();
}

/**
 * Returns OpenGL context providers supported by this build.
 *
 * See `mln_opengl_supported_context_provider_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export function openglSupportedContextProviderMask(): OpenglContextProviderFlag {
  return native.mln_opengl_supported_context_provider_mask();
}

/**
 * Returns OpenGL surface descriptor defaults for this C API version.
 *
 * See `mln_opengl_surface_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export function openglSurfaceDescriptorDefault(): OpenglSurfaceDescriptor {
  return native.mln_opengl_surface_descriptor_default();
}

/**
 * Returns the process-wide `mln_plugin_register_v1` entry point; never null.
 *
 * See `mln_plugin_get_register_function_v1` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
 */
export function pluginGetRegisterFunctionV1(): bigint {
  return native.mln_plugin_get_register_function_v1();
}

/**
 * Returns a default premultiplied RGBA8 image descriptor.
 *
 * See `mln_premultiplied_rgba8_image_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export function premultipliedRgba8ImageDefault(): PremultipliedRgba8Image {
  return native.mln_premultiplied_rgba8_image_default();
}

/**
 * Converts a geographic coordinate to spherical Mercator projected meters.
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
 * Returns empty axonometric rendering options initialized for this C API
 * version.
 *
 * See `mln_projection_mode_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
export function projectionModeDefault(): ProjectionMode {
  return native.mln_projection_mode_default();
}

/**
 * Returns attachment options initialized for this C API version, with each
 * field at its documented default.
 *
 * See `mln_render_session_attach_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export function renderSessionAttachOptionsDefault(): RenderSessionAttachOptions {
  return native.mln_render_session_attach_options_default();
}

/**
 * Returns default rendered feature query options.
 *
 * See `mln_rendered_feature_query_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export function renderedFeatureQueryOptionsDefault(): RenderedFeatureQueryOptions {
  return native.mln_rendered_feature_query_options_default();
}

/**
 * Returns an exact-match route for every resource kind.
 *
 * See `mln_resource_route_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export function resourceRouteDefault(): ResourceRoute {
  return native.mln_resource_route_default();
}

/**
 * Creates a runtime.
 *
 * See `mln_runtime_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export function runtimeCreate(options: RuntimeOptions = {}): RuntimeHandle {
  const handle = native.mln_runtime_create(options);
  return adopt(
    handle,
    () => new RuntimeHandle(ADOPT, handle),
    (disposed) => native.mln_runtime_dispose(disposed, undefined),
  );
}

/**
 * Returns runtime options initialized for this C API version.
 *
 * See `mln_runtime_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export function runtimeOptionsDefault(): RuntimeOptions {
  return native.mln_runtime_options_default();
}

/**
 * Returns default source feature query options.
 *
 * See `mln_source_feature_query_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export function sourceFeatureQueryOptionsDefault(): SourceFeatureQueryOptions {
  return native.mln_source_feature_query_options_default();
}

/**
 * Parses a style expression.
 *
 * See `mln_style_expression_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export function styleExpressionCreate(
  expression: string,
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
 * Returns default style expression inputs, which set no input.
 *
 * See `mln_style_expression_inputs_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export function styleExpressionInputsDefault(): StyleExpressionInputs {
  return native.mln_style_expression_inputs_default();
}

/**
 * Returns default style expression options.
 *
 * See `mln_style_expression_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export function styleExpressionOptionsDefault(): StyleExpressionOptions {
  return native.mln_style_expression_options_default();
}

/**
 * Returns default runtime style image options.
 *
 * See `mln_style_image_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export function styleImageOptionsDefault(): StyleImageOptions {
  return native.mln_style_image_options_default();
}

/**
 * Returns default global style transition options.
 *
 * See `mln_style_transition_options_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export function styleTransitionOptionsDefault(): StyleTransitionOptions {
  return native.mln_style_transition_options_default();
}

/**
 * Reports the render backends available in this native library build.
 *
 * See `mln_supported_render_backend_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export function supportedRenderBackendMask(): RenderBackendFlag {
  return native.mln_supported_render_backend_mask();
}

/**
 * Returns Vulkan borrowed-texture descriptor defaults for this C API version.
 *
 * See `mln_vulkan_borrowed_texture_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export function vulkanBorrowedTextureDescriptorDefault(): VulkanBorrowedTextureDescriptor {
  return native.mln_vulkan_borrowed_texture_descriptor_default();
}

/**
 * Returns Vulkan owned-texture descriptor defaults for this C API version.
 *
 * See `mln_vulkan_owned_texture_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export function vulkanOwnedTextureDescriptorDefault(): VulkanOwnedTextureDescriptor {
  return native.mln_vulkan_owned_texture_descriptor_default();
}

/**
 * Returns Vulkan surface descriptor defaults for this C API version.
 *
 * See `mln_vulkan_surface_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export function vulkanSurfaceDescriptorDefault(): VulkanSurfaceDescriptor {
  return native.mln_vulkan_surface_descriptor_default();
}

/**
 * Returns WebGPU borrowed-texture descriptor defaults for this C API version.
 *
 * See `mln_webgpu_borrowed_texture_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export function webgpuBorrowedTextureDescriptorDefault(): WebgpuBorrowedTextureDescriptor {
  return native.mln_webgpu_borrowed_texture_descriptor_default();
}

/**
 * Returns WebGPU owned-texture descriptor defaults for this C API version.
 *
 * See `mln_webgpu_owned_texture_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export function webgpuOwnedTextureDescriptorDefault(): WebgpuOwnedTextureDescriptor {
  return native.mln_webgpu_owned_texture_descriptor_default();
}

/**
 * Returns WebGPU surface descriptor defaults for this C API version.
 *
 * See `mln_webgpu_surface_descriptor_default` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export function webgpuSurfaceDescriptorDefault(): WebgpuSurfaceDescriptor {
  return native.mln_webgpu_surface_descriptor_default();
}

/**
 * A rendered frame that a render session lends until its release.
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
   * Consumes an acquired frame without consumer GPU synchronization, so it
   * quarantines the frame's slot of the texture ring.
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
   * Copies common metadata for an acquired frame.
   *
   * See `mln_acquired_frame_get_result` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  getResult(): RenderFrameResult {
    return native.mln_acquired_frame_get_result(this.id);
  }

  /**
   * Releases an acquired frame after the host's GPU work that reads it.
   *
   * See `mln_acquired_frame_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  release(consumerCompletion?: GpuSync): void {
    closeSync(this, (handle, token) =>
      native.mln_acquired_frame_release(handle, token, consumerCompletion),
    );
  }

  /**
   * Copies Metal-native metadata from an acquired frame.
   *
   * See `mln_acquired_frame_get_metal_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
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
   * Copies OpenGL-native metadata from an acquired frame.
   *
   * See `mln_acquired_frame_get_opengl_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  withOpenglTexture<T>(use: (view: ScopedOpenglTextureFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedOpenglTextureFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_opengl_texture(this.id),
        ),
      ),
    );
  }

  /**
   * Copies the producer synchronization for an acquired texture frame.
   *
   * See `mln_acquired_frame_get_producer_sync` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
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
   * Copies Vulkan-native metadata from an acquired frame.
   *
   * See `mln_acquired_frame_get_vulkan_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
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
   * Copies WebGPU-native metadata from an acquired frame.
   *
   * See `mln_acquired_frame_get_webgpu_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  withWebgpuTexture<T>(use: (view: ScopedWebgpuTextureFrame) => T): T {
    return new ViewScope(
      ADOPT,
      this,
      (handle) => native.mln_acquired_frame_view_begin(handle),
      (scope) => native.mln_acquired_frame_view_end(scope),
    ).use((scope) =>
      use(
        new ScopedWebgpuTextureFrame(
          ADOPT,
          scope,
          native.mln_acquired_frame_get_webgpu_texture(this.id),
        ),
      ),
    );
  }
}
defineDisposers(AcquiredFrameHandle, undefined, undefined);

/**
 * Prepared GeoJSON data that sources retain independently of its handle.
 *
 * See `mln_geojson_source_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class GeojsonSourceDataHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "GeojsonSourceDataHandle", { registrations });
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
   * Releases prepared GeoJSON source data.
   *
   * See `mln_geojson_source_data_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  close(): void {
    closeSync(this, (handle, token) =>
      native.mln_geojson_source_data_release(handle, token),
    );
  }
}
defineDisposers(GeojsonSourceDataHandle, undefined, "close");

/**
 * A map, which holds map state independent of any render target.
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
   * Adds a custom geometry source.
   *
   * See `mln_map_add_custom_geometry_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addCustomGeometrySource(
    sourceId: string,
    options?: CustomGeometrySourceOptions,
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
   * Adds a custom MVT vector source.
   *
   * See `mln_map_add_custom_mvt_vector_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addCustomMvtVectorSource(
    sourceId: string,
    options?: CustomMvtVectorSourceOptions,
  ): Promise<CommandCompletion> {
    return withRegistrations((registrations) =>
      command((completion) => {
        native.mln_map_add_custom_mvt_vector_source(
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
   * Adds a GeoJSON source with prepared inline data.
   *
   * See `mln_map_add_geojson_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addGeojsonSourceData(
    sourceId: string,
    data: GeojsonSourceDataHandle,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_geojson_source_data(
        this.id,
        sourceId,
        data.id,
        completion,
      ),
    );
  }

  /**
   * Adds a GeoJSON source with URL data.
   *
   * See `mln_map_add_geojson_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addGeojsonSourceUrl(
    sourceId: string,
    url: string,
    options?: GeojsonSourceOptions,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_add_geojson_source_url(
        this.id,
        sourceId,
        url,
        options,
        completion,
      ),
    );
  }

  /**
   * Adds an image source with inline image pixels.
   *
   * See `mln_map_add_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addImageSourceImage(
    sourceId: string,
    coordinates: readonly LatLng[],
    image: PremultipliedRgba8Image = {},
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
   * Adds an image source that loads its image from a URL.
   *
   * See `mln_map_add_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
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
   * Adds one style layer from a full style-spec layer JSON object.
   *
   * See `mln_map_add_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addStyleLayerJson(
    layerJson: string,
    beforeLayerId?: string,
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
   * Adds one style source from a style-spec source JSON object.
   *
   * See `mln_map_add_style_source_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addStyleSourceJson(
    sourceId: string,
    sourceJson: string,
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
   * Submits one atomic relative camera update.
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
   * Starts attachment of a ring of caller-owned Metal textures.
   *
   * See `mln_map_attach_metal_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
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
      return { session, completed };
    });
  }

  /**
   * Starts attachment of a session-owned Metal texture ring.
   *
   * See `mln_map_attach_metal_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
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
      return { session, completed };
    });
  }

  /**
   * Starts attachment of a Metal surface target.
   *
   * See `mln_map_attach_metal_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  attachMetalSurface(
    descriptor: MetalSurfaceDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
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
      return { session, completed };
    });
  }

  /**
   * Starts attachment of a ring of caller-owned OpenGL textures.
   *
   * See `mln_map_attach_opengl_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  attachOpenglBorrowedTexture(
    descriptor: OpenglBorrowedTextureDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
        (completion) =>
          native.mln_map_attach_opengl_borrowed_texture(
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
      return { session, completed };
    });
  }

  /**
   * Starts attachment of a session-owned OpenGL texture ring.
   *
   * See `mln_map_attach_opengl_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  attachOpenglOwnedTexture(
    descriptor: OpenglOwnedTextureDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
        (completion) =>
          native.mln_map_attach_opengl_owned_texture(
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
      return { session, completed };
    });
  }

  /**
   * Starts attachment of an OpenGL surface target.
   *
   * See `mln_map_attach_opengl_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  attachOpenglSurface(
    descriptor: OpenglSurfaceDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
        (completion) =>
          native.mln_map_attach_opengl_surface(
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
      return { session, completed };
    });
  }

  /**
   * Starts attachment of a ring of caller-owned Vulkan images.
   *
   * See `mln_map_attach_vulkan_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
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
      return { session, completed };
    });
  }

  /**
   * Starts attachment of a session-owned Vulkan texture ring.
   *
   * See `mln_map_attach_vulkan_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
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
      return { session, completed };
    });
  }

  /**
   * Starts attachment of a Vulkan surface target.
   *
   * See `mln_map_attach_vulkan_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  attachVulkanSurface(
    descriptor: VulkanSurfaceDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
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
      return { session, completed };
    });
  }

  /**
   * Starts attachment of a ring of caller-owned WebGPU textures.
   *
   * See `mln_map_attach_webgpu_borrowed_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  attachWebgpuBorrowedTexture(
    descriptor: WebgpuBorrowedTextureDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
        (completion) =>
          native.mln_map_attach_webgpu_borrowed_texture(
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
      return { session, completed };
    });
  }

  /**
   * Starts attachment of a session-owned WebGPU texture ring.
   *
   * See `mln_map_attach_webgpu_owned_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  attachWebgpuOwnedTexture(
    descriptor: WebgpuOwnedTextureDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
        (completion) =>
          native.mln_map_attach_webgpu_owned_texture(
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
      return { session, completed };
    });
  }

  /**
   * Starts attachment of a WebGPU surface target.
   *
   * See `mln_map_attach_webgpu_surface` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  attachWebgpuSurface(
    descriptor: WebgpuSurfaceDescriptor = {},
    options: RenderSessionAttachOptions = {},
  ): RenderSessionAttachment {
    return withRegistrations((registrations) => {
      const [session, completed] = attach(
        (completion) =>
          native.mln_map_attach_webgpu_surface(
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
      return { session, completed };
    });
  }

  /**
   * Begins a command group, which holds this map's render updates until the
   * group ends.
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
   * Starts an ordered query for a camera that fits a GeoJSON geometry.
   *
   * See `mln_map_camera_for_geometry` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cameraForGeometry(
    geometry: string,
    fitOptions?: CameraFitOptions,
  ): Promise<CameraOptions> {
    return query<CameraOptions>(Decoder.cameraOptions, (completion) =>
      native.mln_map_camera_for_geometry(
        this.id,
        geometry,
        fitOptions,
        completion,
      ),
    );
  }

  /**
   * Starts an ordered query for a camera that fits geographic bounds.
   *
   * See `mln_map_camera_for_lat_lng_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cameraForLatLngBounds(
    bounds: LatLngBounds = {},
    fitOptions?: CameraFitOptions,
  ): Promise<CameraOptions> {
    return query<CameraOptions>(Decoder.cameraOptions, (completion) =>
      native.mln_map_camera_for_lat_lng_bounds(
        this.id,
        bounds,
        fitOptions,
        completion,
      ),
    );
  }

  /**
   * Starts an ordered query for a camera that fits geographic coordinates.
   *
   * See `mln_map_camera_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cameraForLatLngs(
    coordinates: readonly LatLng[],
    fitOptions?: CameraFitOptions,
  ): Promise<CameraOptions> {
    return query<CameraOptions>(Decoder.cameraOptions, (completion) =>
      native.mln_map_camera_for_lat_lngs(
        this.id,
        coordinates,
        fitOptions,
        completion,
      ),
    );
  }

  /**
   * Cancels the camera transitions of the commands whose animation carried
   * transition_id, and leaves every other transition running.
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
   * Cancels the camera transitions running when this command commits.
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
   * Releases a map asynchronously.
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
   * Consumes a map handle without observing its asynchronous retirement.
   *
   * See `mln_map_dispose` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  dispose(): void {
    closeSync(this, (handle, token) => native.mln_map_dispose(handle, token));
  }

  /**
   * Submits an ordered debug-log command.
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
   * Ends the innermost command group that `mln_map_begin_command_group()`
   * began.
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
   * Starts an ordered camera read.
   *
   * See `mln_map_get_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  getCamera(): Promise<CameraQueryResult> {
    return query<CameraQueryResult>(Decoder.cameraQueryResult, (completion) =>
      native.mln_map_get_camera(this.id, completion),
    );
  }

  /**
   * Starts an ordered read of per-feature state from this map.
   *
   * See `mln_map_get_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getFeatureState(selector: FeatureStateSelector): Promise<string> {
    return query<string>(Decoder.bufferView, (completion) =>
      native.mln_map_get_feature_state(this.id, selector, completion),
    );
  }

  /**
   * Queries the global-state JSON object, including style defaults. Completion
   * borrows one `mln_buffer_view` for the duration of the callback.
   *
   * See `mln_map_get_global_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getGlobalState(): Promise<string> {
    return query<string>(Decoder.bufferView, (completion) =>
      native.mln_map_get_global_state(this.id, completion),
    );
  }

  /**
   * Copies image source coordinates.
   *
   * See `mln_map_get_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getImageSourceCoordinates(sourceId: string): Promise<readonly LatLng[]> {
    return query<readonly LatLng[]>(Decoder.latLngList, (completion) =>
      native.mln_map_get_image_source_coordinates(
        this.id,
        sourceId,
        completion,
      ),
    );
  }

  /**
   * Starts an ordered copy of the last successfully parsed style document.
   *
   * See `mln_map_get_loaded_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getLoadedStyleJson(): Promise<string> {
    return query<string>(Decoder.bufferView, (completion) =>
      native.mln_map_get_loaded_style_json(this.id, completion),
    );
  }

  /**
   * Copies the map's latest published snapshot.
   *
   * See `mln_map_get_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getSnapshot(): MapSnapshot {
    return native.mln_map_get_snapshot(this.id);
  }

  /**
   * Copies one complete runtime style image.
   *
   * See `mln_map_get_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleImage(imageId: string): Promise<StyleImageInfo | undefined> {
    return query<StyleImageInfo | undefined>(
      Decoder.styleImageInfoOptional,
      (completion) =>
        native.mln_map_get_style_image(this.id, imageId, completion),
    );
  }

  /**
   * Copies the complete metadata of one style layer.
   *
   * See `mln_map_get_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleLayer(layerId: string): Promise<StyleLayerInfo | undefined> {
    return query<StyleLayerInfo | undefined>(
      Decoder.styleLayerInfoOptional,
      (completion) =>
        native.mln_map_get_style_layer(this.id, layerId, completion),
    );
  }

  /**
   * Serializes one style layer as a full style-spec layer JSON object.
   *
   * See `mln_map_get_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleLayerJson(layerId: string): Promise<string | undefined> {
    return query<string | undefined>(Decoder.bufferViewOptional, (completion) =>
      native.mln_map_get_style_layer_json(this.id, layerId, completion),
    );
  }

  /**
   * Serializes one layer property as a style-spec JSON value.
   *
   * See `mln_map_get_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleLayerProperty(
    layerId: string,
    propertyName: string,
  ): Promise<string | undefined> {
    return query<string | undefined>(Decoder.bufferViewOptional, (completion) =>
      native.mln_map_get_style_layer_property(
        this.id,
        layerId,
        propertyName,
        completion,
      ),
    );
  }

  /**
   * Serializes one style light property as a style-spec JSON value.
   *
   * See `mln_map_get_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleLightProperty(propertyName: string): Promise<string | undefined> {
    return query<string | undefined>(Decoder.bufferViewOptional, (completion) =>
      native.mln_map_get_style_light_property(
        this.id,
        propertyName,
        completion,
      ),
    );
  }

  /**
   * Copies the complete metadata of one style source.
   *
   * See `mln_map_get_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleSource(sourceId: string): Promise<StyleSourceInfo | undefined> {
    return query<StyleSourceInfo | undefined>(
      Decoder.styleSourceInfoOptional,
      (completion) =>
        native.mln_map_get_style_source(this.id, sourceId, completion),
    );
  }

  /**
   * Reads the style's global transition options.
   *
   * See `mln_map_get_style_transition_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  getStyleTransitionOptions(): Promise<StyleTransitionOptions> {
    return query<StyleTransitionOptions>(
      Decoder.styleTransitionOptions,
      (completion) =>
        native.mln_map_get_style_transition_options(this.id, completion),
    );
  }

  /**
   * Starts an ordered copy of the last requested style URL.
   *
   * See `mln_map_get_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getStyleUrl(): Promise<string> {
    return query<string>(Decoder.bufferView, (completion) =>
      native.mln_map_get_style_url(this.id, completion),
    );
  }

  /**
   * Invalidates custom geometry source data inside one geographic region.
   *
   * See `mln_map_invalidate_custom_geometry_source_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
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
   * Invalidates custom geometry source data for one canonical tile.
   *
   * See `mln_map_invalidate_custom_geometry_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
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
   * Invalidates custom MVT vector source data for one canonical tile.
   *
   * See `mln_map_invalidate_custom_mvt_vector_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  invalidateCustomMvtVectorSourceTile(
    sourceId: string,
    tileId: CanonicalTileId = {},
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_invalidate_custom_mvt_vector_source_tile(
        this.id,
        sourceId,
        tileId,
        completion,
      ),
    );
  }

  /**
   * Starts an ordered wrapped-bounds query for a copied camera.
   *
   * See `mln_map_lat_lng_bounds_for_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  latLngBoundsForCamera(camera: CameraOptions = {}): Promise<LatLngBounds> {
    return query<LatLngBounds>(Decoder.latLngBounds, (completion) =>
      native.mln_map_lat_lng_bounds_for_camera(this.id, camera, completion),
    );
  }

  /**
   * Starts an ordered unwrapped-bounds query for a copied camera.
   *
   * See `mln_map_lat_lng_bounds_for_camera_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  latLngBoundsForCameraUnwrapped(
    camera: CameraOptions = {},
  ): Promise<LatLngBounds> {
    return query<LatLngBounds>(Decoder.latLngBounds, (completion) =>
      native.mln_map_lat_lng_bounds_for_camera_unwrapped(
        this.id,
        camera,
        completion,
      ),
    );
  }

  /**
   * Starts an ordered query of every style layer in style order.
   *
   * See `mln_map_list_style_layers` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  listStyleLayers(): Promise<readonly StyleLayerInfo[]> {
    return query<readonly StyleLayerInfo[]>(
      Decoder.styleLayerInfoList,
      (completion) => native.mln_map_list_style_layers(this.id, completion),
    );
  }

  /**
   * Lists every style source in style order.
   *
   * See `mln_map_list_style_sources` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  listStyleSources(): Promise<readonly StyleSourceInfo[]> {
    return query<readonly StyleSourceInfo[]>(
      Decoder.styleSourceInfoList,
      (completion) => native.mln_map_list_style_sources(this.id, completion),
    );
  }

  /**
   * Moves one style layer before another layer or to the top.
   *
   * See `mln_map_move_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  moveStyleLayer(
    layerId: string,
    beforeLayerId?: string,
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
   * Removes per-feature state from this map.
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
   * Removes one runtime style image by ID.
   *
   * See `mln_map_remove_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleImage(imageId: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_remove_style_image(this.id, imageId, completion),
    );
  }

  /**
   * Removes one style layer by ID.
   *
   * See `mln_map_remove_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleLayer(layerId: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_remove_style_layer(this.id, layerId, completion),
    );
  }

  /**
   * Removes one style source by ID.
   *
   * See `mln_map_remove_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleSource(sourceId: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_remove_style_source(this.id, sourceId, completion),
    );
  }

  /**
   * Requests a repaint for a continuous map.
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
   * Requests one still image for a static or tile map.
   *
   * See `mln_map_request_still_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  requestStillImage(): Promise<void> {
    return run((completion) =>
      native.mln_map_request_still_image(this.id, completion),
    );
  }

  /**
   * Submits the sole post-creation logical extent update.
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
   * Submits a copied camera-constraint command.
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
   * Sets custom geometry source data for one canonical tile.
   *
   * See `mln_map_set_custom_geometry_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomGeometrySourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    data: string,
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
   * Reports a custom geometry source error for one canonical tile.
   *
   * See `mln_map_set_custom_geometry_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
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
   * Sets custom MVT vector source data for one canonical tile.
   *
   * See `mln_map_set_custom_mvt_vector_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomMvtVectorSourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    data: Uint8Array,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_custom_mvt_vector_source_tile_data(
        this.id,
        sourceId,
        tileId,
        data,
        completion,
      ),
    );
  }

  /**
   * Reports a custom MVT vector source error for one canonical tile.
   *
   * See `mln_map_set_custom_mvt_vector_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomMvtVectorSourceTileError(
    sourceId: string,
    tileId: CanonicalTileId,
    message: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_custom_mvt_vector_source_tile_error(
        this.id,
        sourceId,
        tileId,
        message,
        completion,
      ),
    );
  }

  /**
   * Submits a debug-overlay command.
   *
   * See `mln_map_set_debug_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setDebugOptions(options: MapDebugOption): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_debug_options(this.id, options, completion),
    );
  }

  /**
   * Selects which map-originated event types this map queues.
   *
   * See `mln_map_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setEventMask(mask: RuntimeEventMask): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_event_mask(this.id, mask, completion),
    );
  }

  /**
   * Submits a copied per-feature-state command.
   *
   * See `mln_map_set_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setFeatureState(
    selector: FeatureStateSelector,
    state: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_feature_state(this.id, selector, state, completion),
    );
  }

  /**
   * Submits a copied free-camera command.
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
   * Updates one GeoJSON source with prepared inline data.
   *
   * See `mln_map_set_geojson_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeojsonSourceData(
    sourceId: string,
    data: GeojsonSourceDataHandle,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_geojson_source_data(
        this.id,
        sourceId,
        data.id,
        completion,
      ),
    );
  }

  /**
   * Overrides one GeoJSON source's synchronous tiling at runtime.
   *
   * See `mln_map_set_geojson_source_synchronous_tiling` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeojsonSourceSynchronousTiling(
    sourceId: string,
    enabled: boolean,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_geojson_source_synchronous_tiling(
        this.id,
        sourceId,
        enabled,
        completion,
      ),
    );
  }

  /**
   * Updates one GeoJSON source to load data from a URL.
   *
   * See `mln_map_set_geojson_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeojsonSourceUrl(
    sourceId: string,
    url: string,
  ): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_geojson_source_url(this.id, sourceId, url, completion),
    );
  }

  /**
   * Submits a global-state JSON value. JSON null restores the style default.
   *
   * See `mln_map_set_global_state_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGlobalStateProperty(
    propertyName: string,
    value: string,
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
   * Updates image source coordinates.
   *
   * See `mln_map_set_image_source_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
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
   * Updates an image source with inline image pixels.
   *
   * See `mln_map_set_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setImageSourceImage(
    sourceId: string,
    image: PremultipliedRgba8Image = {},
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
   * Updates an image source to load its image from a URL.
   *
   * See `mln_map_set_image_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setImageSourceUrl(sourceId: string, url: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_image_source_url(this.id, sourceId, url, completion),
    );
  }

  /**
   * Submits copied axonometric rendering option fields.
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
   * Submits a rendering-stats visibility command.
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
   * Sets one runtime style image.
   *
   * See `mln_map_set_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleImage(
    imageId: string,
    image: PremultipliedRgba8Image = {},
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
   * Queues an inline style JSON command.
   *
   * See `mln_map_set_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setStyleJson(json: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_json(this.id, json, completion),
    );
  }

  /**
   * Sets one layer property using its MapLibre style-spec property name.
   *
   * See `mln_map_set_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLayerProperty(
    layerId: string,
    propertyName: string,
    value: string,
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
   * Sets the style light from a style-spec light JSON object.
   *
   * See `mln_map_set_style_light_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLightJson(lightJson: string): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_map_set_style_light_json(this.id, lightJson, completion),
    );
  }

  /**
   * Sets one style light property using its MapLibre style-spec property name.
   *
   * See `mln_map_set_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLightProperty(
    propertyName: string,
    value: string,
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
   * Sets whether one style source stores fetched tiles in the persistent cache.
   *
   * See `mln_map_set_style_source_volatile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
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
   * Sets the style's global transition options.
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
   * Queues a style URL command.
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
   * Submits a copied tile-options command.
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
   * Submits a copied viewport-options command.
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
   * Submits one atomic camera update.
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
   * Adds a custom geometry source.
   *
   * See `mln_map_add_custom_geometry_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addCustomGeometrySource(
    sourceId: string,
    options?: CustomGeometrySourceOptions,
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
   * Adds a custom MVT vector source.
   *
   * See `mln_map_add_custom_mvt_vector_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addCustomMvtVectorSource(
    sourceId: string,
    options?: CustomMvtVectorSourceOptions,
  ): void {
    withRegistrations((registrations) => {
      native.mln_map_add_custom_mvt_vector_source(
        this.#owner.id,
        sourceId,
        options,
        undefined,
      );
      registrations.commit(rootsOf(this.#owner));
    });
  }

  /**
   * Adds a GeoJSON source with prepared inline data.
   *
   * See `mln_map_add_geojson_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addGeojsonSourceData(sourceId: string, data: GeojsonSourceDataHandle): void {
    native.mln_map_add_geojson_source_data(
      this.#owner.id,
      sourceId,
      data.id,
      undefined,
    );
  }

  /**
   * Adds a GeoJSON source with URL data.
   *
   * See `mln_map_add_geojson_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addGeojsonSourceUrl(
    sourceId: string,
    url: string,
    options?: GeojsonSourceOptions,
  ): void {
    native.mln_map_add_geojson_source_url(
      this.#owner.id,
      sourceId,
      url,
      options,
      undefined,
    );
  }

  /**
   * Adds an image source with inline image pixels.
   *
   * See `mln_map_add_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addImageSourceImage(
    sourceId: string,
    coordinates: readonly LatLng[],
    image: PremultipliedRgba8Image = {},
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
   * Adds an image source that loads its image from a URL.
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
   * Adds one style layer from a full style-spec layer JSON object.
   *
   * See `mln_map_add_style_layer_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addStyleLayerJson(layerJson: string, beforeLayerId?: string): void {
    native.mln_map_add_style_layer_json(
      this.#owner.id,
      layerJson,
      beforeLayerId,
      undefined,
    );
  }

  /**
   * Adds one style source from a style-spec source JSON object.
   *
   * See `mln_map_add_style_source_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  addStyleSourceJson(sourceId: string, sourceJson: string): void {
    native.mln_map_add_style_source_json(
      this.#owner.id,
      sourceId,
      sourceJson,
      undefined,
    );
  }

  /**
   * Submits one atomic relative camera update.
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
   * Begins a command group, which holds this map's render updates until the
   * group ends.
   *
   * See `mln_map_begin_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  beginCommandGroup(): void {
    native.mln_map_begin_command_group(this.#owner.id, undefined);
  }

  /**
   * Cancels the camera transitions of the commands whose animation carried
   * transition_id, and leaves every other transition running.
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
   * Cancels the camera transitions running when this command commits.
   *
   * See `mln_map_cancel_transitions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  cancelTransitions(): void {
    native.mln_map_cancel_transitions(this.#owner.id, undefined);
  }

  /**
   * Submits an ordered debug-log command.
   *
   * See `mln_map_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  dumpDebugLogs(): void {
    native.mln_map_dump_debug_logs(this.#owner.id, undefined);
  }

  /**
   * Ends the innermost command group that `mln_map_begin_command_group()`
   * began.
   *
   * See `mln_map_end_command_group` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  endCommandGroup(): void {
    native.mln_map_end_command_group(this.#owner.id, undefined);
  }

  /**
   * Invalidates custom geometry source data inside one geographic region.
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
   * Invalidates custom geometry source data for one canonical tile.
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
   * Invalidates custom MVT vector source data for one canonical tile.
   *
   * See `mln_map_invalidate_custom_mvt_vector_source_tile` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  invalidateCustomMvtVectorSourceTile(
    sourceId: string,
    tileId: CanonicalTileId = {},
  ): void {
    native.mln_map_invalidate_custom_mvt_vector_source_tile(
      this.#owner.id,
      sourceId,
      tileId,
      undefined,
    );
  }

  /**
   * Moves one style layer before another layer or to the top.
   *
   * See `mln_map_move_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  moveStyleLayer(layerId: string, beforeLayerId?: string): void {
    native.mln_map_move_style_layer(
      this.#owner.id,
      layerId,
      beforeLayerId,
      undefined,
    );
  }

  /**
   * Removes per-feature state from this map.
   *
   * See `mln_map_remove_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  removeFeatureState(selector: FeatureStateSelector): void {
    native.mln_map_remove_feature_state(this.#owner.id, selector, undefined);
  }

  /**
   * Removes one runtime style image by ID.
   *
   * See `mln_map_remove_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleImage(imageId: string): void {
    native.mln_map_remove_style_image(this.#owner.id, imageId, undefined);
  }

  /**
   * Removes one style layer by ID.
   *
   * See `mln_map_remove_style_layer` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleLayer(layerId: string): void {
    native.mln_map_remove_style_layer(this.#owner.id, layerId, undefined);
  }

  /**
   * Removes one style source by ID.
   *
   * See `mln_map_remove_style_source` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  removeStyleSource(sourceId: string): void {
    native.mln_map_remove_style_source(this.#owner.id, sourceId, undefined);
  }

  /**
   * Requests a repaint for a continuous map.
   *
   * See `mln_map_request_repaint` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  requestRepaint(): void {
    native.mln_map_request_repaint(this.#owner.id, undefined);
  }

  /**
   * Submits the sole post-creation logical extent update.
   *
   * See `mln_map_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  resize(extent: LogicalExtent = {}): void {
    native.mln_map_resize(this.#owner.id, extent, undefined);
  }

  /**
   * Submits a copied camera-constraint command.
   *
   * See `mln_map_set_bounds` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setBounds(options: BoundOptions = {}): void {
    native.mln_map_set_bounds(this.#owner.id, options, undefined);
  }

  /**
   * Sets custom geometry source data for one canonical tile.
   *
   * See `mln_map_set_custom_geometry_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomGeometrySourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    data: string,
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
   * Reports a custom geometry source error for one canonical tile.
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
   * Sets custom MVT vector source data for one canonical tile.
   *
   * See `mln_map_set_custom_mvt_vector_source_tile_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomMvtVectorSourceTileData(
    sourceId: string,
    tileId: CanonicalTileId,
    data: Uint8Array,
  ): void {
    native.mln_map_set_custom_mvt_vector_source_tile_data(
      this.#owner.id,
      sourceId,
      tileId,
      data,
      undefined,
    );
  }

  /**
   * Reports a custom MVT vector source error for one canonical tile.
   *
   * See `mln_map_set_custom_mvt_vector_source_tile_error` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setCustomMvtVectorSourceTileError(
    sourceId: string,
    tileId: CanonicalTileId,
    message: string,
  ): void {
    native.mln_map_set_custom_mvt_vector_source_tile_error(
      this.#owner.id,
      sourceId,
      tileId,
      message,
      undefined,
    );
  }

  /**
   * Submits a debug-overlay command.
   *
   * See `mln_map_set_debug_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setDebugOptions(options: MapDebugOption): void {
    native.mln_map_set_debug_options(this.#owner.id, options, undefined);
  }

  /**
   * Selects which map-originated event types this map queues.
   *
   * See `mln_map_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setEventMask(mask: RuntimeEventMask): void {
    native.mln_map_set_event_mask(this.#owner.id, mask, undefined);
  }

  /**
   * Submits a copied per-feature-state command.
   *
   * See `mln_map_set_feature_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setFeatureState(selector: FeatureStateSelector, state: string): void {
    native.mln_map_set_feature_state(
      this.#owner.id,
      selector,
      state,
      undefined,
    );
  }

  /**
   * Submits a copied free-camera command.
   *
   * See `mln_map_set_free_camera_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setFreeCameraOptions(options: FreeCameraOptions = {}): void {
    native.mln_map_set_free_camera_options(this.#owner.id, options, undefined);
  }

  /**
   * Updates one GeoJSON source with prepared inline data.
   *
   * See `mln_map_set_geojson_source_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeojsonSourceData(sourceId: string, data: GeojsonSourceDataHandle): void {
    native.mln_map_set_geojson_source_data(
      this.#owner.id,
      sourceId,
      data.id,
      undefined,
    );
  }

  /**
   * Overrides one GeoJSON source's synchronous tiling at runtime.
   *
   * See `mln_map_set_geojson_source_synchronous_tiling` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeojsonSourceSynchronousTiling(sourceId: string, enabled: boolean): void {
    native.mln_map_set_geojson_source_synchronous_tiling(
      this.#owner.id,
      sourceId,
      enabled,
      undefined,
    );
  }

  /**
   * Updates one GeoJSON source to load data from a URL.
   *
   * See `mln_map_set_geojson_source_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGeojsonSourceUrl(sourceId: string, url: string): void {
    native.mln_map_set_geojson_source_url(
      this.#owner.id,
      sourceId,
      url,
      undefined,
    );
  }

  /**
   * Submits a global-state JSON value. JSON null restores the style default.
   *
   * See `mln_map_set_global_state_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setGlobalStateProperty(propertyName: string, value: string): void {
    native.mln_map_set_global_state_property(
      this.#owner.id,
      propertyName,
      value,
      undefined,
    );
  }

  /**
   * Updates image source coordinates.
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
   * Updates an image source with inline image pixels.
   *
   * See `mln_map_set_image_source_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setImageSourceImage(
    sourceId: string,
    image: PremultipliedRgba8Image = {},
  ): void {
    native.mln_map_set_image_source_image(
      this.#owner.id,
      sourceId,
      image,
      undefined,
    );
  }

  /**
   * Updates an image source to load its image from a URL.
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
   * Submits copied axonometric rendering option fields.
   *
   * See `mln_map_set_projection_mode` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setProjectionMode(mode: ProjectionMode = {}): void {
    native.mln_map_set_projection_mode(this.#owner.id, mode, undefined);
  }

  /**
   * Submits a rendering-stats visibility command.
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
   * Sets one runtime style image.
   *
   * See `mln_map_set_style_image` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleImage(
    imageId: string,
    image: PremultipliedRgba8Image = {},
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
   * Queues an inline style JSON command.
   *
   * See `mln_map_set_style_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setStyleJson(json: string): void {
    native.mln_map_set_style_json(this.#owner.id, json, undefined);
  }

  /**
   * Sets one layer property using its MapLibre style-spec property name.
   *
   * See `mln_map_set_style_layer_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLayerProperty(
    layerId: string,
    propertyName: string,
    value: string,
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
   * Sets the style light from a style-spec light JSON object.
   *
   * See `mln_map_set_style_light_json` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLightJson(lightJson: string): void {
    native.mln_map_set_style_light_json(this.#owner.id, lightJson, undefined);
  }

  /**
   * Sets one style light property using its MapLibre style-spec property name.
   *
   * See `mln_map_set_style_light_property` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  setStyleLightProperty(propertyName: string, value: string): void {
    native.mln_map_set_style_light_property(
      this.#owner.id,
      propertyName,
      value,
      undefined,
    );
  }

  /**
   * Sets whether one style source stores fetched tiles in the persistent cache.
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
   * Sets the style's global transition options.
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
   * Queues a style URL command.
   *
   * See `mln_map_set_style_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  setStyleUrl(url: string): void {
    native.mln_map_set_style_url(this.#owner.id, url, undefined);
  }

  /**
   * Submits a copied tile-options command.
   *
   * See `mln_map_set_tile_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setTileOptions(options: MapTileOptions = {}): void {
    native.mln_map_set_tile_options(this.#owner.id, options, undefined);
  }

  /**
   * Submits a copied viewport-options command.
   *
   * See `mln_map_set_viewport_options` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  setViewportOptions(options: MapViewportOptions = {}): void {
    native.mln_map_set_viewport_options(this.#owner.id, options, undefined);
  }

  /**
   * Submits one atomic camera update.
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
 * A standalone projection of one map view.
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
   * Releases a standalone projection.
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
   * Copies the projection camera into out_camera.
   *
   * See `mln_map_projection_get_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  getCamera(): CameraOptions {
    return native.mln_map_projection_get_camera(this.id);
  }

  /**
   * Converts a screen point to a geographic coordinate.
   *
   * See `mln_map_projection_lat_lng_for_pixel` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  latLngForPixel(point: ScreenPoint = {}): LatLng {
    return native.mln_map_projection_lat_lng_for_pixel(this.id, point);
  }

  /**
   * Converts a screen point to an unwrapped geographic coordinate.
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
   * Converts screen points to geographic coordinates.
   *
   * See `mln_map_projection_lat_lngs_for_pixels` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  latLngsForPixels(points: readonly ScreenPoint[]): readonly LatLng[] {
    return native.mln_map_projection_lat_lngs_for_pixels(this.id, points);
  }

  /**
   * Converts screen points to unwrapped geographic coordinates.
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
   * Reads the ground distance covered by one logical map pixel at a latitude
   * for the helper camera zoom.
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
   * Converts a geographic coordinate to a screen point.
   *
   * See `mln_map_projection_pixel_for_lat_lng` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  pixelForLatLng(coordinate: LatLng = {}): ScreenPoint {
    return native.mln_map_projection_pixel_for_lat_lng(this.id, coordinate);
  }

  /**
   * Converts geographic coordinates to screen points.
   *
   * See `mln_map_projection_pixels_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  pixelsForLatLngs(coordinates: readonly LatLng[]): readonly ScreenPoint[] {
    return native.mln_map_projection_pixels_for_lat_lngs(this.id, coordinates);
  }

  /**
   * Applies a camera update to a standalone projection.
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
 * An owned batch of frame results from one drain.
 *
 * See `mln_render_frame_batch` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export class RenderFrameBatchHandle extends Owner {
  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, handle: bigint, registrations?: Registrations) {
    super(key, handle, "RenderFrameBatchHandle", { registrations });
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
   * Releases a frame-result batch.
   *
   * See `mln_render_frame_batch_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  close(): void {
    closeSync(this, (handle, token) =>
      native.mln_render_frame_batch_release(handle, token),
    );
  }

  /**
   * Borrows the result view stored by an owned frame-result batch.
   *
   * See `mln_render_frame_batch_get` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  get(): RenderFrameBatchView {
    return native.mln_render_frame_batch_get(this.id);
  }
}
defineDisposers(RenderFrameBatchHandle, undefined, "close");

/**
 * A render session, which renders one map to one render target.
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
   * Irreversibly ends session work and destroys the session's graphics objects
   * where that is safe.
   *
   * See `mln_render_session_abandon` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  abandon(): RenderAbandonResult {
    return native.mln_render_session_abandon(this.id);
  }

  /**
   * Acquires the oldest rendered frame that is not already acquired. The frame
   * owns its slot until release. The call is nonblocking.
   *
   * See `mln_render_session_acquire_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
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
   * Ends every frame demand that has not started with
   * `MLN_RENDER_RESULT_NO_UPDATE` and completes after earlier driver work has
   * finished. A running demand finishes its attempt and ends any wait for a map
   * update or a free slot. After completion, a new frame demand is required to
   * render again.
   *
   * See `mln_render_session_barrier` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  barrier(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_barrier(this.id, completion),
    );
  }

  /**
   * Starts asynchronous renderer-data clearing.
   *
   * See `mln_render_session_clear_data` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  clearData(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_clear_data(this.id, completion),
    );
  }

  /**
   * Consumes a session in any state and retires it, then runs completion, if
   * given, once the host may destroy the target's graphics objects.
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
   * Starts normal graphics-owner teardown and map detachment.
   *
   * See `mln_render_session_detach` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  detach(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_detach(this.id, completion),
    );
  }

  /**
   * Consumes a session in any state and retires it, then runs completion, if
   * given, once the host may destroy the target's graphics objects.
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
   * Drains every currently queued terminal frame result into an independently
   * owned batch. The records remain stable until the batch is released.
   *
   * See `mln_render_session_drain_frame_results` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  drainFrameResults(): RenderFrameBatchHandle | undefined {
    const handle = native.mln_render_session_drain_frame_results(this.id);
    return handle === undefined
      ? undefined
      : adopt(
          handle,
          () => new RenderFrameBatchHandle(ADOPT, handle),
          (disposed) =>
            native.mln_render_frame_batch_release(disposed, undefined),
        );
  }

  /**
   * Starts asynchronous renderer diagnostic-log emission.
   *
   * See `mln_render_session_dump_debug_logs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  dumpDebugLogs(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_dump_debug_logs(this.id, completion),
    );
  }

  /**
   * Returns the immutable capabilities fixed during attachment.
   *
   * See `mln_render_session_get_capabilities` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  getCapabilities(): RenderSessionCapabilities {
    return native.mln_render_session_get_capabilities(this.id);
  }

  /**
   * Copies the latest render-session snapshot from any native thread.
   *
   * See `mln_render_session_get_snapshot` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  getSnapshot(): RenderSessionSnapshot {
    return native.mln_render_session_get_snapshot(this.id);
  }

  /**
   * Starts a feature-extension query against the latest driver state. The
   * completion borrows one `mln_buffer_view` holding UTF-8 JSON (value_count
   * 1), valid only for the callback.
   *
   * See `mln_render_session_query_feature_extensions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  queryFeatureExtensions(
    sourceId: string,
    feature: string,
    extension: string,
    extensionField: string,
    argumentsValue?: string,
  ): Promise<string> {
    return query<string>(Decoder.bufferView, (completion) =>
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
   * Starts a rendered-feature query against the session's latest driver state.
   *
   * See `mln_render_session_query_rendered_features` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options?: RenderedFeatureQueryOptions,
  ): Promise<readonly RenderedFeature[]> {
    return query<readonly RenderedFeature[]>(
      Decoder.renderedFeatureList,
      (completion) =>
        native.mln_render_session_query_rendered_features(
          this.id,
          geometry,
          options,
          completion,
        ),
    );
  }

  /**
   * Starts a source-feature query against the session's latest driver state.
   *
   * See `mln_render_session_query_source_features` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  querySourceFeatures(
    sourceId: string,
    options?: SourceFeatureQueryOptions,
  ): Promise<readonly SourceFeature[]> {
    return query<readonly SourceFeature[]>(
      Decoder.sourceFeatureList,
      (completion) =>
        native.mln_render_session_query_source_features(
          this.id,
          sourceId,
          options,
          completion,
        ),
    );
  }

  /**
   * Reads back the latest frame of the session's owned texture as premultiplied
   * RGBA8.
   *
   * See `mln_render_session_read_texture` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  readTexture(): Promise<TextureReadbackResult> {
    return query<TextureReadbackResult>(
      Decoder.textureReadbackResult,
      (completion) =>
        native.mln_render_session_read_texture(this.id, completion),
    );
  }

  /**
   * Starts best-effort release of renderer caches.
   *
   * See `mln_render_session_reduce_memory_use` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  reduceMemoryUse(): Promise<void> {
    return run((completion) =>
      native.mln_render_session_reduce_memory_use(this.id, completion),
    );
  }

  /**
   * Requests a frame without waiting. Every accepted demand produces one
   * terminal result record. Caller-driver sessions require service on the
   * graphics thread. Core-worker sessions need no host service.
   *
   * See `mln_render_session_request_frame` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  requestFrame(demand: FrameDemand = {}): void {
    native.mln_render_session_request_frame(this.id, demand);
  }

  /**
   * Starts an ordered logical resize of the driver and the map. The driver
   * applies the extent in order with its other work, and later driver work does
   * not wait for the map. The completion runs after both have applied it and
   * the map has published its snapshot. Frames render once a map update matches
   * the applied extent; static and tile maps publish that update for their next
   * still image.
   *
   * See `mln_render_session_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  resize(extent: LogicalExtent = {}): Promise<CommandCompletion> {
    return command((completion) =>
      native.mln_render_session_resize(this.id, extent, completion),
    );
  }

  /**
   * Services every queued item for a caller-graphics-thread driver. The first
   * successful service call fixes the session's graphics-thread identity; later
   * calls from another native thread return `MLN_STATUS_WRONG_THREAD`. The
   * target context must be current. Core-worker sessions return
   * `MLN_STATUS_INVALID_STATE`.
   *
   * See `mln_render_session_service_driver_work` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  serviceDriverWork(): void {
    native.mln_render_session_service_driver_work(this.id);
  }

  /**
   * Starts an ordered replacement of every texture of a caller-owned Metal
   * ring.
   *
   * See `mln_render_session_set_metal_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  setMetalBorrowedTextureTarget(
    descriptor: MetalBorrowedTextureDescriptor = {},
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
   * Starts an ordered Metal surface replacement.
   *
   * See `mln_render_session_set_metal_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  setMetalSurfaceTarget(
    descriptor: MetalSurfaceDescriptor = {},
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_metal_surface_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Starts an ordered replacement of every texture of a caller-owned OpenGL
   * ring.
   *
   * See `mln_render_session_set_opengl_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  setOpenglBorrowedTextureTarget(
    descriptor: OpenglBorrowedTextureDescriptor = {},
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_opengl_borrowed_texture_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Starts an ordered OpenGL surface replacement.
   *
   * See `mln_render_session_set_opengl_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  setOpenglSurfaceTarget(
    descriptor: OpenglSurfaceDescriptor = {},
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_opengl_surface_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Starts an ordered replacement of every image of a caller-owned Vulkan ring.
   *
   * See `mln_render_session_set_vulkan_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  setVulkanBorrowedTextureTarget(
    descriptor: VulkanBorrowedTextureDescriptor = {},
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
   * Starts an ordered Vulkan surface replacement.
   *
   * See `mln_render_session_set_vulkan_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  setVulkanSurfaceTarget(
    descriptor: VulkanSurfaceDescriptor = {},
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_vulkan_surface_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Starts an ordered replacement of every texture of a caller-owned WebGPU
   * ring.
   *
   * See `mln_render_session_set_webgpu_borrowed_texture_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  setWebgpuBorrowedTextureTarget(
    descriptor: WebgpuBorrowedTextureDescriptor = {},
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_webgpu_borrowed_texture_target(
        this.id,
        descriptor,
        completion,
      ),
    );
  }

  /**
   * Starts an ordered WebGPU surface replacement.
   *
   * See `mln_render_session_set_webgpu_surface_target` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  setWebgpuSurfaceTarget(
    descriptor: WebgpuSurfaceDescriptor = {},
  ): Promise<void> {
    return run((completion) =>
      native.mln_render_session_set_webgpu_surface_target(
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
   * Starts an ordered logical resize of the driver and the map. The driver
   * applies the extent in order with its other work, and later driver work does
   * not wait for the map. The completion runs after both have applied it and
   * the map has published its snapshot. Frames render once a map update matches
   * the applied extent; static and tile maps publish that update for their next
   * still image.
   *
   * See `mln_render_session_resize` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  resize(extent: LogicalExtent = {}): void {
    native.mln_render_session_resize(this.#owner.id, extent, undefined);
  }
}

/**
 * A resource request that a resource provider handles.
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
   * Releases the provider's reference to a resource request handle.
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
   * Completes a C API resource provider request.
   *
   * See `mln_resource_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  complete(response: ResourceResponse = {}): void {
    native.mln_resource_request_complete(this.id, response);
  }

  /**
   * Reports whether MapLibre has cancelled a C API resource provider request.
   *
   * See `mln_resource_request_is_cancelled` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  isCancelled(): boolean {
    return native.mln_resource_request_is_cancelled(this.id);
  }

  /**
   * Hands an outstanding provider request to the built-in network loader.
   *
   * See `mln_resource_request_pass_through` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  passThrough(): void {
    native.mln_resource_request_pass_through(this.id);
  }

  /**
   * Registers a callback that runs when MapLibre cancels a C API resource
   * provider request.
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
 * A resource URL transform that waits for its host's answer.
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
   * Answers a resource transform request with a replacement URL.
   *
   * See `mln_resource_transform_request_complete` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  complete(url: string): void {
    native.mln_resource_transform_request_complete(this.id, url);
  }
}
defineDisposers(ResourceTransformRequestHandle, undefined, "close");

/**
 * A runtime: the native scheduler thread and event store for its maps.
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
   * Stops event delivery and retires the previous handler.
   *
   * See `mln_runtime_clear_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearEventHandler(): Promise<void> {
    return run((completion) =>
      native.mln_runtime_clear_event_handler(this.id, completion),
    );
  }

  /**
   * Clears the runtime-scoped outgoing HTTP header transform.
   *
   * See `mln_runtime_clear_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearHttpHeaderTransform(): Promise<void> {
    return run((completion) =>
      native.mln_runtime_clear_http_header_transform(this.id, completion),
    );
  }

  /**
   * Clears the runtime-scoped network resource provider.
   *
   * See `mln_runtime_clear_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearResourceProvider(): Promise<void> {
    return run((completion) =>
      native.mln_runtime_clear_resource_provider(this.id, completion),
    );
  }

  /**
   * Clears the runtime-scoped URL transform for network resources.
   *
   * See `mln_runtime_clear_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  clearResourceTransform(): Promise<void> {
    return run((completion) =>
      native.mln_runtime_clear_resource_transform(this.id, completion),
    );
  }

  /**
   * Starts release of a runtime after checking its children.
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
   * Starts creation of a map on the runtime.
   *
   * See `mln_runtime_create_map` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  createMap(options: MapOptions = {}): Promise<MapHandle> {
    return queryOwned(
      Decoder.map,
      (completion) =>
        native.mln_runtime_create_map(this.id, options, completion),
      (handle) =>
        adopt(
          handle,
          () => new MapHandle(ADOPT, handle, this),
          (disposed) => native.mln_map_dispose(disposed, undefined),
        ),
    );
  }

  /**
   * Starts creating an offline region.
   *
   * See `mln_runtime_create_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  createOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: Uint8Array,
  ): Promise<OfflineRegionInfo> {
    return query<OfflineRegionInfo>(Decoder.offlineRegionInfo, (completion) =>
      native.mln_runtime_create_offline_region(
        this.id,
        definition,
        metadata,
        completion,
      ),
    );
  }

  /**
   * Deletes an offline region.
   *
   * See `mln_runtime_delete_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  deleteOfflineRegion(regionId: bigint): Promise<void> {
    return run((completion) =>
      native.mln_runtime_delete_offline_region(this.id, regionId, completion),
    );
  }

  /**
   * Consumes a runtime handle without observing its asynchronous retirement.
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
   * Reports which runtime-scoped event types this runtime queues.
   *
   * See `mln_runtime_get_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  getEventMask(): RuntimeEventMask {
    return native.mln_runtime_get_event_mask(this.id);
  }

  /**
   * Starts getting one offline region by ID.
   *
   * See `mln_runtime_get_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getOfflineRegion(regionId: bigint): Promise<OfflineRegionInfo | undefined> {
    return query<OfflineRegionInfo | undefined>(
      Decoder.offlineRegionInfoOptional,
      (completion) =>
        native.mln_runtime_get_offline_region(this.id, regionId, completion),
    );
  }

  /**
   * Starts getting the current download status for an offline region.
   *
   * See `mln_runtime_get_offline_region_status` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  getOfflineRegionStatus(regionId: bigint): Promise<OfflineRegionStatus> {
    return query<OfflineRegionStatus>(
      Decoder.offlineRegionStatus,
      (completion) =>
        native.mln_runtime_get_offline_region_status(
          this.id,
          regionId,
          completion,
        ),
    );
  }

  /**
   * Invalidates cached resources for an offline region.
   *
   * See `mln_runtime_invalidate_offline_region` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
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
   * Starts listing the offline regions in the runtime database.
   *
   * See `mln_runtime_list_offline_regions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  listOfflineRegions(): Promise<readonly OfflineRegionInfo[]> {
    return query<readonly OfflineRegionInfo[]>(
      Decoder.offlineRegionInfoList,
      (completion) =>
        native.mln_runtime_list_offline_regions(this.id, completion),
    );
  }

  /**
   * Starts merging offline regions from another MapLibre offline database.
   *
   * See `mln_runtime_merge_offline_regions` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  mergeOfflineRegions(
    sideDatabasePath: string,
  ): Promise<readonly OfflineRegionInfo[]> {
    return query<readonly OfflineRegionInfo[]>(
      Decoder.offlineRegionInfoList,
      (completion) =>
        native.mln_runtime_merge_offline_regions(
          this.id,
          sideDatabasePath,
          completion,
        ),
    );
  }

  /**
   * Starts a MapLibre ambient cache maintenance operation for this runtime.
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
   * Selects how runtime and map events reach the host.
   *
   * See `mln_runtime_set_event_handler` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setEventHandler(handler?: RuntimeEventHandler): Promise<void> {
    return withRegistrations((registrations) =>
      run((completion) => {
        native.mln_runtime_set_event_handler(this.id, handler, completion);
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Selects which runtime-scoped event types this runtime queues.
   *
   * See `mln_runtime_set_event_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setEventMask(mask: RuntimeEventMask): void {
    native.mln_runtime_set_event_mask(this.id, mask);
  }

  /**
   * Registers or replaces the runtime-scoped outgoing HTTP header transform.
   *
   * See `mln_runtime_set_http_header_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setHttpHeaderTransform(transform: HttpHeaderTransform): Promise<void> {
    return withRegistrations((registrations) =>
      run((completion) => {
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
   * Starts a change to this runtime's maximum ambient cache size.
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
   * Sets an offline region's native download state.
   *
   * See `mln_runtime_set_offline_region_download_state` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
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
   * Enables or disables runtime events for an offline region.
   *
   * See `mln_runtime_set_offline_region_observed` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
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
   * Registers or replaces a runtime-scoped network resource provider.
   *
   * See `mln_runtime_set_resource_provider` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setResourceProvider(provider: ResourceProvider): Promise<void> {
    return withRegistrations((registrations) =>
      run((completion) => {
        native.mln_runtime_set_resource_provider(this.id, provider, completion);
        registrations.commit(rootsOf(this));
      }),
    );
  }

  /**
   * Registers or updates a runtime-scoped URL transform for network resources.
   *
   * See `mln_runtime_set_resource_transform` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  setResourceTransform(transform: ResourceTransform): Promise<void> {
    return withRegistrations((registrations) =>
      run((completion) => {
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
   * Starts updating opaque binary metadata for an offline region.
   *
   * See `mln_runtime_update_offline_region_metadata` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  updateOfflineRegionMetadata(
    regionId: bigint,
    metadata: Uint8Array,
  ): Promise<OfflineRegionInfo> {
    return query<OfflineRegionInfo>(Decoder.offlineRegionInfo, (completion) =>
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
 * A parsed style expression, immutable and independent of any map.
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
   * Releases a style expression.
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
   * Evaluates a parsed expression and writes its result as UTF-8 JSON.
   *
   * See `mln_style_expression_evaluate` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  evaluate(inputs?: StyleExpressionInputs): string {
    return native.mln_style_expression_evaluate(this.id, inputs);
  }

  /**
   * Copies what a parsed expression depends on into out_info.
   *
   * See `mln_style_expression_get_info` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  getInfo(): StyleExpressionInfo {
    return native.mln_style_expression_get_info(this.id);
  }
}
defineDisposers(StyleExpressionHandle, undefined, "close");

/** A new session and the completion of the attachment that created it. */
export interface RenderSessionAttachment {
  /** The session, usable at once while attachment completes. */
  readonly session: RenderSessionHandle;
  /** Settles after native attachment finishes. */
  readonly completed: Promise<void>;
}

/**
 * A `GpuSync` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withProducerSync} runs.
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
    return this.#scope.active(this.#value).kind;
  }

  get object(): GpuSync["object"] {
    return this.#scope.active(this.#value).object;
  }
}

/**
 * A `MetalTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withMetalTexture} runs.
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
    return this.#scope.active(this.#value).generation;
  }

  get width(): MetalTextureFrame["width"] {
    return this.#scope.active(this.#value).width;
  }

  get height(): MetalTextureFrame["height"] {
    return this.#scope.active(this.#value).height;
  }

  get scaleFactor(): MetalTextureFrame["scaleFactor"] {
    return this.#scope.active(this.#value).scaleFactor;
  }

  get frameId(): MetalTextureFrame["frameId"] {
    return this.#scope.active(this.#value).frameId;
  }

  get slot(): MetalTextureFrame["slot"] {
    return this.#scope.active(this.#value).slot;
  }

  get texture(): MetalTextureFrame["texture"] {
    return this.#scope.active(this.#value).texture;
  }

  get device(): MetalTextureFrame["device"] {
    return this.#scope.active(this.#value).device;
  }

  get pixelFormat(): MetalTextureFrame["pixelFormat"] {
    return this.#scope.active(this.#value).pixelFormat;
  }
}

/**
 * A `OpenglTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withOpenglTexture} runs.
 */
export class ScopedOpenglTextureFrame {
  readonly #value: OpenglTextureFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: OpenglTextureFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get generation(): OpenglTextureFrame["generation"] {
    return this.#scope.active(this.#value).generation;
  }

  get width(): OpenglTextureFrame["width"] {
    return this.#scope.active(this.#value).width;
  }

  get height(): OpenglTextureFrame["height"] {
    return this.#scope.active(this.#value).height;
  }

  get scaleFactor(): OpenglTextureFrame["scaleFactor"] {
    return this.#scope.active(this.#value).scaleFactor;
  }

  get frameId(): OpenglTextureFrame["frameId"] {
    return this.#scope.active(this.#value).frameId;
  }

  get slot(): OpenglTextureFrame["slot"] {
    return this.#scope.active(this.#value).slot;
  }

  get texture(): OpenglTextureFrame["texture"] {
    return this.#scope.active(this.#value).texture;
  }

  get target(): OpenglTextureFrame["target"] {
    return this.#scope.active(this.#value).target;
  }

  get internalFormat(): OpenglTextureFrame["internalFormat"] {
    return this.#scope.active(this.#value).internalFormat;
  }

  get format(): OpenglTextureFrame["format"] {
    return this.#scope.active(this.#value).format;
  }

  get type(): OpenglTextureFrame["type"] {
    return this.#scope.active(this.#value).type;
  }
}

/**
 * A `VulkanTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withVulkanTexture} runs.
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
    return this.#scope.active(this.#value).generation;
  }

  get width(): VulkanTextureFrame["width"] {
    return this.#scope.active(this.#value).width;
  }

  get height(): VulkanTextureFrame["height"] {
    return this.#scope.active(this.#value).height;
  }

  get scaleFactor(): VulkanTextureFrame["scaleFactor"] {
    return this.#scope.active(this.#value).scaleFactor;
  }

  get frameId(): VulkanTextureFrame["frameId"] {
    return this.#scope.active(this.#value).frameId;
  }

  get slot(): VulkanTextureFrame["slot"] {
    return this.#scope.active(this.#value).slot;
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
    return this.#scope.active(this.#value).format;
  }

  get layout(): VulkanTextureFrame["layout"] {
    return this.#scope.active(this.#value).layout;
  }
}

/**
 * A `WebgpuTextureFrame` whose native objects stay valid only while the
 * callback of {@link AcquiredFrameHandle.withWebgpuTexture} runs.
 */
export class ScopedWebgpuTextureFrame {
  readonly #value: WebgpuTextureFrame;
  readonly #scope: ViewScope;

  /**
   * Created by the API, which passes a key that only it holds.
   *
   * @internal
   */
  constructor(key: AdoptKey, scope: ViewScope, value: WebgpuTextureFrame) {
    checkKey(key);
    this.#value = value;
    this.#scope = scope;
  }

  get generation(): WebgpuTextureFrame["generation"] {
    return this.#scope.active(this.#value).generation;
  }

  get width(): WebgpuTextureFrame["width"] {
    return this.#scope.active(this.#value).width;
  }

  get height(): WebgpuTextureFrame["height"] {
    return this.#scope.active(this.#value).height;
  }

  get scaleFactor(): WebgpuTextureFrame["scaleFactor"] {
    return this.#scope.active(this.#value).scaleFactor;
  }

  get frameId(): WebgpuTextureFrame["frameId"] {
    return this.#scope.active(this.#value).frameId;
  }

  get slot(): WebgpuTextureFrame["slot"] {
    return this.#scope.active(this.#value).slot;
  }

  get texture(): WebgpuTextureFrame["texture"] {
    return this.#scope.active(this.#value).texture;
  }

  get textureView(): WebgpuTextureFrame["textureView"] {
    return this.#scope.active(this.#value).textureView;
  }

  get device(): WebgpuTextureFrame["device"] {
    return this.#scope.active(this.#value).device;
  }

  get format(): WebgpuTextureFrame["format"] {
    return this.#scope.active(this.#value).format;
  }
}

// How each drained delivery reaches the host value that its registration
// rooted, by delivery type.
defineDeliveries({
  // MLN_DELIVERY_TYPE_LOG_RECORD
  [2]: (target, payload) => {
    const callback = target as LogHandler;
    const [severity, event, code, message] = [
      payload.severity,
      payload.event,
      payload.code,
      payload.message,
    ] as Parameters<LogHandler>;
    callback(severity, event, code, message);
  },
  // MLN_DELIVERY_TYPE_RESOURCE_REQUEST
  [3]: (target, payload, index) => {
    const callback = (target as ResourceProvider).callback;
    const [request, handleValue] = [
      payload.request,
      payload.handle,
    ] as Parameters<NonNullable<ResourceProvider["callback"]>>;
    const owner = takeDelivered(
      index,
      handleValue as unknown as bigint,
      (handle) => new ResourceRequestHandle(ADOPT, handle),
      (disposed) => native.mln_resource_request_release(disposed, undefined),
    );
    try {
      callback(request, owner);
    } catch (error) {
      owner.close();
      throw error;
    }
  },
  // MLN_DELIVERY_TYPE_RESOURCE_REQUEST_CANCEL
  [5]: (target, _payload) => {
    const callback = target as ResourceRequestCancelHandler;
    callback();
  },
  // MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END
  [6]: (target, payload) => {
    const callback = target as CameraTransitionHandler;
    const [end] = [payload.end] as Parameters<CameraTransitionHandler>;
    callback(end);
  },
  // MLN_DELIVERY_TYPE_CUSTOM_SOURCE_FETCH_TILE
  [7]: (target, payload) => {
    const callback = (target as CustomGeometrySourceOptions).fetchTile;
    if (callback === undefined) return;
    const [tileId] = [payload.tileId] as Parameters<
      NonNullable<CustomGeometrySourceOptions["fetchTile"]>
    >;
    callback(tileId);
  },
  // MLN_DELIVERY_TYPE_CUSTOM_SOURCE_CANCEL_TILE
  [8]: (target, payload) => {
    const callback = (target as CustomGeometrySourceOptions).cancelTile;
    if (callback === undefined) return;
    const [tileId] = [payload.tileId] as Parameters<
      NonNullable<CustomGeometrySourceOptions["cancelTile"]>
    >;
    callback(tileId);
  },
  // MLN_DELIVERY_TYPE_RUNTIME_EVENTS
  [9]: (target, payload) => {
    const callback = target as RuntimeEventHandler;
    const [events] = [payload.events] as Parameters<RuntimeEventHandler>;
    callback(events);
  },
  // MLN_DELIVERY_TYPE_RESOURCE_TRANSFORM
  [10]: (target, payload, index) => {
    const callback = target as ResourceTransform;
    const [kind, url, request] = [
      payload.kind,
      payload.url,
      payload.request,
    ] as Parameters<ResourceTransform>;
    const owner = takeDelivered(
      index,
      request as unknown as bigint,
      (handle) => new ResourceTransformRequestHandle(ADOPT, handle),
      (disposed) =>
        native.mln_resource_transform_request_release(disposed, undefined),
    );
    try {
      callback(kind, url, owner);
    } catch (error) {
      owner.close();
      throw error;
    }
  },
});
