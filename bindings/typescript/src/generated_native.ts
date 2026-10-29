// Generated from the C headers by tools/bindgen. Do not edit.

import type {
  AmbientCacheOperation,
  BoundOptions,
  CameraDelta,
  CameraFitOptions,
  CameraOptions,
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
  SourceFeatureQueryOptions,
  StyleExpressionInfo,
  StyleExpressionInputs,
  StyleExpressionOptions,
  StyleImageOptions,
  StyleTransitionOptions,
  StyleVectorTileEncoding,
  VulkanBorrowedTextureDescriptor,
  VulkanOwnedTextureDescriptor,
  VulkanSurfaceDescriptor,
  VulkanTextureFrame,
  WebGpuBorrowedTextureDescriptor,
  WebGpuOwnedTextureDescriptor,
  WebGpuSurfaceDescriptor,
  WebGpuTextureFrame,
} from "./generated_values.ts";
import type { NativeCompletion, NativeThreadStart } from "./internal/addon.ts";

/** The addon function of each generated operation, by C function name. */
export interface NativeOperations {
  mln_acquired_frame_discard(frame: bigint, token: object | undefined): void;
  mln_acquired_frame_get_metal_texture(frame: bigint): MetalTextureFrame;
  mln_acquired_frame_get_open_gl_texture(frame: bigint): OpenGlTextureFrame;
  mln_acquired_frame_get_producer_sync(frame: bigint): GpuSync;
  mln_acquired_frame_get_result(frame: bigint): RenderFrameResult;
  mln_acquired_frame_get_vulkan_texture(frame: bigint): VulkanTextureFrame;
  mln_acquired_frame_get_web_gpu_texture(frame: bigint): WebGpuTextureFrame;
  mln_acquired_frame_release(
    frame: bigint,
    token: object | undefined,
    consumerCompletion: GpuSync | undefined,
  ): void;
  mln_c_version(): number;
  mln_geo_json_source_data_create(
    data: unknown,
    options: GeoJsonSourceOptions | undefined,
  ): bigint;
  mln_geo_json_source_data_release(
    data: bigint,
    token: object | undefined,
  ): void;
  mln_graphics_device_create(
    options: GraphicsDeviceOptions | undefined,
  ): bigint;
  mln_graphics_device_release(device: bigint, token: object | undefined): void;
  mln_lat_lng_for_projected_meters(meters: ProjectedMeters | undefined): LatLng;
  mln_log_clear_callback(): void;
  mln_log_set_async_severity_mask(mask: LogSeverityFlag): void;
  mln_log_set_callback(handler: LogHandler): void;
  mln_logical_extent_physical_size(extent: LogicalExtent | undefined): {
    readonly width: number;
    readonly height: number;
  };
  mln_map_add_custom_geometry_source(
    map: bigint,
    sourceId: string,
    options: CustomGeometrySourceOptions,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_add_custom_vector_source(
    map: bigint,
    sourceId: string,
    options: CustomVectorSourceOptions,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_add_geo_json_source_data(
    map: bigint,
    sourceId: string,
    data: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_add_geo_json_source_url(
    map: bigint,
    sourceId: string,
    url: string,
    options: GeoJsonSourceOptions | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_add_image_source_image(
    map: bigint,
    sourceId: string,
    coordinates: readonly LatLng[],
    image: PremultipliedRgba8Image,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_add_image_source_url(
    map: bigint,
    sourceId: string,
    coordinates: readonly LatLng[],
    url: string,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_add_style_layer_json(
    map: bigint,
    layerJson: unknown,
    beforeLayerId: string,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_add_style_source_json(
    map: bigint,
    sourceId: string,
    sourceJson: unknown,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_apply_camera_delta(
    map: bigint,
    delta: CameraDelta | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_attach_graphics_device_texture(
    map: bigint,
    device: bigint,
    descriptor: GraphicsDeviceTextureDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_metal_borrowed_texture(
    map: bigint,
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_metal_owned_texture(
    map: bigint,
    descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_metal_surface(
    map: bigint,
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_open_gl_borrowed_texture(
    map: bigint,
    descriptor: OpenGlBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_open_gl_owned_texture(
    map: bigint,
    descriptor: OpenGlOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_open_gl_surface(
    map: bigint,
    descriptor: OpenGlSurfaceDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_vulkan_borrowed_texture(
    map: bigint,
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_vulkan_owned_texture(
    map: bigint,
    descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_vulkan_surface(
    map: bigint,
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_web_gpu_borrowed_texture(
    map: bigint,
    descriptor: WebGpuBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_web_gpu_owned_texture(
    map: bigint,
    descriptor: WebGpuOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_attach_web_gpu_surface(
    map: bigint,
    descriptor: WebGpuSurfaceDescriptor,
    options: RenderSessionAttachOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_map_begin_command_group(
    map: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_camera_for_geometry(
    map: bigint,
    geometry: unknown,
    fitOptions: CameraFitOptions | undefined,
    completion: NativeCompletion,
  ): void;
  mln_map_camera_for_lat_lng_bounds(
    map: bigint,
    bounds: LatLngBounds | undefined,
    fitOptions: CameraFitOptions | undefined,
    completion: NativeCompletion,
  ): void;
  mln_map_camera_for_lat_lngs(
    map: bigint,
    coordinates: readonly LatLng[],
    fitOptions: CameraFitOptions | undefined,
    completion: NativeCompletion,
  ): void;
  mln_map_cancel_camera_transition(
    map: bigint,
    transitionId: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_cancel_transitions(
    map: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_dispose(map: bigint, token: object | undefined): void;
  mln_map_dump_debug_logs(
    map: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_end_command_group(
    map: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_get_camera(map: bigint, completion: NativeCompletion): void;
  mln_map_get_feature_state(
    map: bigint,
    selector: FeatureStateSelector,
    completion: NativeCompletion,
  ): void;
  mln_map_get_global_state(map: bigint, completion: NativeCompletion): void;
  mln_map_get_image_source_coordinates(
    map: bigint,
    sourceId: string,
    completion: NativeCompletion,
  ): void;
  mln_map_get_loaded_style_json(
    map: bigint,
    completion: NativeCompletion,
  ): void;
  mln_map_get_snapshot(map: bigint): MapSnapshot;
  mln_map_get_style_image(
    map: bigint,
    imageId: string,
    completion: NativeCompletion,
  ): void;
  mln_map_get_style_layer(
    map: bigint,
    layerId: string,
    completion: NativeCompletion,
  ): void;
  mln_map_get_style_layer_json(
    map: bigint,
    layerId: string,
    completion: NativeCompletion,
  ): void;
  mln_map_get_style_layer_property(
    map: bigint,
    layerId: string,
    propertyName: string,
    completion: NativeCompletion,
  ): void;
  mln_map_get_style_light_property(
    map: bigint,
    propertyName: string,
    completion: NativeCompletion,
  ): void;
  mln_map_get_style_source(
    map: bigint,
    sourceId: string,
    completion: NativeCompletion,
  ): void;
  mln_map_get_style_transition_options(
    map: bigint,
    completion: NativeCompletion,
  ): void;
  mln_map_get_style_url(map: bigint, completion: NativeCompletion): void;
  mln_map_invalidate_custom_geometry_source_region(
    map: bigint,
    sourceId: string,
    bounds: LatLngBounds | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_invalidate_custom_geometry_source_tile(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_invalidate_custom_vector_source_tile(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_lat_lng_bounds_for_camera(
    map: bigint,
    camera: CameraOptions | undefined,
    completion: NativeCompletion,
  ): void;
  mln_map_lat_lng_bounds_for_camera_unwrapped(
    map: bigint,
    camera: CameraOptions | undefined,
    completion: NativeCompletion,
  ): void;
  mln_map_list_style_layers(map: bigint, completion: NativeCompletion): void;
  mln_map_list_style_sources(map: bigint, completion: NativeCompletion): void;
  mln_map_move_style_layer(
    map: bigint,
    layerId: string,
    beforeLayerId: string,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_projection_create(view: MapView | undefined): bigint;
  mln_map_projection_get_camera(projection: bigint): CameraOptions;
  mln_map_projection_lat_lng_for_pixel(
    projection: bigint,
    point: ScreenPoint | undefined,
  ): LatLng;
  mln_map_projection_lat_lng_for_pixel_unwrapped(
    projection: bigint,
    point: ScreenPoint | undefined,
  ): LatLng;
  mln_map_projection_lat_lngs_for_pixels(
    projection: bigint,
    points: readonly ScreenPoint[],
  ): readonly LatLng[];
  mln_map_projection_lat_lngs_for_pixels_unwrapped(
    projection: bigint,
    points: readonly ScreenPoint[],
  ): readonly LatLng[];
  mln_map_projection_meters_per_pixel_at_latitude(
    projection: bigint,
    latitude: number,
  ): number;
  mln_map_projection_pixel_for_lat_lng(
    projection: bigint,
    coordinate: LatLng | undefined,
  ): ScreenPoint;
  mln_map_projection_pixels_for_lat_lngs(
    projection: bigint,
    coordinates: readonly LatLng[],
  ): readonly ScreenPoint[];
  mln_map_projection_release(
    projection: bigint,
    token: object | undefined,
  ): void;
  mln_map_projection_set_camera(
    projection: bigint,
    camera: CameraOptions | undefined,
  ): void;
  mln_map_release(
    map: bigint,
    token: object | undefined,
    completion: NativeCompletion,
  ): void;
  mln_map_remove_feature_state(
    map: bigint,
    selector: FeatureStateSelector,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_remove_style_image(
    map: bigint,
    imageId: string,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_remove_style_layer(
    map: bigint,
    layerId: string,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_remove_style_source(
    map: bigint,
    sourceId: string,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_request_repaint(
    map: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_request_still_image(map: bigint, completion: NativeCompletion): void;
  mln_map_resize(
    map: bigint,
    extent: LogicalExtent | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_bounds(
    map: bigint,
    options: BoundOptions | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_custom_geometry_source_tile_data(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    data: unknown,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_custom_geometry_source_tile_error(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    message: string,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_custom_vector_source_tile_data(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    encoding: StyleVectorTileEncoding,
    data: Uint8Array,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_custom_vector_source_tile_error(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    message: string,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_debug_options(
    map: bigint,
    options: MapDebugFlag,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_event_mask(
    map: bigint,
    mask: RuntimeEventFlag,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_feature_state(
    map: bigint,
    selector: FeatureStateSelector,
    state: unknown,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_free_camera_options(
    map: bigint,
    options: FreeCameraOptions | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_geo_json_source_data(
    map: bigint,
    sourceId: string,
    data: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_geo_json_source_synchronous_tiling(
    map: bigint,
    sourceId: string,
    enabled: boolean,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_geo_json_source_url(
    map: bigint,
    sourceId: string,
    url: string,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_global_state_property(
    map: bigint,
    propertyName: string,
    value: unknown,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_image_source_coordinates(
    map: bigint,
    sourceId: string,
    coordinates: readonly LatLng[],
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_image_source_image(
    map: bigint,
    sourceId: string,
    image: PremultipliedRgba8Image,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_image_source_url(
    map: bigint,
    sourceId: string,
    url: string,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_projection_mode(
    map: bigint,
    mode: ProjectionMode | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_rendering_stats_view_enabled(
    map: bigint,
    enabled: boolean,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_style_image(
    map: bigint,
    imageId: string,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_style_json(
    map: bigint,
    json: unknown,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_style_layer_property(
    map: bigint,
    layerId: string,
    propertyName: string,
    value: unknown,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_style_light_json(
    map: bigint,
    lightJson: unknown,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_style_light_property(
    map: bigint,
    propertyName: string,
    value: unknown,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_style_source_volatile(
    map: bigint,
    sourceId: string,
    isVolatile: boolean,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_style_transition_options(
    map: bigint,
    options: StyleTransitionOptions | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_style_url(
    map: bigint,
    url: string,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_tile_options(
    map: bigint,
    options: MapTileOptions | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_set_viewport_options(
    map: bigint,
    options: MapViewportOptions | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_map_update_camera(
    map: bigint,
    update: CameraUpdate | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_network_get_status(): NetworkStatus;
  mln_network_set_status(status: NetworkStatus): void;
  mln_open_gl_supported_context_provider_mask(): OpenGlContextProviderFlag;
  mln_plugin_get_register_function_v1(): bigint;
  mln_projected_meters_for_lat_lng(
    coordinate: LatLng | undefined,
  ): ProjectedMeters;
  mln_render_session_abandon(session: bigint): RenderAbandonResult;
  mln_render_session_acquire_frame(session: bigint): bigint | undefined;
  mln_render_session_barrier(
    session: bigint,
    completion: NativeCompletion,
  ): void;
  mln_render_session_clear_data(
    session: bigint,
    completion: NativeCompletion,
  ): void;
  mln_render_session_detach(
    session: bigint,
    completion: NativeCompletion,
  ): void;
  mln_render_session_dispose(
    session: bigint,
    token: object | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_render_session_drain_frame_results(
    session: bigint,
  ): readonly RenderFrameResult[] | undefined;
  mln_render_session_dump_debug_logs(
    session: bigint,
    completion: NativeCompletion,
  ): void;
  mln_render_session_get_capabilities(
    session: bigint,
  ): RenderSessionCapabilities;
  mln_render_session_get_failure(session: bigint): void;
  mln_render_session_get_snapshot(session: bigint): RenderSessionSnapshot;
  mln_render_session_query_feature_extensions(
    session: bigint,
    sourceId: string,
    feature: unknown,
    extension: string,
    extensionField: string,
    argumentsValue: unknown,
    completion: NativeCompletion,
  ): void;
  mln_render_session_query_rendered_features(
    session: bigint,
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions | undefined,
    completion: NativeCompletion,
  ): void;
  mln_render_session_query_source_features(
    session: bigint,
    sourceId: string,
    options: SourceFeatureQueryOptions | undefined,
    completion: NativeCompletion,
  ): void;
  mln_render_session_read_texture(
    session: bigint,
    completion: NativeCompletion,
  ): void;
  mln_render_session_reduce_memory_use(
    session: bigint,
    completion: NativeCompletion,
  ): void;
  mln_render_session_request_frame(
    session: bigint,
    demand: FrameDemand | undefined,
  ): void;
  mln_render_session_resize(
    session: bigint,
    extent: LogicalExtent | undefined,
    completion: NativeCompletion | undefined,
  ): void;
  mln_render_session_service_driver_work(session: bigint): void;
  mln_render_session_set_metal_borrowed_texture_target(
    session: bigint,
    descriptor: MetalBorrowedTextureDescriptor,
    completion: NativeCompletion,
  ): void;
  mln_render_session_set_metal_surface_target(
    session: bigint,
    descriptor: MetalSurfaceDescriptor,
    completion: NativeCompletion,
  ): void;
  mln_render_session_set_open_gl_borrowed_texture_target(
    session: bigint,
    descriptor: OpenGlBorrowedTextureDescriptor,
    completion: NativeCompletion,
  ): void;
  mln_render_session_set_open_gl_surface_target(
    session: bigint,
    descriptor: OpenGlSurfaceDescriptor,
    completion: NativeCompletion,
  ): void;
  mln_render_session_set_vulkan_borrowed_texture_target(
    session: bigint,
    descriptor: VulkanBorrowedTextureDescriptor,
    completion: NativeCompletion,
  ): void;
  mln_render_session_set_vulkan_surface_target(
    session: bigint,
    descriptor: VulkanSurfaceDescriptor,
    completion: NativeCompletion,
  ): void;
  mln_render_session_set_web_gpu_borrowed_texture_target(
    session: bigint,
    descriptor: WebGpuBorrowedTextureDescriptor,
    completion: NativeCompletion,
  ): void;
  mln_render_session_set_web_gpu_surface_target(
    session: bigint,
    descriptor: WebGpuSurfaceDescriptor,
    completion: NativeCompletion,
  ): void;
  mln_resource_request_complete(
    handle: bigint,
    token: object | undefined,
    response: ResourceResponse | undefined,
  ): void;
  mln_resource_request_is_cancelled(handle: bigint): boolean;
  mln_resource_request_pass_through(
    handle: bigint,
    token: object | undefined,
  ): void;
  mln_resource_request_release(handle: bigint, token: object | undefined): void;
  mln_resource_request_set_cancel_callback(
    handle: bigint,
    handler: ResourceRequestCancelHandler,
  ): boolean;
  mln_resource_transform_request_complete(
    request: bigint,
    token: object | undefined,
    url: string,
  ): void;
  mln_resource_transform_request_release(
    request: bigint,
    token: object | undefined,
  ): void;
  mln_runtime_clear_event_handler(
    runtime: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_runtime_clear_http_header_transform(
    runtime: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_runtime_clear_resource_provider(
    runtime: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_runtime_clear_resource_transform(
    runtime: bigint,
    completion: NativeCompletion | undefined,
  ): void;
  mln_runtime_create(
    options: RuntimeOptions | undefined,
    start: NativeThreadStart,
  ): void;
  mln_runtime_create_map(
    runtime: bigint,
    options: MapOptions,
    completion: NativeCompletion,
  ): bigint;
  mln_runtime_create_offline_region(
    runtime: bigint,
    definition: OfflineRegionDefinition,
    metadata: Uint8Array,
    completion: NativeCompletion,
  ): void;
  mln_runtime_delete_offline_region(
    runtime: bigint,
    regionId: bigint,
    completion: NativeCompletion,
  ): void;
  mln_runtime_dispose(runtime: bigint, token: object | undefined): void;
  mln_runtime_get_event_mask(runtime: bigint): RuntimeEventFlag;
  mln_runtime_get_offline_region(
    runtime: bigint,
    regionId: bigint,
    completion: NativeCompletion,
  ): void;
  mln_runtime_get_offline_region_status(
    runtime: bigint,
    regionId: bigint,
    completion: NativeCompletion,
  ): void;
  mln_runtime_invalidate_offline_region(
    runtime: bigint,
    regionId: bigint,
    completion: NativeCompletion,
  ): void;
  mln_runtime_list_offline_regions(
    runtime: bigint,
    completion: NativeCompletion,
  ): void;
  mln_runtime_merge_offline_regions(
    runtime: bigint,
    sideDatabasePath: string,
    completion: NativeCompletion,
  ): void;
  mln_runtime_release(
    runtime: bigint,
    token: object | undefined,
    completion: NativeCompletion,
  ): void;
  mln_runtime_run_ambient_cache_operation(
    runtime: bigint,
    operation: AmbientCacheOperation,
    completion: NativeCompletion,
  ): void;
  mln_runtime_set_event_handler(
    runtime: bigint,
    handler: RuntimeEventHandler,
    completion: NativeCompletion | undefined,
  ): void;
  mln_runtime_set_event_mask(runtime: bigint, mask: RuntimeEventFlag): void;
  mln_runtime_set_http_header_transform(
    runtime: bigint,
    transform: HttpHeaderTransform,
    completion: NativeCompletion | undefined,
  ): void;
  mln_runtime_set_maximum_ambient_cache_size(
    runtime: bigint,
    size: bigint,
    completion: NativeCompletion,
  ): void;
  mln_runtime_set_offline_region_download_state(
    runtime: bigint,
    regionId: bigint,
    state: OfflineRegionDownloadState,
    completion: NativeCompletion,
  ): void;
  mln_runtime_set_offline_region_observed(
    runtime: bigint,
    regionId: bigint,
    observed: boolean,
    completion: NativeCompletion,
  ): void;
  mln_runtime_set_resource_provider(
    runtime: bigint,
    provider: ResourceProvider,
    completion: NativeCompletion | undefined,
  ): void;
  mln_runtime_set_resource_transform(
    runtime: bigint,
    transform: ResourceTransform,
    completion: NativeCompletion | undefined,
  ): void;
  mln_runtime_update_offline_region_metadata(
    runtime: bigint,
    regionId: bigint,
    metadata: Uint8Array,
    completion: NativeCompletion,
  ): void;
  mln_style_expression_create(
    expression: unknown,
    options: StyleExpressionOptions | undefined,
  ): bigint;
  mln_style_expression_evaluate(
    expression: bigint,
    inputs: StyleExpressionInputs | undefined,
  ): unknown;
  mln_style_expression_get_info(expression: bigint): StyleExpressionInfo;
  mln_style_expression_release(
    expression: bigint,
    token: object | undefined,
  ): void;
  mln_supported_render_backend_mask(): RenderBackendFlag;
  mln_acquired_frame_view_begin(handle: bigint): bigint;
  mln_acquired_frame_view_end(scope: bigint): void;
}

/** The C ABI contract version of the headers this binding was generated from. */
export const MLN_C_VERSION = 0;
