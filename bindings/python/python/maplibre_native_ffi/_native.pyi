from collections.abc import Callable, Sequence
from concurrent.futures import Future
from typing import Any

# Hand-maintained: PyO3's generated stubs only support inline Rust modules, and
# this module uses a function-style #[pymodule].
# https://pyo3.rs/main/python-typing-hints

type _Point = tuple[float, float]
type _Vec3 = tuple[float, float, float]
type _Quat = tuple[float, float, float, float]
type _Insets = tuple[float, float, float, float]
type _Bounds = tuple[_Point, _Point]
type _Animation = tuple[
    float | None, float | None, float | None, _Insets | None, int | None
]
type _WireDict = dict[str, Any]

class _RuntimeHandle:
    @property
    def closed(self) -> bool: ...
    def close(self) -> None: ...
    def barrier(self) -> Future[None]: ...
    def run_ambient_cache_operation(self, operation: int) -> Future[None]: ...
    def set_maximum_ambient_cache_size(self, size: int) -> Future[None]: ...
    def create_offline_region(
        self, definition: object, metadata: bytes
    ) -> Future[_WireDict]: ...
    def get_offline_region(self, region_id: int) -> Future[_WireDict | None]: ...
    def list_offline_regions(self) -> Future[list[_WireDict]]: ...
    def merge_offline_regions_database(
        self, side_database_path: str
    ) -> Future[list[_WireDict]]: ...
    def update_offline_region_metadata(
        self, region_id: int, metadata: bytes
    ) -> Future[_WireDict]: ...
    def get_offline_region_status(self, region_id: int) -> Future[_WireDict]: ...
    def set_offline_region_observed(
        self, region_id: int, observed: bool
    ) -> Future[None]: ...
    def set_offline_region_download_state(
        self, region_id: int, state: int
    ) -> Future[None]: ...
    def invalidate_offline_region(self, region_id: int) -> Future[None]: ...
    def delete_offline_region(self, region_id: int) -> Future[None]: ...
    def set_resource_provider(
        self,
        callback: Callable[[_WireDict, _ResourceRequestHandle], int],
        max_pending_callbacks: int,
    ) -> Future[_WireDict]: ...
    def clear_resource_provider(self) -> Future[_WireDict]: ...
    def set_resource_transform(
        self, callback: Callable[[_WireDict], str | None], max_pending_callbacks: int
    ) -> Future[_WireDict]: ...
    def clear_resource_transform(self) -> Future[_WireDict]: ...
    def set_http_header_transform(
        self,
        callback: Callable[[_WireDict], Sequence[_WireDict]],
        max_pending_callbacks: int,
    ) -> Future[_WireDict]: ...
    def clear_http_header_transform(self) -> Future[_WireDict]: ...
    def drain_events(self) -> _WireDict: ...
    def set_event_mask(self, mask: int) -> None: ...
    def get_event_mask(self) -> int: ...

class _MapHandle:
    @property
    def closed(self) -> bool: ...
    def close(self) -> None: ...
    def id(self) -> int: ...
    def request_repaint(self) -> int: ...
    def request_still_image(self) -> Future[_WireDict]: ...
    def dump_debug_logs(self) -> int: ...
    def set_event_mask(self, mask: int) -> int: ...
    def get_event_mask(self) -> int: ...
    def set_debug_options(self, options: int) -> int: ...
    def set_rendering_stats_view_enabled(self, enabled: bool) -> int: ...
    def get_size(self) -> tuple[int, int, float]: ...
    def resize(self, width: int, height: int, scale_factor: float) -> int: ...
    def snapshot(self) -> _WireDict: ...
    def set_viewport_options(
        self,
        north_orientation: int | None,
        constrain_mode: int | None,
        viewport_mode: int | None,
        frustum_offset: _Insets | None,
    ) -> int: ...
    def set_tile_options(
        self,
        prefetch_zoom_delta: int | None,
        lod_min_radius: float | None,
        lod_scale: float | None,
        lod_pitch_threshold: float | None,
        lod_zoom_shift: float | None,
        lod_mode: int | None,
    ) -> int: ...
    def create_projection(self) -> _MapProjectionHandle: ...
    def set_style_url(self, url: str) -> int: ...
    def set_style_json(self, json: bytes) -> int: ...
    def set_feature_state(
        self,
        source_id: str,
        source_layer_id: str | None,
        feature_id: str | None,
        state_key: str | None,
        state_value: bytes,
    ) -> int: ...
    def get_feature_state(
        self,
        source_id: str,
        source_layer_id: str | None,
        feature_id: str | None,
        state_key: str | None,
    ) -> bytes: ...
    def remove_feature_state(
        self,
        source_id: str,
        source_layer_id: str | None,
        feature_id: str | None,
        state_key: str | None,
    ) -> int: ...
    def copy_loaded_style_json(self) -> bytes: ...
    def copy_style_url(self) -> str: ...
    def get_camera(self) -> _WireDict: ...
    def get_camera_ordered(self) -> _WireDict: ...
    def set_camera(
        self,
        center: _Point | None,
        zoom: float | None,
        bearing: float | None,
        pitch: float | None,
        center_altitude: float | None,
        padding: _Insets | None,
        anchor: _Point | None,
        roll: float | None,
        field_of_view: float | None,
    ) -> int: ...
    def set_visible_coordinates(
        self, coordinates: Sequence[_Point], padding: _Insets
    ) -> int: ...
    def set_visible_geometry(self, geometry: bytes, padding: _Insets) -> int: ...
    def jump_to(
        self,
        center: _Point | None,
        zoom: float | None,
        bearing: float | None,
        pitch: float | None,
        center_altitude: float | None,
        padding: _Insets | None,
        anchor: _Point | None,
        roll: float | None,
        field_of_view: float | None,
    ) -> int: ...
    def ease_to(
        self,
        center: _Point | None,
        zoom: float | None,
        bearing: float | None,
        pitch: float | None,
        center_altitude: float | None,
        padding: _Insets | None,
        anchor: _Point | None,
        roll: float | None,
        field_of_view: float | None,
        animation: _Animation | None,
    ) -> int: ...
    def fly_to(
        self,
        center: _Point | None,
        zoom: float | None,
        bearing: float | None,
        pitch: float | None,
        center_altitude: float | None,
        padding: _Insets | None,
        anchor: _Point | None,
        roll: float | None,
        field_of_view: float | None,
        animation: _Animation | None,
    ) -> int: ...
    def apply_camera_delta(
        self,
        kind: int,
        offset: _Point,
        amount: float,
        anchor: _Point | None,
        animation: _Animation | None,
    ) -> int: ...
    def camera_for_lat_lng_bounds(
        self,
        southwest: _Point,
        northeast: _Point,
        fit_padding: _Insets | None,
        fit_bearing: float | None,
        fit_pitch: float | None,
    ) -> _WireDict: ...
    def camera_for_lat_lngs(
        self,
        coordinates: Sequence[_Point],
        fit_padding: _Insets | None,
        fit_bearing: float | None,
        fit_pitch: float | None,
    ) -> _WireDict: ...
    def camera_for_geometry(
        self,
        geometry: bytes,
        fit_padding: _Insets | None,
        fit_bearing: float | None,
        fit_pitch: float | None,
    ) -> _WireDict: ...
    def lat_lng_bounds_for_camera(
        self,
        center: _Point | None,
        zoom: float | None,
        bearing: float | None,
        pitch: float | None,
        center_altitude: float | None,
        padding: _Insets | None,
        anchor: _Point | None,
        roll: float | None,
        field_of_view: float | None,
        unwrapped: bool,
    ) -> _WireDict: ...
    def set_bounds(
        self,
        bounds: _Bounds | None,
        unbounded: bool,
        min_zoom: float | None,
        max_zoom: float | None,
        min_pitch: float | None,
        max_pitch: float | None,
    ) -> int: ...
    def set_free_camera_options(
        self, position: _Vec3 | None, orientation: _Quat | None
    ) -> int: ...
    def set_projection_mode(
        self, axonometric: bool | None, x_skew: float | None, y_skew: float | None
    ) -> int: ...
    def pixel_for_lat_lng(self, latitude: float, longitude: float) -> _WireDict: ...
    def lat_lng_for_pixel(self, x: float, y: float) -> _WireDict: ...
    def pixels_for_lat_lngs(self, coordinates: Sequence[_Point]) -> list[_WireDict]: ...
    def lat_lngs_for_pixels(self, points: Sequence[_Point]) -> list[_WireDict]: ...
    def add_style_source_json(self, source_id: str, source_json: bytes) -> int: ...
    def add_geojson_source_url(
        self,
        source_id: str,
        url: str,
        min_zoom: float | None,
        max_zoom: float | None,
        tolerance: float | None,
        cluster_max_zoom: float | None,
        cluster_properties: bytes | None,
        tile_size: int | None,
        buffer: int | None,
        cluster_radius: int | None,
        cluster_min_points: int | None,
        line_metrics: bool | None,
        cluster: bool | None,
        synchronous_tiling: bool | None,
    ) -> int: ...
    def add_geojson_source_data(
        self, source_id: str, data: _GeoJsonSourceDataHandle
    ) -> int: ...
    def set_geojson_source_url(self, source_id: str, url: str) -> int: ...
    def set_geojson_source_data(
        self, source_id: str, data: _GeoJsonSourceDataHandle
    ) -> int: ...
    def set_geojson_source_synchronous_tiling(
        self, source_id: str, enabled: bool
    ) -> int: ...
    def add_vector_source_url(
        self,
        source_id: str,
        url: str,
        min_zoom: float | None,
        max_zoom: float | None,
        attribution: str | None,
        scheme: int | None,
        bounds: _Bounds | None,
        tile_size: int | None,
        vector_encoding: int | None,
        raster_dem_encoding: int | None,
    ) -> int: ...
    def add_raster_source_url(
        self,
        source_id: str,
        url: str,
        min_zoom: float | None,
        max_zoom: float | None,
        attribution: str | None,
        scheme: int | None,
        bounds: _Bounds | None,
        tile_size: int | None,
        vector_encoding: int | None,
        raster_dem_encoding: int | None,
    ) -> int: ...
    def add_raster_dem_source_url(
        self,
        source_id: str,
        url: str,
        min_zoom: float | None,
        max_zoom: float | None,
        attribution: str | None,
        scheme: int | None,
        bounds: _Bounds | None,
        tile_size: int | None,
        vector_encoding: int | None,
        raster_dem_encoding: int | None,
    ) -> int: ...
    def add_vector_source_tiles(
        self,
        source_id: str,
        tiles: Sequence[str],
        min_zoom: float | None,
        max_zoom: float | None,
        attribution: str | None,
        scheme: int | None,
        bounds: _Bounds | None,
        tile_size: int | None,
        vector_encoding: int | None,
        raster_dem_encoding: int | None,
    ) -> int: ...
    def add_raster_source_tiles(
        self,
        source_id: str,
        tiles: Sequence[str],
        min_zoom: float | None,
        max_zoom: float | None,
        attribution: str | None,
        scheme: int | None,
        bounds: _Bounds | None,
        tile_size: int | None,
        vector_encoding: int | None,
        raster_dem_encoding: int | None,
    ) -> int: ...
    def add_raster_dem_source_tiles(
        self,
        source_id: str,
        tiles: Sequence[str],
        min_zoom: float | None,
        max_zoom: float | None,
        attribution: str | None,
        scheme: int | None,
        bounds: _Bounds | None,
        tile_size: int | None,
        vector_encoding: int | None,
        raster_dem_encoding: int | None,
    ) -> int: ...
    def remove_style_source(self, source_id: str) -> int: ...
    def get_style_source_info(self, source_id: str) -> _WireDict | None: ...
    def list_style_source_ids(self) -> list[str]: ...
    def add_hillshade_layer(
        self, layer_id: str, source_id: str, before_layer_id: str | None
    ) -> int: ...
    def add_color_relief_layer(
        self, layer_id: str, source_id: str, before_layer_id: str | None
    ) -> int: ...
    def add_location_indicator_layer(
        self, layer_id: str, before_layer_id: str | None
    ) -> int: ...
    def set_location_indicator_location(
        self, layer_id: str, latitude: float, longitude: float, altitude: float
    ) -> int: ...
    def set_location_indicator_bearing(self, layer_id: str, bearing: float) -> int: ...
    def set_location_indicator_accuracy_radius(
        self, layer_id: str, radius: float
    ) -> int: ...
    def set_location_indicator_image_name(
        self, layer_id: str, image_kind: int, image_id: str
    ) -> int: ...
    def add_style_layer_json(
        self, layer_json: bytes, before_layer_id: str | None
    ) -> int: ...
    def get_style_layer_json(self, layer_id: str) -> bytes | None: ...
    def set_style_light_json(self, light_json: bytes) -> int: ...
    def set_style_light_property(self, property_name: str, value: bytes) -> int: ...
    def get_style_light_property(self, property_name: str) -> bytes | None: ...
    def set_style_transition_options(
        self,
        duration_ms: float | None,
        delay_ms: float | None,
        enable_placement_transitions: bool | None,
    ) -> int: ...
    def get_style_transition_options(self) -> _WireDict: ...
    def set_layer_property(
        self, layer_id: str, property_name: str, value: bytes
    ) -> int: ...
    def get_layer_property(self, layer_id: str, property_name: str) -> bytes | None: ...
    def set_layer_source_layer(self, layer_id: str, source_layer: str) -> int: ...
    def copy_layer_source_layer(self, layer_id: str) -> str: ...
    def set_layer_source_id(self, layer_id: str, source_id: str) -> int: ...
    def copy_layer_source_id(self, layer_id: str) -> str: ...
    def set_layer_min_zoom(self, layer_id: str, min_zoom: float) -> int: ...
    def set_layer_max_zoom(self, layer_id: str, max_zoom: float) -> int: ...
    def set_layer_visibility(self, layer_id: str, visibility: int) -> int: ...
    def set_layer_filter(self, layer_id: str, filter: bytes | None) -> int: ...
    def get_layer_filter(self, layer_id: str) -> bytes | None: ...
    def remove_style_layer(self, layer_id: str) -> int: ...
    def get_style_layer_info(self, layer_id: str) -> _WireDict | None: ...
    def list_style_layer_ids(self) -> list[str]: ...
    def move_style_layer(self, layer_id: str, before_layer_id: str | None) -> int: ...
    def set_style_image(
        self,
        image_id: str,
        width: int,
        height: int,
        stride: int,
        pixels: bytes,
        pixel_ratio: float | None,
        sdf: bool | None,
        stretch_x: list[tuple[float, float]] | None,
        stretch_y: list[tuple[float, float]] | None,
        content: tuple[float, float, float, float] | None,
        text_fit_width: int | None,
        text_fit_height: int | None,
    ) -> int: ...
    def copy_style_image_stretches(
        self, image_id: str
    ) -> tuple[list[tuple[float, float]], list[tuple[float, float]]] | None: ...
    def remove_style_image(self, image_id: str) -> int: ...
    def get_style_image_info(self, image_id: str) -> _WireDict | None: ...
    def copy_style_image_premultiplied_rgba8(
        self, image_id: str
    ) -> _WireDict | None: ...
    def add_image_source_url(
        self, source_id: str, coordinates: Sequence[_Point], url: str
    ) -> int: ...
    def add_image_source_image(
        self,
        source_id: str,
        coordinates: Sequence[_Point],
        width: int,
        height: int,
        stride: int,
        pixels: bytes,
    ) -> int: ...
    def set_image_source_url(self, source_id: str, url: str) -> int: ...
    def set_image_source_image(
        self, source_id: str, width: int, height: int, stride: int, pixels: bytes
    ) -> int: ...
    def set_image_source_coordinates(
        self, source_id: str, coordinates: Sequence[_Point]
    ) -> int: ...
    def get_image_source_coordinates(
        self, source_id: str
    ) -> list[_WireDict] | None: ...
    def add_custom_geometry_source(
        self,
        source_id: str,
        max_queued_events: int,
        min_zoom: float | None,
        max_zoom: float | None,
        tolerance: float | None,
        tile_size: int | None,
        buffer: int | None,
        clip: bool | None,
        wrap: bool | None,
        has_cancel_tile: bool,
    ) -> tuple[_CustomGeometrySourceHandle, int]: ...
    def set_custom_geometry_source_tile_data(
        self, source_id: str, z: int, x: int, y: int, data: bytes
    ) -> int: ...
    def invalidate_custom_geometry_source_tile(
        self, source_id: str, z: int, x: int, y: int
    ) -> int: ...
    def invalidate_custom_geometry_source_region(
        self, source_id: str, southwest: _Point, northeast: _Point
    ) -> int: ...
    def add_custom_mvt_vector_source(
        self,
        source_id: str,
        max_queued_events: int,
        min_zoom: float | None,
        max_zoom: float | None,
        has_cancel_tile: bool,
    ) -> tuple[_CustomMvtVectorSourceHandle, int]: ...
    def set_custom_mvt_vector_source_tile_data(
        self, source_id: str, z: int, x: int, y: int, data: bytes
    ) -> int: ...
    def set_custom_mvt_vector_source_tile_error(
        self, source_id: str, z: int, x: int, y: int, message: str
    ) -> int: ...
    def invalidate_custom_mvt_vector_source_tile(
        self, source_id: str, z: int, x: int, y: int
    ) -> int: ...

class _MapProjectionHandle:
    @property
    def closed(self) -> bool: ...
    def close(self) -> None: ...
    def get_camera(self) -> _WireDict: ...
    def set_camera(
        self,
        center: _Point | None,
        zoom: float | None,
        bearing: float | None,
        pitch: float | None,
        center_altitude: float | None,
        padding: _Insets | None,
        anchor: _Point | None,
        roll: float | None,
        field_of_view: float | None,
    ) -> None: ...
    def set_visible_coordinates(
        self, coordinates: Sequence[_Point], padding: _Insets
    ) -> None: ...
    def set_visible_geometry(self, geometry: bytes, padding: _Insets) -> None: ...
    def pixel_for_lat_lng(self, latitude: float, longitude: float) -> _WireDict: ...
    def lat_lng_for_pixel(self, x: float, y: float) -> _WireDict: ...

class _GeoJsonSourceDataHandle:
    @property
    def closed(self) -> bool: ...
    def close(self) -> None: ...

class _RenderSessionHandle:
    @property
    def closed(self) -> bool: ...
    def close(self) -> None: ...
    def capabilities(self) -> _WireDict: ...
    def snapshot(self) -> _WireDict: ...
    def request_frame(
        self,
        flags: int,
        token: int,
        coalescing_boundary: int,
        timeout_ns: int,
    ) -> None: ...
    def drain_frame_results(self) -> list[_WireDict]: ...
    def service_driver_work(self, max_work: int) -> int: ...
    def resize(self, width: int, height: int, scale_factor: float) -> Future[None]: ...
    def barrier(self) -> Future[None]: ...
    def detach(self) -> Future[None]: ...
    def abandon(self) -> _WireDict: ...
    def reduce_memory_use(self) -> Future[None]: ...
    def clear_data(self) -> Future[None]: ...
    def dump_debug_logs(self) -> Future[None]: ...
    def set_metal_surface_target(
        self,
        width: int,
        height: int,
        scale_factor: float,
        device_address: int,
        layer_address: int,
    ) -> Future[None]: ...
    def set_vulkan_surface_target(
        self,
        width: int,
        height: int,
        scale_factor: float,
        instance_address: int,
        physical_device_address: int,
        device_address: int,
        graphics_queue_address: int,
        graphics_queue_family_index: int,
        get_instance_proc_addr: int,
        get_device_proc_addr: int,
        surface_address: int,
    ) -> Future[None]: ...
    def set_webgpu_surface_target(
        self,
        width: int,
        height: int,
        scale_factor: float,
        instance_address: int,
        device_address: int,
        queue_address: int,
        surface_address: int,
        format: int,
    ) -> Future[None]: ...
    def set_opengl_surface_target(
        self,
        width: int,
        height: int,
        scale_factor: float,
        context_platform: int,
        context_ownership: int,
        context_address_1: int,
        context_address_2: int,
        share_context_address: int,
        client_api: int,
        get_proc_address: int,
        surface_address: int,
    ) -> Future[None]: ...
    def set_metal_borrowed_texture_target(
        self,
        width: int,
        height: int,
        scale_factor: float,
        physical_width: int,
        physical_height: int,
        texture_address: int,
    ) -> Future[None]: ...
    def set_vulkan_borrowed_texture_target(
        self,
        width: int,
        height: int,
        scale_factor: float,
        physical_width: int,
        physical_height: int,
        instance_address: int,
        physical_device_address: int,
        device_address: int,
        graphics_queue_address: int,
        graphics_queue_family_index: int,
        get_instance_proc_addr: int,
        get_device_proc_addr: int,
        image_address: int,
        image_view_address: int,
        format: int,
        initial_layout: int,
        final_layout: int,
    ) -> Future[None]: ...
    def set_webgpu_borrowed_texture_target(
        self,
        width: int,
        height: int,
        scale_factor: float,
        physical_width: int,
        physical_height: int,
        instance_address: int,
        device_address: int,
        queue_address: int,
        texture_address: int,
        texture_view_address: int,
        format: int,
    ) -> Future[None]: ...
    def set_opengl_borrowed_texture_target(
        self,
        width: int,
        height: int,
        scale_factor: float,
        physical_width: int,
        physical_height: int,
        context_platform: int,
        context_ownership: int,
        context_address_1: int,
        context_address_2: int,
        share_context_address: int,
        client_api: int,
        get_proc_address: int,
        texture: int,
        target: int,
    ) -> Future[None]: ...
    def query_rendered_features(
        self, geometry: object, layer_ids: Sequence[str] | None, filter: bytes | None
    ) -> Future[list[_WireDict]]: ...
    def query_source_features(
        self,
        source_id: str,
        source_layer_ids: Sequence[str] | None,
        filter: bytes | None,
    ) -> Future[list[_WireDict]]: ...
    def query_feature_extensions(
        self,
        source_id: str,
        feature: bytes,
        extension: str,
        extension_field: str,
        arguments: bytes | None,
    ) -> Future[bytes]: ...
    def read_premultiplied_rgba8(self) -> Future[_WireDict]: ...
    def acquire_metal_owned_texture_frame(self) -> _MetalOwnedTextureFrameHandle: ...
    def acquire_vulkan_owned_texture_frame(self) -> _VulkanOwnedTextureFrameHandle: ...
    def acquire_opengl_owned_texture_frame(self) -> _OpenGLOwnedTextureFrameHandle: ...
    def acquire_webgpu_owned_texture_frame(self) -> _WebGPUOwnedTextureFrameHandle: ...

class _MetalOwnedTextureFrameHandle:
    @property
    def closed(self) -> bool: ...
    def release(self, kind: int, object_address: int, value: int) -> None: ...
    def producer_sync(self) -> _WireDict: ...
    def result(self) -> _WireDict: ...
    def frame(self) -> _WireDict: ...
    def texture_address(self) -> int: ...
    def device_address(self) -> int: ...

class _VulkanOwnedTextureFrameHandle:
    @property
    def closed(self) -> bool: ...
    def release(self, kind: int, object_address: int, value: int) -> None: ...
    def producer_sync(self) -> _WireDict: ...
    def result(self) -> _WireDict: ...
    def frame(self) -> _WireDict: ...
    def image_address(self) -> int: ...
    def image_view_address(self) -> int: ...
    def device_address(self) -> int: ...

class _WebGPUOwnedTextureFrameHandle:
    @property
    def closed(self) -> bool: ...
    def release(self, kind: int, object_address: int, value: int) -> None: ...
    def producer_sync(self) -> _WireDict: ...
    def result(self) -> _WireDict: ...
    def frame(self) -> _WireDict: ...
    def texture_address(self) -> int: ...
    def texture_view_address(self) -> int: ...
    def device_address(self) -> int: ...

class _OpenGLOwnedTextureFrameHandle:
    @property
    def closed(self) -> bool: ...
    def release(self, kind: int, object_address: int, value: int) -> None: ...
    def producer_sync(self) -> _WireDict: ...
    def result(self) -> _WireDict: ...
    def frame(self) -> _WireDict: ...
    def texture(self) -> int: ...

class _ResourceRequestHandle:
    def validate_completion_response(self, response: object) -> None: ...
    def complete(self, response: object) -> None: ...
    def is_cancelled(self) -> bool: ...
    def close(self) -> None: ...

class _LogReceiver:
    @property
    def dropped_record_count(self) -> int: ...
    def poll_record(self) -> _WireDict | None: ...

class _CustomGeometrySourceHandle:
    @property
    def closed(self) -> bool: ...
    @property
    def dropped_event_count(self) -> int: ...
    def close(self) -> None: ...
    def poll_event(self) -> _WireDict | None: ...
    def push_fetch_for_test(self, z: int, x: int, y: int) -> None: ...
    def push_cancel_for_test(self, z: int, x: int, y: int) -> None: ...

class _CustomMvtVectorSourceHandle:
    @property
    def closed(self) -> bool: ...
    @property
    def dropped_event_count(self) -> int: ...
    def close(self) -> None: ...
    def poll_event(self) -> _WireDict | None: ...
    def push_fetch_for_test(self, z: int, x: int, y: int) -> None: ...
    def push_cancel_for_test(self, z: int, x: int, y: int) -> None: ...

def runtime_options_default_event_mask() -> int: ...
def map_options_default_event_mask() -> int: ...
def expected_c_abi_version() -> int: ...
def c_version() -> int: ...
def supported_render_backends_raw() -> int: ...
def supported_opengl_context_providers_raw() -> int: ...
def render_target_extent_physical_size(
    width: int, height: int, scale_factor: float
) -> tuple[int, int]: ...
def create_geojson_source_data(
    data: bytes,
    min_zoom: float | None,
    max_zoom: float | None,
    tolerance: float | None,
    cluster_max_zoom: float | None,
    cluster_properties: bytes | None,
    tile_size: int | None,
    buffer: int | None,
    cluster_radius: int | None,
    cluster_min_points: int | None,
    line_metrics: bool | None,
    cluster: bool | None,
    synchronous_tiling: bool | None,
) -> _GeoJsonSourceDataHandle: ...
def network_status_raw() -> int: ...
def set_network_status_raw(raw_status: int) -> None: ...
def set_network_status_raw_unchecked_for_test(raw_status: int) -> None: ...
def map_size_by_id_for_test(id: int) -> tuple[int, int, float]: ...
def status_error_for_test(raw_status: int, diagnostic: str) -> None: ...
def status_error_after_support_call_for_test(
    raw_status: int, diagnostic: str
) -> None: ...
def create_runtime_with_abi_version_for_test(
    actual_abi_version: int,
    asset_path: str | None,
    cache_path: str | None,
) -> _RuntimeHandle: ...
def synthetic_runtime_event_batch_for_test() -> _WireDict: ...
def runtime_event_stride_for_test(runtime: _RuntimeHandle) -> tuple[int, int]: ...
def projected_meters_for_lat_lng(latitude: float, longitude: float) -> _WireDict: ...
def lat_lng_for_projected_meters(northing: float, easting: float) -> _WireDict: ...
def set_log_callback(max_queued_records: int, consume: bool) -> _LogReceiver: ...
def clear_log_callback() -> None: ...
def set_async_log_severity_mask(mask: int) -> None: ...
def create_runtime(
    asset_path: str | None, cache_path: str | None, event_mask: int
) -> _RuntimeHandle: ...
def create_map(
    runtime: _RuntimeHandle,
    width: int | None,
    height: int | None,
    scale_factor: float | None,
    map_mode: int | None,
    fast_pfor_enabled: bool | None,
    event_mask: int,
) -> _MapHandle: ...
def attach_metal_surface(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    device_address: int,
    layer_address: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
def attach_vulkan_surface(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    instance_address: int,
    physical_device_address: int,
    device_address: int,
    graphics_queue_address: int,
    graphics_queue_family_index: int,
    get_instance_proc_addr: int,
    get_device_proc_addr: int,
    surface_address: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
def attach_metal_owned_texture(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    device_address: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
def attach_metal_borrowed_texture(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    physical_width: int,
    physical_height: int,
    texture_address: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
def attach_vulkan_owned_texture(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    instance_address: int,
    physical_device_address: int,
    device_address: int,
    graphics_queue_address: int,
    graphics_queue_family_index: int,
    get_instance_proc_addr: int,
    get_device_proc_addr: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
def attach_vulkan_borrowed_texture(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    physical_width: int,
    physical_height: int,
    instance_address: int,
    physical_device_address: int,
    device_address: int,
    graphics_queue_address: int,
    graphics_queue_family_index: int,
    get_instance_proc_addr: int,
    get_device_proc_addr: int,
    image_address: int,
    image_view_address: int,
    format: int,
    initial_layout: int,
    final_layout: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
def attach_webgpu_surface(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    instance_address: int,
    device_address: int,
    queue_address: int,
    surface_address: int,
    format: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
def attach_webgpu_owned_texture(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    instance_address: int,
    device_address: int,
    queue_address: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
def attach_webgpu_borrowed_texture(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    physical_width: int,
    physical_height: int,
    instance_address: int,
    device_address: int,
    queue_address: int,
    texture_address: int,
    texture_view_address: int,
    format: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
def attach_opengl_surface(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    context_platform: int,
    context_ownership: int,
    context_address_1: int,
    context_address_2: int,
    share_context_address: int,
    client_api: int,
    get_proc_address: int,
    surface_address: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
def attach_opengl_owned_texture(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    context_platform: int,
    context_ownership: int,
    context_address_1: int,
    context_address_2: int,
    share_context_address: int,
    client_api: int,
    get_proc_address: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
def attach_opengl_borrowed_texture(
    map: _MapHandle,
    width: int,
    height: int,
    scale_factor: float,
    physical_width: int,
    physical_height: int,
    context_platform: int,
    context_ownership: int,
    context_address_1: int,
    context_address_2: int,
    share_context_address: int,
    client_api: int,
    get_proc_address: int,
    texture: int,
    target: int,
    driver: int,
    texture_ring_depth: int,
) -> tuple[_RenderSessionHandle, int]: ...
