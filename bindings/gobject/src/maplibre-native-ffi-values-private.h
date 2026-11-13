// Generated from the C headers by tools/bindgen. Do not edit.
#ifndef MAPLIBRE_NATIVE_FFI_VALUES_PRIVATE_H
#define MAPLIBRE_NATIVE_FFI_VALUES_PRIVATE_H

#include <maplibre_native_c.h>

#include "maplibre-native-ffi-private.h"
#include "maplibre-native-ffi-values.h"

G_BEGIN_DECLS

G_GNUC_INTERNAL void maplibre_native_ffi_screen_point_to_native(
  const MaplibreNativeFfiScreenPoint* self, mln_screen_point* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiScreenPoint*
maplibre_native_ffi_screen_point_from_native(const mln_screen_point* in);
G_GNUC_INTERNAL void maplibre_native_ffi_screen_point_read(
  MaplibreNativeFfiScreenPoint* out, const mln_screen_point* in
);
G_GNUC_INTERNAL gpointer
maplibre_native_ffi_screen_point_copy_func(gconstpointer self, gpointer data);
G_GNUC_INTERNAL void maplibre_native_ffi_unit_bezier_to_native(
  const MaplibreNativeFfiUnitBezier* self, mln_unit_bezier* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_momentum_decay_to_native(
  const MaplibreNativeFfiMomentumDecay* self, mln_momentum_decay* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiMomentumDecay*
maplibre_native_ffi_momentum_decay_copy(
  const MaplibreNativeFfiMomentumDecay* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_momentum_snap_to_native(
  const MaplibreNativeFfiMomentumSnap* self, mln_momentum_snap* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiMomentumSnap*
maplibre_native_ffi_momentum_snap_copy(
  const MaplibreNativeFfiMomentumSnap* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_anchored_momentum_to_native(
  const MaplibreNativeFfiAnchoredMomentum* self, mln_anchored_momentum* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiAnchoredMomentum*
maplibre_native_ffi_anchored_momentum_copy(
  const MaplibreNativeFfiAnchoredMomentum* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_transition_handler_to_native(
  const MaplibreNativeFfiCameraTransitionHandler* self,
  mln_camera_transition_handler* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_animation_options_to_native(
  const MaplibreNativeFfiAnimationOptions* self, mln_animation_options* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_animation_options_copy(
  const MaplibreNativeFfiAnimationOptions* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_lat_lng_to_native(
  const MaplibreNativeFfiLatLng* self, mln_lat_lng* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiLatLng*
maplibre_native_ffi_lat_lng_from_native(const mln_lat_lng* in);
G_GNUC_INTERNAL void maplibre_native_ffi_lat_lng_read(
  MaplibreNativeFfiLatLng* out, const mln_lat_lng* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_lat_lng_bounds_to_native(
  const MaplibreNativeFfiLatLngBounds* self, mln_lat_lng_bounds* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiLatLngBounds*
maplibre_native_ffi_lat_lng_bounds_from_native(const mln_lat_lng_bounds* in);
G_GNUC_INTERNAL void maplibre_native_ffi_lat_lng_bounds_read(
  MaplibreNativeFfiLatLngBounds* out, const mln_lat_lng_bounds* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_bound_options_to_native(
  const MaplibreNativeFfiBoundOptions* self, mln_bound_options* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_edge_insets_to_native(
  const MaplibreNativeFfiEdgeInsets* self, mln_edge_insets* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiEdgeInsets*
maplibre_native_ffi_edge_insets_from_native(const mln_edge_insets* in);
G_GNUC_INTERNAL void maplibre_native_ffi_edge_insets_read(
  MaplibreNativeFfiEdgeInsets* out, const mln_edge_insets* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_to_native(
  const MaplibreNativeFfiCamera* self, mln_camera* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiCamera* maplibre_native_ffi_camera_from_native(
  const mln_camera* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_read(
  MaplibreNativeFfiCamera* out, const mln_camera* in
);
G_GNUC_INTERNAL MaplibreNativeFfiCameraBounds*
maplibre_native_ffi_camera_bounds_from_native(const mln_camera_bounds* in);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_bounds_read(
  MaplibreNativeFfiCameraBounds* out, const mln_camera_bounds* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_pan_momentum_to_native(
  const MaplibreNativeFfiPanMomentum* self, mln_pan_momentum* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiPanMomentum*
maplibre_native_ffi_pan_momentum_copy(const MaplibreNativeFfiPanMomentum* self);
G_GNUC_INTERNAL void maplibre_native_ffi_gesture_momentum_to_native(
  const MaplibreNativeFfiGestureMomentum* self, mln_gesture_momentum* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiGestureMomentum*
maplibre_native_ffi_gesture_momentum_copy(
  const MaplibreNativeFfiGestureMomentum* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_delta_to_native(
  const MaplibreNativeFfiCameraDelta* self, mln_camera_delta* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_fit_options_to_native(
  const MaplibreNativeFfiCameraFitOptions* self, mln_camera_fit_options* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiCameraFitOptions*
maplibre_native_ffi_camera_fit_options_copy(
  const MaplibreNativeFfiCameraFitOptions* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_fit_update_to_native(
  const MaplibreNativeFfiCameraFitUpdate* self, mln_camera_fit_update* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_options_to_native(
  const MaplibreNativeFfiCameraOptions* self, mln_camera_options* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiCameraOptions*
maplibre_native_ffi_camera_options_from_native(const mln_camera_options* in);
G_GNUC_INTERNAL MaplibreNativeFfiCameraOptions*
maplibre_native_ffi_camera_options_copy(
  const MaplibreNativeFfiCameraOptions* self
);
G_GNUC_INTERNAL MaplibreNativeFfiCameraQueryResult*
maplibre_native_ffi_camera_query_result_from_native(
  const mln_camera_query_result* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_query_result_read(
  MaplibreNativeFfiCameraQueryResult* out, const mln_camera_query_result* in
);
G_GNUC_INTERNAL MaplibreNativeFfiCameraVelocity*
maplibre_native_ffi_camera_velocity_from_native(const mln_camera_velocity* in);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_velocity_read(
  MaplibreNativeFfiCameraVelocity* out, const mln_camera_velocity* in
);
G_GNUC_INTERNAL MaplibreNativeFfiCameraTransitionEnd*
maplibre_native_ffi_camera_transition_end_from_native(
  const mln_camera_transition_end* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_transition_end_read(
  MaplibreNativeFfiCameraTransitionEnd* out, const mln_camera_transition_end* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_camera_update_to_native(
  const MaplibreNativeFfiCameraUpdate* self, mln_camera_update* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_canonical_tile_id_to_native(
  const MaplibreNativeFfiCanonicalTileId* self, mln_canonical_tile_id* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiCanonicalTileId*
maplibre_native_ffi_canonical_tile_id_from_native(
  const mln_canonical_tile_id* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_canonical_tile_id_read(
  MaplibreNativeFfiCanonicalTileId* out, const mln_canonical_tile_id* in
);
G_GNUC_INTERNAL void
maplibre_native_ffi_custom_geometry_source_options_to_native(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self,
  mln_custom_geometry_source_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_custom_vector_source_options_to_native(
  const MaplibreNativeFfiCustomVectorSourceOptions* self,
  mln_custom_vector_source_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_dma_buf_format_to_native(
  const MaplibreNativeFfiDmaBufFormat* self, mln_dma_buf_format* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL gpointer
maplibre_native_ffi_dma_buf_format_copy_func(gconstpointer self, gpointer data);
G_GNUC_INTERNAL MaplibreNativeFfiDmaBufPlane*
maplibre_native_ffi_dma_buf_plane_from_native(const mln_dma_buf_plane* in);
G_GNUC_INTERNAL void maplibre_native_ffi_dma_buf_plane_read(
  MaplibreNativeFfiDmaBufPlane* out, const mln_dma_buf_plane* in
);
G_GNUC_INTERNAL gpointer
maplibre_native_ffi_dma_buf_plane_copy_func(gconstpointer self, gpointer data);
G_GNUC_INTERNAL MaplibreNativeFfiDmaBufFrame*
maplibre_native_ffi_dma_buf_frame_from_native(const mln_dma_buf_frame* in);
G_GNUC_INTERNAL void maplibre_native_ffi_logical_size_to_native(
  const MaplibreNativeFfiLogicalSize* self, mln_logical_size* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_dma_buf_ring_descriptor_to_native(
  const MaplibreNativeFfiDmaBufRingDescriptor* self,
  mln_dma_buf_ring_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_egl_context_descriptor_to_native(
  const MaplibreNativeFfiEglContextDescriptor* self,
  mln_egl_context_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiEglContextDescriptor*
maplibre_native_ffi_egl_context_descriptor_copy(
  const MaplibreNativeFfiEglContextDescriptor* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_feature_id_to_native(
  const MaplibreNativeFfiFeatureId* self, mln_feature_id* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_feature_state_selector_to_native(
  const MaplibreNativeFfiFeatureStateSelector* self,
  mln_feature_state_selector* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_frame_demand_to_native(
  const MaplibreNativeFfiFrameDemand* self, mln_frame_demand* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_frame_pacer_options_to_native(
  const MaplibreNativeFfiFramePacerOptions* self, mln_frame_pacer_options* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_vec3_to_native(
  const MaplibreNativeFfiVec3* self, mln_vec3* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiVec3* maplibre_native_ffi_vec3_from_native(
  const mln_vec3* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_vec3_read(
  MaplibreNativeFfiVec3* out, const mln_vec3* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_quaternion_to_native(
  const MaplibreNativeFfiQuaternion* self, mln_quaternion* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiQuaternion*
maplibre_native_ffi_quaternion_from_native(const mln_quaternion* in);
G_GNUC_INTERNAL void maplibre_native_ffi_quaternion_read(
  MaplibreNativeFfiQuaternion* out, const mln_quaternion* in
);
G_GNUC_INTERNAL MaplibreNativeFfiFreeCamera*
maplibre_native_ffi_free_camera_from_native(const mln_free_camera* in);
G_GNUC_INTERNAL void maplibre_native_ffi_free_camera_read(
  MaplibreNativeFfiFreeCamera* out, const mln_free_camera* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_free_camera_options_to_native(
  const MaplibreNativeFfiFreeCameraOptions* self, mln_free_camera_options* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_geo_json_source_options_to_native(
  const MaplibreNativeFfiGeoJsonSourceOptions* self,
  mln_geo_json_source_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_geo_json_source_options_from_native(
  const mln_geo_json_source_options* in
);
G_GNUC_INTERNAL MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_geo_json_source_options_copy(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_gpu_sync_to_native(
  const MaplibreNativeFfiGpuSync* self, mln_gpu_sync* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiGpuSync*
maplibre_native_ffi_gpu_sync_from_native(const mln_gpu_sync* in);
G_GNUC_INTERNAL void maplibre_native_ffi_gpu_sync_read(
  MaplibreNativeFfiGpuSync* out, const mln_gpu_sync* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_http_header_to_native(
  const MaplibreNativeFfiHttpHeader* self, mln_http_header* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL gpointer
maplibre_native_ffi_http_header_copy_func(gconstpointer self, gpointer data);
G_GNUC_INTERNAL MaplibreNativeFfiHttpHeader*
maplibre_native_ffi_http_header_copy(const MaplibreNativeFfiHttpHeader* self);
G_GNUC_INTERNAL void maplibre_native_ffi_http_header_rule_to_native(
  const MaplibreNativeFfiHttpHeaderRule* self, mln_http_header_rule* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL gpointer maplibre_native_ffi_http_header_rule_copy_func(
  gconstpointer self, gpointer data
);
G_GNUC_INTERNAL MaplibreNativeFfiHttpHeaderRule*
maplibre_native_ffi_http_header_rule_copy(
  const MaplibreNativeFfiHttpHeaderRule* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_http_header_transform_to_native(
  const MaplibreNativeFfiHttpHeaderTransform* self,
  mln_http_header_transform* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_image_content_to_native(
  const MaplibreNativeFfiImageContent* self, mln_image_content* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiImageContent*
maplibre_native_ffi_image_content_from_native(const mln_image_content* in);
G_GNUC_INTERNAL void maplibre_native_ffi_image_content_read(
  MaplibreNativeFfiImageContent* out, const mln_image_content* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_image_stretch_to_native(
  const MaplibreNativeFfiImageStretch* self, mln_image_stretch* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiImageStretch*
maplibre_native_ffi_image_stretch_from_native(const mln_image_stretch* in);
G_GNUC_INTERNAL void maplibre_native_ffi_image_stretch_read(
  MaplibreNativeFfiImageStretch* out, const mln_image_stretch* in
);
G_GNUC_INTERNAL gpointer
maplibre_native_ffi_image_stretch_copy_func(gconstpointer self, gpointer data);
G_GNUC_INTERNAL MaplibreNativeFfiIoSurfaceFrame*
maplibre_native_ffi_io_surface_frame_from_native(
  const mln_io_surface_frame* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_metal_context_descriptor_to_native(
  const MaplibreNativeFfiMetalContextDescriptor* self,
  mln_metal_context_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_context_descriptor_copy(
  const MaplibreNativeFfiMetalContextDescriptor* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_io_surface_ring_descriptor_to_native(
  const MaplibreNativeFfiIoSurfaceRingDescriptor* self,
  mln_io_surface_ring_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_log_handler_to_native(
  const MaplibreNativeFfiLogHandler* self, mln_log_handler* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_logical_extent_to_native(
  const MaplibreNativeFfiLogicalExtent* self, mln_logical_extent* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiLogicalExtent*
maplibre_native_ffi_logical_extent_from_native(const mln_logical_extent* in);
G_GNUC_INTERNAL void maplibre_native_ffi_logical_extent_read(
  MaplibreNativeFfiLogicalExtent* out, const mln_logical_extent* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_map_options_to_native(
  const MaplibreNativeFfiMapOptions* self, mln_map_options* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiMapProjectionCameraResult*
maplibre_native_ffi_map_projection_camera_result_from_native(
  const mln_map_projection_camera_result* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_map_projection_camera_result_read(
  MaplibreNativeFfiMapProjectionCameraResult* out,
  const mln_map_projection_camera_result* in
);
G_GNUC_INTERNAL MaplibreNativeFfiMapTileSettings*
maplibre_native_ffi_map_tile_settings_from_native(
  const mln_map_tile_settings* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_map_tile_settings_read(
  MaplibreNativeFfiMapTileSettings* out, const mln_map_tile_settings* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_projection_mode_to_native(
  const MaplibreNativeFfiProjectionMode* self, mln_projection_mode* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiProjectionMode*
maplibre_native_ffi_projection_mode_from_native(const mln_projection_mode* in);
G_GNUC_INTERNAL void maplibre_native_ffi_projection_mode_read(
  MaplibreNativeFfiProjectionMode* out, const mln_projection_mode* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_map_viewport_to_native(
  const MaplibreNativeFfiMapViewport* self, mln_map_viewport* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiMapViewport*
maplibre_native_ffi_map_viewport_from_native(const mln_map_viewport* in);
G_GNUC_INTERNAL void maplibre_native_ffi_map_viewport_read(
  MaplibreNativeFfiMapViewport* out, const mln_map_viewport* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_projection_definition_to_native(
  const MaplibreNativeFfiProjectionDefinition* self,
  mln_projection_definition* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiProjectionDefinition*
maplibre_native_ffi_projection_definition_from_native(
  const mln_projection_definition* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_projection_definition_read(
  MaplibreNativeFfiProjectionDefinition* out,
  const mln_projection_definition* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_map_view_to_native(
  const MaplibreNativeFfiMapView* self, mln_map_view* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiMapView*
maplibre_native_ffi_map_view_from_native(const mln_map_view* in);
G_GNUC_INTERNAL void maplibre_native_ffi_map_view_read(
  MaplibreNativeFfiMapView* out, const mln_map_view* in
);
G_GNUC_INTERNAL MaplibreNativeFfiMapSnapshot*
maplibre_native_ffi_map_snapshot_from_native(const mln_map_snapshot* in);
G_GNUC_INTERNAL void maplibre_native_ffi_map_snapshot_read(
  MaplibreNativeFfiMapSnapshot* out, const mln_map_snapshot* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_map_tile_options_to_native(
  const MaplibreNativeFfiMapTileOptions* self, mln_map_tile_options* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_map_viewport_options_to_native(
  const MaplibreNativeFfiMapViewportOptions* self,
  mln_map_viewport_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_metal_borrowed_texture_to_native(
  const MaplibreNativeFfiMetalBorrowedTexture* self,
  mln_metal_borrowed_texture* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL gpointer maplibre_native_ffi_metal_borrowed_texture_copy_func(
  gconstpointer self, gpointer data
);
G_GNUC_INTERNAL MaplibreNativeFfiMetalBorrowedTexture*
maplibre_native_ffi_metal_borrowed_texture_copy(
  const MaplibreNativeFfiMetalBorrowedTexture* self
);
G_GNUC_INTERNAL void
maplibre_native_ffi_metal_borrowed_texture_descriptor_to_native(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self,
  mln_metal_borrowed_texture_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void
maplibre_native_ffi_metal_borrowed_texture_replacement_to_native(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self,
  mln_metal_borrowed_texture_replacement* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_metal_surface_descriptor_to_native(
  const MaplibreNativeFfiMetalSurfaceDescriptor* self,
  mln_metal_surface_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_metal_surface_replacement_to_native(
  const MaplibreNativeFfiMetalSurfaceReplacement* self,
  mln_metal_surface_replacement* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiMetalTextureFrame*
maplibre_native_ffi_metal_texture_frame_from_native(
  const mln_metal_texture_frame* in
);
G_GNUC_INTERNAL void
maplibre_native_ffi_offline_geometry_region_definition_to_native(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self,
  mln_offline_geometry_region_definition* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiOfflineGeometryRegionDefinition*
maplibre_native_ffi_offline_geometry_region_definition_from_native(
  const mln_offline_geometry_region_definition* in
);
G_GNUC_INTERNAL MaplibreNativeFfiOfflineGeometryRegionDefinition*
maplibre_native_ffi_offline_geometry_region_definition_copy(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);
G_GNUC_INTERNAL void
maplibre_native_ffi_offline_tile_pyramid_region_definition_to_native(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self,
  mln_offline_tile_pyramid_region_definition* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiOfflineTilePyramidRegionDefinition*
maplibre_native_ffi_offline_tile_pyramid_region_definition_from_native(
  const mln_offline_tile_pyramid_region_definition* in
);
G_GNUC_INTERNAL MaplibreNativeFfiOfflineTilePyramidRegionDefinition*
maplibre_native_ffi_offline_tile_pyramid_region_definition_copy(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_offline_region_definition_to_native(
  const MaplibreNativeFfiOfflineRegionDefinition* self,
  mln_offline_region_definition* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiOfflineRegionDefinition*
maplibre_native_ffi_offline_region_definition_from_native(
  const mln_offline_region_definition* in
);
G_GNUC_INTERNAL MaplibreNativeFfiOfflineRegionInfo*
maplibre_native_ffi_offline_region_info_from_native(
  const mln_offline_region_info* in
);
G_GNUC_INTERNAL MaplibreNativeFfiOfflineRegionStatus*
maplibre_native_ffi_offline_region_status_from_native(
  const mln_offline_region_status* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_offline_region_status_read(
  MaplibreNativeFfiOfflineRegionStatus* out, const mln_offline_region_status* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_offscreen_descriptor_to_native(
  const MaplibreNativeFfiOffscreenDescriptor* self,
  mln_offscreen_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_offscreen_device_options_to_native(
  const MaplibreNativeFfiOffscreenDeviceOptions* self,
  mln_offscreen_device_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_open_gl_borrowed_texture_to_native(
  const MaplibreNativeFfiOpenGlBorrowedTexture* self,
  mln_open_gl_borrowed_texture* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL gpointer maplibre_native_ffi_open_gl_borrowed_texture_copy_func(
  gconstpointer self, gpointer data
);
G_GNUC_INTERNAL void maplibre_native_ffi_wgl_context_descriptor_to_native(
  const MaplibreNativeFfiWglContextDescriptor* self,
  mln_wgl_context_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiWglContextDescriptor*
maplibre_native_ffi_wgl_context_descriptor_copy(
  const MaplibreNativeFfiWglContextDescriptor* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_web_gl_context_descriptor_to_native(
  const MaplibreNativeFfiWebGlContextDescriptor* self,
  mln_web_gl_context_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void
maplibre_native_ffi_open_gl_context_descriptor_data_to_native(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self,
  mln_open_gl_context_platform* out_tag,
  mln_open_gl_context_descriptor_data* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_open_gl_context_descriptor_to_native(
  const MaplibreNativeFfiOpenGlContextDescriptor* self,
  mln_open_gl_context_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_copy(
  const MaplibreNativeFfiOpenGlContextDescriptor* self
);
G_GNUC_INTERNAL void
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_to_native(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  mln_open_gl_borrowed_texture_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void
maplibre_native_ffi_open_gl_borrowed_texture_replacement_to_native(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self,
  mln_open_gl_borrowed_texture_replacement* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_open_gl_surface_descriptor_to_native(
  const MaplibreNativeFfiOpenGlSurfaceDescriptor* self,
  mln_open_gl_surface_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_open_gl_surface_replacement_to_native(
  const MaplibreNativeFfiOpenGlSurfaceReplacement* self,
  mln_open_gl_surface_replacement* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiOpenGlTextureFrame*
maplibre_native_ffi_open_gl_texture_frame_from_native(
  const mln_open_gl_texture_frame* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_open_gl_texture_frame_read(
  MaplibreNativeFfiOpenGlTextureFrame* out, const mln_open_gl_texture_frame* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_premultiplied_rgba8_image_to_native(
  const MaplibreNativeFfiPremultipliedRgba8Image* self,
  mln_premultiplied_rgba8_image* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_projected_meters_to_native(
  const MaplibreNativeFfiProjectedMeters* self, mln_projected_meters* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiProjectedMeters*
maplibre_native_ffi_projected_meters_from_native(
  const mln_projected_meters* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_projected_meters_read(
  MaplibreNativeFfiProjectedMeters* out, const mln_projected_meters* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_projection_mode_options_to_native(
  const MaplibreNativeFfiProjectionModeOptions* self,
  mln_projection_mode_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_queue_lock_to_native(
  const MaplibreNativeFfiQueueLock* self, mln_queue_lock* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiRenderAbandonResult*
maplibre_native_ffi_render_abandon_result_from_native(
  const mln_render_abandon_result* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_render_abandon_result_read(
  MaplibreNativeFfiRenderAbandonResult* out, const mln_render_abandon_result* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRenderFrameResult*
maplibre_native_ffi_render_frame_result_from_native(
  const mln_render_frame_result* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_render_frame_result_read(
  MaplibreNativeFfiRenderFrameResult* out, const mln_render_frame_result* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_wake_to_native(
  const MaplibreNativeFfiWake* self, mln_wake* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void
maplibre_native_ffi_render_session_attach_options_to_native(
  const MaplibreNativeFfiRenderSessionAttachOptions* self,
  mln_render_session_attach_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiRenderSessionCapabilities*
maplibre_native_ffi_render_session_capabilities_from_native(
  const mln_render_session_capabilities* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_render_session_capabilities_read(
  MaplibreNativeFfiRenderSessionCapabilities* out,
  const mln_render_session_capabilities* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRenderSessionSnapshot*
maplibre_native_ffi_render_session_snapshot_from_native(
  const mln_render_session_snapshot* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_render_session_snapshot_read(
  MaplibreNativeFfiRenderSessionSnapshot* out,
  const mln_render_session_snapshot* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRenderedFeature*
maplibre_native_ffi_rendered_feature_from_native(
  const mln_rendered_feature* in
);
G_GNUC_INTERNAL void
maplibre_native_ffi_rendered_feature_query_options_to_native(
  const MaplibreNativeFfiRenderedFeatureQueryOptions* self,
  mln_rendered_feature_query_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_screen_box_to_native(
  const MaplibreNativeFfiScreenBox* self, mln_screen_box* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_screen_line_string_to_native(
  const MaplibreNativeFfiScreenLineString* self, mln_screen_line_string* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiScreenLineString*
maplibre_native_ffi_screen_line_string_copy(
  const MaplibreNativeFfiScreenLineString* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_rendered_query_geometry_to_native(
  const MaplibreNativeFfiRenderedQueryGeometry* self,
  mln_rendered_query_geometry* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiRenderingStats*
maplibre_native_ffi_rendering_stats_from_native(const mln_rendering_stats* in);
G_GNUC_INTERNAL void maplibre_native_ffi_rendering_stats_read(
  MaplibreNativeFfiRenderingStats* out, const mln_rendering_stats* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_resource_route_to_native(
  const MaplibreNativeFfiResourceRoute* self, mln_resource_route* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL gpointer
maplibre_native_ffi_resource_route_copy_func(gconstpointer self, gpointer data);
G_GNUC_INTERNAL MaplibreNativeFfiResourceRoute*
maplibre_native_ffi_resource_route_copy(
  const MaplibreNativeFfiResourceRoute* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_resource_provider_to_native(
  const MaplibreNativeFfiResourceProvider* self, mln_resource_provider* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiResourceRange*
maplibre_native_ffi_resource_range_from_native(const mln_resource_range* in);
G_GNUC_INTERNAL void maplibre_native_ffi_resource_range_read(
  MaplibreNativeFfiResourceRange* out, const mln_resource_range* in
);
G_GNUC_INTERNAL void
maplibre_native_ffi_resource_request_cancel_handler_to_native(
  const MaplibreNativeFfiResourceRequestCancelHandler* self,
  mln_resource_request_cancel_handler* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiResourceRequestInfo*
maplibre_native_ffi_resource_request_info_from_native(
  const mln_resource_request_info* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_resource_response_to_native(
  const MaplibreNativeFfiResourceResponse* self, mln_resource_response* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_url_query_parameter_to_native(
  const MaplibreNativeFfiUrlQueryParameter* self, mln_url_query_parameter* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL gpointer maplibre_native_ffi_url_query_parameter_copy_func(
  gconstpointer self, gpointer data
);
G_GNUC_INTERNAL MaplibreNativeFfiUrlQueryParameter*
maplibre_native_ffi_url_query_parameter_copy(
  const MaplibreNativeFfiUrlQueryParameter* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_resource_transform_rule_to_native(
  const MaplibreNativeFfiResourceTransformRule* self,
  mln_resource_transform_rule* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL gpointer maplibre_native_ffi_resource_transform_rule_copy_func(
  gconstpointer self, gpointer data
);
G_GNUC_INTERNAL MaplibreNativeFfiResourceTransformRule*
maplibre_native_ffi_resource_transform_rule_copy(
  const MaplibreNativeFfiResourceTransformRule* self
);
G_GNUC_INTERNAL void maplibre_native_ffi_resource_transform_to_native(
  const MaplibreNativeFfiResourceTransform* self, mln_resource_transform* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_resource_transform_error_to_native(
  const MaplibreNativeFfiResourceTransformError* self,
  mln_resource_transform_error* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventCameraChange*
maplibre_native_ffi_runtime_event_camera_change_from_native(
  const mln_runtime_event_camera_change* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_runtime_event_camera_change_read(
  MaplibreNativeFfiRuntimeEventCameraChange* out,
  const mln_runtime_event_camera_change* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventLoadingFailed*
maplibre_native_ffi_runtime_event_loading_failed_from_native(
  const mln_runtime_event_loading_failed* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_runtime_event_loading_failed_read(
  MaplibreNativeFfiRuntimeEventLoadingFailed* out,
  const mln_runtime_event_loading_failed* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventRenderFrame*
maplibre_native_ffi_runtime_event_render_frame_from_native(
  const mln_runtime_event_render_frame* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_runtime_event_render_frame_read(
  MaplibreNativeFfiRuntimeEventRenderFrame* out,
  const mln_runtime_event_render_frame* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventStyleImage*
maplibre_native_ffi_runtime_event_style_image_from_native(
  const mln_runtime_event_style_image* in
);
G_GNUC_INTERNAL MaplibreNativeFfiTileId*
maplibre_native_ffi_tile_id_from_native(const mln_tile_id* in);
G_GNUC_INTERNAL void maplibre_native_ffi_tile_id_read(
  MaplibreNativeFfiTileId* out, const mln_tile_id* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventTileAction*
maplibre_native_ffi_runtime_event_tile_action_from_native(
  const mln_runtime_event_tile_action* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventSourceChanged*
maplibre_native_ffi_runtime_event_source_changed_from_native(
  const mln_runtime_event_source_changed* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventOfflineRegionStatus*
maplibre_native_ffi_runtime_event_offline_region_status_from_native(
  const mln_runtime_event_offline_region_status* in
);
G_GNUC_INTERNAL void
maplibre_native_ffi_runtime_event_offline_region_status_read(
  MaplibreNativeFfiRuntimeEventOfflineRegionStatus* out,
  const mln_runtime_event_offline_region_status* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventOfflineRegionResponseError*
maplibre_native_ffi_runtime_event_offline_region_response_error_from_native(
  const mln_runtime_event_offline_region_response_error* in
);
G_GNUC_INTERNAL void
maplibre_native_ffi_runtime_event_offline_region_response_error_read(
  MaplibreNativeFfiRuntimeEventOfflineRegionResponseError* out,
  const mln_runtime_event_offline_region_response_error* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit*
maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_from_native(
  const mln_runtime_event_offline_region_tile_count_limit* in
);
G_GNUC_INTERNAL void
maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_read(
  MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit* out,
  const mln_runtime_event_offline_region_tile_count_limit* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventGlyphs*
maplibre_native_ffi_runtime_event_glyphs_from_native(
  const mln_runtime_event_glyphs* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventSprite*
maplibre_native_ffi_runtime_event_sprite_from_native(
  const mln_runtime_event_sprite* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEventPayload*
maplibre_native_ffi_runtime_event_payload_from_native(
  gint64 tag, const mln_runtime_event_payload* in
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntimeEvent*
maplibre_native_ffi_runtime_event_from_native(const mln_runtime_event* in);
G_GNUC_INTERNAL void maplibre_native_ffi_runtime_event_handler_to_native(
  const MaplibreNativeFfiRuntimeEventHandler* self,
  mln_runtime_event_handler* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_runtime_options_to_native(
  const MaplibreNativeFfiRuntimeOptions* self, mln_runtime_options* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiSourceFeature*
maplibre_native_ffi_source_feature_from_native(const mln_source_feature* in);
G_GNUC_INTERNAL void maplibre_native_ffi_source_feature_query_options_to_native(
  const MaplibreNativeFfiSourceFeatureQueryOptions* self,
  mln_source_feature_query_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiStyleExpressionInfo*
maplibre_native_ffi_style_expression_info_from_native(
  const mln_style_expression_info* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_style_expression_info_read(
  MaplibreNativeFfiStyleExpressionInfo* out, const mln_style_expression_info* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_style_expression_inputs_to_native(
  const MaplibreNativeFfiStyleExpressionInputs* self,
  mln_style_expression_inputs* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_style_expression_type_to_native(
  const MaplibreNativeFfiStyleExpressionType* self,
  mln_style_expression_type* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_style_expression_options_to_native(
  const MaplibreNativeFfiStyleExpressionOptions* self,
  mln_style_expression_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiStyleImageInfo*
maplibre_native_ffi_style_image_info_from_native(
  const mln_style_image_info* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_style_image_options_to_native(
  const MaplibreNativeFfiStyleImageOptions* self, mln_style_image_options* out,
  MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiStyleLayerInfo*
maplibre_native_ffi_style_layer_info_from_native(
  const mln_style_layer_info* in
);
G_GNUC_INTERNAL MaplibreNativeFfiStyleSourceTileInfo*
maplibre_native_ffi_style_source_tile_info_from_native(
  const mln_style_source_tile_info* in
);
G_GNUC_INTERNAL MaplibreNativeFfiStyleSourceInfo*
maplibre_native_ffi_style_source_info_from_native(
  const mln_style_source_info* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_style_transition_options_to_native(
  const MaplibreNativeFfiStyleTransitionOptions* self,
  mln_style_transition_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiStyleTransitionOptions*
maplibre_native_ffi_style_transition_options_from_native(
  const mln_style_transition_options* in
);
G_GNUC_INTERNAL MaplibreNativeFfiTextureImageInfo*
maplibre_native_ffi_texture_image_info_from_native(
  const mln_texture_image_info* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_texture_image_info_read(
  MaplibreNativeFfiTextureImageInfo* out, const mln_texture_image_info* in
);
G_GNUC_INTERNAL MaplibreNativeFfiTextureReadbackResult*
maplibre_native_ffi_texture_readback_result_from_native(
  const mln_texture_readback_result* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_vulkan_borrowed_texture_to_native(
  const MaplibreNativeFfiVulkanBorrowedTexture* self,
  mln_vulkan_borrowed_texture* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL gpointer maplibre_native_ffi_vulkan_borrowed_texture_copy_func(
  gconstpointer self, gpointer data
);
G_GNUC_INTERNAL void maplibre_native_ffi_vulkan_context_descriptor_to_native(
  const MaplibreNativeFfiVulkanContextDescriptor* self,
  mln_vulkan_context_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_context_descriptor_from_native(
  const mln_vulkan_context_descriptor* in
);
G_GNUC_INTERNAL MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_context_descriptor_copy(
  const MaplibreNativeFfiVulkanContextDescriptor* self
);
G_GNUC_INTERNAL void
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_to_native(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  mln_vulkan_borrowed_texture_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void
maplibre_native_ffi_vulkan_borrowed_texture_replacement_to_native(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self,
  mln_vulkan_borrowed_texture_replacement* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiVulkanDeviceContextResult*
maplibre_native_ffi_vulkan_device_context_result_from_native(
  const mln_vulkan_device_context_result* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_vulkan_device_options_to_native(
  const MaplibreNativeFfiVulkanDeviceOptions* self,
  mln_vulkan_device_options* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_vulkan_surface_descriptor_to_native(
  const MaplibreNativeFfiVulkanSurfaceDescriptor* self,
  mln_vulkan_surface_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_vulkan_surface_replacement_to_native(
  const MaplibreNativeFfiVulkanSurfaceReplacement* self,
  mln_vulkan_surface_replacement* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL MaplibreNativeFfiVulkanTextureFrame*
maplibre_native_ffi_vulkan_texture_frame_from_native(
  const mln_vulkan_texture_frame* in
);
G_GNUC_INTERNAL void maplibre_native_ffi_wayland_surface_descriptor_to_native(
  const MaplibreNativeFfiWaylandSurfaceDescriptor* self,
  mln_wayland_surface_descriptor* out, MaplibreNativeFfiCall* call
);
G_GNUC_INTERNAL void maplibre_native_ffi_wayland_surface_replacement_to_native(
  const MaplibreNativeFfiWaylandSurfaceReplacement* self,
  mln_wayland_surface_replacement* out, MaplibreNativeFfiCall* call
);

G_END_DECLS

#endif
