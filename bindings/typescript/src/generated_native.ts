// Generated from the C headers by tools/bindgen. Do not edit.

import type {
  AmbientCacheOperation,
  AnimationOptions,
  BoundOptions,
  CameraDelta,
  CameraFitOptions,
  CameraOptions,
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
  RenderFrameResult,
  RenderSessionAttachOptions,
  RenderSessionCapabilities,
  RenderSessionSnapshot,
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
  SourceFeatureQueryOptions,
  StyleExpressionInfo,
  StyleExpressionInputs,
  StyleExpressionOptions,
  StyleImageOptions,
  StyleTransitionOptions,
  VulkanBorrowedTextureDescriptor,
  VulkanOwnedTextureDescriptor,
  VulkanSurfaceDescriptor,
  VulkanTextureFrame,
} from "./generated_values.ts";

/** The addon function of each generated operation, by C function name. */
export interface NativeOperations {
  mln_acquired_frame_discard(frame: bigint, token: object | undefined): void;
  mln_acquired_frame_get_metal_texture(frame: bigint): MetalTextureFrame;
  mln_acquired_frame_get_opengl_texture(frame: bigint): OpenglTextureFrame;
  mln_acquired_frame_get_producer_sync(frame: bigint): GpuSync;
  mln_acquired_frame_get_result(frame: bigint): RenderFrameResult;
  mln_acquired_frame_get_vulkan_texture(frame: bigint): VulkanTextureFrame;
  mln_acquired_frame_release(
    frame: bigint,
    token: object | undefined,
    consumerCompletion: GpuSync | undefined,
  ): void;
  mln_animation_options_default(): AnimationOptions;
  mln_bound_options_default(): BoundOptions;
  mln_c_version(): number;
  mln_camera_delta_default(): CameraDelta;
  mln_camera_fit_options_default(): CameraFitOptions;
  mln_camera_options_default(): CameraOptions;
  mln_camera_update_default(): CameraUpdate;
  mln_custom_geometry_source_options_default(): CustomGeometrySourceOptions;
  mln_custom_mvt_vector_source_options_default(): CustomMvtVectorSourceOptions;
  mln_frame_demand_default(): FrameDemand;
  mln_free_camera_options_default(): FreeCameraOptions;
  mln_geojson_source_data_create(
    data: Uint8Array,
    options: GeojsonSourceOptions | undefined,
  ): bigint;
  mln_geojson_source_data_release(
    data: bigint,
    token: object | undefined,
  ): void;
  mln_geojson_source_options_default(): GeojsonSourceOptions;
  mln_gpu_sync_default(): GpuSync;
  mln_http_header_rule_default(): HttpHeaderRule;
  mln_lat_lng_for_projected_meters(meters: ProjectedMeters | undefined): LatLng;
  mln_log_clear_callback(): void;
  mln_log_set_async_severity_mask(mask: LogSeverityMask): void;
  mln_log_set_callback(handler: LogHandler): void;
  mln_logical_extent_default(): LogicalExtent;
  mln_logical_extent_physical_size(extent: LogicalExtent | undefined): {
    readonly width: number;
    readonly height: number;
  };
  mln_map_add_custom_geometry_source(
    map: bigint,
    sourceId: string,
    options: CustomGeometrySourceOptions | undefined,
    completion: number | undefined,
  ): void;
  mln_map_add_custom_mvt_vector_source(
    map: bigint,
    sourceId: string,
    options: CustomMvtVectorSourceOptions | undefined,
    completion: number | undefined,
  ): void;
  mln_map_add_geojson_source_data(
    map: bigint,
    sourceId: string,
    data: bigint,
    completion: number | undefined,
  ): void;
  mln_map_add_geojson_source_url(
    map: bigint,
    sourceId: string,
    url: string,
    options: GeojsonSourceOptions | undefined,
    completion: number | undefined,
  ): void;
  mln_map_add_image_source_image(
    map: bigint,
    sourceId: string,
    coordinates: readonly LatLng[],
    image: PremultipliedRgba8Image | undefined,
    completion: number | undefined,
  ): void;
  mln_map_add_image_source_url(
    map: bigint,
    sourceId: string,
    coordinates: readonly LatLng[],
    url: string,
    completion: number | undefined,
  ): void;
  mln_map_add_style_layer_json(
    map: bigint,
    layerJson: string,
    beforeLayerId: string | undefined,
    completion: number | undefined,
  ): void;
  mln_map_add_style_source_json(
    map: bigint,
    sourceId: string,
    sourceJson: string,
    completion: number | undefined,
  ): void;
  mln_map_apply_camera_delta(
    map: bigint,
    delta: CameraDelta | undefined,
    completion: number | undefined,
  ): void;
  mln_map_attach_metal_borrowed_texture(
    map: bigint,
    descriptor: MetalBorrowedTextureDescriptor | undefined,
    options: RenderSessionAttachOptions | undefined,
    completion: number,
  ): bigint;
  mln_map_attach_metal_owned_texture(
    map: bigint,
    descriptor: MetalOwnedTextureDescriptor | undefined,
    options: RenderSessionAttachOptions | undefined,
    completion: number,
  ): bigint;
  mln_map_attach_metal_surface(
    map: bigint,
    descriptor: MetalSurfaceDescriptor | undefined,
    options: RenderSessionAttachOptions | undefined,
    completion: number,
  ): bigint;
  mln_map_attach_opengl_borrowed_texture(
    map: bigint,
    descriptor: OpenglBorrowedTextureDescriptor | undefined,
    options: RenderSessionAttachOptions | undefined,
    completion: number,
  ): bigint;
  mln_map_attach_opengl_owned_texture(
    map: bigint,
    descriptor: OpenglOwnedTextureDescriptor | undefined,
    options: RenderSessionAttachOptions | undefined,
    completion: number,
  ): bigint;
  mln_map_attach_opengl_surface(
    map: bigint,
    descriptor: OpenglSurfaceDescriptor | undefined,
    options: RenderSessionAttachOptions | undefined,
    completion: number,
  ): bigint;
  mln_map_attach_vulkan_borrowed_texture(
    map: bigint,
    descriptor: VulkanBorrowedTextureDescriptor | undefined,
    options: RenderSessionAttachOptions | undefined,
    completion: number,
  ): bigint;
  mln_map_attach_vulkan_owned_texture(
    map: bigint,
    descriptor: VulkanOwnedTextureDescriptor | undefined,
    options: RenderSessionAttachOptions | undefined,
    completion: number,
  ): bigint;
  mln_map_attach_vulkan_surface(
    map: bigint,
    descriptor: VulkanSurfaceDescriptor | undefined,
    options: RenderSessionAttachOptions | undefined,
    completion: number,
  ): bigint;
  mln_map_begin_command_group(
    map: bigint,
    completion: number | undefined,
  ): void;
  mln_map_camera_for_geometry(
    map: bigint,
    geometry: string,
    fitOptions: CameraFitOptions | undefined,
    completion: number,
  ): void;
  mln_map_camera_for_lat_lng_bounds(
    map: bigint,
    bounds: LatLngBounds | undefined,
    fitOptions: CameraFitOptions | undefined,
    completion: number,
  ): void;
  mln_map_camera_for_lat_lngs(
    map: bigint,
    coordinates: readonly LatLng[],
    fitOptions: CameraFitOptions | undefined,
    completion: number,
  ): void;
  mln_map_cancel_camera_transition(
    map: bigint,
    transitionId: bigint,
    completion: number | undefined,
  ): void;
  mln_map_cancel_transitions(map: bigint, completion: number | undefined): void;
  mln_map_dispose(map: bigint, token: object | undefined): void;
  mln_map_dump_debug_logs(map: bigint, completion: number | undefined): void;
  mln_map_end_command_group(map: bigint, completion: number | undefined): void;
  mln_map_get_camera(map: bigint, completion: number): void;
  mln_map_get_feature_state(
    map: bigint,
    selector: FeatureStateSelector,
    completion: number,
  ): void;
  mln_map_get_global_state(map: bigint, completion: number): void;
  mln_map_get_image_source_coordinates(
    map: bigint,
    sourceId: string,
    completion: number,
  ): void;
  mln_map_get_loaded_style_json(map: bigint, completion: number): void;
  mln_map_get_snapshot(map: bigint): MapSnapshot;
  mln_map_get_style_image(
    map: bigint,
    imageId: string,
    completion: number,
  ): void;
  mln_map_get_style_layer(
    map: bigint,
    layerId: string,
    completion: number,
  ): void;
  mln_map_get_style_layer_json(
    map: bigint,
    layerId: string,
    completion: number,
  ): void;
  mln_map_get_style_layer_property(
    map: bigint,
    layerId: string,
    propertyName: string,
    completion: number,
  ): void;
  mln_map_get_style_light_property(
    map: bigint,
    propertyName: string,
    completion: number,
  ): void;
  mln_map_get_style_source(
    map: bigint,
    sourceId: string,
    completion: number,
  ): void;
  mln_map_get_style_transition_options(map: bigint, completion: number): void;
  mln_map_get_style_url(map: bigint, completion: number): void;
  mln_map_invalidate_custom_geometry_source_region(
    map: bigint,
    sourceId: string,
    bounds: LatLngBounds | undefined,
    completion: number | undefined,
  ): void;
  mln_map_invalidate_custom_geometry_source_tile(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    completion: number | undefined,
  ): void;
  mln_map_invalidate_custom_mvt_vector_source_tile(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    completion: number | undefined,
  ): void;
  mln_map_lat_lng_bounds_for_camera(
    map: bigint,
    camera: CameraOptions | undefined,
    completion: number,
  ): void;
  mln_map_lat_lng_bounds_for_camera_unwrapped(
    map: bigint,
    camera: CameraOptions | undefined,
    completion: number,
  ): void;
  mln_map_list_style_layers(map: bigint, completion: number): void;
  mln_map_list_style_sources(map: bigint, completion: number): void;
  mln_map_move_style_layer(
    map: bigint,
    layerId: string,
    beforeLayerId: string | undefined,
    completion: number | undefined,
  ): void;
  mln_map_options_default(): MapOptions;
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
    completion: number,
  ): void;
  mln_map_remove_feature_state(
    map: bigint,
    selector: FeatureStateSelector,
    completion: number | undefined,
  ): void;
  mln_map_remove_style_image(
    map: bigint,
    imageId: string,
    completion: number | undefined,
  ): void;
  mln_map_remove_style_layer(
    map: bigint,
    layerId: string,
    completion: number | undefined,
  ): void;
  mln_map_remove_style_source(
    map: bigint,
    sourceId: string,
    completion: number | undefined,
  ): void;
  mln_map_request_repaint(map: bigint, completion: number | undefined): void;
  mln_map_request_still_image(map: bigint, completion: number): void;
  mln_map_resize(
    map: bigint,
    extent: LogicalExtent | undefined,
    completion: number | undefined,
  ): void;
  mln_map_set_bounds(
    map: bigint,
    options: BoundOptions | undefined,
    completion: number | undefined,
  ): void;
  mln_map_set_custom_geometry_source_tile_data(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    data: string,
    completion: number | undefined,
  ): void;
  mln_map_set_custom_geometry_source_tile_error(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    message: string,
    completion: number | undefined,
  ): void;
  mln_map_set_custom_mvt_vector_source_tile_data(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    data: Uint8Array,
    completion: number | undefined,
  ): void;
  mln_map_set_custom_mvt_vector_source_tile_error(
    map: bigint,
    sourceId: string,
    tileId: CanonicalTileId | undefined,
    message: string,
    completion: number | undefined,
  ): void;
  mln_map_set_debug_options(
    map: bigint,
    options: MapDebugOption,
    completion: number | undefined,
  ): void;
  mln_map_set_event_mask(
    map: bigint,
    mask: RuntimeEventMask,
    completion: number | undefined,
  ): void;
  mln_map_set_feature_state(
    map: bigint,
    selector: FeatureStateSelector,
    state: string,
    completion: number | undefined,
  ): void;
  mln_map_set_free_camera_options(
    map: bigint,
    options: FreeCameraOptions | undefined,
    completion: number | undefined,
  ): void;
  mln_map_set_geojson_source_data(
    map: bigint,
    sourceId: string,
    data: bigint,
    completion: number | undefined,
  ): void;
  mln_map_set_geojson_source_synchronous_tiling(
    map: bigint,
    sourceId: string,
    enabled: boolean,
    completion: number | undefined,
  ): void;
  mln_map_set_geojson_source_url(
    map: bigint,
    sourceId: string,
    url: string,
    completion: number | undefined,
  ): void;
  mln_map_set_global_state_property(
    map: bigint,
    propertyName: string,
    value: string,
    completion: number | undefined,
  ): void;
  mln_map_set_image_source_coordinates(
    map: bigint,
    sourceId: string,
    coordinates: readonly LatLng[],
    completion: number | undefined,
  ): void;
  mln_map_set_image_source_image(
    map: bigint,
    sourceId: string,
    image: PremultipliedRgba8Image | undefined,
    completion: number | undefined,
  ): void;
  mln_map_set_image_source_url(
    map: bigint,
    sourceId: string,
    url: string,
    completion: number | undefined,
  ): void;
  mln_map_set_projection_mode(
    map: bigint,
    mode: ProjectionMode | undefined,
    completion: number | undefined,
  ): void;
  mln_map_set_rendering_stats_view_enabled(
    map: bigint,
    enabled: boolean,
    completion: number | undefined,
  ): void;
  mln_map_set_style_image(
    map: bigint,
    imageId: string,
    image: PremultipliedRgba8Image | undefined,
    options: StyleImageOptions | undefined,
    completion: number | undefined,
  ): void;
  mln_map_set_style_json(
    map: bigint,
    json: string,
    completion: number | undefined,
  ): void;
  mln_map_set_style_layer_property(
    map: bigint,
    layerId: string,
    propertyName: string,
    value: string,
    completion: number | undefined,
  ): void;
  mln_map_set_style_light_json(
    map: bigint,
    lightJson: string,
    completion: number | undefined,
  ): void;
  mln_map_set_style_light_property(
    map: bigint,
    propertyName: string,
    value: string,
    completion: number | undefined,
  ): void;
  mln_map_set_style_source_volatile(
    map: bigint,
    sourceId: string,
    isVolatile: boolean,
    completion: number | undefined,
  ): void;
  mln_map_set_style_transition_options(
    map: bigint,
    options: StyleTransitionOptions | undefined,
    completion: number | undefined,
  ): void;
  mln_map_set_style_url(
    map: bigint,
    url: string,
    completion: number | undefined,
  ): void;
  mln_map_set_tile_options(
    map: bigint,
    options: MapTileOptions | undefined,
    completion: number | undefined,
  ): void;
  mln_map_set_viewport_options(
    map: bigint,
    options: MapViewportOptions | undefined,
    completion: number | undefined,
  ): void;
  mln_map_tile_options_default(): MapTileOptions;
  mln_map_update_camera(
    map: bigint,
    update: CameraUpdate | undefined,
    completion: number | undefined,
  ): void;
  mln_map_viewport_options_default(): MapViewportOptions;
  mln_metal_borrowed_texture_descriptor_default(): MetalBorrowedTextureDescriptor;
  mln_metal_owned_texture_descriptor_default(): MetalOwnedTextureDescriptor;
  mln_metal_surface_descriptor_default(): MetalSurfaceDescriptor;
  mln_network_get_status(): NetworkStatus;
  mln_network_set_status(status: NetworkStatus): void;
  mln_opengl_borrowed_texture_descriptor_default(): OpenglBorrowedTextureDescriptor;
  mln_opengl_owned_texture_descriptor_default(): OpenglOwnedTextureDescriptor;
  mln_opengl_supported_context_provider_mask(): OpenglContextProviderFlag;
  mln_opengl_surface_descriptor_default(): OpenglSurfaceDescriptor;
  mln_plugin_get_register_function_v1(): bigint;
  mln_premultiplied_rgba8_image_default(): PremultipliedRgba8Image;
  mln_projected_meters_for_lat_lng(
    coordinate: LatLng | undefined,
  ): ProjectedMeters;
  mln_projection_mode_default(): ProjectionMode;
  mln_render_session_abandon(session: bigint): RenderAbandonResult;
  mln_render_session_acquire_frame(session: bigint): bigint | undefined;
  mln_render_session_attach_options_default(): RenderSessionAttachOptions;
  mln_render_session_barrier(session: bigint, completion: number): void;
  mln_render_session_clear_data(session: bigint, completion: number): void;
  mln_render_session_detach(session: bigint, completion: number): void;
  mln_render_session_dispose(
    session: bigint,
    token: object | undefined,
    completion: number | undefined,
  ): void;
  mln_render_session_drain_frame_results(
    session: bigint,
  ): readonly RenderFrameResult[] | undefined;
  mln_render_session_dump_debug_logs(session: bigint, completion: number): void;
  mln_render_session_get_capabilities(
    session: bigint,
  ): RenderSessionCapabilities;
  mln_render_session_get_snapshot(session: bigint): RenderSessionSnapshot;
  mln_render_session_query_feature_extensions(
    session: bigint,
    sourceId: string,
    feature: string,
    extension: string,
    extensionField: string,
    argumentsValue: string | undefined,
    completion: number,
  ): void;
  mln_render_session_query_rendered_features(
    session: bigint,
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions | undefined,
    completion: number,
  ): void;
  mln_render_session_query_source_features(
    session: bigint,
    sourceId: string,
    options: SourceFeatureQueryOptions | undefined,
    completion: number,
  ): void;
  mln_render_session_read_texture(session: bigint, completion: number): void;
  mln_render_session_reduce_memory_use(
    session: bigint,
    completion: number,
  ): void;
  mln_render_session_request_frame(
    session: bigint,
    demand: FrameDemand | undefined,
  ): void;
  mln_render_session_resize(
    session: bigint,
    extent: LogicalExtent | undefined,
    completion: number | undefined,
  ): void;
  mln_render_session_service_driver_work(session: bigint): void;
  mln_render_session_set_metal_borrowed_texture_target(
    session: bigint,
    descriptor: MetalBorrowedTextureDescriptor | undefined,
    completion: number,
  ): void;
  mln_render_session_set_metal_surface_target(
    session: bigint,
    descriptor: MetalSurfaceDescriptor | undefined,
    completion: number,
  ): void;
  mln_render_session_set_opengl_borrowed_texture_target(
    session: bigint,
    descriptor: OpenglBorrowedTextureDescriptor | undefined,
    completion: number,
  ): void;
  mln_render_session_set_opengl_surface_target(
    session: bigint,
    descriptor: OpenglSurfaceDescriptor | undefined,
    completion: number,
  ): void;
  mln_render_session_set_vulkan_borrowed_texture_target(
    session: bigint,
    descriptor: VulkanBorrowedTextureDescriptor | undefined,
    completion: number,
  ): void;
  mln_render_session_set_vulkan_surface_target(
    session: bigint,
    descriptor: VulkanSurfaceDescriptor | undefined,
    completion: number,
  ): void;
  mln_rendered_feature_query_options_default(): RenderedFeatureQueryOptions;
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
  mln_resource_route_default(): ResourceRoute;
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
    completion: number | undefined,
  ): void;
  mln_runtime_clear_http_header_transform(
    runtime: bigint,
    completion: number | undefined,
  ): void;
  mln_runtime_clear_resource_provider(
    runtime: bigint,
    completion: number | undefined,
  ): void;
  mln_runtime_clear_resource_transform(
    runtime: bigint,
    completion: number | undefined,
  ): void;
  mln_runtime_create(options: RuntimeOptions | undefined): bigint;
  mln_runtime_create_map(
    runtime: bigint,
    options: MapOptions | undefined,
    completion: number,
  ): bigint;
  mln_runtime_create_offline_region(
    runtime: bigint,
    definition: OfflineRegionDefinition,
    metadata: Uint8Array,
    completion: number,
  ): void;
  mln_runtime_delete_offline_region(
    runtime: bigint,
    regionId: bigint,
    completion: number,
  ): void;
  mln_runtime_dispose(runtime: bigint, token: object | undefined): void;
  mln_runtime_get_event_mask(runtime: bigint): RuntimeEventMask;
  mln_runtime_get_offline_region(
    runtime: bigint,
    regionId: bigint,
    completion: number,
  ): void;
  mln_runtime_get_offline_region_status(
    runtime: bigint,
    regionId: bigint,
    completion: number,
  ): void;
  mln_runtime_invalidate_offline_region(
    runtime: bigint,
    regionId: bigint,
    completion: number,
  ): void;
  mln_runtime_list_offline_regions(runtime: bigint, completion: number): void;
  mln_runtime_merge_offline_regions(
    runtime: bigint,
    sideDatabasePath: string,
    completion: number,
  ): void;
  mln_runtime_options_default(): RuntimeOptions;
  mln_runtime_release(
    runtime: bigint,
    token: object | undefined,
    completion: number,
  ): void;
  mln_runtime_run_ambient_cache_operation(
    runtime: bigint,
    operation: AmbientCacheOperation,
    completion: number,
  ): void;
  mln_runtime_set_event_handler(
    runtime: bigint,
    handler: RuntimeEventHandler | undefined,
    completion: number | undefined,
  ): void;
  mln_runtime_set_event_mask(runtime: bigint, mask: RuntimeEventMask): void;
  mln_runtime_set_http_header_transform(
    runtime: bigint,
    transform: HttpHeaderTransform,
    completion: number | undefined,
  ): void;
  mln_runtime_set_maximum_ambient_cache_size(
    runtime: bigint,
    size: bigint,
    completion: number,
  ): void;
  mln_runtime_set_offline_region_download_state(
    runtime: bigint,
    regionId: bigint,
    state: OfflineRegionDownloadState,
    completion: number,
  ): void;
  mln_runtime_set_offline_region_observed(
    runtime: bigint,
    regionId: bigint,
    observed: boolean,
    completion: number,
  ): void;
  mln_runtime_set_resource_provider(
    runtime: bigint,
    provider: ResourceProvider,
    completion: number | undefined,
  ): void;
  mln_runtime_set_resource_transform(
    runtime: bigint,
    transform: ResourceTransform,
    completion: number | undefined,
  ): void;
  mln_runtime_update_offline_region_metadata(
    runtime: bigint,
    regionId: bigint,
    metadata: Uint8Array,
    completion: number,
  ): void;
  mln_source_feature_query_options_default(): SourceFeatureQueryOptions;
  mln_style_expression_create(
    expression: string,
    options: StyleExpressionOptions | undefined,
  ): bigint;
  mln_style_expression_evaluate(
    expression: bigint,
    inputs: StyleExpressionInputs | undefined,
  ): string;
  mln_style_expression_get_info(expression: bigint): StyleExpressionInfo;
  mln_style_expression_inputs_default(): StyleExpressionInputs;
  mln_style_expression_options_default(): StyleExpressionOptions;
  mln_style_expression_release(
    expression: bigint,
    token: object | undefined,
  ): void;
  mln_style_image_options_default(): StyleImageOptions;
  mln_style_transition_options_default(): StyleTransitionOptions;
  mln_supported_render_backend_mask(): RenderBackendFlag;
  mln_vulkan_borrowed_texture_descriptor_default(): VulkanBorrowedTextureDescriptor;
  mln_vulkan_owned_texture_descriptor_default(): VulkanOwnedTextureDescriptor;
  mln_vulkan_surface_descriptor_default(): VulkanSurfaceDescriptor;
  mln_acquired_frame_view_begin(handle: bigint): bigint;
  mln_acquired_frame_view_end(scope: bigint): void;
}
