// Generated from the C headers by tools/bindgen. Do not edit.
#ifndef MAPLIBRE_NATIVE_FFI_VALUES_H
#define MAPLIBRE_NATIVE_FFI_VALUES_H

#include <glib-object.h>

#include "maplibre-native-ffi-enums.h"

G_BEGIN_DECLS

/**
 * MAPLIBRE_NATIVE_FFI_DIAGNOSTIC_MESSAGE_CAPACITY:
 *
 * The size of a diagnostic message in bytes, with its terminator.
 *
 * See `MLN_DIAGNOSTIC_MESSAGE_CAPACITY` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_DIAGNOSTIC_MESSAGE_CAPACITY 4096

/**
 * MAPLIBRE_NATIVE_FFI_FRAME_DEMAND_TOKEN_STILL_IMAGE:
 *
 * The token of the frame results that a session adds when it renders an image
 * for `maplibre_native_ffi_map_request_still_image_async()`. Your own demands
 * can't use it.
 *
 * See `MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_FRAME_DEMAND_TOKEN_STILL_IMAGE \
  G_GUINT64_CONSTANT(18446744073709551615)

/**
 * MAPLIBRE_NATIVE_FFI_TEXTURE_RING_MAX_DEPTH:
 *
 * The most textures that a render session's texture ring can have.
 *
 * See `MLN_TEXTURE_RING_MAX_DEPTH` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TEXTURE_RING_MAX_DEPTH 8

/**
 * MAPLIBRE_NATIVE_FFI_VULKAN_NON_DISPATCHABLE_HANDLE_NULL:
 *
 * The null Vulkan non-dispatchable handle.
 *
 * See `MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_VULKAN_NON_DISPATCHABLE_HANDLE_NULL 0

typedef struct _MaplibreNativeFfiAcquiredFrame MaplibreNativeFfiAcquiredFrame;
typedef struct _MaplibreNativeFfiFramePacer MaplibreNativeFfiFramePacer;
typedef struct _MaplibreNativeFfiMap MaplibreNativeFfiMap;
typedef struct _MaplibreNativeFfiMapProjection MaplibreNativeFfiMapProjection;
typedef struct _MaplibreNativeFfiOffscreenDevice
  MaplibreNativeFfiOffscreenDevice;
typedef struct _MaplibreNativeFfiRenderSession MaplibreNativeFfiRenderSession;
typedef struct _MaplibreNativeFfiResourceRequest
  MaplibreNativeFfiResourceRequest;
typedef struct _MaplibreNativeFfiResourceTransformRequest
  MaplibreNativeFfiResourceTransformRequest;
typedef struct _MaplibreNativeFfiRuntime MaplibreNativeFfiRuntime;
typedef struct _MaplibreNativeFfiStyleExpression
  MaplibreNativeFfiStyleExpression;
typedef struct _MaplibreNativeFfiVulkanDevice MaplibreNativeFfiVulkanDevice;
#define MAPLIBRE_NATIVE_FFI_TYPE_MOMENTUM_DECAY \
  (maplibre_native_ffi_momentum_decay_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMomentumDecay, maplibre_native_ffi_momentum_decay,
  MAPLIBRE_NATIVE_FFI, MOMENTUM_DECAY, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_MOMENTUM_SNAP \
  (maplibre_native_ffi_momentum_snap_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMomentumSnap, maplibre_native_ffi_momentum_snap,
  MAPLIBRE_NATIVE_FFI, MOMENTUM_SNAP, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_ANCHORED_MOMENTUM \
  (maplibre_native_ffi_anchored_momentum_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiAnchoredMomentum, maplibre_native_ffi_anchored_momentum,
  MAPLIBRE_NATIVE_FFI, ANCHORED_MOMENTUM, GObject
)
typedef struct _MaplibreNativeFfiCameraTransitionHandler
  MaplibreNativeFfiCameraTransitionHandler;
#define MAPLIBRE_NATIVE_FFI_TYPE_ANIMATION_OPTIONS \
  (maplibre_native_ffi_animation_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiAnimationOptions, maplibre_native_ffi_animation_options,
  MAPLIBRE_NATIVE_FFI, ANIMATION_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_BOUND_OPTIONS \
  (maplibre_native_ffi_bound_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiBoundOptions, maplibre_native_ffi_bound_options,
  MAPLIBRE_NATIVE_FFI, BOUND_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_PAN_MOMENTUM \
  (maplibre_native_ffi_pan_momentum_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiPanMomentum, maplibre_native_ffi_pan_momentum,
  MAPLIBRE_NATIVE_FFI, PAN_MOMENTUM, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_GESTURE_MOMENTUM \
  (maplibre_native_ffi_gesture_momentum_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiGestureMomentum, maplibre_native_ffi_gesture_momentum,
  MAPLIBRE_NATIVE_FFI, GESTURE_MOMENTUM, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_DELTA \
  (maplibre_native_ffi_camera_delta_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiCameraDelta, maplibre_native_ffi_camera_delta,
  MAPLIBRE_NATIVE_FFI, CAMERA_DELTA, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_FIT_OPTIONS \
  (maplibre_native_ffi_camera_fit_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiCameraFitOptions, maplibre_native_ffi_camera_fit_options,
  MAPLIBRE_NATIVE_FFI, CAMERA_FIT_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_FIT_UPDATE \
  (maplibre_native_ffi_camera_fit_update_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiCameraFitUpdate, maplibre_native_ffi_camera_fit_update,
  MAPLIBRE_NATIVE_FFI, CAMERA_FIT_UPDATE, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_OPTIONS \
  (maplibre_native_ffi_camera_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiCameraOptions, maplibre_native_ffi_camera_options,
  MAPLIBRE_NATIVE_FFI, CAMERA_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_UPDATE \
  (maplibre_native_ffi_camera_update_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiCameraUpdate, maplibre_native_ffi_camera_update,
  MAPLIBRE_NATIVE_FFI, CAMERA_UPDATE, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_CUSTOM_GEOMETRY_SOURCE_OPTIONS \
  (maplibre_native_ffi_custom_geometry_source_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiCustomGeometrySourceOptions,
  maplibre_native_ffi_custom_geometry_source_options, MAPLIBRE_NATIVE_FFI,
  CUSTOM_GEOMETRY_SOURCE_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_CUSTOM_VECTOR_SOURCE_OPTIONS \
  (maplibre_native_ffi_custom_vector_source_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiCustomVectorSourceOptions,
  maplibre_native_ffi_custom_vector_source_options, MAPLIBRE_NATIVE_FFI,
  CUSTOM_VECTOR_SOURCE_OPTIONS, GObject
)
typedef struct _MaplibreNativeFfiDmaBufFrame MaplibreNativeFfiDmaBufFrame;
#define MAPLIBRE_NATIVE_FFI_TYPE_DMA_BUF_RING_DESCRIPTOR \
  (maplibre_native_ffi_dma_buf_ring_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiDmaBufRingDescriptor,
  maplibre_native_ffi_dma_buf_ring_descriptor, MAPLIBRE_NATIVE_FFI,
  DMA_BUF_RING_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_EGL_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_egl_context_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiEglContextDescriptor,
  maplibre_native_ffi_egl_context_descriptor, MAPLIBRE_NATIVE_FFI,
  EGL_CONTEXT_DESCRIPTOR, GObject
)
typedef struct _MaplibreNativeFfiFeatureId MaplibreNativeFfiFeatureId;
#define MAPLIBRE_NATIVE_FFI_TYPE_FEATURE_STATE_SELECTOR \
  (maplibre_native_ffi_feature_state_selector_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiFeatureStateSelector,
  maplibre_native_ffi_feature_state_selector, MAPLIBRE_NATIVE_FFI,
  FEATURE_STATE_SELECTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_FRAME_PACER_OPTIONS \
  (maplibre_native_ffi_frame_pacer_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiFramePacerOptions, maplibre_native_ffi_frame_pacer_options,
  MAPLIBRE_NATIVE_FFI, FRAME_PACER_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_FREE_CAMERA_OPTIONS \
  (maplibre_native_ffi_free_camera_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiFreeCameraOptions, maplibre_native_ffi_free_camera_options,
  MAPLIBRE_NATIVE_FFI, FREE_CAMERA_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_GEO_JSON_SOURCE_OPTIONS \
  (maplibre_native_ffi_geo_json_source_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiGeoJsonSourceOptions,
  maplibre_native_ffi_geo_json_source_options, MAPLIBRE_NATIVE_FFI,
  GEO_JSON_SOURCE_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_HTTP_HEADER \
  (maplibre_native_ffi_http_header_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiHttpHeader, maplibre_native_ffi_http_header,
  MAPLIBRE_NATIVE_FFI, HTTP_HEADER, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_HTTP_HEADER_RULE \
  (maplibre_native_ffi_http_header_rule_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiHttpHeaderRule, maplibre_native_ffi_http_header_rule,
  MAPLIBRE_NATIVE_FFI, HTTP_HEADER_RULE, GObject
)
typedef struct _MaplibreNativeFfiHttpHeaderTransform
  MaplibreNativeFfiHttpHeaderTransform;
typedef struct _MaplibreNativeFfiIoSurfaceFrame MaplibreNativeFfiIoSurfaceFrame;
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_metal_context_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMetalContextDescriptor,
  maplibre_native_ffi_metal_context_descriptor, MAPLIBRE_NATIVE_FFI,
  METAL_CONTEXT_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_IO_SURFACE_RING_DESCRIPTOR \
  (maplibre_native_ffi_io_surface_ring_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiIoSurfaceRingDescriptor,
  maplibre_native_ffi_io_surface_ring_descriptor, MAPLIBRE_NATIVE_FFI,
  IO_SURFACE_RING_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_LOG_HANDLER \
  (maplibre_native_ffi_log_handler_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiLogHandler, maplibre_native_ffi_log_handler,
  MAPLIBRE_NATIVE_FFI, LOG_HANDLER, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_OPTIONS \
  (maplibre_native_ffi_map_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMapOptions, maplibre_native_ffi_map_options,
  MAPLIBRE_NATIVE_FFI, MAP_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_TILE_OPTIONS \
  (maplibre_native_ffi_map_tile_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMapTileOptions, maplibre_native_ffi_map_tile_options,
  MAPLIBRE_NATIVE_FFI, MAP_TILE_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_VIEWPORT_OPTIONS \
  (maplibre_native_ffi_map_viewport_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMapViewportOptions, maplibre_native_ffi_map_viewport_options,
  MAPLIBRE_NATIVE_FFI, MAP_VIEWPORT_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_BORROWED_TEXTURE \
  (maplibre_native_ffi_metal_borrowed_texture_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMetalBorrowedTexture,
  maplibre_native_ffi_metal_borrowed_texture, MAPLIBRE_NATIVE_FFI,
  METAL_BORROWED_TEXTURE, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_BORROWED_TEXTURE_DESCRIPTOR \
  (maplibre_native_ffi_metal_borrowed_texture_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor,
  maplibre_native_ffi_metal_borrowed_texture_descriptor, MAPLIBRE_NATIVE_FFI,
  METAL_BORROWED_TEXTURE_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_BORROWED_TEXTURE_REPLACEMENT \
  (maplibre_native_ffi_metal_borrowed_texture_replacement_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMetalBorrowedTextureReplacement,
  maplibre_native_ffi_metal_borrowed_texture_replacement, MAPLIBRE_NATIVE_FFI,
  METAL_BORROWED_TEXTURE_REPLACEMENT, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_SURFACE_DESCRIPTOR \
  (maplibre_native_ffi_metal_surface_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMetalSurfaceDescriptor,
  maplibre_native_ffi_metal_surface_descriptor, MAPLIBRE_NATIVE_FFI,
  METAL_SURFACE_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_SURFACE_REPLACEMENT \
  (maplibre_native_ffi_metal_surface_replacement_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMetalSurfaceReplacement,
  maplibre_native_ffi_metal_surface_replacement, MAPLIBRE_NATIVE_FFI,
  METAL_SURFACE_REPLACEMENT, GObject
)
typedef struct _MaplibreNativeFfiMetalTextureFrame
  MaplibreNativeFfiMetalTextureFrame;
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFLINE_GEOMETRY_REGION_DEFINITION \
  (maplibre_native_ffi_offline_geometry_region_definition_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiOfflineGeometryRegionDefinition,
  maplibre_native_ffi_offline_geometry_region_definition, MAPLIBRE_NATIVE_FFI,
  OFFLINE_GEOMETRY_REGION_DEFINITION, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFLINE_TILE_PYRAMID_REGION_DEFINITION \
  (maplibre_native_ffi_offline_tile_pyramid_region_definition_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition,
  maplibre_native_ffi_offline_tile_pyramid_region_definition,
  MAPLIBRE_NATIVE_FFI, OFFLINE_TILE_PYRAMID_REGION_DEFINITION, GObject
)
typedef struct _MaplibreNativeFfiOfflineRegionDefinition
  MaplibreNativeFfiOfflineRegionDefinition;
typedef struct _MaplibreNativeFfiOfflineRegionInfo
  MaplibreNativeFfiOfflineRegionInfo;
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFSCREEN_DEVICE_OPTIONS \
  (maplibre_native_ffi_offscreen_device_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiOffscreenDeviceOptions,
  maplibre_native_ffi_offscreen_device_options, MAPLIBRE_NATIVE_FFI,
  OFFSCREEN_DEVICE_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_WGL_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_wgl_context_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiWglContextDescriptor,
  maplibre_native_ffi_wgl_context_descriptor, MAPLIBRE_NATIVE_FFI,
  WGL_CONTEXT_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_WEB_GL_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_web_gl_context_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiWebGlContextDescriptor,
  maplibre_native_ffi_web_gl_context_descriptor, MAPLIBRE_NATIVE_FFI,
  WEB_GL_CONTEXT_DESCRIPTOR, GObject
)
typedef struct _MaplibreNativeFfiOpenGlContextDescriptorData
  MaplibreNativeFfiOpenGlContextDescriptorData;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_open_gl_context_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiOpenGlContextDescriptor,
  maplibre_native_ffi_open_gl_context_descriptor, MAPLIBRE_NATIVE_FFI,
  OPEN_GL_CONTEXT_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR \
  (maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor,
  maplibre_native_ffi_open_gl_borrowed_texture_descriptor, MAPLIBRE_NATIVE_FFI,
  OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT \
  (maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement,
  maplibre_native_ffi_open_gl_borrowed_texture_replacement, MAPLIBRE_NATIVE_FFI,
  OPEN_GL_BORROWED_TEXTURE_REPLACEMENT, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_SURFACE_DESCRIPTOR \
  (maplibre_native_ffi_open_gl_surface_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiOpenGlSurfaceDescriptor,
  maplibre_native_ffi_open_gl_surface_descriptor, MAPLIBRE_NATIVE_FFI,
  OPEN_GL_SURFACE_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_SURFACE_REPLACEMENT \
  (maplibre_native_ffi_open_gl_surface_replacement_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiOpenGlSurfaceReplacement,
  maplibre_native_ffi_open_gl_surface_replacement, MAPLIBRE_NATIVE_FFI,
  OPEN_GL_SURFACE_REPLACEMENT, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_PREMULTIPLIED_RGBA8_IMAGE \
  (maplibre_native_ffi_premultiplied_rgba8_image_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiPremultipliedRgba8Image,
  maplibre_native_ffi_premultiplied_rgba8_image, MAPLIBRE_NATIVE_FFI,
  PREMULTIPLIED_RGBA8_IMAGE, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_PROJECTION_MODE_OPTIONS \
  (maplibre_native_ffi_projection_mode_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiProjectionModeOptions,
  maplibre_native_ffi_projection_mode_options, MAPLIBRE_NATIVE_FFI,
  PROJECTION_MODE_OPTIONS, GObject
)
typedef struct _MaplibreNativeFfiQueueLock MaplibreNativeFfiQueueLock;
typedef struct _MaplibreNativeFfiWake MaplibreNativeFfiWake;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_SESSION_ATTACH_OPTIONS \
  (maplibre_native_ffi_render_session_attach_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiRenderSessionAttachOptions,
  maplibre_native_ffi_render_session_attach_options, MAPLIBRE_NATIVE_FFI,
  RENDER_SESSION_ATTACH_OPTIONS, GObject
)
typedef struct _MaplibreNativeFfiRenderedFeature
  MaplibreNativeFfiRenderedFeature;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDERED_FEATURE_QUERY_OPTIONS \
  (maplibre_native_ffi_rendered_feature_query_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiRenderedFeatureQueryOptions,
  maplibre_native_ffi_rendered_feature_query_options, MAPLIBRE_NATIVE_FFI,
  RENDERED_FEATURE_QUERY_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_SCREEN_LINE_STRING \
  (maplibre_native_ffi_screen_line_string_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiScreenLineString, maplibre_native_ffi_screen_line_string,
  MAPLIBRE_NATIVE_FFI, SCREEN_LINE_STRING, GObject
)
typedef struct _MaplibreNativeFfiRenderedQueryGeometry
  MaplibreNativeFfiRenderedQueryGeometry;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_ROUTE \
  (maplibre_native_ffi_resource_route_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiResourceRoute, maplibre_native_ffi_resource_route,
  MAPLIBRE_NATIVE_FFI, RESOURCE_ROUTE, GObject
)
typedef struct _MaplibreNativeFfiResourceProvider
  MaplibreNativeFfiResourceProvider;
typedef struct _MaplibreNativeFfiResourceRequestCancelHandler
  MaplibreNativeFfiResourceRequestCancelHandler;
typedef struct _MaplibreNativeFfiResourceRequestInfo
  MaplibreNativeFfiResourceRequestInfo;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_RESPONSE \
  (maplibre_native_ffi_resource_response_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiResourceResponse, maplibre_native_ffi_resource_response,
  MAPLIBRE_NATIVE_FFI, RESOURCE_RESPONSE, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_URL_QUERY_PARAMETER \
  (maplibre_native_ffi_url_query_parameter_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiUrlQueryParameter, maplibre_native_ffi_url_query_parameter,
  MAPLIBRE_NATIVE_FFI, URL_QUERY_PARAMETER, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_TRANSFORM_RULE \
  (maplibre_native_ffi_resource_transform_rule_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiResourceTransformRule,
  maplibre_native_ffi_resource_transform_rule, MAPLIBRE_NATIVE_FFI,
  RESOURCE_TRANSFORM_RULE, GObject
)
typedef struct _MaplibreNativeFfiResourceTransform
  MaplibreNativeFfiResourceTransform;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_TRANSFORM_ERROR \
  (maplibre_native_ffi_resource_transform_error_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiResourceTransformError,
  maplibre_native_ffi_resource_transform_error, MAPLIBRE_NATIVE_FFI,
  RESOURCE_TRANSFORM_ERROR, GObject
)
typedef struct _MaplibreNativeFfiRuntimeEventStyleImage
  MaplibreNativeFfiRuntimeEventStyleImage;
typedef struct _MaplibreNativeFfiRuntimeEventTileAction
  MaplibreNativeFfiRuntimeEventTileAction;
typedef struct _MaplibreNativeFfiRuntimeEventSourceChanged
  MaplibreNativeFfiRuntimeEventSourceChanged;
typedef struct _MaplibreNativeFfiRuntimeEventGlyphs
  MaplibreNativeFfiRuntimeEventGlyphs;
typedef struct _MaplibreNativeFfiRuntimeEventSprite
  MaplibreNativeFfiRuntimeEventSprite;
typedef struct _MaplibreNativeFfiRuntimeEventPayload
  MaplibreNativeFfiRuntimeEventPayload;
typedef struct _MaplibreNativeFfiRuntimeEvent MaplibreNativeFfiRuntimeEvent;
typedef struct _MaplibreNativeFfiRuntimeEventHandler
  MaplibreNativeFfiRuntimeEventHandler;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_OPTIONS \
  (maplibre_native_ffi_runtime_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiRuntimeOptions, maplibre_native_ffi_runtime_options,
  MAPLIBRE_NATIVE_FFI, RUNTIME_OPTIONS, GObject
)
typedef struct _MaplibreNativeFfiSourceFeature MaplibreNativeFfiSourceFeature;
#define MAPLIBRE_NATIVE_FFI_TYPE_SOURCE_FEATURE_QUERY_OPTIONS \
  (maplibre_native_ffi_source_feature_query_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiSourceFeatureQueryOptions,
  maplibre_native_ffi_source_feature_query_options, MAPLIBRE_NATIVE_FFI,
  SOURCE_FEATURE_QUERY_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION_INPUTS \
  (maplibre_native_ffi_style_expression_inputs_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiStyleExpressionInputs,
  maplibre_native_ffi_style_expression_inputs, MAPLIBRE_NATIVE_FFI,
  STYLE_EXPRESSION_INPUTS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION_OPTIONS \
  (maplibre_native_ffi_style_expression_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiStyleExpressionOptions,
  maplibre_native_ffi_style_expression_options, MAPLIBRE_NATIVE_FFI,
  STYLE_EXPRESSION_OPTIONS, GObject
)
typedef struct _MaplibreNativeFfiStyleImageInfo MaplibreNativeFfiStyleImageInfo;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_IMAGE_OPTIONS \
  (maplibre_native_ffi_style_image_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiStyleImageOptions, maplibre_native_ffi_style_image_options,
  MAPLIBRE_NATIVE_FFI, STYLE_IMAGE_OPTIONS, GObject
)
typedef struct _MaplibreNativeFfiStyleLayerInfo MaplibreNativeFfiStyleLayerInfo;
typedef struct _MaplibreNativeFfiStyleSourceTileInfo
  MaplibreNativeFfiStyleSourceTileInfo;
typedef struct _MaplibreNativeFfiStyleSourceInfo
  MaplibreNativeFfiStyleSourceInfo;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_TRANSITION_OPTIONS \
  (maplibre_native_ffi_style_transition_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiStyleTransitionOptions,
  maplibre_native_ffi_style_transition_options, MAPLIBRE_NATIVE_FFI,
  STYLE_TRANSITION_OPTIONS, GObject
)
typedef struct _MaplibreNativeFfiTextureReadbackResult
  MaplibreNativeFfiTextureReadbackResult;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_vulkan_context_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiVulkanContextDescriptor,
  maplibre_native_ffi_vulkan_context_descriptor, MAPLIBRE_NATIVE_FFI,
  VULKAN_CONTEXT_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_BORROWED_TEXTURE_DESCRIPTOR \
  (maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor,
  maplibre_native_ffi_vulkan_borrowed_texture_descriptor, MAPLIBRE_NATIVE_FFI,
  VULKAN_BORROWED_TEXTURE_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_BORROWED_TEXTURE_REPLACEMENT \
  (maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement,
  maplibre_native_ffi_vulkan_borrowed_texture_replacement, MAPLIBRE_NATIVE_FFI,
  VULKAN_BORROWED_TEXTURE_REPLACEMENT, GObject
)
typedef struct _MaplibreNativeFfiVulkanDeviceContextResult
  MaplibreNativeFfiVulkanDeviceContextResult;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_DEVICE_OPTIONS \
  (maplibre_native_ffi_vulkan_device_options_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiVulkanDeviceOptions,
  maplibre_native_ffi_vulkan_device_options, MAPLIBRE_NATIVE_FFI,
  VULKAN_DEVICE_OPTIONS, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_SURFACE_DESCRIPTOR \
  (maplibre_native_ffi_vulkan_surface_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiVulkanSurfaceDescriptor,
  maplibre_native_ffi_vulkan_surface_descriptor, MAPLIBRE_NATIVE_FFI,
  VULKAN_SURFACE_DESCRIPTOR, GObject
)
typedef struct _MaplibreNativeFfiVulkanTextureFrame
  MaplibreNativeFfiVulkanTextureFrame;
#define MAPLIBRE_NATIVE_FFI_TYPE_WAYLAND_SURFACE_DESCRIPTOR \
  (maplibre_native_ffi_wayland_surface_descriptor_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiWaylandSurfaceDescriptor,
  maplibre_native_ffi_wayland_surface_descriptor, MAPLIBRE_NATIVE_FFI,
  WAYLAND_SURFACE_DESCRIPTOR, GObject
)
#define MAPLIBRE_NATIVE_FFI_TYPE_WAYLAND_SURFACE_REPLACEMENT \
  (maplibre_native_ffi_wayland_surface_replacement_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiWaylandSurfaceReplacement,
  maplibre_native_ffi_wayland_surface_replacement, MAPLIBRE_NATIVE_FFI,
  WAYLAND_SURFACE_REPLACEMENT, GObject
)

/**
 * MaplibreNativeFfiScreenPoint:
 * @x: x
 * @y: y
 *
 * A point on the screen, in logical pixels from the map's top-left corner.
 *
 * See `mln_screen_point` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  gdouble x;
  gdouble y;
} MaplibreNativeFfiScreenPoint;

/**
 * MaplibreNativeFfiUnitBezier:
 * @x1: x1
 * @y1: y1
 * @x2: x2
 * @y2: y2
 *
 * A timing curve from (0, 0) to (1, 1), with the control points (`x1`, `y1`)
 * and (`x2`, `y2`).
 *
 * `x1` and `x2` are between 0 and 1.
 *
 * See `mln_unit_bezier` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  gdouble x1;
  gdouble y1;
  gdouble x2;
  gdouble y2;
} MaplibreNativeFfiUnitBezier;

/**
 * MaplibreNativeFfiLatLng:
 * @latitude: latitude
 * @longitude: longitude
 *
 * A geographic coordinate, in degrees.
 *
 * See `mln_lat_lng` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  gdouble latitude;
  gdouble longitude;
} MaplibreNativeFfiLatLng;

/**
 * MaplibreNativeFfiLatLngBounds:
 * @southwest: southwest
 * @northeast: northeast
 *
 * A rectangle of coordinates, from its southwest to its northeast corner.
 *
 * See `mln_lat_lng_bounds` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  MaplibreNativeFfiLatLng southwest;
  MaplibreNativeFfiLatLng northeast;
} MaplibreNativeFfiLatLngBounds;

/**
 * MaplibreNativeFfiEdgeInsets:
 * @top: top
 * @left: left
 * @bottom: bottom
 * @right: right
 *
 * Distances in logical pixels from each edge of the map.
 *
 * See `mln_edge_insets` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  gdouble top;
  gdouble left;
  gdouble bottom;
  gdouble right;
} MaplibreNativeFfiEdgeInsets;

/**
 * MaplibreNativeFfiCamera:
 * @center: center
 * @center_altitude: The altitude of the center, in meters above sea level.
 * @padding: Insets from the map's edges. The camera centers within the rest.
 * @zoom: zoom
 * @bearing: The rotation from north, in degrees.
 * @pitch: The tilt toward the horizon, in degrees. 0 looks straight down.
 * @roll: The roll, in degrees.
 * @field_of_view: The vertical field of view, in degrees.
 *
 * A map's whole camera.
 *
 * See `mln_camera` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  MaplibreNativeFfiLatLng center;
  gdouble center_altitude;
  MaplibreNativeFfiEdgeInsets padding;
  gdouble zoom;
  gdouble bearing;
  gdouble pitch;
  gdouble roll;
  gdouble field_of_view;
} MaplibreNativeFfiCamera;

/**
 * MaplibreNativeFfiCameraBounds:
 * @bounds: The area that the camera's center stays within. When `unbounded` is
 *   true, it covers the whole world.
 * @unbounded: Whether no area limits the camera's center.
 * @min_zoom: min_zoom
 * @max_zoom: max_zoom
 * @min_pitch: The lowest pitch, in degrees.
 * @max_pitch: The highest pitch, in degrees.
 *
 * All the limits on a map's camera.
 *
 * Its members mean the same as those of `MaplibreNativeFfiBoundOptions`.
 *
 * See `mln_camera_bounds` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  MaplibreNativeFfiLatLngBounds bounds;
  gboolean unbounded;
  gdouble min_zoom;
  gdouble max_zoom;
  gdouble min_pitch;
  gdouble max_pitch;
} MaplibreNativeFfiCameraBounds;

/**
 * MaplibreNativeFfiCameraQueryResult:
 * @generation: generation
 * @camera: camera
 *
 * A map's camera, with the snapshot generation that it matches.
 *
 * See `mln_camera_query_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  guint64 generation;
  MaplibreNativeFfiCamera camera;
} MaplibreNativeFfiCameraQueryResult;

/**
 * MaplibreNativeFfiCameraVelocity:
 * @pan: How fast the map's content moves on the screen, in logical pixels per
 *   second.
 * @zoom: In zoom levels per second.
 * @bearing: In degrees per second.
 * @pitch: In degrees per second.
 *
 * How fast each part of a camera change moves.
 *
 * See `mln_camera_velocity` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  MaplibreNativeFfiScreenPoint pan;
  gdouble zoom;
  gdouble bearing;
  gdouble pitch;
} MaplibreNativeFfiCameraVelocity;

/**
 * MaplibreNativeFfiCameraTransitionEnd:
 * @outcome: outcome
 * @generation: The generation of the first map snapshot that shows where the
 *   change left the camera, or 0 when the map closed first.
 * @velocity: How fast a gesture's momentum still moved when it was cancelled,
 *   which the next gesture can add to its own release velocities. Zero for
 *   every other end.
 *
 * The end of a camera change.
 *
 * See `mln_camera_transition_end` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  MaplibreNativeFfiCameraTransitionOutcome outcome;
  guint64 generation;
  MaplibreNativeFfiCameraVelocity velocity;
} MaplibreNativeFfiCameraTransitionEnd;

/**
 * MaplibreNativeFfiCanonicalTileId:
 * @z: z
 * @x: x
 * @y: y
 *
 * A tile's zoom level `z`, column `x`, and row `y`, with row 0 at the north.
 *
 * See `mln_canonical_tile_id` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
typedef struct {
  guint32 z;
  guint32 x;
  guint32 y;
} MaplibreNativeFfiCanonicalTileId;

/**
 * MaplibreNativeFfiDmaBufFormat:
 * @fourcc: A DRM fourcc code, such as DRM_FORMAT_ABGR8888.
 * @modifier: A DRM format modifier, such as DRM_FORMAT_MOD_LINEAR.
 *
 * A DRM format and modifier that you can import.
 *
 * See `mln_dma_buf_format` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
 */
typedef struct {
  guint32 fourcc;
  guint64 modifier;
} MaplibreNativeFfiDmaBufFormat;

/**
 * MaplibreNativeFfiDmaBufPlane:
 * @fd: The file descriptor of the dmabuf that holds the plane. The planes of a
 *   buffer often share one.
 * @offset: The bytes from the start of the dmabuf to the plane's first byte.
 * @stride: The bytes from the start of one row to the start of the next.
 *
 * Where one plane of a dmabuf keeps its data.
 *
 * See `mln_dma_buf_plane` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
 */
typedef struct {
  gint32 fd;
  guint32 offset;
  guint32 stride;
} MaplibreNativeFfiDmaBufPlane;

/**
 * MaplibreNativeFfiLogicalSize:
 * @width: width
 * @height: height
 *
 * A size in logical pixels, which the map's scale factor turns into pixels.
 *
 * See `mln_logical_size` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  guint32 width;
  guint32 height;
} MaplibreNativeFfiLogicalSize;

/**
 * MaplibreNativeFfiFrameDemand:
 * @flags: flags
 * @token: A value of your choice that the demand's frame result carries.
 *
 * A demand for one frame.
 *
 * See `mln_frame_demand` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
typedef struct {
  MaplibreNativeFfiFrameDemandFlag flags;
  guint64 token;
} MaplibreNativeFfiFrameDemand;

/**
 * MaplibreNativeFfiVec3:
 * @x: x
 * @y: y
 * @z: z
 *
 * A vector in three dimensions.
 *
 * See `mln_vec3` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  gdouble x;
  gdouble y;
  gdouble z;
} MaplibreNativeFfiVec3;

/**
 * MaplibreNativeFfiQuaternion:
 * @x: x
 * @y: y
 * @z: z
 * @w: w
 *
 * A rotation, as a quaternion.
 *
 * See `mln_quaternion` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  gdouble x;
  gdouble y;
  gdouble z;
  gdouble w;
} MaplibreNativeFfiQuaternion;

/**
 * MaplibreNativeFfiFreeCamera:
 * @position: position
 * @orientation: orientation
 *
 * A map camera's whole position and orientation in three dimensions.
 *
 * Its members mean the same as those of `MaplibreNativeFfiFreeCameraOptions`.
 *
 * See `mln_free_camera` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  MaplibreNativeFfiVec3 position;
  MaplibreNativeFfiQuaternion orientation;
} MaplibreNativeFfiFreeCamera;

/**
 * MaplibreNativeFfiGpuSync:
 * @kind: kind
 * @handle: The object to wait on, for the kinds that name one.
 * @value: value
 *
 * A point in GPU work that the other side waits for before using a texture.
 *
 * See `mln_gpu_sync` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
typedef struct {
  MaplibreNativeFfiGpuSyncKind kind;
  guint64 handle;
  guint64 value;
} MaplibreNativeFfiGpuSync;

/**
 * MaplibreNativeFfiImageContent:
 * @left: left
 * @top: top
 * @right: right
 * @bottom: bottom
 *
 * The area of an image, in image pixels, that text fits inside.
 *
 * See `mln_image_content` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
typedef struct {
  gfloat left;
  gfloat top;
  gfloat right;
  gfloat bottom;
} MaplibreNativeFfiImageContent;

/**
 * MaplibreNativeFfiImageStretch:
 * @from: from
 * @to: to
 *
 * A range of image pixels, from `from` to `to`, that stretches to fit text.
 *
 * See `mln_image_stretch` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
typedef struct {
  gfloat from;
  gfloat to;
} MaplibreNativeFfiImageStretch;

/**
 * MaplibreNativeFfiLogicalExtent:
 * @width: width
 * @height: height
 * @scale_factor: scale_factor
 *
 * A size in logical pixels, and the physical pixels in each logical pixel.
 *
 * See `mln_logical_extent` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  guint32 width;
  guint32 height;
  gdouble scale_factor;
} MaplibreNativeFfiLogicalExtent;

/**
 * MaplibreNativeFfiMapProjectionCameraResult:
 * @camera: camera
 *
 * A projection's camera, as `maplibre_native_ffi_map_projection_get_camera()`
 * reports it.
 *
 * See `mln_map_projection_camera_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
typedef struct {
  MaplibreNativeFfiCamera camera;
} MaplibreNativeFfiMapProjectionCameraResult;

/**
 * MaplibreNativeFfiMapTileSettings:
 * @prefetch_zoom_delta: prefetch_zoom_delta
 * @lod_min_radius: lod_min_radius
 * @lod_scale: lod_scale
 * @lod_pitch_threshold: The pitch, in degrees, above which the map lowers the
 *   detail.
 * @lod_zoom_shift: lod_zoom_shift
 * @lod_mode: lod_mode
 *
 * All the settings for how a map picks the tiles to load.
 *
 * Its members mean the same as those of `MaplibreNativeFfiMapTileOptions`.
 *
 * See `mln_map_tile_settings` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  guint32 prefetch_zoom_delta;
  gdouble lod_min_radius;
  gdouble lod_scale;
  gdouble lod_pitch_threshold;
  gdouble lod_zoom_shift;
  MaplibreNativeFfiTileLodMode lod_mode;
} MaplibreNativeFfiMapTileSettings;

/**
 * MaplibreNativeFfiProjectionMode:
 * @axonometric: axonometric
 * @x_skew: x_skew
 * @y_skew: y_skew
 *
 * A map's whole projection mode.
 *
 * Its members mean the same as those of
 * `MaplibreNativeFfiProjectionModeOptions`.
 *
 * See `mln_projection_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  gboolean axonometric;
  gdouble x_skew;
  gdouble y_skew;
} MaplibreNativeFfiProjectionMode;

/**
 * MaplibreNativeFfiMapViewport:
 * @north_orientation: north_orientation
 * @constrain_mode: constrain_mode
 * @viewport_mode: viewport_mode
 * @frustum_offset: frustum_offset
 *
 * A map's whole viewport.
 *
 * Its members mean the same as those of `MaplibreNativeFfiMapViewportOptions`.
 *
 * See `mln_map_viewport` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  MaplibreNativeFfiNorthOrientation north_orientation;
  MaplibreNativeFfiConstrainMode constrain_mode;
  MaplibreNativeFfiViewportMode viewport_mode;
  MaplibreNativeFfiEdgeInsets frustum_offset;
} MaplibreNativeFfiMapViewport;

/**
 * MaplibreNativeFfiMapView:
 * @camera: camera
 * @extent: extent
 * @projection_mode: projection_mode
 * @viewport: viewport
 *
 * One view of a map: its camera, size, and projection.
 *
 * See `mln_map_view` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  MaplibreNativeFfiCamera camera;
  MaplibreNativeFfiLogicalExtent extent;
  MaplibreNativeFfiProjectionMode projection_mode;
  MaplibreNativeFfiMapViewport viewport;
} MaplibreNativeFfiMapView;

/**
 * MaplibreNativeFfiMapSnapshot:
 * @debug_flags: debug_flags
 * @generation: Grows with each change to the map's state.
 * @fully_loaded: Whether the style and every resource that the view needs have
 *   loaded.
 * @rendering_stats_view_enabled: rendering_stats_view_enabled
 * @gesture_in_progress: gesture_in_progress
 * @transition_in_progress: transition_in_progress
 * @event_mask: event_mask
 * @latest_render_update_generation: Counts the render updates that the map has
 *   made for its render session.
 * @tile: tile
 * @bounds: bounds
 * @free_camera: free_camera
 * @view: view
 *
 * A map's state, as `maplibre_native_ffi_map_get_snapshot()` reads it.
 *
 * See `mln_map_snapshot` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  MaplibreNativeFfiMapDebugFlag debug_flags;
  guint64 generation;
  gboolean fully_loaded;
  gboolean rendering_stats_view_enabled;
  gboolean gesture_in_progress;
  gboolean transition_in_progress;
  MaplibreNativeFfiRuntimeEventFlag event_mask;
  guint64 latest_render_update_generation;
  MaplibreNativeFfiMapTileSettings tile;
  MaplibreNativeFfiCameraBounds bounds;
  MaplibreNativeFfiFreeCamera free_camera;
  MaplibreNativeFfiMapView view;
} MaplibreNativeFfiMapSnapshot;

/**
 * MaplibreNativeFfiOfflineRegionStatus:
 * @download_state: download_state
 * @completed_resource_count: The resources downloaded so far, tiles included.
 * @completed_resource_size: The size of the downloaded resources in bytes,
 *   tiles included.
 * @completed_tile_count: completed_tile_count
 * @required_tile_count: required_tile_count
 * @completed_tile_size: The size of the downloaded tiles in bytes.
 * @required_resource_count: The resources that the region needs, tiles
 *   included. Until `required_resource_count_is_precise` is true, this is only
 *   a lower bound.
 * @required_resource_count_is_precise: required_resource_count_is_precise
 * @complete: Whether the region has downloaded every resource it needs.
 *
 * The download progress of an offline region.
 *
 * See `mln_offline_region_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct {
  MaplibreNativeFfiOfflineRegionDownloadState download_state;
  guint64 completed_resource_count;
  guint64 completed_resource_size;
  guint64 completed_tile_count;
  guint64 required_tile_count;
  guint64 completed_tile_size;
  guint64 required_resource_count;
  gboolean required_resource_count_is_precise;
  gboolean complete;
} MaplibreNativeFfiOfflineRegionStatus;

/**
 * MaplibreNativeFfiOffscreenDescriptor:
 * @logical_size: logical_size
 *
 * The image that `maplibre_native_ffi_map_attach_offscreen()` renders. The
 * session renders it at the map's scale factor.
 *
 * See `mln_offscreen_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
 */
typedef struct {
  MaplibreNativeFfiLogicalSize logical_size;
} MaplibreNativeFfiOffscreenDescriptor;

/**
 * MaplibreNativeFfiOpenGlBorrowedTexture:
 * @texture: Your texture name.
 *
 * An OpenGL texture of a borrowed ring.
 *
 * See `mln_open_gl_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
typedef struct {
  guint32 texture;
} MaplibreNativeFfiOpenGlBorrowedTexture;

/**
 * MaplibreNativeFfiOpenGlTextureFrame:
 * @width: width
 * @height: height
 * @scale_factor: scale_factor
 * @slot: The texture's index in the descriptor's `textures`.
 * @texture: texture
 * @target: target
 *
 * The OpenGL texture of an acquired frame.
 *
 * See `mln_open_gl_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
typedef struct {
  guint32 width;
  guint32 height;
  gdouble scale_factor;
  guint32 slot;
  guint32 texture;
  guint32 target;
} MaplibreNativeFfiOpenGlTextureFrame;

/**
 * MaplibreNativeFfiProjectedMeters:
 * @northing: northing
 * @easting: easting
 *
 * A point in the Web Mercator projection, in meters.
 *
 * See `mln_projected_meters` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  gdouble northing;
  gdouble easting;
} MaplibreNativeFfiProjectedMeters;

/**
 * MaplibreNativeFfiRenderAbandonResult:
 * @disposition: disposition
 * @quarantined_resource_count: quarantined_resource_count
 *
 * The outcome of `maplibre_native_ffi_render_session_abandon()`.
 *
 * See `mln_render_abandon_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
typedef struct {
  MaplibreNativeFfiRenderAbandonDisposition disposition;
  guint32 quarantined_resource_count;
} MaplibreNativeFfiRenderAbandonResult;

/**
 * MaplibreNativeFfiRenderFrameResult:
 * @disposition: disposition
 * @token: token
 * @map_update_generation: The map update that a rendered frame shows. The frame
 *   includes a command's change when this is at least the command's
 *   `MaplibreNativeFfiCommandCompletion.render_update_generation`.
 * @generation: generation
 * @frame_generation: frame_generation
 * @needs_repaint: needs_repaint
 * @view: The camera and extent that a rendered frame shows.
 * @mode: mode
 *
 * How one frame demand finished.
 *
 * See `mln_render_frame_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
typedef struct {
  MaplibreNativeFfiRenderResult disposition;
  guint64 token;
  guint64 map_update_generation;
  guint64 generation;
  guint64 frame_generation;
  gboolean needs_repaint;
  MaplibreNativeFfiMapView view;
  MaplibreNativeFfiRenderMode mode;
} MaplibreNativeFfiRenderFrameResult;

/**
 * MaplibreNativeFfiRenderSessionCapabilities:
 * @driver: driver
 * @texture_ring_depth: How many textures the session's texture ring has.
 * @flags: flags
 * @producer_sync_kind: The kind of `MaplibreNativeFfiGpuSync` that
 *   `maplibre_native_ffi_acquired_frame_get_producer_sync()` reports.
 * @consumer_sync_kinds: The kinds of `MaplibreNativeFfiGpuSync` that
 *   `maplibre_native_ffi_acquired_frame_release()` accepts. They depend on the
 *   target and on what the device supports.
 *
 * What an attached render session supports.
 *
 * See `mln_render_session_capabilities` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
typedef struct {
  MaplibreNativeFfiRenderDriverKind driver;
  guint32 texture_ring_depth;
  MaplibreNativeFfiRenderSessionCapabilityFlag flags;
  MaplibreNativeFfiGpuSyncKind producer_sync_kind;
  MaplibreNativeFfiGpuSyncKindFlag consumer_sync_kinds;
} MaplibreNativeFfiRenderSessionCapabilities;

/**
 * MaplibreNativeFfiRenderSessionSnapshot:
 * @state: state
 * @extent: The target's logical size, and the map's scale factor.
 * @generation: generation
 * @frame_generation: frame_generation
 * @acquired_frame_count: acquired_frame_count
 * @failure_status: `0` until the session fails. Then
 *   `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when it failed because it lost its
 *   target, or `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when it failed for
 *   another reason.
 * @has_target: Whether the session is attached and has a target to render to.
 *   It's false from when
 *   `maplibre_native_ffi_render_session_release_target_async()` completes until
 *   you give the session a new target.
 *
 * The state of a render session at one moment.
 *
 * See `mln_render_session_snapshot` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
typedef struct {
  MaplibreNativeFfiRenderSessionState state;
  MaplibreNativeFfiLogicalExtent extent;
  guint64 generation;
  guint64 frame_generation;
  guint32 acquired_frame_count;
  MaplibreNativeFfiError failure_status;
  gboolean has_target;
} MaplibreNativeFfiRenderSessionSnapshot;

/**
 * MaplibreNativeFfiScreenBox:
 * @min: min
 * @max: max
 *
 * A rectangle of the screen, from its `min` corner to its `max` corner.
 *
 * See `mln_screen_box` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
typedef struct {
  MaplibreNativeFfiScreenPoint min;
  MaplibreNativeFfiScreenPoint max;
} MaplibreNativeFfiScreenBox;

/**
 * MaplibreNativeFfiRenderingStats:
 * @encoding_time_ms: The time that the frame took to encode, in milliseconds.
 * @rendering_time_ms: The time that the frame took to render, in milliseconds.
 * @frame_count: The frames rendered so far.
 * @draw_call_count: The draw calls in the frame.
 * @total_draw_call_count: The draw calls in every frame so far.
 *
 * Statistics about the frames that a map renders.
 *
 * See `mln_rendering_stats` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct {
  gdouble encoding_time_ms;
  gdouble rendering_time_ms;
  gint64 frame_count;
  gint64 draw_call_count;
  gint64 total_draw_call_count;
} MaplibreNativeFfiRenderingStats;

/**
 * MaplibreNativeFfiResourceRange:
 * @start: start
 * @end: end
 *
 * The bytes from `start` to `end` of a resource, both included.
 *
 * See `mln_resource_range` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct {
  guint64 start;
  guint64 end;
} MaplibreNativeFfiResourceRange;

/**
 * MaplibreNativeFfiRuntimeEventCameraChange:
 * @mode: mode
 * @source: source
 *
 * The details of `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE` and
 * `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE`.
 *
 * See `mln_runtime_event_camera_change` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct {
  MaplibreNativeFfiCameraChangeMode mode;
  MaplibreNativeFfiCameraChangeSource source;
} MaplibreNativeFfiRuntimeEventCameraChange;

/**
 * MaplibreNativeFfiRuntimeEventLoadingFailed:
 * @error: error
 *
 * The details of `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_LOADING_FAILED`.
 *
 * See `mln_runtime_event_loading_failed` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct {
  MaplibreNativeFfiMapLoadError error;
} MaplibreNativeFfiRuntimeEventLoadingFailed;

/**
 * MaplibreNativeFfiRuntimeEventRenderFrame:
 * @mode: mode
 * @needs_repaint: needs_repaint
 * @placement_changed: placement_changed
 * @stats: stats
 *
 * The details of `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED`.
 *
 * See `mln_runtime_event_render_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct {
  MaplibreNativeFfiRenderMode mode;
  gboolean needs_repaint;
  gboolean placement_changed;
  MaplibreNativeFfiRenderingStats stats;
} MaplibreNativeFfiRuntimeEventRenderFrame;

/**
 * MaplibreNativeFfiTileId:
 * @overscaled_z: overscaled_z
 * @wrap: wrap
 * @canonical_z: canonical_z
 * @canonical_x: canonical_x
 * @canonical_y: canonical_y
 *
 * The ID of a tile.
 *
 * `canonical_x` and `canonical_y` place the tile in the source's grid at zoom
 * `canonical_z`. `overscaled_z` is the zoom that the map draws the tile at, and
 * is greater when the source has no tiles that deep. `wrap` says which copy of
 * the world holds the tile: zero for the main one, negative to the west, and
 * positive to the east.
 *
 * See `mln_tile_id` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct {
  guint32 overscaled_z;
  gint32 wrap;
  guint32 canonical_z;
  guint32 canonical_x;
  guint32 canonical_y;
} MaplibreNativeFfiTileId;

/**
 * MaplibreNativeFfiRuntimeEventOfflineRegionStatus:
 * @region_id: region_id
 * @status: status
 *
 * The details of
 * `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED`.
 *
 * See `mln_runtime_event_offline_region_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct {
  gint64 region_id;
  MaplibreNativeFfiOfflineRegionStatus status;
} MaplibreNativeFfiRuntimeEventOfflineRegionStatus;

/**
 * MaplibreNativeFfiRuntimeEventOfflineRegionResponseError:
 * @region_id: region_id
 * @reason: reason
 *
 * The details of
 * `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR`.
 *
 * See `mln_runtime_event_offline_region_response_error` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct {
  gint64 region_id;
  MaplibreNativeFfiResourceErrorReason reason;
} MaplibreNativeFfiRuntimeEventOfflineRegionResponseError;

/**
 * MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit:
 * @region_id: region_id
 * @limit: The number of demo server tiles that the cache file's offline regions
 *   may hold together.
 *
 * The details of
 * `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED`.
 *
 * See `mln_runtime_event_offline_region_tile_count_limit` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct {
  gint64 region_id;
  guint64 limit;
} MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit;

/**
 * MaplibreNativeFfiStyleExpressionInfo:
 * @is_feature_constant: True when the result doesn't depend on the feature or
 *   its state.
 * @is_zoom_constant: True when the result doesn't depend on the zoom level.
 *
 * What a style expression's result depends on.
 *
 * See `mln_style_expression_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
typedef struct {
  gboolean is_feature_constant;
  gboolean is_zoom_constant;
} MaplibreNativeFfiStyleExpressionInfo;

/**
 * MaplibreNativeFfiStyleExpressionType:
 * @kind: kind
 * @item_kind: The type of each item when `kind` is
 *   `MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_ARRAY`. It can't be an array
 *   itself.
 * @item_count: The length of an array, or 0 for any length.
 *
 * The type of a style expression's result.
 *
 * See `mln_style_expression_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
typedef struct {
  MaplibreNativeFfiStyleExpressionTypeKind kind;
  MaplibreNativeFfiStyleExpressionTypeKind item_kind;
  guint32 item_count;
} MaplibreNativeFfiStyleExpressionType;

/**
 * MaplibreNativeFfiTextureImageInfo:
 * @width: width
 * @height: height
 * @stride: The bytes from the start of one row to the start of the next.
 * @byte_length: byte_length
 *
 * The layout of the pixels that
 * `maplibre_native_ffi_render_session_read_texture_async()` reads.
 *
 * See `mln_texture_image_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
typedef struct {
  guint32 width;
  guint32 height;
  guint32 stride;
  gsize byte_length;
} MaplibreNativeFfiTextureImageInfo;

/**
 * MaplibreNativeFfiVulkanBorrowedTexture:
 * @image: Your VkImage.
 * @image_view: A 2D VkImageView of `image` with one mip level and one layer.
 *
 * A Vulkan image of a borrowed ring.
 *
 * See `mln_vulkan_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
typedef struct {
  guint64 image;
  guint64 image_view;
} MaplibreNativeFfiVulkanBorrowedTexture;

/**
 * MaplibreNativeFfiVulkanSurfaceReplacement:
 * @logical_size: logical_size
 * @surface: Your VkSurfaceKHR.
 *
 * A new Vulkan surface for
 * `maplibre_native_ffi_render_session_set_vulkan_surface_target_async()`.
 *
 * The session presents to it with the context that it attached with, so the
 * queue family of that context's `graphics_queue_family_index` must support
 * presenting to the surface. It uses the surface until a replacement or release
 * of the target completes, a detach or release of the session completes, or an
 * abandon of it returns.
 *
 * See `mln_vulkan_surface_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
typedef struct {
  MaplibreNativeFfiLogicalSize logical_size;
  guint64 surface;
} MaplibreNativeFfiVulkanSurfaceReplacement;

/**
 * MaplibreNativeFfiCameraTransitionEndCallback:
 * @end: (transfer none): the value
 * @user_data: (closure): the data that the callback's setter received
 *
 * Receives the end of a camera change.
 *
 * See `mln_camera_transition_end_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef void (*MaplibreNativeFfiCameraTransitionEndCallback)(
  MaplibreNativeFfiCameraTransitionEnd* end, gpointer user_data
);

/**
 * MaplibreNativeFfiCustomSourceCancelTileCallback:
 * @map: (transfer none): the handle
 * @source_id: (transfer none): the text
 * @tile_id: (transfer none): the value
 * @user_data: (closure): the data that the callback's setter received
 *
 * Reports that a custom source no longer needs a tile.
 *
 * `map` and `source_id` name the source.
 *
 * See `mln_custom_source_cancel_tile_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
typedef void (*MaplibreNativeFfiCustomSourceCancelTileCallback)(
  MaplibreNativeFfiMap* map, const char* source_id,
  MaplibreNativeFfiCanonicalTileId* tile_id, gpointer user_data
);

/**
 * MaplibreNativeFfiCustomSourceFetchTileCallback:
 * @map: (transfer none): the handle
 * @source_id: (transfer none): the text
 * @tile_id: (transfer none): the value
 * @user_data: (closure): the data that the callback's setter received
 *
 * Asks for the data of a tile in a custom source.
 *
 * `map` and `source_id` name the source that asks. Return quickly. Once the
 * data is ready, pass it to
 * `maplibre_native_ffi_map_set_custom_geometry_source_tile_data_async()` or
 * `maplibre_native_ffi_map_set_custom_vector_source_tile_data_async()`. If it
 * fails to load, report that with
 * `maplibre_native_ffi_map_set_custom_geometry_source_tile_error_async()` or
 * `maplibre_native_ffi_map_set_custom_vector_source_tile_error_async()`.
 *
 * See `mln_custom_source_fetch_tile_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
typedef void (*MaplibreNativeFfiCustomSourceFetchTileCallback)(
  MaplibreNativeFfiMap* map, const char* source_id,
  MaplibreNativeFfiCanonicalTileId* tile_id, gpointer user_data
);

/**
 * MaplibreNativeFfiLogCallback:
 * @severity: the value
 * @event: the value
 * @code: the value
 * @message: (transfer none): the text
 * @user_data: (closure): the data that the callback's setter received
 *
 * Receives a log message.
 *
 * Runs on the thread-default main context of the thread that started the call.
 *
 * See `mln_log_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
typedef void (*MaplibreNativeFfiLogCallback)(
  MaplibreNativeFfiLogSeverity severity, MaplibreNativeFfiLogEvent event,
  gint64 code, const char* message, gpointer user_data
);

/**
 * MaplibreNativeFfiQueueLockCallback:
 * @user_data: (closure): the data that the callback's setter received
 *
 * Locks or unlocks the Vulkan queue that you share with a render session.
 *
 * It can run on a library thread, and it can't call the library.
 *
 * See `mln_queue_lock_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
typedef void (*MaplibreNativeFfiQueueLockCallback)(gpointer user_data);

/**
 * MaplibreNativeFfiResourceProviderCallback:
 * @request: (transfer none): the value
 * @handle: (transfer none): the handle
 * @user_data: (closure): the data that the callback's setter received
 *
 * Serves a network request in place of the library.
 *
 * Answer with `maplibre_native_ffi_resource_request_complete()`, or hand the
 * request back to the library with
 * `maplibre_native_ffi_resource_request_pass_through()`. Either can come now or
 * later, from any thread. A request released without an answer fails.
 *
 * See `mln_resource_provider_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef void (*MaplibreNativeFfiResourceProviderCallback)(
  MaplibreNativeFfiResourceRequestInfo* request,
  MaplibreNativeFfiResourceRequest* handle, gpointer user_data
);

/**
 * MaplibreNativeFfiResourceRequestCancelCallback:
 * @user_data: (closure): the data that the callback's setter received
 *
 * Reports that the library no longer needs a resource request.
 *
 * The request no longer accepts an answer, so stop working on it and release
 * it.
 *
 * See `mln_resource_request_cancel_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef void (*MaplibreNativeFfiResourceRequestCancelCallback)(
  gpointer user_data
);

/**
 * MaplibreNativeFfiResourceTransformCallback:
 * @kind: the value
 * @url: (transfer none): the text
 * @request: (transfer none): the handle
 * @user_data: (closure): the data that the callback's setter received
 *
 * Rewrites the URL of a network request before the library sends it.
 *
 * The request waits until you answer with
 * `maplibre_native_ffi_resource_transform_request_complete()` or
 * `maplibre_native_ffi_resource_transform_request_fail()`. To keep `url`,
 * complete with it. Releasing `request` without an answer fails the request.
 * You can answer now or later, from any thread.
 *
 * See `mln_resource_transform_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef void (*MaplibreNativeFfiResourceTransformCallback)(
  MaplibreNativeFfiResourceKind kind, const char* url,
  MaplibreNativeFfiResourceTransformRequest* request, gpointer user_data
);

/**
 * MaplibreNativeFfiRuntimeEventCallback:
 * @events: (element-type MaplibreNativeFfiRuntimeEvent) (transfer none): the items
 * @user_data: (closure): the data that the callback's setter received
 *
 * Receives a batch of events.
 *
 * See `mln_runtime_event_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef void (*MaplibreNativeFfiRuntimeEventCallback)(
  GPtrArray* events, gpointer user_data
);

/**
 * MaplibreNativeFfiWakeCallback:
 * @user_data: (closure): the data that the callback's setter received
 *
 * Tells the caller that work is ready for it.
 *
 * One call can stand for several pieces of ready work, so take all of it each
 * time.
 *
 * Runs on the thread-default main context of the thread that started the call.
 *
 * See `mln_wake_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
 */
typedef void (*MaplibreNativeFfiWakeCallback)(gpointer user_data);

GType maplibre_native_ffi_screen_point_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_SCREEN_POINT \
  (maplibre_native_ffi_screen_point_get_type())

/**
 * maplibre_native_ffi_screen_point_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiScreenPoint* maplibre_native_ffi_screen_point_copy(
  const MaplibreNativeFfiScreenPoint* self
);

/**
 * maplibre_native_ffi_screen_point_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_screen_point_free(MaplibreNativeFfiScreenPoint* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiScreenPoint, maplibre_native_ffi_screen_point_free
)

GType maplibre_native_ffi_unit_bezier_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_UNIT_BEZIER \
  (maplibre_native_ffi_unit_bezier_get_type())

/**
 * maplibre_native_ffi_unit_bezier_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiUnitBezier* maplibre_native_ffi_unit_bezier_copy(
  const MaplibreNativeFfiUnitBezier* self
);

/**
 * maplibre_native_ffi_unit_bezier_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_unit_bezier_free(MaplibreNativeFfiUnitBezier* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiUnitBezier, maplibre_native_ffi_unit_bezier_free
)

/**
 * MaplibreNativeFfiMomentumDecay:
 *
 * How one part of a gesture's momentum slows from its release speed.
 *
 * Speeds and distances are in the part's unit: logical pixels for pan, zoom
 * levels for zoom, and degrees for bearing and pitch. A setting whose bit isn't
 * set takes the part's default, which `MaplibreNativeFfiGestureMomentum` lists.
 * `min_speed` is finite and not negative, and every other number is finite and
 * positive. A friction decay can serve every release, as a preset that matches
 * a platform's scrolling. An eased glide takes its direction from the part's
 * velocity, and its distance and duration from the caller's plan.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_momentum_decay` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_momentum_decay_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMomentumDecay` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMomentumDecay* maplibre_native_ffi_momentum_decay_new(void);

/**
 * MaplibreNativeFfiMomentumDecay:model: (setter set_model) (getter get_model)
 *
 * The `model` member.
 */
/**
 * maplibre_native_ffi_momentum_decay_get_model: (get-property model)
 *
 * Gets `MaplibreNativeFfiMomentumDecay:model`.
 *
 * Returns: the value
 */
MaplibreNativeFfiMomentumDecayModel
maplibre_native_ffi_momentum_decay_get_model(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_has_model:
 *
 * Whether `MaplibreNativeFfiMomentumDecay:model` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_decay_has_model(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_set_model: (set-property model)
 *
 * Sets `MaplibreNativeFfiMomentumDecay:model`.
 */
void maplibre_native_ffi_momentum_decay_set_model(
  MaplibreNativeFfiMomentumDecay* self,
  MaplibreNativeFfiMomentumDecayModel value
);

/**
 * MaplibreNativeFfiMomentumDecay:half-life-ms: (setter set_half_life_ms) (getter get_half_life_ms)
 *
 * For `MAPLIBRE_NATIVE_FFI_MOMENTUM_DECAY_MODEL_FRICTION`, how long the speed
 * takes to halve.
 */
/**
 * maplibre_native_ffi_momentum_decay_get_half_life_ms: (get-property half-life-ms)
 *
 * Gets `MaplibreNativeFfiMomentumDecay:half-life-ms`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_momentum_decay_get_half_life_ms(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_has_half_life_ms:
 *
 * Whether `MaplibreNativeFfiMomentumDecay:half-life-ms` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_decay_has_half_life_ms(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_set_half_life_ms: (set-property half-life-ms)
 *
 * Sets `MaplibreNativeFfiMomentumDecay:half-life-ms`.
 */
void maplibre_native_ffi_momentum_decay_set_half_life_ms(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
);

/**
 * MaplibreNativeFfiMomentumDecay:stop-speed: (setter set_stop_speed) (getter get_stop_speed)
 *
 * For `MAPLIBRE_NATIVE_FFI_MOMENTUM_DECAY_MODEL_FRICTION`, the speed at which
 * it ends.
 */
/**
 * maplibre_native_ffi_momentum_decay_get_stop_speed: (get-property stop-speed)
 *
 * Gets `MaplibreNativeFfiMomentumDecay:stop-speed`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_momentum_decay_get_stop_speed(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_has_stop_speed:
 *
 * Whether `MaplibreNativeFfiMomentumDecay:stop-speed` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_decay_has_stop_speed(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_set_stop_speed: (set-property stop-speed)
 *
 * Sets `MaplibreNativeFfiMomentumDecay:stop-speed`.
 */
void maplibre_native_ffi_momentum_decay_set_stop_speed(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
);

/**
 * MaplibreNativeFfiMomentumDecay:distance: (setter set_distance) (getter get_distance)
 *
 * Required for `MAPLIBRE_NATIVE_FFI_MOMENTUM_DECAY_MODEL_EASE`.
 */
/**
 * maplibre_native_ffi_momentum_decay_get_distance: (get-property distance)
 *
 * Gets `MaplibreNativeFfiMomentumDecay:distance`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_momentum_decay_get_distance(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_has_distance:
 *
 * Whether `MaplibreNativeFfiMomentumDecay:distance` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_decay_has_distance(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_set_distance: (set-property distance)
 *
 * Sets `MaplibreNativeFfiMomentumDecay:distance`.
 */
void maplibre_native_ffi_momentum_decay_set_distance(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
);

/**
 * MaplibreNativeFfiMomentumDecay:duration-ms: (setter set_duration_ms) (getter get_duration_ms)
 *
 * Required for `MAPLIBRE_NATIVE_FFI_MOMENTUM_DECAY_MODEL_EASE`.
 */
/**
 * maplibre_native_ffi_momentum_decay_get_duration_ms: (get-property duration-ms)
 *
 * Gets `MaplibreNativeFfiMomentumDecay:duration-ms`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_momentum_decay_get_duration_ms(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_has_duration_ms:
 *
 * Whether `MaplibreNativeFfiMomentumDecay:duration-ms` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_decay_has_duration_ms(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_set_duration_ms: (set-property duration-ms)
 *
 * Sets `MaplibreNativeFfiMomentumDecay:duration-ms`.
 */
void maplibre_native_ffi_momentum_decay_set_duration_ms(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
);

/**
 * MaplibreNativeFfiMomentumDecay:easing: (setter set_easing) (getter get_easing)
 *
 * For `MAPLIBRE_NATIVE_FFI_MOMENTUM_DECAY_MODEL_EASE`, the share of `distance`
 * covered over the share of `duration_ms`. Its control points are between 0 and
 * 1, so the glide never passes its distance. Defaults to (0, 0, 0.25, 1), the
 * curve that camera animations ease along by default. Android's scroll fling
 * follows (0.175, 0.5, 0.35, 1).
 */
/**
 * maplibre_native_ffi_momentum_decay_get_easing: (get-property easing)
 *
 * Gets `MaplibreNativeFfiMomentumDecay:easing`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiUnitBezier* maplibre_native_ffi_momentum_decay_get_easing(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_has_easing:
 *
 * Whether `MaplibreNativeFfiMomentumDecay:easing` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_decay_has_easing(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_set_easing: (set-property easing)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiMomentumDecay:easing`.
 */
void maplibre_native_ffi_momentum_decay_set_easing(
  MaplibreNativeFfiMomentumDecay* self, const MaplibreNativeFfiUnitBezier* value
);

/**
 * MaplibreNativeFfiMomentumDecay:min-speed: (setter set_min_speed) (getter get_min_speed)
 *
 * A slower release starts no momentum.
 */
/**
 * maplibre_native_ffi_momentum_decay_get_min_speed: (get-property min-speed)
 *
 * Gets `MaplibreNativeFfiMomentumDecay:min-speed`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_momentum_decay_get_min_speed(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_has_min_speed:
 *
 * Whether `MaplibreNativeFfiMomentumDecay:min-speed` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_decay_has_min_speed(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_set_min_speed: (set-property min-speed)
 *
 * Sets `MaplibreNativeFfiMomentumDecay:min-speed`.
 */
void maplibre_native_ffi_momentum_decay_set_min_speed(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
);

/**
 * MaplibreNativeFfiMomentumDecay:max-speed: (setter set_max_speed) (getter get_max_speed)
 *
 * For `MAPLIBRE_NATIVE_FFI_MOMENTUM_DECAY_MODEL_FRICTION`, a faster release
 * counts as this.
 */
/**
 * maplibre_native_ffi_momentum_decay_get_max_speed: (get-property max-speed)
 *
 * Gets `MaplibreNativeFfiMomentumDecay:max-speed`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_momentum_decay_get_max_speed(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_has_max_speed:
 *
 * Whether `MaplibreNativeFfiMomentumDecay:max-speed` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_decay_has_max_speed(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_set_max_speed: (set-property max-speed)
 *
 * Sets `MaplibreNativeFfiMomentumDecay:max-speed`.
 */
void maplibre_native_ffi_momentum_decay_set_max_speed(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
);

/**
 * MaplibreNativeFfiMomentumDecay:max-distance: (setter set_max_distance) (getter get_max_distance)
 *
 * The farthest the part moves. A friction release that would move farther
 * starts slower, so it ends sooner, and a planned glide moves at most this far
 * over its duration.
 */
/**
 * maplibre_native_ffi_momentum_decay_get_max_distance: (get-property max-distance)
 *
 * Gets `MaplibreNativeFfiMomentumDecay:max-distance`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_momentum_decay_get_max_distance(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_has_max_distance:
 *
 * Whether `MaplibreNativeFfiMomentumDecay:max-distance` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_decay_has_max_distance(
  MaplibreNativeFfiMomentumDecay* self
);

/**
 * maplibre_native_ffi_momentum_decay_set_max_distance: (set-property max-distance)
 *
 * Sets `MaplibreNativeFfiMomentumDecay:max-distance`.
 */
void maplibre_native_ffi_momentum_decay_set_max_distance(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
);

/**
 * MaplibreNativeFfiMomentumSnap:
 *
 * Where a part settles after a gesture that changed it.
 *
 * The targets are every `spacing` from `offset`, in the part's unit. Bearing
 * targets wrap around the circle. Once the part's glide ends, or right away
 * when it doesn't move, the part eases to the nearest target if it is within
 * `tolerance` of it, and otherwise stays where it is. A setting whose bit isn't
 * set takes the part's default, which `MaplibreNativeFfiGestureMomentum` lists.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_momentum_snap` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_momentum_snap_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMomentumSnap` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMomentumSnap* maplibre_native_ffi_momentum_snap_new(void);

/**
 * MaplibreNativeFfiMomentumSnap:spacing: (setter set_spacing) (getter get_spacing)
 *
 * Positive.
 */
/**
 * maplibre_native_ffi_momentum_snap_get_spacing: (get-property spacing)
 *
 * Gets `MaplibreNativeFfiMomentumSnap:spacing`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_momentum_snap_get_spacing(
  MaplibreNativeFfiMomentumSnap* self
);

/**
 * maplibre_native_ffi_momentum_snap_has_spacing:
 *
 * Whether `MaplibreNativeFfiMomentumSnap:spacing` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_snap_has_spacing(
  MaplibreNativeFfiMomentumSnap* self
);

/**
 * maplibre_native_ffi_momentum_snap_set_spacing: (set-property spacing)
 *
 * Sets `MaplibreNativeFfiMomentumSnap:spacing`.
 */
void maplibre_native_ffi_momentum_snap_set_spacing(
  MaplibreNativeFfiMomentumSnap* self, gdouble value
);

/**
 * MaplibreNativeFfiMomentumSnap:offset: (setter set_offset) (getter get_offset)
 *
 * Defaults to 0.
 */
/**
 * maplibre_native_ffi_momentum_snap_get_offset: (get-property offset)
 *
 * Gets `MaplibreNativeFfiMomentumSnap:offset`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_momentum_snap_get_offset(
  MaplibreNativeFfiMomentumSnap* self
);

/**
 * maplibre_native_ffi_momentum_snap_has_offset:
 *
 * Whether `MaplibreNativeFfiMomentumSnap:offset` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_snap_has_offset(
  MaplibreNativeFfiMomentumSnap* self
);

/**
 * maplibre_native_ffi_momentum_snap_set_offset: (set-property offset)
 *
 * Sets `MaplibreNativeFfiMomentumSnap:offset`.
 */
void maplibre_native_ffi_momentum_snap_set_offset(
  MaplibreNativeFfiMomentumSnap* self, gdouble value
);

/**
 * MaplibreNativeFfiMomentumSnap:tolerance: (setter set_tolerance) (getter get_tolerance)
 *
 * Not negative.
 */
/**
 * maplibre_native_ffi_momentum_snap_get_tolerance: (get-property tolerance)
 *
 * Gets `MaplibreNativeFfiMomentumSnap:tolerance`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_momentum_snap_get_tolerance(
  MaplibreNativeFfiMomentumSnap* self
);

/**
 * maplibre_native_ffi_momentum_snap_has_tolerance:
 *
 * Whether `MaplibreNativeFfiMomentumSnap:tolerance` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_snap_has_tolerance(
  MaplibreNativeFfiMomentumSnap* self
);

/**
 * maplibre_native_ffi_momentum_snap_set_tolerance: (set-property tolerance)
 *
 * Sets `MaplibreNativeFfiMomentumSnap:tolerance`.
 */
void maplibre_native_ffi_momentum_snap_set_tolerance(
  MaplibreNativeFfiMomentumSnap* self, gdouble value
);

/**
 * MaplibreNativeFfiMomentumSnap:duration-ms: (setter set_duration_ms) (getter get_duration_ms)
 *
 * Zero settles without easing. Defaults to 300.
 */
/**
 * maplibre_native_ffi_momentum_snap_get_duration_ms: (get-property duration-ms)
 *
 * Gets `MaplibreNativeFfiMomentumSnap:duration-ms`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_momentum_snap_get_duration_ms(
  MaplibreNativeFfiMomentumSnap* self
);

/**
 * maplibre_native_ffi_momentum_snap_has_duration_ms:
 *
 * Whether `MaplibreNativeFfiMomentumSnap:duration-ms` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_momentum_snap_has_duration_ms(
  MaplibreNativeFfiMomentumSnap* self
);

/**
 * maplibre_native_ffi_momentum_snap_set_duration_ms: (set-property duration-ms)
 *
 * Sets `MaplibreNativeFfiMomentumSnap:duration-ms`.
 */
void maplibre_native_ffi_momentum_snap_set_duration_ms(
  MaplibreNativeFfiMomentumSnap* self, gdouble value
);

/**
 * MaplibreNativeFfiAnchoredMomentum:
 *
 * How a gesture's zoom, bearing, or pitch keeps changing after release.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_anchored_momentum` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_anchored_momentum_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiAnchoredMomentum` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiAnchoredMomentum* maplibre_native_ffi_anchored_momentum_new(
  void
);

/**
 * MaplibreNativeFfiAnchoredMomentum:velocity: (setter set_velocity) (getter get_velocity)
 *
 * How fast the value changed at release: zoom levels per second for zoom, and
 * degrees per second for bearing and pitch. The part moves only when this is
 * set.
 */
/**
 * maplibre_native_ffi_anchored_momentum_get_velocity: (get-property velocity)
 *
 * Gets `MaplibreNativeFfiAnchoredMomentum:velocity`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_anchored_momentum_get_velocity(
  MaplibreNativeFfiAnchoredMomentum* self
);

/**
 * maplibre_native_ffi_anchored_momentum_has_velocity:
 *
 * Whether `MaplibreNativeFfiAnchoredMomentum:velocity` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_anchored_momentum_has_velocity(
  MaplibreNativeFfiAnchoredMomentum* self
);

/**
 * maplibre_native_ffi_anchored_momentum_set_velocity: (set-property velocity)
 *
 * Sets `MaplibreNativeFfiAnchoredMomentum:velocity`.
 */
void maplibre_native_ffi_anchored_momentum_set_velocity(
  MaplibreNativeFfiAnchoredMomentum* self, gdouble value
);

/**
 * MaplibreNativeFfiAnchoredMomentum:anchor: (setter set_anchor) (getter get_anchor)
 *
 * The screen point that the change keeps in place. Without one, the change
 * keeps the center of the view in place.
 */
/**
 * maplibre_native_ffi_anchored_momentum_get_anchor: (get-property anchor)
 *
 * Gets `MaplibreNativeFfiAnchoredMomentum:anchor`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiScreenPoint* maplibre_native_ffi_anchored_momentum_get_anchor(
  MaplibreNativeFfiAnchoredMomentum* self
);

/**
 * maplibre_native_ffi_anchored_momentum_has_anchor:
 *
 * Whether `MaplibreNativeFfiAnchoredMomentum:anchor` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_anchored_momentum_has_anchor(
  MaplibreNativeFfiAnchoredMomentum* self
);

/**
 * maplibre_native_ffi_anchored_momentum_set_anchor: (set-property anchor)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiAnchoredMomentum:anchor`.
 */
void maplibre_native_ffi_anchored_momentum_set_anchor(
  MaplibreNativeFfiAnchoredMomentum* self,
  const MaplibreNativeFfiScreenPoint* value
);

/**
 * MaplibreNativeFfiAnchoredMomentum:decay: (setter set_decay) (getter get_decay)
 *
 * The `decay` member.
 */
/**
 * maplibre_native_ffi_anchored_momentum_get_decay: (get-property decay)
 *
 * Gets `MaplibreNativeFfiAnchoredMomentum:decay`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiMomentumDecay* maplibre_native_ffi_anchored_momentum_get_decay(
  MaplibreNativeFfiAnchoredMomentum* self
);

/**
 * maplibre_native_ffi_anchored_momentum_set_decay: (set-property decay)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiAnchoredMomentum:decay`.
 */
void maplibre_native_ffi_anchored_momentum_set_decay(
  MaplibreNativeFfiAnchoredMomentum* self, MaplibreNativeFfiMomentumDecay* value
);

/**
 * MaplibreNativeFfiAnchoredMomentum:snap: (setter set_snap) (getter get_snap)
 *
 * Where the part settles. Without this, it stays where the gesture and its
 * glide leave it.
 */
/**
 * maplibre_native_ffi_anchored_momentum_get_snap: (get-property snap)
 *
 * Gets `MaplibreNativeFfiAnchoredMomentum:snap`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiMomentumSnap* maplibre_native_ffi_anchored_momentum_get_snap(
  MaplibreNativeFfiAnchoredMomentum* self
);

/**
 * maplibre_native_ffi_anchored_momentum_has_snap:
 *
 * Whether `MaplibreNativeFfiAnchoredMomentum:snap` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_anchored_momentum_has_snap(
  MaplibreNativeFfiAnchoredMomentum* self
);

/**
 * maplibre_native_ffi_anchored_momentum_set_snap: (set-property snap)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiAnchoredMomentum:snap`.
 */
void maplibre_native_ffi_anchored_momentum_set_snap(
  MaplibreNativeFfiAnchoredMomentum* self, MaplibreNativeFfiMomentumSnap* value
);

/**
 * MaplibreNativeFfiCameraTransitionHandler:
 *
 * A callback for the end of a camera change, for
 * `MaplibreNativeFfiAnimationOptions`.
 *
 * The callback runs once for each camera command, after every part of the
 * change that the command started has ended.
 *
 * See `mln_camera_transition_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_camera_transition_handler_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_TRANSITION_HANDLER \
  (maplibre_native_ffi_camera_transition_handler_get_type())

/**
 * maplibre_native_ffi_camera_transition_handler_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCameraTransitionHandler` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCameraTransitionHandler*
maplibre_native_ffi_camera_transition_handler_new(void);

/**
 * maplibre_native_ffi_camera_transition_handler_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCameraTransitionHandler*
maplibre_native_ffi_camera_transition_handler_copy(
  const MaplibreNativeFfiCameraTransitionHandler* self
);

/**
 * maplibre_native_ffi_camera_transition_handler_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_camera_transition_handler_free(
  MaplibreNativeFfiCameraTransitionHandler* self
);

/**
 * maplibre_native_ffi_camera_transition_handler_set_callback:
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Receives the end of a camera change.
 *
 * See `mln_camera_transition_end_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
void maplibre_native_ffi_camera_transition_handler_set_callback(
  MaplibreNativeFfiCameraTransitionHandler* self,
  MaplibreNativeFfiCameraTransitionEndCallback callback, gpointer user_data,
  GDestroyNotify notify
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCameraTransitionHandler,
  maplibre_native_ffi_camera_transition_handler_free
)

/**
 * MaplibreNativeFfiAnimationOptions:
 *
 * How a camera change animates.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_animation_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_animation_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiAnimationOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiAnimationOptions* maplibre_native_ffi_animation_options_new(
  void
);

/**
 * MaplibreNativeFfiAnimationOptions:duration-ms: (setter set_duration_ms) (getter get_duration_ms)
 *
 * The `duration_ms` member.
 */
/**
 * maplibre_native_ffi_animation_options_get_duration_ms: (get-property duration-ms)
 *
 * Gets `MaplibreNativeFfiAnimationOptions:duration-ms`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_animation_options_get_duration_ms(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_has_duration_ms:
 *
 * Whether `MaplibreNativeFfiAnimationOptions:duration-ms` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_animation_options_has_duration_ms(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_duration_ms: (set-property duration-ms)
 *
 * Sets `MaplibreNativeFfiAnimationOptions:duration-ms`.
 */
void maplibre_native_ffi_animation_options_set_duration_ms(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiAnimationOptions:velocity: (setter set_velocity) (getter get_velocity)
 *
 * The average speed of a fly animation, in screens per second. `duration_ms`
 * overrides it.
 */
/**
 * maplibre_native_ffi_animation_options_get_velocity: (get-property velocity)
 *
 * Gets `MaplibreNativeFfiAnimationOptions:velocity`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_animation_options_get_velocity(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_has_velocity:
 *
 * Whether `MaplibreNativeFfiAnimationOptions:velocity` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_animation_options_has_velocity(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_velocity: (set-property velocity)
 *
 * Sets `MaplibreNativeFfiAnimationOptions:velocity`.
 */
void maplibre_native_ffi_animation_options_set_velocity(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiAnimationOptions:min-zoom: (setter set_min_zoom) (getter get_min_zoom)
 *
 * The zoom level at the peak of a fly animation.
 */
/**
 * maplibre_native_ffi_animation_options_get_min_zoom: (get-property min-zoom)
 *
 * Gets `MaplibreNativeFfiAnimationOptions:min-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_animation_options_get_min_zoom(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_has_min_zoom:
 *
 * Whether `MaplibreNativeFfiAnimationOptions:min-zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_animation_options_has_min_zoom(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_min_zoom: (set-property min-zoom)
 *
 * Sets `MaplibreNativeFfiAnimationOptions:min-zoom`.
 */
void maplibre_native_ffi_animation_options_set_min_zoom(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiAnimationOptions:max-duration-ms: (setter set_max_duration_ms) (getter get_max_duration_ms)
 *
 * The longest a fly animation may take. A fly animation whose duration, set or
 * derived from `velocity`, is longer moves the camera at once instead, and so
 * does one too slow for the library to time, without this field. Other
 * animations ignore it.
 */
/**
 * maplibre_native_ffi_animation_options_get_max_duration_ms: (get-property max-duration-ms)
 *
 * Gets `MaplibreNativeFfiAnimationOptions:max-duration-ms`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_animation_options_get_max_duration_ms(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_has_max_duration_ms:
 *
 * Whether `MaplibreNativeFfiAnimationOptions:max-duration-ms` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_animation_options_has_max_duration_ms(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_max_duration_ms: (set-property max-duration-ms)
 *
 * Sets `MaplibreNativeFfiAnimationOptions:max-duration-ms`.
 */
void maplibre_native_ffi_animation_options_set_max_duration_ms(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiAnimationOptions:easing: (setter set_easing) (getter get_easing)
 *
 * The `easing` member.
 */
/**
 * maplibre_native_ffi_animation_options_get_easing: (get-property easing)
 *
 * Gets `MaplibreNativeFfiAnimationOptions:easing`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiUnitBezier* maplibre_native_ffi_animation_options_get_easing(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_has_easing:
 *
 * Whether `MaplibreNativeFfiAnimationOptions:easing` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_animation_options_has_easing(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_easing: (set-property easing)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiAnimationOptions:easing`.
 */
void maplibre_native_ffi_animation_options_set_easing(
  MaplibreNativeFfiAnimationOptions* self,
  const MaplibreNativeFfiUnitBezier* value
);

/**
 * MaplibreNativeFfiAnimationOptions:transition-id: (setter set_transition_id) (getter get_transition_id)
 *
 * An ID that
 * `maplibre_native_ffi_map_cancel_camera_transitions_with_id_async()` cancels
 * the change by. Several changes can share one.
 */
/**
 * maplibre_native_ffi_animation_options_get_transition_id: (get-property transition-id)
 *
 * Gets `MaplibreNativeFfiAnimationOptions:transition-id`.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_animation_options_get_transition_id(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_has_transition_id:
 *
 * Whether `MaplibreNativeFfiAnimationOptions:transition-id` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_animation_options_has_transition_id(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_transition_id: (set-property transition-id)
 *
 * Sets `MaplibreNativeFfiAnimationOptions:transition-id`.
 */
void maplibre_native_ffi_animation_options_set_transition_id(
  MaplibreNativeFfiAnimationOptions* self, guint64 value
);

/**
 * MaplibreNativeFfiAnimationOptions:end-handler: (setter set_end_handler) (getter get_end_handler)
 *
 * The `end_handler` member.
 */
/**
 * maplibre_native_ffi_animation_options_get_end_handler: (get-property end-handler)
 *
 * Gets `MaplibreNativeFfiAnimationOptions:end-handler`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiCameraTransitionHandler*
maplibre_native_ffi_animation_options_get_end_handler(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_end_handler: (set-property end-handler)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiAnimationOptions:end-handler`.
 */
void maplibre_native_ffi_animation_options_set_end_handler(
  MaplibreNativeFfiAnimationOptions* self,
  const MaplibreNativeFfiCameraTransitionHandler* value
);

GType maplibre_native_ffi_lat_lng_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_LAT_LNG \
  (maplibre_native_ffi_lat_lng_get_type())

/**
 * maplibre_native_ffi_lat_lng_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiLatLng* maplibre_native_ffi_lat_lng_copy(
  const MaplibreNativeFfiLatLng* self
);

/**
 * maplibre_native_ffi_lat_lng_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_lat_lng_free(MaplibreNativeFfiLatLng* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiLatLng, maplibre_native_ffi_lat_lng_free
)

GType maplibre_native_ffi_lat_lng_bounds_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_LAT_LNG_BOUNDS \
  (maplibre_native_ffi_lat_lng_bounds_get_type())

/**
 * maplibre_native_ffi_lat_lng_bounds_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiLatLngBounds* maplibre_native_ffi_lat_lng_bounds_copy(
  const MaplibreNativeFfiLatLngBounds* self
);

/**
 * maplibre_native_ffi_lat_lng_bounds_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_lat_lng_bounds_free(
  MaplibreNativeFfiLatLngBounds* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiLatLngBounds, maplibre_native_ffi_lat_lng_bounds_free
)

/**
 * MaplibreNativeFfiBoundOptions:
 *
 * Limits on the camera, for `maplibre_native_ffi_map_set_bounds_async()`.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_bound_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_bound_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiBoundOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiBoundOptions* maplibre_native_ffi_bound_options_new(void);

/**
 * MaplibreNativeFfiBoundOptions:bounds: (setter set_bounds) (getter get_bounds)
 *
 * The area that the camera's center stays within.
 */
/**
 * maplibre_native_ffi_bound_options_get_bounds: (get-property bounds)
 *
 * Gets `MaplibreNativeFfiBoundOptions:bounds`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLatLngBounds* maplibre_native_ffi_bound_options_get_bounds(
  MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_has_bounds:
 *
 * Whether `MaplibreNativeFfiBoundOptions:bounds` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_bound_options_has_bounds(
  MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_set_bounds: (set-property bounds)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiBoundOptions:bounds`.
 */
void maplibre_native_ffi_bound_options_set_bounds(
  MaplibreNativeFfiBoundOptions* self,
  const MaplibreNativeFfiLatLngBounds* value
);

/**
 * MaplibreNativeFfiBoundOptions:min-zoom: (setter set_min_zoom) (getter get_min_zoom)
 *
 * The `min_zoom` member.
 */
/**
 * maplibre_native_ffi_bound_options_get_min_zoom: (get-property min-zoom)
 *
 * Gets `MaplibreNativeFfiBoundOptions:min-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_bound_options_get_min_zoom(
  MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_has_min_zoom:
 *
 * Whether `MaplibreNativeFfiBoundOptions:min-zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_bound_options_has_min_zoom(
  MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_set_min_zoom: (set-property min-zoom)
 *
 * Sets `MaplibreNativeFfiBoundOptions:min-zoom`.
 */
void maplibre_native_ffi_bound_options_set_min_zoom(
  MaplibreNativeFfiBoundOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiBoundOptions:max-zoom: (setter set_max_zoom) (getter get_max_zoom)
 *
 * The `max_zoom` member.
 */
/**
 * maplibre_native_ffi_bound_options_get_max_zoom: (get-property max-zoom)
 *
 * Gets `MaplibreNativeFfiBoundOptions:max-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_bound_options_get_max_zoom(
  MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_has_max_zoom:
 *
 * Whether `MaplibreNativeFfiBoundOptions:max-zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_bound_options_has_max_zoom(
  MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_set_max_zoom: (set-property max-zoom)
 *
 * Sets `MaplibreNativeFfiBoundOptions:max-zoom`.
 */
void maplibre_native_ffi_bound_options_set_max_zoom(
  MaplibreNativeFfiBoundOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiBoundOptions:min-pitch: (setter set_min_pitch) (getter get_min_pitch)
 *
 * The lowest pitch, in degrees.
 */
/**
 * maplibre_native_ffi_bound_options_get_min_pitch: (get-property min-pitch)
 *
 * Gets `MaplibreNativeFfiBoundOptions:min-pitch`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_bound_options_get_min_pitch(
  MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_has_min_pitch:
 *
 * Whether `MaplibreNativeFfiBoundOptions:min-pitch` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_bound_options_has_min_pitch(
  MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_set_min_pitch: (set-property min-pitch)
 *
 * Sets `MaplibreNativeFfiBoundOptions:min-pitch`.
 */
void maplibre_native_ffi_bound_options_set_min_pitch(
  MaplibreNativeFfiBoundOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiBoundOptions:max-pitch: (setter set_max_pitch) (getter get_max_pitch)
 *
 * The highest pitch, in degrees.
 */
/**
 * maplibre_native_ffi_bound_options_get_max_pitch: (get-property max-pitch)
 *
 * Gets `MaplibreNativeFfiBoundOptions:max-pitch`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_bound_options_get_max_pitch(
  MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_has_max_pitch:
 *
 * Whether `MaplibreNativeFfiBoundOptions:max-pitch` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_bound_options_has_max_pitch(
  MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_set_max_pitch: (set-property max-pitch)
 *
 * Sets `MaplibreNativeFfiBoundOptions:max-pitch`.
 */
void maplibre_native_ffi_bound_options_set_max_pitch(
  MaplibreNativeFfiBoundOptions* self, gdouble value
);

GType maplibre_native_ffi_edge_insets_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_EDGE_INSETS \
  (maplibre_native_ffi_edge_insets_get_type())

/**
 * maplibre_native_ffi_edge_insets_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiEdgeInsets* maplibre_native_ffi_edge_insets_copy(
  const MaplibreNativeFfiEdgeInsets* self
);

/**
 * maplibre_native_ffi_edge_insets_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_edge_insets_free(MaplibreNativeFfiEdgeInsets* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiEdgeInsets, maplibre_native_ffi_edge_insets_free
)

GType maplibre_native_ffi_camera_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA (maplibre_native_ffi_camera_get_type())

/**
 * maplibre_native_ffi_camera_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCamera* maplibre_native_ffi_camera_copy(
  const MaplibreNativeFfiCamera* self
);

/**
 * maplibre_native_ffi_camera_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_camera_free(MaplibreNativeFfiCamera* self);

/**
 * maplibre_native_ffi_camera_to_options:
 *
 * Converts this `MaplibreNativeFfiCamera` to `MaplibreNativeFfiCameraOptions`
 * that set each of its members.
 *
 * Returns: (transfer full): the options
 */
MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_to_options(
  const MaplibreNativeFfiCamera* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCamera, maplibre_native_ffi_camera_free
)

GType maplibre_native_ffi_camera_bounds_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_BOUNDS \
  (maplibre_native_ffi_camera_bounds_get_type())

/**
 * maplibre_native_ffi_camera_bounds_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCameraBounds* maplibre_native_ffi_camera_bounds_copy(
  const MaplibreNativeFfiCameraBounds* self
);

/**
 * maplibre_native_ffi_camera_bounds_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_camera_bounds_free(
  MaplibreNativeFfiCameraBounds* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCameraBounds, maplibre_native_ffi_camera_bounds_free
)

/**
 * MaplibreNativeFfiPanMomentum:
 *
 * How a gesture's pan keeps moving the map's content after release.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_pan_momentum` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_pan_momentum_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiPanMomentum` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiPanMomentum* maplibre_native_ffi_pan_momentum_new(void);

/**
 * MaplibreNativeFfiPanMomentum:velocity: (setter set_velocity) (getter get_velocity)
 *
 * How fast the map's content moved on the screen at release, in logical pixels
 * per second. The pan moves only when this is set.
 */
/**
 * maplibre_native_ffi_pan_momentum_get_velocity: (get-property velocity)
 *
 * Gets `MaplibreNativeFfiPanMomentum:velocity`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiScreenPoint* maplibre_native_ffi_pan_momentum_get_velocity(
  MaplibreNativeFfiPanMomentum* self
);

/**
 * maplibre_native_ffi_pan_momentum_has_velocity:
 *
 * Whether `MaplibreNativeFfiPanMomentum:velocity` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_pan_momentum_has_velocity(
  MaplibreNativeFfiPanMomentum* self
);

/**
 * maplibre_native_ffi_pan_momentum_set_velocity: (set-property velocity)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiPanMomentum:velocity`.
 */
void maplibre_native_ffi_pan_momentum_set_velocity(
  MaplibreNativeFfiPanMomentum* self, const MaplibreNativeFfiScreenPoint* value
);

/**
 * MaplibreNativeFfiPanMomentum:decay: (setter set_decay) (getter get_decay)
 *
 * The `decay` member.
 */
/**
 * maplibre_native_ffi_pan_momentum_get_decay: (get-property decay)
 *
 * Gets `MaplibreNativeFfiPanMomentum:decay`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiMomentumDecay* maplibre_native_ffi_pan_momentum_get_decay(
  MaplibreNativeFfiPanMomentum* self
);

/**
 * maplibre_native_ffi_pan_momentum_set_decay: (set-property decay)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiPanMomentum:decay`.
 */
void maplibre_native_ffi_pan_momentum_set_decay(
  MaplibreNativeFfiPanMomentum* self, MaplibreNativeFfiMomentumDecay* value
);

/**
 * MaplibreNativeFfiGestureMomentum:
 *
 * How a gesture keeps moving the camera after the user lets go.
 *
 * A part moves only when its velocity is set, and each part slows on its own.
 * Every part defaults to `MAPLIBRE_NATIVE_FFI_MOMENTUM_DECAY_MODEL_FRICTION`
 * with a half-life of 140 ms, and these other defaults:
 *
 * - Pan stops at 10 logical pixels per second, needs a release of at least 200,
 *   and counts a release faster than 10000 as 10000.
 * - Zoom stops at 0.01 levels per second and moves at most 2.5 levels. Its snap
 *   settles on the nearest whole level.
 * - Bearing stops at 1 degree per second and needs a release of at least 60.
 *   Its snap settles on north within 7 degrees.
 * - Pitch stops at 1 degree per second and needs a release of at least 5. Its
 *   snap settles flat within 7 degrees.
 *
 * See `mln_gesture_momentum` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_gesture_momentum_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiGestureMomentum` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiGestureMomentum* maplibre_native_ffi_gesture_momentum_new(
  void
);

/**
 * MaplibreNativeFfiGestureMomentum:pan: (setter set_pan) (getter get_pan)
 *
 * The `pan` member.
 */
/**
 * maplibre_native_ffi_gesture_momentum_get_pan: (get-property pan)
 *
 * Gets `MaplibreNativeFfiGestureMomentum:pan`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiPanMomentum* maplibre_native_ffi_gesture_momentum_get_pan(
  MaplibreNativeFfiGestureMomentum* self
);

/**
 * maplibre_native_ffi_gesture_momentum_set_pan: (set-property pan)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiGestureMomentum:pan`.
 */
void maplibre_native_ffi_gesture_momentum_set_pan(
  MaplibreNativeFfiGestureMomentum* self, MaplibreNativeFfiPanMomentum* value
);

/**
 * MaplibreNativeFfiGestureMomentum:zoom: (setter set_zoom) (getter get_zoom)
 *
 * The `zoom` member.
 */
/**
 * maplibre_native_ffi_gesture_momentum_get_zoom: (get-property zoom)
 *
 * Gets `MaplibreNativeFfiGestureMomentum:zoom`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiAnchoredMomentum*
maplibre_native_ffi_gesture_momentum_get_zoom(
  MaplibreNativeFfiGestureMomentum* self
);

/**
 * maplibre_native_ffi_gesture_momentum_set_zoom: (set-property zoom)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiGestureMomentum:zoom`.
 */
void maplibre_native_ffi_gesture_momentum_set_zoom(
  MaplibreNativeFfiGestureMomentum* self,
  MaplibreNativeFfiAnchoredMomentum* value
);

/**
 * MaplibreNativeFfiGestureMomentum:bearing: (setter set_bearing) (getter get_bearing)
 *
 * The `bearing` member.
 */
/**
 * maplibre_native_ffi_gesture_momentum_get_bearing: (get-property bearing)
 *
 * Gets `MaplibreNativeFfiGestureMomentum:bearing`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiAnchoredMomentum*
maplibre_native_ffi_gesture_momentum_get_bearing(
  MaplibreNativeFfiGestureMomentum* self
);

/**
 * maplibre_native_ffi_gesture_momentum_set_bearing: (set-property bearing)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiGestureMomentum:bearing`.
 */
void maplibre_native_ffi_gesture_momentum_set_bearing(
  MaplibreNativeFfiGestureMomentum* self,
  MaplibreNativeFfiAnchoredMomentum* value
);

/**
 * MaplibreNativeFfiGestureMomentum:pitch: (setter set_pitch) (getter get_pitch)
 *
 * The `pitch` member.
 */
/**
 * maplibre_native_ffi_gesture_momentum_get_pitch: (get-property pitch)
 *
 * Gets `MaplibreNativeFfiGestureMomentum:pitch`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiAnchoredMomentum*
maplibre_native_ffi_gesture_momentum_get_pitch(
  MaplibreNativeFfiGestureMomentum* self
);

/**
 * maplibre_native_ffi_gesture_momentum_set_pitch: (set-property pitch)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiGestureMomentum:pitch`.
 */
void maplibre_native_ffi_gesture_momentum_set_pitch(
  MaplibreNativeFfiGestureMomentum* self,
  MaplibreNativeFfiAnchoredMomentum* value
);

/**
 * MaplibreNativeFfiCameraDelta:
 *
 * A change to the camera from where it is now, for gestures.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_camera_delta` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_camera_delta_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCameraDelta` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCameraDelta* maplibre_native_ffi_camera_delta_new(void);

/**
 * MaplibreNativeFfiCameraDelta:offset: (setter set_offset) (getter get_offset)
 *
 * Moves the map's content this far on the screen, as a drag does.
 */
/**
 * maplibre_native_ffi_camera_delta_get_offset: (get-property offset)
 *
 * Gets `MaplibreNativeFfiCameraDelta:offset`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_delta_get_offset(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_has_offset:
 *
 * Whether `MaplibreNativeFfiCameraDelta:offset` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_delta_has_offset(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_offset: (set-property offset)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraDelta:offset`.
 */
void maplibre_native_ffi_camera_delta_set_offset(
  MaplibreNativeFfiCameraDelta* self, const MaplibreNativeFfiScreenPoint* value
);

/**
 * MaplibreNativeFfiCameraDelta:scale: (setter set_scale) (getter get_scale)
 *
 * Multiplies the map's scale. 2 zooms in by one level.
 */
/**
 * maplibre_native_ffi_camera_delta_get_scale: (get-property scale)
 *
 * Gets `MaplibreNativeFfiCameraDelta:scale`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_delta_get_scale(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_has_scale:
 *
 * Whether `MaplibreNativeFfiCameraDelta:scale` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_delta_has_scale(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_scale: (set-property scale)
 *
 * Sets `MaplibreNativeFfiCameraDelta:scale`.
 */
void maplibre_native_ffi_camera_delta_set_scale(
  MaplibreNativeFfiCameraDelta* self, gdouble value
);

/**
 * MaplibreNativeFfiCameraDelta:bearing: (setter set_bearing) (getter get_bearing)
 *
 * Degrees to add to the bearing.
 */
/**
 * maplibre_native_ffi_camera_delta_get_bearing: (get-property bearing)
 *
 * Gets `MaplibreNativeFfiCameraDelta:bearing`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_delta_get_bearing(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_has_bearing:
 *
 * Whether `MaplibreNativeFfiCameraDelta:bearing` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_delta_has_bearing(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_bearing: (set-property bearing)
 *
 * Sets `MaplibreNativeFfiCameraDelta:bearing`.
 */
void maplibre_native_ffi_camera_delta_set_bearing(
  MaplibreNativeFfiCameraDelta* self, gdouble value
);

/**
 * MaplibreNativeFfiCameraDelta:pitch: (setter set_pitch) (getter get_pitch)
 *
 * Degrees to add to the pitch.
 */
/**
 * maplibre_native_ffi_camera_delta_get_pitch: (get-property pitch)
 *
 * Gets `MaplibreNativeFfiCameraDelta:pitch`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_delta_get_pitch(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_has_pitch:
 *
 * Whether `MaplibreNativeFfiCameraDelta:pitch` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_delta_has_pitch(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_pitch: (set-property pitch)
 *
 * Sets `MaplibreNativeFfiCameraDelta:pitch`.
 */
void maplibre_native_ffi_camera_delta_set_pitch(
  MaplibreNativeFfiCameraDelta* self, gdouble value
);

/**
 * MaplibreNativeFfiCameraDelta:anchor: (setter set_anchor) (getter get_anchor)
 *
 * The screen point that `scale`, `bearing`, and `pitch` keep in place. It needs
 * one of them, and it combines with `offset` only in a delta without a
 * duration.
 */
/**
 * maplibre_native_ffi_camera_delta_get_anchor: (get-property anchor)
 *
 * Gets `MaplibreNativeFfiCameraDelta:anchor`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_delta_get_anchor(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_has_anchor:
 *
 * Whether `MaplibreNativeFfiCameraDelta:anchor` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_delta_has_anchor(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_anchor: (set-property anchor)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraDelta:anchor`.
 */
void maplibre_native_ffi_camera_delta_set_anchor(
  MaplibreNativeFfiCameraDelta* self, const MaplibreNativeFfiScreenPoint* value
);

/**
 * MaplibreNativeFfiCameraDelta:momentum: (setter set_momentum) (getter get_momentum)
 *
 * How the gesture keeps moving after this delta. Only a delta in the
 * `MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_END` phase carries it.
 */
/**
 * maplibre_native_ffi_camera_delta_get_momentum: (get-property momentum)
 *
 * Gets `MaplibreNativeFfiCameraDelta:momentum`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiGestureMomentum* maplibre_native_ffi_camera_delta_get_momentum(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_has_momentum:
 *
 * Whether `MaplibreNativeFfiCameraDelta:momentum` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_delta_has_momentum(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_momentum: (set-property momentum)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraDelta:momentum`.
 */
void maplibre_native_ffi_camera_delta_set_momentum(
  MaplibreNativeFfiCameraDelta* self, MaplibreNativeFfiGestureMomentum* value
);

/**
 * MaplibreNativeFfiCameraDelta:animation: (setter set_animation) (getter get_animation)
 *
 * The `animation` member.
 */
/**
 * maplibre_native_ffi_camera_delta_get_animation: (get-property animation)
 *
 * Gets `MaplibreNativeFfiCameraDelta:animation`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_delta_get_animation(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_animation: (set-property animation)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraDelta:animation`.
 */
void maplibre_native_ffi_camera_delta_set_animation(
  MaplibreNativeFfiCameraDelta* self, MaplibreNativeFfiAnimationOptions* value
);

/**
 * MaplibreNativeFfiCameraDelta:gesture-phase: (setter set_gesture_phase) (getter get_gesture_phase)
 *
 * The `gesture_phase` member.
 */
/**
 * maplibre_native_ffi_camera_delta_get_gesture_phase: (get-property gesture-phase)
 *
 * Gets `MaplibreNativeFfiCameraDelta:gesture-phase`.
 *
 * Returns: the value
 */
MaplibreNativeFfiGesturePhase
maplibre_native_ffi_camera_delta_get_gesture_phase(
  MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_gesture_phase: (set-property gesture-phase)
 *
 * Sets `MaplibreNativeFfiCameraDelta:gesture-phase`.
 */
void maplibre_native_ffi_camera_delta_set_gesture_phase(
  MaplibreNativeFfiCameraDelta* self, MaplibreNativeFfiGesturePhase value
);

/**
 * MaplibreNativeFfiCameraFitOptions:
 *
 * Options for finding the camera that fits an area.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_camera_fit_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_camera_fit_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCameraFitOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCameraFitOptions* maplibre_native_ffi_camera_fit_options_new(
  void
);

/**
 * MaplibreNativeFfiCameraFitOptions:padding: (setter set_padding) (getter get_padding)
 *
 * Space to leave between the area and each edge of the map. The left and right
 * insets must add up to less than the map's width, and the top and bottom to
 * less than its height.
 */
/**
 * maplibre_native_ffi_camera_fit_options_get_padding: (get-property padding)
 *
 * Gets `MaplibreNativeFfiCameraFitOptions:padding`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiEdgeInsets* maplibre_native_ffi_camera_fit_options_get_padding(
  MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_has_padding:
 *
 * Whether `MaplibreNativeFfiCameraFitOptions:padding` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_fit_options_has_padding(
  MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_set_padding: (set-property padding)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraFitOptions:padding`.
 */
void maplibre_native_ffi_camera_fit_options_set_padding(
  MaplibreNativeFfiCameraFitOptions* self,
  const MaplibreNativeFfiEdgeInsets* value
);

/**
 * MaplibreNativeFfiCameraFitOptions:bearing: (setter set_bearing) (getter get_bearing)
 *
 * The rotation from north, in degrees.
 */
/**
 * maplibre_native_ffi_camera_fit_options_get_bearing: (get-property bearing)
 *
 * Gets `MaplibreNativeFfiCameraFitOptions:bearing`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_fit_options_get_bearing(
  MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_has_bearing:
 *
 * Whether `MaplibreNativeFfiCameraFitOptions:bearing` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_fit_options_has_bearing(
  MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_set_bearing: (set-property bearing)
 *
 * Sets `MaplibreNativeFfiCameraFitOptions:bearing`.
 */
void maplibre_native_ffi_camera_fit_options_set_bearing(
  MaplibreNativeFfiCameraFitOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiCameraFitOptions:pitch: (setter set_pitch) (getter get_pitch)
 *
 * The tilt toward the horizon, in degrees.
 */
/**
 * maplibre_native_ffi_camera_fit_options_get_pitch: (get-property pitch)
 *
 * Gets `MaplibreNativeFfiCameraFitOptions:pitch`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_fit_options_get_pitch(
  MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_has_pitch:
 *
 * Whether `MaplibreNativeFfiCameraFitOptions:pitch` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_fit_options_has_pitch(
  MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_set_pitch: (set-property pitch)
 *
 * Sets `MaplibreNativeFfiCameraFitOptions:pitch`.
 */
void maplibre_native_ffi_camera_fit_options_set_pitch(
  MaplibreNativeFfiCameraFitOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiCameraFitUpdate:
 *
 * A camera change that fits an area, for
 * `maplibre_native_ffi_map_fit_camera_to_lat_lng_bounds_async()` and the other
 * fit commands.
 *
 * A jump uses only the end handler of `animation`, as in
 * `MaplibreNativeFfiCameraUpdate`.
 *
 * See `mln_camera_fit_update` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_camera_fit_update_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCameraFitUpdate` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCameraFitUpdate* maplibre_native_ffi_camera_fit_update_new(
  void
);

/**
 * MaplibreNativeFfiCameraFitUpdate:mode: (setter set_mode) (getter get_mode)
 *
 * The `mode` member.
 */
/**
 * maplibre_native_ffi_camera_fit_update_get_mode: (get-property mode)
 *
 * Gets `MaplibreNativeFfiCameraFitUpdate:mode`.
 *
 * Returns: the value
 */
MaplibreNativeFfiCameraUpdateMode
maplibre_native_ffi_camera_fit_update_get_mode(
  MaplibreNativeFfiCameraFitUpdate* self
);

/**
 * maplibre_native_ffi_camera_fit_update_set_mode: (set-property mode)
 *
 * Sets `MaplibreNativeFfiCameraFitUpdate:mode`.
 */
void maplibre_native_ffi_camera_fit_update_set_mode(
  MaplibreNativeFfiCameraFitUpdate* self,
  MaplibreNativeFfiCameraUpdateMode value
);

/**
 * MaplibreNativeFfiCameraFitUpdate:fit: (setter set_fit) (getter get_fit)
 *
 * The `fit` member.
 */
/**
 * maplibre_native_ffi_camera_fit_update_get_fit: (get-property fit)
 *
 * Gets `MaplibreNativeFfiCameraFitUpdate:fit`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiCameraFitOptions*
maplibre_native_ffi_camera_fit_update_get_fit(
  MaplibreNativeFfiCameraFitUpdate* self
);

/**
 * maplibre_native_ffi_camera_fit_update_set_fit: (set-property fit)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraFitUpdate:fit`.
 */
void maplibre_native_ffi_camera_fit_update_set_fit(
  MaplibreNativeFfiCameraFitUpdate* self,
  MaplibreNativeFfiCameraFitOptions* value
);

/**
 * MaplibreNativeFfiCameraFitUpdate:animation: (setter set_animation) (getter get_animation)
 *
 * The `animation` member.
 */
/**
 * maplibre_native_ffi_camera_fit_update_get_animation: (get-property animation)
 *
 * Gets `MaplibreNativeFfiCameraFitUpdate:animation`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_fit_update_get_animation(
  MaplibreNativeFfiCameraFitUpdate* self
);

/**
 * maplibre_native_ffi_camera_fit_update_set_animation: (set-property animation)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraFitUpdate:animation`.
 */
void maplibre_native_ffi_camera_fit_update_set_animation(
  MaplibreNativeFfiCameraFitUpdate* self,
  MaplibreNativeFfiAnimationOptions* value
);

/**
 * MaplibreNativeFfiCameraOptions:
 *
 * A camera, with only the members that `fields` selects.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_camera_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_camera_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCameraOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_options_new(void);

/**
 * MaplibreNativeFfiCameraOptions:center: (setter set_center) (getter get_center)
 *
 * The `center` member.
 */
/**
 * maplibre_native_ffi_camera_options_get_center: (get-property center)
 *
 * Gets `MaplibreNativeFfiCameraOptions:center`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLatLng* maplibre_native_ffi_camera_options_get_center(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_center:
 *
 * Whether `MaplibreNativeFfiCameraOptions:center` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_center(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_center: (set-property center)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraOptions:center`.
 */
void maplibre_native_ffi_camera_options_set_center(
  MaplibreNativeFfiCameraOptions* self, const MaplibreNativeFfiLatLng* value
);

/**
 * MaplibreNativeFfiCameraOptions:center-altitude: (setter set_center_altitude) (getter get_center_altitude)
 *
 * The altitude of the center, in meters above sea level.
 */
/**
 * maplibre_native_ffi_camera_options_get_center_altitude: (get-property center-altitude)
 *
 * Gets `MaplibreNativeFfiCameraOptions:center-altitude`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_center_altitude(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_center_altitude:
 *
 * Whether `MaplibreNativeFfiCameraOptions:center-altitude` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_center_altitude(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_center_altitude: (set-property center-altitude)
 *
 * Sets `MaplibreNativeFfiCameraOptions:center-altitude`.
 */
void maplibre_native_ffi_camera_options_set_center_altitude(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiCameraOptions:padding: (setter set_padding) (getter get_padding)
 *
 * Insets from the map's edges. The camera centers within the rest.
 */
/**
 * maplibre_native_ffi_camera_options_get_padding: (get-property padding)
 *
 * Gets `MaplibreNativeFfiCameraOptions:padding`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiEdgeInsets* maplibre_native_ffi_camera_options_get_padding(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_padding:
 *
 * Whether `MaplibreNativeFfiCameraOptions:padding` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_padding(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_padding: (set-property padding)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraOptions:padding`.
 */
void maplibre_native_ffi_camera_options_set_padding(
  MaplibreNativeFfiCameraOptions* self, const MaplibreNativeFfiEdgeInsets* value
);

/**
 * MaplibreNativeFfiCameraOptions:anchor: (setter set_anchor) (getter get_anchor)
 *
 * The screen point that a change to the zoom, bearing, or pitch keeps in place.
 * A `center` overrides it.
 */
/**
 * maplibre_native_ffi_camera_options_get_anchor: (get-property anchor)
 *
 * Gets `MaplibreNativeFfiCameraOptions:anchor`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_options_get_anchor(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_anchor:
 *
 * Whether `MaplibreNativeFfiCameraOptions:anchor` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_anchor(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_anchor: (set-property anchor)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraOptions:anchor`.
 */
void maplibre_native_ffi_camera_options_set_anchor(
  MaplibreNativeFfiCameraOptions* self,
  const MaplibreNativeFfiScreenPoint* value
);

/**
 * MaplibreNativeFfiCameraOptions:zoom: (setter set_zoom) (getter get_zoom)
 *
 * The `zoom` member.
 */
/**
 * maplibre_native_ffi_camera_options_get_zoom: (get-property zoom)
 *
 * Gets `MaplibreNativeFfiCameraOptions:zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_zoom(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_zoom:
 *
 * Whether `MaplibreNativeFfiCameraOptions:zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_zoom(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_zoom: (set-property zoom)
 *
 * Sets `MaplibreNativeFfiCameraOptions:zoom`.
 */
void maplibre_native_ffi_camera_options_set_zoom(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiCameraOptions:bearing: (setter set_bearing) (getter get_bearing)
 *
 * The rotation from north, in degrees.
 */
/**
 * maplibre_native_ffi_camera_options_get_bearing: (get-property bearing)
 *
 * Gets `MaplibreNativeFfiCameraOptions:bearing`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_bearing(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_bearing:
 *
 * Whether `MaplibreNativeFfiCameraOptions:bearing` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_bearing(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_bearing: (set-property bearing)
 *
 * Sets `MaplibreNativeFfiCameraOptions:bearing`.
 */
void maplibre_native_ffi_camera_options_set_bearing(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiCameraOptions:pitch: (setter set_pitch) (getter get_pitch)
 *
 * The tilt toward the horizon, in degrees. 0 looks straight down.
 */
/**
 * maplibre_native_ffi_camera_options_get_pitch: (get-property pitch)
 *
 * Gets `MaplibreNativeFfiCameraOptions:pitch`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_pitch(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_pitch:
 *
 * Whether `MaplibreNativeFfiCameraOptions:pitch` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_pitch(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_pitch: (set-property pitch)
 *
 * Sets `MaplibreNativeFfiCameraOptions:pitch`.
 */
void maplibre_native_ffi_camera_options_set_pitch(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiCameraOptions:roll: (setter set_roll) (getter get_roll)
 *
 * The roll, in degrees.
 */
/**
 * maplibre_native_ffi_camera_options_get_roll: (get-property roll)
 *
 * Gets `MaplibreNativeFfiCameraOptions:roll`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_roll(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_roll:
 *
 * Whether `MaplibreNativeFfiCameraOptions:roll` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_roll(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_roll: (set-property roll)
 *
 * Sets `MaplibreNativeFfiCameraOptions:roll`.
 */
void maplibre_native_ffi_camera_options_set_roll(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiCameraOptions:field-of-view: (setter set_field_of_view) (getter get_field_of_view)
 *
 * The vertical field of view, in degrees.
 */
/**
 * maplibre_native_ffi_camera_options_get_field_of_view: (get-property field-of-view)
 *
 * Gets `MaplibreNativeFfiCameraOptions:field-of-view`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_field_of_view(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_field_of_view:
 *
 * Whether `MaplibreNativeFfiCameraOptions:field-of-view` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_field_of_view(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_field_of_view: (set-property field-of-view)
 *
 * Sets `MaplibreNativeFfiCameraOptions:field-of-view`.
 */
void maplibre_native_ffi_camera_options_set_field_of_view(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

GType maplibre_native_ffi_camera_query_result_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_QUERY_RESULT \
  (maplibre_native_ffi_camera_query_result_get_type())

/**
 * maplibre_native_ffi_camera_query_result_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCameraQueryResult*
maplibre_native_ffi_camera_query_result_copy(
  const MaplibreNativeFfiCameraQueryResult* self
);

/**
 * maplibre_native_ffi_camera_query_result_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_camera_query_result_free(
  MaplibreNativeFfiCameraQueryResult* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCameraQueryResult,
  maplibre_native_ffi_camera_query_result_free
)

GType maplibre_native_ffi_camera_velocity_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_VELOCITY \
  (maplibre_native_ffi_camera_velocity_get_type())

/**
 * maplibre_native_ffi_camera_velocity_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCameraVelocity* maplibre_native_ffi_camera_velocity_copy(
  const MaplibreNativeFfiCameraVelocity* self
);

/**
 * maplibre_native_ffi_camera_velocity_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_camera_velocity_free(
  MaplibreNativeFfiCameraVelocity* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCameraVelocity, maplibre_native_ffi_camera_velocity_free
)

GType maplibre_native_ffi_camera_transition_end_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_TRANSITION_END \
  (maplibre_native_ffi_camera_transition_end_get_type())

/**
 * maplibre_native_ffi_camera_transition_end_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCameraTransitionEnd*
maplibre_native_ffi_camera_transition_end_copy(
  const MaplibreNativeFfiCameraTransitionEnd* self
);

/**
 * maplibre_native_ffi_camera_transition_end_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_camera_transition_end_free(
  MaplibreNativeFfiCameraTransitionEnd* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCameraTransitionEnd,
  maplibre_native_ffi_camera_transition_end_free
)

/**
 * MaplibreNativeFfiCameraUpdate:
 *
 * A camera change for `maplibre_native_ffi_map_update_camera_async()`.
 *
 * A jump uses only the end handler of `animation`, and the handler runs as soon
 * as the camera moves.
 *
 * See `mln_camera_update` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_camera_update_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCameraUpdate` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCameraUpdate* maplibre_native_ffi_camera_update_new(void);

/**
 * MaplibreNativeFfiCameraUpdate:mode: (setter set_mode) (getter get_mode)
 *
 * The `mode` member.
 */
/**
 * maplibre_native_ffi_camera_update_get_mode: (get-property mode)
 *
 * Gets `MaplibreNativeFfiCameraUpdate:mode`.
 *
 * Returns: the value
 */
MaplibreNativeFfiCameraUpdateMode maplibre_native_ffi_camera_update_get_mode(
  MaplibreNativeFfiCameraUpdate* self
);

/**
 * maplibre_native_ffi_camera_update_set_mode: (set-property mode)
 *
 * Sets `MaplibreNativeFfiCameraUpdate:mode`.
 */
void maplibre_native_ffi_camera_update_set_mode(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiCameraUpdateMode value
);

/**
 * MaplibreNativeFfiCameraUpdate:camera: (setter set_camera) (getter get_camera)
 *
 * The `camera` member.
 */
/**
 * maplibre_native_ffi_camera_update_get_camera: (get-property camera)
 *
 * Gets `MaplibreNativeFfiCameraUpdate:camera`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_update_get_camera(
  MaplibreNativeFfiCameraUpdate* self
);

/**
 * maplibre_native_ffi_camera_update_set_camera: (set-property camera)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraUpdate:camera`.
 */
void maplibre_native_ffi_camera_update_set_camera(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiCameraOptions* value
);

/**
 * MaplibreNativeFfiCameraUpdate:animation: (setter set_animation) (getter get_animation)
 *
 * The `animation` member.
 */
/**
 * maplibre_native_ffi_camera_update_get_animation: (get-property animation)
 *
 * Gets `MaplibreNativeFfiCameraUpdate:animation`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_update_get_animation(
  MaplibreNativeFfiCameraUpdate* self
);

/**
 * maplibre_native_ffi_camera_update_set_animation: (set-property animation)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiCameraUpdate:animation`.
 */
void maplibre_native_ffi_camera_update_set_animation(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiAnimationOptions* value
);

/**
 * MaplibreNativeFfiCameraUpdate:gesture-phase: (setter set_gesture_phase) (getter get_gesture_phase)
 *
 * The `gesture_phase` member.
 */
/**
 * maplibre_native_ffi_camera_update_get_gesture_phase: (get-property gesture-phase)
 *
 * Gets `MaplibreNativeFfiCameraUpdate:gesture-phase`.
 *
 * Returns: the value
 */
MaplibreNativeFfiGesturePhase
maplibre_native_ffi_camera_update_get_gesture_phase(
  MaplibreNativeFfiCameraUpdate* self
);

/**
 * maplibre_native_ffi_camera_update_set_gesture_phase: (set-property gesture-phase)
 *
 * Sets `MaplibreNativeFfiCameraUpdate:gesture-phase`.
 */
void maplibre_native_ffi_camera_update_set_gesture_phase(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiGesturePhase value
);

GType maplibre_native_ffi_canonical_tile_id_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CANONICAL_TILE_ID \
  (maplibre_native_ffi_canonical_tile_id_get_type())

/**
 * maplibre_native_ffi_canonical_tile_id_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCanonicalTileId* maplibre_native_ffi_canonical_tile_id_copy(
  const MaplibreNativeFfiCanonicalTileId* self
);

/**
 * maplibre_native_ffi_canonical_tile_id_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_canonical_tile_id_free(
  MaplibreNativeFfiCanonicalTileId* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCanonicalTileId, maplibre_native_ffi_canonical_tile_id_free
)

/**
 * MaplibreNativeFfiCustomGeometrySourceOptions:
 *
 * The callbacks and options of a custom geometry source.
 *
 * Zoom levels are whole numbers from 0 to 32.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_custom_geometry_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
/**
 * maplibre_native_ffi_custom_geometry_source_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCustomGeometrySourceOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCustomGeometrySourceOptions*
maplibre_native_ffi_custom_geometry_source_options_new(void);

/**
 * MaplibreNativeFfiCustomGeometrySourceOptions:min-zoom: (setter set_min_zoom) (getter get_min_zoom)
 *
 * The `min_zoom` member.
 */
/**
 * maplibre_native_ffi_custom_geometry_source_options_get_min_zoom: (get-property min-zoom)
 *
 * Gets `MaplibreNativeFfiCustomGeometrySourceOptions:min-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_custom_geometry_source_options_get_min_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_min_zoom:
 *
 * Whether `MaplibreNativeFfiCustomGeometrySourceOptions:min-zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_min_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_min_zoom: (set-property min-zoom)
 *
 * Sets `MaplibreNativeFfiCustomGeometrySourceOptions:min-zoom`.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_min_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiCustomGeometrySourceOptions:max-zoom: (setter set_max_zoom) (getter get_max_zoom)
 *
 * The `max_zoom` member.
 */
/**
 * maplibre_native_ffi_custom_geometry_source_options_get_max_zoom: (get-property max-zoom)
 *
 * Gets `MaplibreNativeFfiCustomGeometrySourceOptions:max-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_custom_geometry_source_options_get_max_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_max_zoom:
 *
 * Whether `MaplibreNativeFfiCustomGeometrySourceOptions:max-zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_max_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_max_zoom: (set-property max-zoom)
 *
 * Sets `MaplibreNativeFfiCustomGeometrySourceOptions:max-zoom`.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_max_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiCustomGeometrySourceOptions:tolerance: (setter set_tolerance) (getter get_tolerance)
 *
 * How far, in pixels, simplified geometry can stray from the original.
 */
/**
 * maplibre_native_ffi_custom_geometry_source_options_get_tolerance: (get-property tolerance)
 *
 * Gets `MaplibreNativeFfiCustomGeometrySourceOptions:tolerance`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_custom_geometry_source_options_get_tolerance(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_tolerance:
 *
 * Whether `MaplibreNativeFfiCustomGeometrySourceOptions:tolerance` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_tolerance(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_tolerance: (set-property tolerance)
 *
 * Sets `MaplibreNativeFfiCustomGeometrySourceOptions:tolerance`.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_tolerance(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiCustomGeometrySourceOptions:tile-size: (setter set_tile_size) (getter get_tile_size)
 *
 * The width of a tile in pixels, a power of two no larger than 8192.
 */
/**
 * maplibre_native_ffi_custom_geometry_source_options_get_tile_size: (get-property tile-size)
 *
 * Gets `MaplibreNativeFfiCustomGeometrySourceOptions:tile-size`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_custom_geometry_source_options_get_tile_size(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_tile_size:
 *
 * Whether `MaplibreNativeFfiCustomGeometrySourceOptions:tile-size` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_tile_size(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_tile_size: (set-property tile-size)
 *
 * Sets `MaplibreNativeFfiCustomGeometrySourceOptions:tile-size`.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_tile_size(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, guint32 value
);

/**
 * MaplibreNativeFfiCustomGeometrySourceOptions:buffer: (setter set_buffer) (getter get_buffer)
 *
 * The width in pixels of the margin around a tile that holds geometry from
 * beyond its edges. `buffer` times 8192 / `tile_size` must be at most 65535.
 */
/**
 * maplibre_native_ffi_custom_geometry_source_options_get_buffer: (get-property buffer)
 *
 * Gets `MaplibreNativeFfiCustomGeometrySourceOptions:buffer`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_custom_geometry_source_options_get_buffer(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_buffer:
 *
 * Whether `MaplibreNativeFfiCustomGeometrySourceOptions:buffer` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_buffer(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_buffer: (set-property buffer)
 *
 * Sets `MaplibreNativeFfiCustomGeometrySourceOptions:buffer`.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_buffer(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, guint32 value
);

/**
 * MaplibreNativeFfiCustomGeometrySourceOptions:clip: (setter set_clip) (getter get_clip)
 *
 * Whether to cut geometry at the outer edge of a tile's margin.
 */
/**
 * maplibre_native_ffi_custom_geometry_source_options_get_clip: (get-property clip)
 *
 * Gets `MaplibreNativeFfiCustomGeometrySourceOptions:clip`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_get_clip(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_clip:
 *
 * Whether `MaplibreNativeFfiCustomGeometrySourceOptions:clip` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_clip(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_clip: (set-property clip)
 *
 * Sets `MaplibreNativeFfiCustomGeometrySourceOptions:clip`.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_clip(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gboolean value
);

/**
 * MaplibreNativeFfiCustomGeometrySourceOptions:wrap: (setter set_wrap) (getter get_wrap)
 *
 * Whether to wrap geometry that crosses the antimeridian.
 */
/**
 * maplibre_native_ffi_custom_geometry_source_options_get_wrap: (get-property wrap)
 *
 * Gets `MaplibreNativeFfiCustomGeometrySourceOptions:wrap`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_get_wrap(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_wrap:
 *
 * Whether `MaplibreNativeFfiCustomGeometrySourceOptions:wrap` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_wrap(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_wrap: (set-property wrap)
 *
 * Sets `MaplibreNativeFfiCustomGeometrySourceOptions:wrap`.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_wrap(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gboolean value
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_fetch_tile:
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Asks for the data of a tile in a custom source.
 *
 * `map` and `source_id` name the source that asks. Return quickly. Once the
 * data is ready, pass it to
 * `maplibre_native_ffi_map_set_custom_geometry_source_tile_data_async()` or
 * `maplibre_native_ffi_map_set_custom_vector_source_tile_data_async()`. If it
 * fails to load, report that with
 * `maplibre_native_ffi_map_set_custom_geometry_source_tile_error_async()` or
 * `maplibre_native_ffi_map_set_custom_vector_source_tile_error_async()`.
 *
 * See `mln_custom_source_fetch_tile_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_custom_geometry_source_options_set_fetch_tile(
  MaplibreNativeFfiCustomGeometrySourceOptions* self,
  MaplibreNativeFfiCustomSourceFetchTileCallback callback, gpointer user_data,
  GDestroyNotify notify
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_cancel_tile:
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Reports that a custom source no longer needs a tile.
 *
 * `map` and `source_id` name the source.
 *
 * See `mln_custom_source_cancel_tile_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_custom_geometry_source_options_set_cancel_tile(
  MaplibreNativeFfiCustomGeometrySourceOptions* self,
  MaplibreNativeFfiCustomSourceCancelTileCallback callback, gpointer user_data,
  GDestroyNotify notify
);

/**
 * MaplibreNativeFfiCustomVectorSourceOptions:
 *
 * The callbacks and options of a custom vector source.
 *
 * Zoom levels are whole numbers from 0 to 32.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_custom_vector_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
/**
 * maplibre_native_ffi_custom_vector_source_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCustomVectorSourceOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCustomVectorSourceOptions*
maplibre_native_ffi_custom_vector_source_options_new(void);

/**
 * MaplibreNativeFfiCustomVectorSourceOptions:min-zoom: (setter set_min_zoom) (getter get_min_zoom)
 *
 * The `min_zoom` member.
 */
/**
 * maplibre_native_ffi_custom_vector_source_options_get_min_zoom: (get-property min-zoom)
 *
 * Gets `MaplibreNativeFfiCustomVectorSourceOptions:min-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_custom_vector_source_options_get_min_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self
);

/**
 * maplibre_native_ffi_custom_vector_source_options_has_min_zoom:
 *
 * Whether `MaplibreNativeFfiCustomVectorSourceOptions:min-zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_vector_source_options_has_min_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self
);

/**
 * maplibre_native_ffi_custom_vector_source_options_set_min_zoom: (set-property min-zoom)
 *
 * Sets `MaplibreNativeFfiCustomVectorSourceOptions:min-zoom`.
 */
void maplibre_native_ffi_custom_vector_source_options_set_min_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiCustomVectorSourceOptions:max-zoom: (setter set_max_zoom) (getter get_max_zoom)
 *
 * The `max_zoom` member.
 */
/**
 * maplibre_native_ffi_custom_vector_source_options_get_max_zoom: (get-property max-zoom)
 *
 * Gets `MaplibreNativeFfiCustomVectorSourceOptions:max-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_custom_vector_source_options_get_max_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self
);

/**
 * maplibre_native_ffi_custom_vector_source_options_has_max_zoom:
 *
 * Whether `MaplibreNativeFfiCustomVectorSourceOptions:max-zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_vector_source_options_has_max_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self
);

/**
 * maplibre_native_ffi_custom_vector_source_options_set_max_zoom: (set-property max-zoom)
 *
 * Sets `MaplibreNativeFfiCustomVectorSourceOptions:max-zoom`.
 */
void maplibre_native_ffi_custom_vector_source_options_set_max_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_custom_vector_source_options_set_fetch_tile:
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Asks for the data of a tile in a custom source.
 *
 * `map` and `source_id` name the source that asks. Return quickly. Once the
 * data is ready, pass it to
 * `maplibre_native_ffi_map_set_custom_geometry_source_tile_data_async()` or
 * `maplibre_native_ffi_map_set_custom_vector_source_tile_data_async()`. If it
 * fails to load, report that with
 * `maplibre_native_ffi_map_set_custom_geometry_source_tile_error_async()` or
 * `maplibre_native_ffi_map_set_custom_vector_source_tile_error_async()`.
 *
 * See `mln_custom_source_fetch_tile_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_custom_vector_source_options_set_fetch_tile(
  MaplibreNativeFfiCustomVectorSourceOptions* self,
  MaplibreNativeFfiCustomSourceFetchTileCallback callback, gpointer user_data,
  GDestroyNotify notify
);

/**
 * maplibre_native_ffi_custom_vector_source_options_set_cancel_tile:
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Reports that a custom source no longer needs a tile.
 *
 * `map` and `source_id` name the source.
 *
 * See `mln_custom_source_cancel_tile_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_custom_vector_source_options_set_cancel_tile(
  MaplibreNativeFfiCustomVectorSourceOptions* self,
  MaplibreNativeFfiCustomSourceCancelTileCallback callback, gpointer user_data,
  GDestroyNotify notify
);

GType maplibre_native_ffi_dma_buf_format_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_DMA_BUF_FORMAT \
  (maplibre_native_ffi_dma_buf_format_get_type())

/**
 * maplibre_native_ffi_dma_buf_format_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiDmaBufFormat* maplibre_native_ffi_dma_buf_format_copy(
  const MaplibreNativeFfiDmaBufFormat* self
);

/**
 * maplibre_native_ffi_dma_buf_format_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_dma_buf_format_free(
  MaplibreNativeFfiDmaBufFormat* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiDmaBufFormat, maplibre_native_ffi_dma_buf_format_free
)

GType maplibre_native_ffi_dma_buf_plane_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_DMA_BUF_PLANE \
  (maplibre_native_ffi_dma_buf_plane_get_type())

/**
 * maplibre_native_ffi_dma_buf_plane_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiDmaBufPlane* maplibre_native_ffi_dma_buf_plane_copy(
  const MaplibreNativeFfiDmaBufPlane* self
);

/**
 * maplibre_native_ffi_dma_buf_plane_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_dma_buf_plane_free(MaplibreNativeFfiDmaBufPlane* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiDmaBufPlane, maplibre_native_ffi_dma_buf_plane_free
)

/**
 * MaplibreNativeFfiDmaBufFrame:
 *
 * The dmabuf of an acquired frame.
 *
 * See `mln_dma_buf_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
 */
GType maplibre_native_ffi_dma_buf_frame_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_DMA_BUF_FRAME \
  (maplibre_native_ffi_dma_buf_frame_get_type())

/**
 * maplibre_native_ffi_dma_buf_frame_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiDmaBufFrame` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiDmaBufFrame* maplibre_native_ffi_dma_buf_frame_new(void);

/**
 * maplibre_native_ffi_dma_buf_frame_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiDmaBufFrame* maplibre_native_ffi_dma_buf_frame_copy(
  const MaplibreNativeFfiDmaBufFrame* self
);

/**
 * maplibre_native_ffi_dma_buf_frame_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_dma_buf_frame_free(MaplibreNativeFfiDmaBufFrame* self);

/**
 * maplibre_native_ffi_dma_buf_frame_get_width:
 *
 * The `width` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_dma_buf_frame_get_width(
  const MaplibreNativeFfiDmaBufFrame* self
);

/**
 * maplibre_native_ffi_dma_buf_frame_get_height:
 *
 * The `height` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_dma_buf_frame_get_height(
  const MaplibreNativeFfiDmaBufFrame* self
);

/**
 * maplibre_native_ffi_dma_buf_frame_get_scale_factor:
 *
 * The `scale_factor` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_dma_buf_frame_get_scale_factor(
  const MaplibreNativeFfiDmaBufFrame* self
);

/**
 * maplibre_native_ffi_dma_buf_frame_get_slot:
 *
 * The buffer's index in the ring.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_dma_buf_frame_get_slot(
  const MaplibreNativeFfiDmaBufFrame* self
);

/**
 * maplibre_native_ffi_dma_buf_frame_get_fourcc:
 *
 * The `fourcc` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_dma_buf_frame_get_fourcc(
  const MaplibreNativeFfiDmaBufFrame* self
);

/**
 * maplibre_native_ffi_dma_buf_frame_get_modifier:
 *
 * The `modifier` member.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_dma_buf_frame_get_modifier(
  const MaplibreNativeFfiDmaBufFrame* self
);

/**
 * maplibre_native_ffi_dma_buf_frame_get_planes:
 *
 * The buffer's planes, in the order that its fourcc and modifier define. Import
 * all of them, as EGL_EXT_image_dma_buf_import_modifiers planes 0 and up, or
 * with one `zwp_linux_buffer_params_v1.add` call each.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiDmaBufPlane): the items
 */
GPtrArray* maplibre_native_ffi_dma_buf_frame_get_planes(
  const MaplibreNativeFfiDmaBufFrame* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiDmaBufFrame, maplibre_native_ffi_dma_buf_frame_free
)

GType maplibre_native_ffi_logical_size_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE \
  (maplibre_native_ffi_logical_size_get_type())

/**
 * maplibre_native_ffi_logical_size_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiLogicalSize* maplibre_native_ffi_logical_size_copy(
  const MaplibreNativeFfiLogicalSize* self
);

/**
 * maplibre_native_ffi_logical_size_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_logical_size_free(MaplibreNativeFfiLogicalSize* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiLogicalSize, maplibre_native_ffi_logical_size_free
)

/**
 * MaplibreNativeFfiDmaBufRingDescriptor:
 *
 * A ring of dmabufs that the library allocates, for
 * `maplibre_native_ffi_map_attach_dma_buf_ring()`.
 *
 * Each buffer has the map's size in pixels and a format and modifier from
 * `formats`. The library renders only DRM_FORMAT_ABGR8888 and
 * DRM_FORMAT_ARGB8888. A modifier can give a buffer more planes than its format
 * has, such as a plane of compression metadata, and the library takes any such
 * modifier that its GPU can render into and share.
 *
 * See `mln_dma_buf_ring_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
 */
/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiDmaBufRingDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiDmaBufRingDescriptor*
maplibre_native_ffi_dma_buf_ring_descriptor_new(void);

/**
 * MaplibreNativeFfiDmaBufRingDescriptor:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiDmaBufRingDescriptor:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_dma_buf_ring_descriptor_get_logical_size(
  MaplibreNativeFfiDmaBufRingDescriptor* self
);

/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiDmaBufRingDescriptor:logical-size`.
 */
void maplibre_native_ffi_dma_buf_ring_descriptor_set_logical_size(
  MaplibreNativeFfiDmaBufRingDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiDmaBufRingDescriptor:buffer-count: (setter set_buffer_count) (getter get_buffer_count)
 *
 * How many buffers the ring has, from 1 to 8.
 */
/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_get_buffer_count: (get-property buffer-count)
 *
 * Gets `MaplibreNativeFfiDmaBufRingDescriptor:buffer-count`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_dma_buf_ring_descriptor_get_buffer_count(
  MaplibreNativeFfiDmaBufRingDescriptor* self
);

/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_set_buffer_count: (set-property buffer-count)
 *
 * Sets `MaplibreNativeFfiDmaBufRingDescriptor:buffer-count`.
 */
void maplibre_native_ffi_dma_buf_ring_descriptor_set_buffer_count(
  MaplibreNativeFfiDmaBufRingDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_get_formats:
 *
 * Gets the value that `maplibre_native_ffi_dma_buf_ring_descriptor_set_formats` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiDmaBufFormat): the items
 */
GPtrArray* maplibre_native_ffi_dma_buf_ring_descriptor_get_formats(
  MaplibreNativeFfiDmaBufRingDescriptor* self
);

/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_set_formats:
 * @value: (array length=value_count): the items
 *
 * The formats and modifiers that your reader imports, most preferred first,
 * such as those that eglQueryDmaBufModifiersEXT() reports.
 */
void maplibre_native_ffi_dma_buf_ring_descriptor_set_formats(
  MaplibreNativeFfiDmaBufRingDescriptor* self,
  const MaplibreNativeFfiDmaBufFormat* value, gsize value_count
);

/**
 * MaplibreNativeFfiEglContextDescriptor:
 *
 * The EGL objects that a render session makes its OpenGL context from.
 *
 * See `mln_egl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
/**
 * maplibre_native_ffi_egl_context_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiEglContextDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiEglContextDescriptor*
maplibre_native_ffi_egl_context_descriptor_new(void);

/**
 * MaplibreNativeFfiEglContextDescriptor:display: (setter set_display) (getter get_display)
 *
 * An `EGLDisplay`.
 */
/**
 * maplibre_native_ffi_egl_context_descriptor_get_display: (get-property display)
 *
 * Gets `MaplibreNativeFfiEglContextDescriptor:display`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_egl_context_descriptor_get_display(
  MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_set_display: (set-property display)
 *
 * Sets `MaplibreNativeFfiEglContextDescriptor:display`.
 */
void maplibre_native_ffi_egl_context_descriptor_set_display(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiEglContextDescriptor:config: (setter set_config) (getter get_config)
 *
 * An `EGLConfig`.
 */
/**
 * maplibre_native_ffi_egl_context_descriptor_get_config: (get-property config)
 *
 * Gets `MaplibreNativeFfiEglContextDescriptor:config`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_egl_context_descriptor_get_config(
  MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_set_config: (set-property config)
 *
 * Sets `MaplibreNativeFfiEglContextDescriptor:config`.
 */
void maplibre_native_ffi_egl_context_descriptor_set_config(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiEglContextDescriptor:share-context: (setter set_share_context) (getter get_share_context)
 *
 * Your `EGLContext`, for a shared context.
 */
/**
 * maplibre_native_ffi_egl_context_descriptor_get_share_context: (get-property share-context)
 *
 * Gets `MaplibreNativeFfiEglContextDescriptor:share-context`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_egl_context_descriptor_get_share_context(
  MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_set_share_context: (set-property share-context)
 *
 * Sets `MaplibreNativeFfiEglContextDescriptor:share-context`.
 */
void maplibre_native_ffi_egl_context_descriptor_set_share_context(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiEglContextDescriptor:client-api: (setter set_client_api) (getter get_client_api)
 *
 * The API of a dedicated context. A shared context uses the API of yours.
 */
/**
 * maplibre_native_ffi_egl_context_descriptor_get_client_api: (get-property client-api)
 *
 * Gets `MaplibreNativeFfiEglContextDescriptor:client-api`.
 *
 * Returns: the value
 */
MaplibreNativeFfiOpenGlClientApi
maplibre_native_ffi_egl_context_descriptor_get_client_api(
  MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_set_client_api: (set-property client-api)
 *
 * Sets `MaplibreNativeFfiEglContextDescriptor:client-api`.
 */
void maplibre_native_ffi_egl_context_descriptor_set_client_api(
  MaplibreNativeFfiEglContextDescriptor* self,
  MaplibreNativeFfiOpenGlClientApi value
);

/**
 * MaplibreNativeFfiEglContextDescriptor:get-proc-address: (setter set_get_proc_address) (getter get_get_proc_address)
 *
 * Your `eglGetProcAddress`, or null to use the system's.
 */
/**
 * maplibre_native_ffi_egl_context_descriptor_get_get_proc_address: (get-property get-proc-address)
 *
 * Gets `MaplibreNativeFfiEglContextDescriptor:get-proc-address`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_egl_context_descriptor_get_get_proc_address(
  MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_set_get_proc_address: (set-property get-proc-address)
 *
 * Sets `MaplibreNativeFfiEglContextDescriptor:get-proc-address`.
 */
void maplibre_native_ffi_egl_context_descriptor_set_get_proc_address(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiFeatureId:
 *
 * The ID of a feature in a source, as the source's data gives it.
 *
 * The map compares feature IDs as text. A number names the same feature as the
 * string of its shortest decimal form, so 7, 7.0, and "7" name one feature.
 *
 * See `mln_feature_id` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_feature_id_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_FEATURE_ID \
  (maplibre_native_ffi_feature_id_get_type())

/**
 * maplibre_native_ffi_feature_id_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiFeatureId* maplibre_native_ffi_feature_id_copy(
  const MaplibreNativeFfiFeatureId* self
);

/**
 * maplibre_native_ffi_feature_id_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_feature_id_free(MaplibreNativeFfiFeatureId* self);

/**
 * maplibre_native_ffi_feature_id_new_string_value: (constructor)
 * @value: the UTF-8 text
 *
 * The `string_value` variant.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiFeatureId* maplibre_native_ffi_feature_id_new_string_value(
  const char* value
);

/**
 * maplibre_native_ffi_feature_id_is_string_value:
 *
 * Whether the value holds `maplibre_native_ffi_feature_id_get_string_value`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_feature_id_is_string_value(
  const MaplibreNativeFfiFeatureId* self
);

/**
 * maplibre_native_ffi_feature_id_get_string_value:
 *
 * The `string_value` variant.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_feature_id_get_string_value(
  const MaplibreNativeFfiFeatureId* self
);

/**
 * maplibre_native_ffi_feature_id_new_uint_value: (constructor)
 *
 * The `uint_value` variant.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiFeatureId* maplibre_native_ffi_feature_id_new_uint_value(
  guint64 value
);

/**
 * maplibre_native_ffi_feature_id_is_uint_value:
 *
 * Whether the value holds `maplibre_native_ffi_feature_id_get_uint_value`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_feature_id_is_uint_value(
  const MaplibreNativeFfiFeatureId* self
);

/**
 * maplibre_native_ffi_feature_id_get_uint_value:
 *
 * The `uint_value` variant.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_feature_id_get_uint_value(
  const MaplibreNativeFfiFeatureId* self
);

/**
 * maplibre_native_ffi_feature_id_new_int_value: (constructor)
 *
 * The `int_value` variant.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiFeatureId* maplibre_native_ffi_feature_id_new_int_value(
  gint64 value
);

/**
 * maplibre_native_ffi_feature_id_is_int_value:
 *
 * Whether the value holds `maplibre_native_ffi_feature_id_get_int_value`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_feature_id_is_int_value(
  const MaplibreNativeFfiFeatureId* self
);

/**
 * maplibre_native_ffi_feature_id_get_int_value:
 *
 * The `int_value` variant.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_feature_id_get_int_value(
  const MaplibreNativeFfiFeatureId* self
);

/**
 * maplibre_native_ffi_feature_id_new_double_value: (constructor)
 *
 * A finite number.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiFeatureId* maplibre_native_ffi_feature_id_new_double_value(
  gdouble value
);

/**
 * maplibre_native_ffi_feature_id_is_double_value:
 *
 * Whether the value holds `maplibre_native_ffi_feature_id_get_double_value`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_feature_id_is_double_value(
  const MaplibreNativeFfiFeatureId* self
);

/**
 * maplibre_native_ffi_feature_id_get_double_value:
 *
 * A finite number.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_feature_id_get_double_value(
  const MaplibreNativeFfiFeatureId* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiFeatureId, maplibre_native_ffi_feature_id_free
)

/**
 * MaplibreNativeFfiFeatureStateSelector:
 *
 * The features whose state to set, read, or remove.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_feature_state_selector` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_feature_state_selector_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiFeatureStateSelector` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiFeatureStateSelector*
maplibre_native_ffi_feature_state_selector_new(void);

/**
 * MaplibreNativeFfiFeatureStateSelector:source-id: (setter set_source_id) (getter get_source_id)
 *
 * The `source_id` member.
 */
/**
 * maplibre_native_ffi_feature_state_selector_get_source_id: (get-property source-id)
 *
 * Gets `MaplibreNativeFfiFeatureStateSelector:source-id`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_feature_state_selector_get_source_id(
  MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_set_source_id: (set-property source-id)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiFeatureStateSelector:source-id`.
 */
void maplibre_native_ffi_feature_state_selector_set_source_id(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
);

/**
 * MaplibreNativeFfiFeatureStateSelector:source-layer-id: (setter set_source_layer_id) (getter get_source_layer_id)
 *
 * The source layer, for a vector source. Empty for other sources.
 */
/**
 * maplibre_native_ffi_feature_state_selector_get_source_layer_id: (get-property source-layer-id)
 *
 * Gets `MaplibreNativeFfiFeatureStateSelector:source-layer-id`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_feature_state_selector_get_source_layer_id(
  MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_set_source_layer_id: (set-property source-layer-id)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiFeatureStateSelector:source-layer-id`.
 */
void maplibre_native_ffi_feature_state_selector_set_source_layer_id(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
);

/**
 * MaplibreNativeFfiFeatureStateSelector:feature-id: (setter set_feature_id) (getter get_feature_id)
 *
 * The feature's ID. Setting or reading state needs it. Without it,
 * `maplibre_native_ffi_map_remove_feature_state_async()` removes the state of
 * every feature in the source layer.
 */
/**
 * maplibre_native_ffi_feature_state_selector_get_feature_id: (get-property feature-id)
 *
 * Gets `MaplibreNativeFfiFeatureStateSelector:feature-id`.
 *
 * Returns: (transfer none) (nullable): the value
 */
MaplibreNativeFfiFeatureId*
maplibre_native_ffi_feature_state_selector_get_feature_id(
  MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_has_feature_id:
 *
 * Whether `MaplibreNativeFfiFeatureStateSelector:feature-id` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_feature_state_selector_has_feature_id(
  MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_set_feature_id: (set-property feature-id)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiFeatureStateSelector:feature-id`.
 */
void maplibre_native_ffi_feature_state_selector_set_feature_id(
  MaplibreNativeFfiFeatureStateSelector* self,
  const MaplibreNativeFfiFeatureId* value
);

/**
 * MaplibreNativeFfiFeatureStateSelector:state-key: (setter set_state_key) (getter get_state_key)
 *
 * The one key that `maplibre_native_ffi_map_remove_feature_state_async()`
 * removes. It needs `feature_id`. Empty removes every key.
 */
/**
 * maplibre_native_ffi_feature_state_selector_get_state_key: (get-property state-key)
 *
 * Gets `MaplibreNativeFfiFeatureStateSelector:state-key`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_feature_state_selector_get_state_key(
  MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_set_state_key: (set-property state-key)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiFeatureStateSelector:state-key`.
 */
void maplibre_native_ffi_feature_state_selector_set_state_key(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
);

GType maplibre_native_ffi_frame_demand_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_FRAME_DEMAND \
  (maplibre_native_ffi_frame_demand_get_type())

/**
 * maplibre_native_ffi_frame_demand_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiFrameDemand* maplibre_native_ffi_frame_demand_copy(
  const MaplibreNativeFfiFrameDemand* self
);

/**
 * maplibre_native_ffi_frame_demand_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_frame_demand_free(MaplibreNativeFfiFrameDemand* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiFrameDemand, maplibre_native_ffi_frame_demand_free
)

/**
 * MaplibreNativeFfiFramePacerOptions:
 *
 * Options for creating a frame pacer.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_frame_pacer_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
/**
 * maplibre_native_ffi_frame_pacer_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiFramePacerOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiFramePacerOptions* maplibre_native_ffi_frame_pacer_options_new(
  void
);

/**
 * MaplibreNativeFfiFramePacerOptions:maximum-frames-per-second: (setter set_maximum_frames_per_second) (getter get_maximum_frames_per_second)
 *
 * The most frames per second that the pacer demands, greater than zero. The
 * pacer skips display refreshes to stay within it. By default, it can demand
 * frames at every refresh.
 */
/**
 * maplibre_native_ffi_frame_pacer_options_get_maximum_frames_per_second: (get-property maximum-frames-per-second)
 *
 * Gets `MaplibreNativeFfiFramePacerOptions:maximum-frames-per-second`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_frame_pacer_options_get_maximum_frames_per_second(
  MaplibreNativeFfiFramePacerOptions* self
);

/**
 * maplibre_native_ffi_frame_pacer_options_has_maximum_frames_per_second:
 *
 * Whether `MaplibreNativeFfiFramePacerOptions:maximum-frames-per-second` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_frame_pacer_options_has_maximum_frames_per_second(
  MaplibreNativeFfiFramePacerOptions* self
);

/**
 * maplibre_native_ffi_frame_pacer_options_set_maximum_frames_per_second: (set-property maximum-frames-per-second)
 *
 * Sets `MaplibreNativeFfiFramePacerOptions:maximum-frames-per-second`.
 */
void maplibre_native_ffi_frame_pacer_options_set_maximum_frames_per_second(
  MaplibreNativeFfiFramePacerOptions* self, guint32 value
);

GType maplibre_native_ffi_vec3_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VEC3 (maplibre_native_ffi_vec3_get_type())

/**
 * maplibre_native_ffi_vec3_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiVec3* maplibre_native_ffi_vec3_copy(
  const MaplibreNativeFfiVec3* self
);

/**
 * maplibre_native_ffi_vec3_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_vec3_free(MaplibreNativeFfiVec3* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiVec3, maplibre_native_ffi_vec3_free
)

GType maplibre_native_ffi_quaternion_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_QUATERNION \
  (maplibre_native_ffi_quaternion_get_type())

/**
 * maplibre_native_ffi_quaternion_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiQuaternion* maplibre_native_ffi_quaternion_copy(
  const MaplibreNativeFfiQuaternion* self
);

/**
 * maplibre_native_ffi_quaternion_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_quaternion_free(MaplibreNativeFfiQuaternion* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiQuaternion, maplibre_native_ffi_quaternion_free
)

GType maplibre_native_ffi_free_camera_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_FREE_CAMERA \
  (maplibre_native_ffi_free_camera_get_type())

/**
 * maplibre_native_ffi_free_camera_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiFreeCamera* maplibre_native_ffi_free_camera_copy(
  const MaplibreNativeFfiFreeCamera* self
);

/**
 * maplibre_native_ffi_free_camera_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_free_camera_free(MaplibreNativeFfiFreeCamera* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiFreeCamera, maplibre_native_ffi_free_camera_free
)

/**
 * MaplibreNativeFfiFreeCameraOptions:
 *
 * The camera's position and orientation in three dimensions.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_free_camera_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_free_camera_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiFreeCameraOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiFreeCameraOptions* maplibre_native_ffi_free_camera_options_new(
  void
);

/**
 * MaplibreNativeFfiFreeCameraOptions:position: (setter set_position) (getter get_position)
 *
 * The camera's position in Web Mercator units, where the world is 1 wide, (0,
 * 0) is its northwest corner, and (1, 1) is its southeast corner. `z` is the
 * altitude in the same units.
 */
/**
 * maplibre_native_ffi_free_camera_options_get_position: (get-property position)
 *
 * Gets `MaplibreNativeFfiFreeCameraOptions:position`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiVec3* maplibre_native_ffi_free_camera_options_get_position(
  MaplibreNativeFfiFreeCameraOptions* self
);

/**
 * maplibre_native_ffi_free_camera_options_has_position:
 *
 * Whether `MaplibreNativeFfiFreeCameraOptions:position` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_free_camera_options_has_position(
  MaplibreNativeFfiFreeCameraOptions* self
);

/**
 * maplibre_native_ffi_free_camera_options_set_position: (set-property position)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiFreeCameraOptions:position`.
 */
void maplibre_native_ffi_free_camera_options_set_position(
  MaplibreNativeFfiFreeCameraOptions* self, const MaplibreNativeFfiVec3* value
);

/**
 * MaplibreNativeFfiFreeCameraOptions:orientation: (setter set_orientation) (getter get_orientation)
 *
 * The camera's rotation from looking straight down with north at the top of the
 * screen.
 */
/**
 * maplibre_native_ffi_free_camera_options_get_orientation: (get-property orientation)
 *
 * Gets `MaplibreNativeFfiFreeCameraOptions:orientation`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiQuaternion*
maplibre_native_ffi_free_camera_options_get_orientation(
  MaplibreNativeFfiFreeCameraOptions* self
);

/**
 * maplibre_native_ffi_free_camera_options_has_orientation:
 *
 * Whether `MaplibreNativeFfiFreeCameraOptions:orientation` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_free_camera_options_has_orientation(
  MaplibreNativeFfiFreeCameraOptions* self
);

/**
 * maplibre_native_ffi_free_camera_options_set_orientation: (set-property orientation)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiFreeCameraOptions:orientation`.
 */
void maplibre_native_ffi_free_camera_options_set_orientation(
  MaplibreNativeFfiFreeCameraOptions* self,
  const MaplibreNativeFfiQuaternion* value
);

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:
 *
 * Options for a GeoJSON
 * \[source\](https://maplibre.org/maplibre-style-spec/sources/#geojson).
 *
 * Zoom levels are whole numbers.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_geo_json_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
/**
 * maplibre_native_ffi_geo_json_source_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiGeoJsonSourceOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_geo_json_source_options_new(void);

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:min-zoom: (setter set_min_zoom) (getter get_min_zoom)
 *
 * The `min_zoom` member.
 */
/**
 * maplibre_native_ffi_geo_json_source_options_get_min_zoom: (get-property min-zoom)
 *
 * Gets `MaplibreNativeFfiGeoJsonSourceOptions:min-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_geo_json_source_options_get_min_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_min_zoom:
 *
 * Whether `MaplibreNativeFfiGeoJsonSourceOptions:min-zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_min_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_min_zoom: (set-property min-zoom)
 *
 * Sets `MaplibreNativeFfiGeoJsonSourceOptions:min-zoom`.
 */
void maplibre_native_ffi_geo_json_source_options_set_min_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:max-zoom: (setter set_max_zoom) (getter get_max_zoom)
 *
 * The `max_zoom` member.
 */
/**
 * maplibre_native_ffi_geo_json_source_options_get_max_zoom: (get-property max-zoom)
 *
 * Gets `MaplibreNativeFfiGeoJsonSourceOptions:max-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_geo_json_source_options_get_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_max_zoom:
 *
 * Whether `MaplibreNativeFfiGeoJsonSourceOptions:max-zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_max_zoom: (set-property max-zoom)
 *
 * Sets `MaplibreNativeFfiGeoJsonSourceOptions:max-zoom`.
 */
void maplibre_native_ffi_geo_json_source_options_set_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:tolerance: (setter set_tolerance) (getter get_tolerance)
 *
 * The `tolerance` member.
 */
/**
 * maplibre_native_ffi_geo_json_source_options_get_tolerance: (get-property tolerance)
 *
 * Gets `MaplibreNativeFfiGeoJsonSourceOptions:tolerance`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_geo_json_source_options_get_tolerance(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_tolerance:
 *
 * Whether `MaplibreNativeFfiGeoJsonSourceOptions:tolerance` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_tolerance(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_tolerance: (set-property tolerance)
 *
 * Sets `MaplibreNativeFfiGeoJsonSourceOptions:tolerance`.
 */
void maplibre_native_ffi_geo_json_source_options_set_tolerance(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:cluster-max-zoom: (setter set_cluster_max_zoom) (getter get_cluster_max_zoom)
 *
 * The last zoom level that has clusters, at most 30.
 */
/**
 * maplibre_native_ffi_geo_json_source_options_get_cluster_max_zoom: (get-property cluster-max-zoom)
 *
 * Gets `MaplibreNativeFfiGeoJsonSourceOptions:cluster-max-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_geo_json_source_options_get_cluster_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_cluster_max_zoom:
 *
 * Whether `MaplibreNativeFfiGeoJsonSourceOptions:cluster-max-zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_cluster_max_zoom: (set-property cluster-max-zoom)
 *
 * Sets `MaplibreNativeFfiGeoJsonSourceOptions:cluster-max-zoom`.
 */
void maplibre_native_ffi_geo_json_source_options_set_cluster_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:cluster-properties: (setter set_cluster_properties) (getter get_cluster_properties)
 *
 * The `clusterProperties` object, or empty for none.
 */
/**
 * maplibre_native_ffi_geo_json_source_options_get_cluster_properties: (get-property cluster-properties)
 *
 * Gets `MaplibreNativeFfiGeoJsonSourceOptions:cluster-properties`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_geo_json_source_options_get_cluster_properties(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_cluster_properties: (set-property cluster-properties)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiGeoJsonSourceOptions:cluster-properties`.
 */
void maplibre_native_ffi_geo_json_source_options_set_cluster_properties(
  MaplibreNativeFfiGeoJsonSourceOptions* self, const char* value
);

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:buffer: (setter set_buffer) (getter get_buffer)
 *
 * The `buffer` member.
 */
/**
 * maplibre_native_ffi_geo_json_source_options_get_buffer: (get-property buffer)
 *
 * Gets `MaplibreNativeFfiGeoJsonSourceOptions:buffer`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_geo_json_source_options_get_buffer(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_buffer:
 *
 * Whether `MaplibreNativeFfiGeoJsonSourceOptions:buffer` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_buffer(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_buffer: (set-property buffer)
 *
 * Sets `MaplibreNativeFfiGeoJsonSourceOptions:buffer`.
 */
void maplibre_native_ffi_geo_json_source_options_set_buffer(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint32 value
);

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:cluster-radius: (setter set_cluster_radius) (getter get_cluster_radius)
 *
 * The `cluster_radius` member.
 */
/**
 * maplibre_native_ffi_geo_json_source_options_get_cluster_radius: (get-property cluster-radius)
 *
 * Gets `MaplibreNativeFfiGeoJsonSourceOptions:cluster-radius`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_geo_json_source_options_get_cluster_radius(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_cluster_radius:
 *
 * Whether `MaplibreNativeFfiGeoJsonSourceOptions:cluster-radius` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_radius(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_cluster_radius: (set-property cluster-radius)
 *
 * Sets `MaplibreNativeFfiGeoJsonSourceOptions:cluster-radius`.
 */
void maplibre_native_ffi_geo_json_source_options_set_cluster_radius(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint32 value
);

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:cluster-min-points: (setter set_cluster_min_points) (getter get_cluster_min_points)
 *
 * At most 4294967295 on a 32-bit target.
 */
/**
 * maplibre_native_ffi_geo_json_source_options_get_cluster_min_points: (get-property cluster-min-points)
 *
 * Gets `MaplibreNativeFfiGeoJsonSourceOptions:cluster-min-points`.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_geo_json_source_options_get_cluster_min_points(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_cluster_min_points:
 *
 * Whether `MaplibreNativeFfiGeoJsonSourceOptions:cluster-min-points` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_min_points(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_cluster_min_points: (set-property cluster-min-points)
 *
 * Sets `MaplibreNativeFfiGeoJsonSourceOptions:cluster-min-points`.
 */
void maplibre_native_ffi_geo_json_source_options_set_cluster_min_points(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint64 value
);

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:line-metrics: (setter set_line_metrics) (getter get_line_metrics)
 *
 * The `line_metrics` member.
 */
/**
 * maplibre_native_ffi_geo_json_source_options_get_line_metrics: (get-property line-metrics)
 *
 * Gets `MaplibreNativeFfiGeoJsonSourceOptions:line-metrics`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_geo_json_source_options_get_line_metrics(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_line_metrics:
 *
 * Whether `MaplibreNativeFfiGeoJsonSourceOptions:line-metrics` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_line_metrics(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_line_metrics: (set-property line-metrics)
 *
 * Sets `MaplibreNativeFfiGeoJsonSourceOptions:line-metrics`.
 */
void maplibre_native_ffi_geo_json_source_options_set_line_metrics(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gboolean value
);

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:cluster: (setter set_cluster) (getter get_cluster)
 *
 * The `cluster` member.
 */
/**
 * maplibre_native_ffi_geo_json_source_options_get_cluster: (get-property cluster)
 *
 * Gets `MaplibreNativeFfiGeoJsonSourceOptions:cluster`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_geo_json_source_options_get_cluster(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_cluster:
 *
 * Whether `MaplibreNativeFfiGeoJsonSourceOptions:cluster` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_cluster(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_cluster: (set-property cluster)
 *
 * Sets `MaplibreNativeFfiGeoJsonSourceOptions:cluster`.
 */
void maplibre_native_ffi_geo_json_source_options_set_cluster(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gboolean value
);

GType maplibre_native_ffi_gpu_sync_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_GPU_SYNC \
  (maplibre_native_ffi_gpu_sync_get_type())

/**
 * maplibre_native_ffi_gpu_sync_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiGpuSync* maplibre_native_ffi_gpu_sync_copy(
  const MaplibreNativeFfiGpuSync* self
);

/**
 * maplibre_native_ffi_gpu_sync_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_gpu_sync_free(MaplibreNativeFfiGpuSync* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiGpuSync, maplibre_native_ffi_gpu_sync_free
)

/**
 * MaplibreNativeFfiHttpHeader:
 *
 * An HTTP header.
 *
 * `name` follows the rules of `maplibre_native_ffi_set_header()`.
 *
 * See `mln_http_header` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
/**
 * maplibre_native_ffi_http_header_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiHttpHeader` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiHttpHeader* maplibre_native_ffi_http_header_new(void);

/**
 * MaplibreNativeFfiHttpHeader:name: (setter set_name) (getter get_name)
 *
 * The `name` member.
 */
/**
 * maplibre_native_ffi_http_header_get_name: (get-property name)
 *
 * Gets `MaplibreNativeFfiHttpHeader:name`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_http_header_get_name(
  MaplibreNativeFfiHttpHeader* self
);

/**
 * maplibre_native_ffi_http_header_set_name: (set-property name)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiHttpHeader:name`.
 */
void maplibre_native_ffi_http_header_set_name(
  MaplibreNativeFfiHttpHeader* self, const char* value
);

/**
 * MaplibreNativeFfiHttpHeader:value: (setter set_value) (getter get_value)
 *
 * The `value` member.
 */
/**
 * maplibre_native_ffi_http_header_get_value: (get-property value)
 *
 * Gets `MaplibreNativeFfiHttpHeader:value`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_http_header_get_value(
  MaplibreNativeFfiHttpHeader* self
);

/**
 * maplibre_native_ffi_http_header_set_value: (set-property value)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiHttpHeader:value`.
 */
void maplibre_native_ffi_http_header_set_value(
  MaplibreNativeFfiHttpHeader* self, const char* value
);

/**
 * MaplibreNativeFfiHttpHeaderRule:
 *
 * Headers to add to the HTTP requests whose URL matches a pattern.
 *
 * See `mln_http_header_rule` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
/**
 * maplibre_native_ffi_http_header_rule_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiHttpHeaderRule` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiHttpHeaderRule* maplibre_native_ffi_http_header_rule_new(void);

/**
 * MaplibreNativeFfiHttpHeaderRule:kinds: (setter set_kinds) (getter get_kinds)
 *
 * The kinds of resource that the rule covers. Zero covers every kind.
 */
/**
 * maplibre_native_ffi_http_header_rule_get_kinds: (get-property kinds)
 *
 * Gets `MaplibreNativeFfiHttpHeaderRule:kinds`.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceKindFlag
maplibre_native_ffi_http_header_rule_get_kinds(
  MaplibreNativeFfiHttpHeaderRule* self
);

/**
 * maplibre_native_ffi_http_header_rule_set_kinds: (set-property kinds)
 *
 * Sets `MaplibreNativeFfiHttpHeaderRule:kinds`.
 */
void maplibre_native_ffi_http_header_rule_set_kinds(
  MaplibreNativeFfiHttpHeaderRule* self, MaplibreNativeFfiResourceKindFlag value
);

/**
 * MaplibreNativeFfiHttpHeaderRule:flags: (setter set_flags) (getter get_flags)
 *
 * The `flags` member.
 */
/**
 * maplibre_native_ffi_http_header_rule_get_flags: (get-property flags)
 *
 * Gets `MaplibreNativeFfiHttpHeaderRule:flags`.
 *
 * Returns: the value
 */
MaplibreNativeFfiUrlMatchFlag maplibre_native_ffi_http_header_rule_get_flags(
  MaplibreNativeFfiHttpHeaderRule* self
);

/**
 * maplibre_native_ffi_http_header_rule_set_flags: (set-property flags)
 *
 * Sets `MaplibreNativeFfiHttpHeaderRule:flags`.
 */
void maplibre_native_ffi_http_header_rule_set_flags(
  MaplibreNativeFfiHttpHeaderRule* self, MaplibreNativeFfiUrlMatchFlag value
);

/**
 * MaplibreNativeFfiHttpHeaderRule:url: (setter set_url) (getter get_url)
 *
 * The `url` member.
 */
/**
 * maplibre_native_ffi_http_header_rule_get_url: (get-property url)
 *
 * Gets `MaplibreNativeFfiHttpHeaderRule:url`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_http_header_rule_get_url(
  MaplibreNativeFfiHttpHeaderRule* self
);

/**
 * maplibre_native_ffi_http_header_rule_set_url: (set-property url)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiHttpHeaderRule:url`.
 */
void maplibre_native_ffi_http_header_rule_set_url(
  MaplibreNativeFfiHttpHeaderRule* self, const char* value
);

/**
 * maplibre_native_ffi_http_header_rule_get_headers:
 *
 * Gets the value that `maplibre_native_ffi_http_header_rule_set_headers` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiHttpHeader): the items
 */
GPtrArray* maplibre_native_ffi_http_header_rule_get_headers(
  MaplibreNativeFfiHttpHeaderRule* self
);

/**
 * maplibre_native_ffi_http_header_rule_set_headers:
 * @value: (array length=value_count): the items
 *
 * The `headers` member.
 */
void maplibre_native_ffi_http_header_rule_set_headers(
  MaplibreNativeFfiHttpHeaderRule* self,
  MaplibreNativeFfiHttpHeader* const* value, gsize value_count
);

/**
 * MaplibreNativeFfiHttpHeaderTransform:
 *
 * Headers to add to HTTP requests, for
 * `maplibre_native_ffi_runtime_set_http_header_transform_async()`.
 *
 * See `mln_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_http_header_transform_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_HTTP_HEADER_TRANSFORM \
  (maplibre_native_ffi_http_header_transform_get_type())

/**
 * maplibre_native_ffi_http_header_transform_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiHttpHeaderTransform` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiHttpHeaderTransform*
maplibre_native_ffi_http_header_transform_new(void);

/**
 * maplibre_native_ffi_http_header_transform_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiHttpHeaderTransform*
maplibre_native_ffi_http_header_transform_copy(
  const MaplibreNativeFfiHttpHeaderTransform* self
);

/**
 * maplibre_native_ffi_http_header_transform_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_http_header_transform_free(
  MaplibreNativeFfiHttpHeaderTransform* self
);

/**
 * maplibre_native_ffi_http_header_transform_get_rules:
 *
 * Gets the value that `maplibre_native_ffi_http_header_transform_set_rules` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiHttpHeaderRule): the items
 */
GPtrArray* maplibre_native_ffi_http_header_transform_get_rules(
  const MaplibreNativeFfiHttpHeaderTransform* self
);

/**
 * maplibre_native_ffi_http_header_transform_set_rules:
 * @value: (array length=value_count): the items
 *
 * The first rule that matches a request adds its headers, and no other.
 */
void maplibre_native_ffi_http_header_transform_set_rules(
  MaplibreNativeFfiHttpHeaderTransform* self,
  MaplibreNativeFfiHttpHeaderRule* const* value, gsize value_count
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiHttpHeaderTransform,
  maplibre_native_ffi_http_header_transform_free
)

GType maplibre_native_ffi_image_content_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_IMAGE_CONTENT \
  (maplibre_native_ffi_image_content_get_type())

/**
 * maplibre_native_ffi_image_content_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiImageContent* maplibre_native_ffi_image_content_copy(
  const MaplibreNativeFfiImageContent* self
);

/**
 * maplibre_native_ffi_image_content_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_image_content_free(
  MaplibreNativeFfiImageContent* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiImageContent, maplibre_native_ffi_image_content_free
)

GType maplibre_native_ffi_image_stretch_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_IMAGE_STRETCH \
  (maplibre_native_ffi_image_stretch_get_type())

/**
 * maplibre_native_ffi_image_stretch_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiImageStretch* maplibre_native_ffi_image_stretch_copy(
  const MaplibreNativeFfiImageStretch* self
);

/**
 * maplibre_native_ffi_image_stretch_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_image_stretch_free(
  MaplibreNativeFfiImageStretch* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiImageStretch, maplibre_native_ffi_image_stretch_free
)

/**
 * MaplibreNativeFfiIoSurfaceFrame:
 *
 * The IOSurface of an acquired frame.
 *
 * See `mln_io_surface_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
 */
GType maplibre_native_ffi_io_surface_frame_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_IO_SURFACE_FRAME \
  (maplibre_native_ffi_io_surface_frame_get_type())

/**
 * maplibre_native_ffi_io_surface_frame_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiIoSurfaceFrame` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiIoSurfaceFrame* maplibre_native_ffi_io_surface_frame_new(void);

/**
 * maplibre_native_ffi_io_surface_frame_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiIoSurfaceFrame* maplibre_native_ffi_io_surface_frame_copy(
  const MaplibreNativeFfiIoSurfaceFrame* self
);

/**
 * maplibre_native_ffi_io_surface_frame_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_io_surface_frame_free(
  MaplibreNativeFfiIoSurfaceFrame* self
);

/**
 * maplibre_native_ffi_io_surface_frame_get_width:
 *
 * The `width` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_io_surface_frame_get_width(
  const MaplibreNativeFfiIoSurfaceFrame* self
);

/**
 * maplibre_native_ffi_io_surface_frame_get_height:
 *
 * The `height` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_io_surface_frame_get_height(
  const MaplibreNativeFfiIoSurfaceFrame* self
);

/**
 * maplibre_native_ffi_io_surface_frame_get_scale_factor:
 *
 * The `scale_factor` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_io_surface_frame_get_scale_factor(
  const MaplibreNativeFfiIoSurfaceFrame* self
);

/**
 * maplibre_native_ffi_io_surface_frame_get_slot:
 *
 * The surface's index in the ring.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_io_surface_frame_get_slot(
  const MaplibreNativeFfiIoSurfaceFrame* self
);

/**
 * maplibre_native_ffi_io_surface_frame_get_surface:
 *
 * The `IOSurfaceRef`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_io_surface_frame_get_surface(
  const MaplibreNativeFfiIoSurfaceFrame* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiIoSurfaceFrame, maplibre_native_ffi_io_surface_frame_free
)

/**
 * MaplibreNativeFfiMetalContextDescriptor:
 *
 * The Metal device that a render session uses.
 *
 * See `mln_metal_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
/**
 * maplibre_native_ffi_metal_context_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMetalContextDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_context_descriptor_new(void);

/**
 * MaplibreNativeFfiMetalContextDescriptor:device: (setter set_device) (getter get_device)
 *
 * An `MTLDevice`.
 */
/**
 * maplibre_native_ffi_metal_context_descriptor_get_device: (get-property device)
 *
 * Gets `MaplibreNativeFfiMetalContextDescriptor:device`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_metal_context_descriptor_get_device(
  MaplibreNativeFfiMetalContextDescriptor* self
);

/**
 * maplibre_native_ffi_metal_context_descriptor_set_device: (set-property device)
 *
 * Sets `MaplibreNativeFfiMetalContextDescriptor:device`.
 */
void maplibre_native_ffi_metal_context_descriptor_set_device(
  MaplibreNativeFfiMetalContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiIoSurfaceRingDescriptor:
 *
 * A ring of IOSurfaces that the library allocates, for
 * `maplibre_native_ffi_map_attach_io_surface_ring()`.
 *
 * Each surface has the map's size in pixels, `logical_size` times the map's
 * scale factor, and the pixel format kCVPixelFormatType_32BGRA, which Metal
 * names MTLPixelFormatBGRA8Unorm.
 *
 * See `mln_io_surface_ring_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
 */
/**
 * maplibre_native_ffi_io_surface_ring_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiIoSurfaceRingDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiIoSurfaceRingDescriptor*
maplibre_native_ffi_io_surface_ring_descriptor_new(void);

/**
 * MaplibreNativeFfiIoSurfaceRingDescriptor:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_io_surface_ring_descriptor_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiIoSurfaceRingDescriptor:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_io_surface_ring_descriptor_get_logical_size(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self
);

/**
 * maplibre_native_ffi_io_surface_ring_descriptor_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiIoSurfaceRingDescriptor:logical-size`.
 */
void maplibre_native_ffi_io_surface_ring_descriptor_set_logical_size(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiIoSurfaceRingDescriptor:buffer-count: (setter set_buffer_count) (getter get_buffer_count)
 *
 * How many surfaces the ring has, from 1 to 8.
 */
/**
 * maplibre_native_ffi_io_surface_ring_descriptor_get_buffer_count: (get-property buffer-count)
 *
 * Gets `MaplibreNativeFfiIoSurfaceRingDescriptor:buffer-count`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_io_surface_ring_descriptor_get_buffer_count(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self
);

/**
 * maplibre_native_ffi_io_surface_ring_descriptor_set_buffer_count: (set-property buffer-count)
 *
 * Sets `MaplibreNativeFfiIoSurfaceRingDescriptor:buffer-count`.
 */
void maplibre_native_ffi_io_surface_ring_descriptor_set_buffer_count(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self, guint32 value
);

/**
 * MaplibreNativeFfiIoSurfaceRingDescriptor:context: (setter set_context) (getter get_context)
 *
 * The MTLDevice to render with. A null device picks the system's default.
 */
/**
 * maplibre_native_ffi_io_surface_ring_descriptor_get_context: (get-property context)
 *
 * Gets `MaplibreNativeFfiIoSurfaceRingDescriptor:context`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_io_surface_ring_descriptor_get_context(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self
);

/**
 * maplibre_native_ffi_io_surface_ring_descriptor_set_context: (set-property context)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiIoSurfaceRingDescriptor:context`.
 */
void maplibre_native_ffi_io_surface_ring_descriptor_set_context(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self,
  MaplibreNativeFfiMetalContextDescriptor* value
);

/**
 * MaplibreNativeFfiLogHandler:
 *
 * A log callback, for `maplibre_native_ffi_set_log_callback()`.
 *
 * Runs on the thread-default main context of the thread that started the call.
 *
 * See `mln_log_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
/**
 * maplibre_native_ffi_log_handler_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiLogHandler` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiLogHandler* maplibre_native_ffi_log_handler_new(void);

/**
 * MaplibreNativeFfiLogHandler:platform-severities: (setter set_platform_severities) (getter get_platform_severities)
 *
 * The severities that also go to the platform's log, as they do without a
 * callback. 0 sends none. A message of these severities goes to the platform's
 * log even when the handler's queue can't accept it.
 */
/**
 * maplibre_native_ffi_log_handler_get_platform_severities: (get-property platform-severities)
 *
 * Gets `MaplibreNativeFfiLogHandler:platform-severities`.
 *
 * Returns: the value
 */
MaplibreNativeFfiLogSeverityFlag
maplibre_native_ffi_log_handler_get_platform_severities(
  MaplibreNativeFfiLogHandler* self
);

/**
 * maplibre_native_ffi_log_handler_set_platform_severities: (set-property platform-severities)
 *
 * Sets `MaplibreNativeFfiLogHandler:platform-severities`.
 */
void maplibre_native_ffi_log_handler_set_platform_severities(
  MaplibreNativeFfiLogHandler* self, MaplibreNativeFfiLogSeverityFlag value
);

/**
 * maplibre_native_ffi_log_handler_set_callback:
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Receives a log message.
 *
 * Runs on the thread-default main context of the thread that started the call.
 *
 * See `mln_log_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
void maplibre_native_ffi_log_handler_set_callback(
  MaplibreNativeFfiLogHandler* self, MaplibreNativeFfiLogCallback callback,
  gpointer user_data, GDestroyNotify notify
);

GType maplibre_native_ffi_logical_extent_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_EXTENT \
  (maplibre_native_ffi_logical_extent_get_type())

/**
 * maplibre_native_ffi_logical_extent_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiLogicalExtent* maplibre_native_ffi_logical_extent_copy(
  const MaplibreNativeFfiLogicalExtent* self
);

/**
 * maplibre_native_ffi_logical_extent_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_logical_extent_free(
  MaplibreNativeFfiLogicalExtent* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiLogicalExtent, maplibre_native_ffi_logical_extent_free
)

/**
 * MaplibreNativeFfiMapOptions:
 *
 * Options for `maplibre_native_ffi_runtime_create_map()`.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_map_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_map_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMapOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMapOptions* maplibre_native_ffi_map_options_new(void);

/**
 * MaplibreNativeFfiMapOptions:initial-extent: (setter set_initial_extent) (getter get_initial_extent)
 *
 * The map's first size, and its scale factor. The scale factor stays fixed for
 * the map's life, and every render session of the map renders at it.
 */
/**
 * maplibre_native_ffi_map_options_get_initial_extent: (get-property initial-extent)
 *
 * Gets `MaplibreNativeFfiMapOptions:initial-extent`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalExtent*
maplibre_native_ffi_map_options_get_initial_extent(
  MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_set_initial_extent: (set-property initial-extent)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiMapOptions:initial-extent`.
 */
void maplibre_native_ffi_map_options_set_initial_extent(
  MaplibreNativeFfiMapOptions* self, const MaplibreNativeFfiLogicalExtent* value
);

/**
 * MaplibreNativeFfiMapOptions:map-mode: (setter set_map_mode) (getter get_map_mode)
 *
 * The `map_mode` member.
 */
/**
 * maplibre_native_ffi_map_options_get_map_mode: (get-property map-mode)
 *
 * Gets `MaplibreNativeFfiMapOptions:map-mode`.
 *
 * Returns: the value
 */
MaplibreNativeFfiMapMode maplibre_native_ffi_map_options_get_map_mode(
  MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_set_map_mode: (set-property map-mode)
 *
 * Sets `MaplibreNativeFfiMapOptions:map-mode`.
 */
void maplibre_native_ffi_map_options_set_map_mode(
  MaplibreNativeFfiMapOptions* self, MaplibreNativeFfiMapMode value
);

/**
 * MaplibreNativeFfiMapOptions:fast-pfor-enabled: (setter set_fast_pfor_enabled) (getter get_fast_pfor_enabled)
 *
 * Whether the map decodes MapLibre Tiles (MLT) that use FastPFOR integer
 * encoding. Without it, such tiles fail to load.
 */
/**
 * maplibre_native_ffi_map_options_get_fast_pfor_enabled: (get-property fast-pfor-enabled)
 *
 * Gets `MaplibreNativeFfiMapOptions:fast-pfor-enabled`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_map_options_get_fast_pfor_enabled(
  MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_set_fast_pfor_enabled: (set-property fast-pfor-enabled)
 *
 * Sets `MaplibreNativeFfiMapOptions:fast-pfor-enabled`.
 */
void maplibre_native_ffi_map_options_set_fast_pfor_enabled(
  MaplibreNativeFfiMapOptions* self, gboolean value
);

/**
 * MaplibreNativeFfiMapOptions:event-mask: (setter set_event_mask) (getter get_event_mask)
 *
 * The events that the map reports. Without it, the map reports them all.
 */
/**
 * maplibre_native_ffi_map_options_get_event_mask: (get-property event-mask)
 *
 * Gets `MaplibreNativeFfiMapOptions:event-mask`.
 *
 * Returns: the value
 */
MaplibreNativeFfiRuntimeEventFlag
maplibre_native_ffi_map_options_get_event_mask(
  MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_has_event_mask:
 *
 * Whether `MaplibreNativeFfiMapOptions:event-mask` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_options_has_event_mask(
  MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_set_event_mask: (set-property event-mask)
 *
 * Sets `MaplibreNativeFfiMapOptions:event-mask`.
 */
void maplibre_native_ffi_map_options_set_event_mask(
  MaplibreNativeFfiMapOptions* self, MaplibreNativeFfiRuntimeEventFlag value
);

/**
 * MaplibreNativeFfiMapOptions:tile-cache-enabled: (setter set_tile_cache_enabled) (getter get_tile_cache_enabled)
 *
 * Whether the map keeps tiles in memory after they leave the view, so they show
 * again without loading. Without it, the map keeps them.
 */
/**
 * maplibre_native_ffi_map_options_get_tile_cache_enabled: (get-property tile-cache-enabled)
 *
 * Gets `MaplibreNativeFfiMapOptions:tile-cache-enabled`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_map_options_get_tile_cache_enabled(
  MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_has_tile_cache_enabled:
 *
 * Whether `MaplibreNativeFfiMapOptions:tile-cache-enabled` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_options_has_tile_cache_enabled(
  MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_set_tile_cache_enabled: (set-property tile-cache-enabled)
 *
 * Sets `MaplibreNativeFfiMapOptions:tile-cache-enabled`.
 */
void maplibre_native_ffi_map_options_set_tile_cache_enabled(
  MaplibreNativeFfiMapOptions* self, gboolean value
);

/**
 * MaplibreNativeFfiMapOptions:fallback-font-faces: (setter set_fallback_font_faces) (getter get_fallback_font_faces)
 *
 * Fonts for the text that a style's own fonts don't draw, such as scripts that
 * need complex shaping, like Myanmar or Devanagari. A JSON array of font files,
 * each an object with a `url` and an optional `unicode-range`, as in a font
 * face of a style's `font-faces`, which every font stack uses after the style's
 * own font faces. Empty for none.
 */
/**
 * maplibre_native_ffi_map_options_get_fallback_font_faces: (get-property fallback-font-faces)
 *
 * Gets `MaplibreNativeFfiMapOptions:fallback-font-faces`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_map_options_get_fallback_font_faces(
  MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_set_fallback_font_faces: (set-property fallback-font-faces)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiMapOptions:fallback-font-faces`.
 */
void maplibre_native_ffi_map_options_set_fallback_font_faces(
  MaplibreNativeFfiMapOptions* self, const char* value
);

GType maplibre_native_ffi_map_projection_camera_result_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_PROJECTION_CAMERA_RESULT \
  (maplibre_native_ffi_map_projection_camera_result_get_type())

/**
 * maplibre_native_ffi_map_projection_camera_result_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMapProjectionCameraResult*
maplibre_native_ffi_map_projection_camera_result_copy(
  const MaplibreNativeFfiMapProjectionCameraResult* self
);

/**
 * maplibre_native_ffi_map_projection_camera_result_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_map_projection_camera_result_free(
  MaplibreNativeFfiMapProjectionCameraResult* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMapProjectionCameraResult,
  maplibre_native_ffi_map_projection_camera_result_free
)

GType maplibre_native_ffi_map_tile_settings_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_TILE_SETTINGS \
  (maplibre_native_ffi_map_tile_settings_get_type())

/**
 * maplibre_native_ffi_map_tile_settings_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMapTileSettings* maplibre_native_ffi_map_tile_settings_copy(
  const MaplibreNativeFfiMapTileSettings* self
);

/**
 * maplibre_native_ffi_map_tile_settings_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_map_tile_settings_free(
  MaplibreNativeFfiMapTileSettings* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMapTileSettings, maplibre_native_ffi_map_tile_settings_free
)

GType maplibre_native_ffi_projection_mode_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_PROJECTION_MODE \
  (maplibre_native_ffi_projection_mode_get_type())

/**
 * maplibre_native_ffi_projection_mode_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiProjectionMode* maplibre_native_ffi_projection_mode_copy(
  const MaplibreNativeFfiProjectionMode* self
);

/**
 * maplibre_native_ffi_projection_mode_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_projection_mode_free(
  MaplibreNativeFfiProjectionMode* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiProjectionMode, maplibre_native_ffi_projection_mode_free
)

GType maplibre_native_ffi_map_viewport_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_VIEWPORT \
  (maplibre_native_ffi_map_viewport_get_type())

/**
 * maplibre_native_ffi_map_viewport_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMapViewport* maplibre_native_ffi_map_viewport_copy(
  const MaplibreNativeFfiMapViewport* self
);

/**
 * maplibre_native_ffi_map_viewport_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_map_viewport_free(MaplibreNativeFfiMapViewport* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMapViewport, maplibre_native_ffi_map_viewport_free
)

GType maplibre_native_ffi_map_view_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_VIEW \
  (maplibre_native_ffi_map_view_get_type())

/**
 * maplibre_native_ffi_map_view_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMapView* maplibre_native_ffi_map_view_copy(
  const MaplibreNativeFfiMapView* self
);

/**
 * maplibre_native_ffi_map_view_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_map_view_free(MaplibreNativeFfiMapView* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMapView, maplibre_native_ffi_map_view_free
)

GType maplibre_native_ffi_map_snapshot_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_SNAPSHOT \
  (maplibre_native_ffi_map_snapshot_get_type())

/**
 * maplibre_native_ffi_map_snapshot_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMapSnapshot* maplibre_native_ffi_map_snapshot_copy(
  const MaplibreNativeFfiMapSnapshot* self
);

/**
 * maplibre_native_ffi_map_snapshot_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_map_snapshot_free(MaplibreNativeFfiMapSnapshot* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMapSnapshot, maplibre_native_ffi_map_snapshot_free
)

/**
 * MaplibreNativeFfiMapTileOptions:
 *
 * How the map picks the tiles to load.
 *
 * The `lod_` members lower the level of detail away from the camera, so the map
 * loads fewer tiles when the pitch is high.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_map_tile_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_map_tile_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMapTileOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMapTileOptions* maplibre_native_ffi_map_tile_options_new(void);

/**
 * MaplibreNativeFfiMapTileOptions:prefetch-zoom-delta: (setter set_prefetch_zoom_delta) (getter get_prefetch_zoom_delta)
 *
 * How many zoom levels lower the map also loads tiles for, so it can show a
 * coarse map while the current tiles load. 0 loads only the current tiles.
 */
/**
 * maplibre_native_ffi_map_tile_options_get_prefetch_zoom_delta: (get-property prefetch-zoom-delta)
 *
 * Gets `MaplibreNativeFfiMapTileOptions:prefetch-zoom-delta`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_map_tile_options_get_prefetch_zoom_delta(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_prefetch_zoom_delta:
 *
 * Whether `MaplibreNativeFfiMapTileOptions:prefetch-zoom-delta` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_prefetch_zoom_delta(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_prefetch_zoom_delta: (set-property prefetch-zoom-delta)
 *
 * Sets `MaplibreNativeFfiMapTileOptions:prefetch-zoom-delta`.
 */
void maplibre_native_ffi_map_tile_options_set_prefetch_zoom_delta(
  MaplibreNativeFfiMapTileOptions* self, guint32 value
);

/**
 * MaplibreNativeFfiMapTileOptions:lod-min-radius: (setter set_lod_min_radius) (getter get_lod_min_radius)
 *
 * The radius around the view point, in tiles, that keeps full detail.
 */
/**
 * maplibre_native_ffi_map_tile_options_get_lod_min_radius: (get-property lod-min-radius)
 *
 * Gets `MaplibreNativeFfiMapTileOptions:lod-min-radius`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_map_tile_options_get_lod_min_radius(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_lod_min_radius:
 *
 * Whether `MaplibreNativeFfiMapTileOptions:lod-min-radius` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_lod_min_radius(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_lod_min_radius: (set-property lod-min-radius)
 *
 * Sets `MaplibreNativeFfiMapTileOptions:lod-min-radius`.
 */
void maplibre_native_ffi_map_tile_options_set_lod_min_radius(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiMapTileOptions:lod-scale: (setter set_lod_scale) (getter get_lod_scale)
 *
 * Scales the distance to the view point. Above 1 lowers the detail.
 */
/**
 * maplibre_native_ffi_map_tile_options_get_lod_scale: (get-property lod-scale)
 *
 * Gets `MaplibreNativeFfiMapTileOptions:lod-scale`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_map_tile_options_get_lod_scale(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_lod_scale:
 *
 * Whether `MaplibreNativeFfiMapTileOptions:lod-scale` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_lod_scale(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_lod_scale: (set-property lod-scale)
 *
 * Sets `MaplibreNativeFfiMapTileOptions:lod-scale`.
 */
void maplibre_native_ffi_map_tile_options_set_lod_scale(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiMapTileOptions:lod-pitch-threshold: (setter set_lod_pitch_threshold) (getter get_lod_pitch_threshold)
 *
 * The pitch, in degrees, above which the map lowers the detail.
 */
/**
 * maplibre_native_ffi_map_tile_options_get_lod_pitch_threshold: (get-property lod-pitch-threshold)
 *
 * Gets `MaplibreNativeFfiMapTileOptions:lod-pitch-threshold`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_map_tile_options_get_lod_pitch_threshold(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_lod_pitch_threshold:
 *
 * Whether `MaplibreNativeFfiMapTileOptions:lod-pitch-threshold` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_lod_pitch_threshold(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_lod_pitch_threshold: (set-property lod-pitch-threshold)
 *
 * Sets `MaplibreNativeFfiMapTileOptions:lod-pitch-threshold`.
 */
void maplibre_native_ffi_map_tile_options_set_lod_pitch_threshold(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiMapTileOptions:lod-zoom-shift: (setter set_lod_zoom_shift) (getter get_lod_zoom_shift)
 *
 * Zoom levels to add for picking the detail. Below 0 loads fewer tiles.
 */
/**
 * maplibre_native_ffi_map_tile_options_get_lod_zoom_shift: (get-property lod-zoom-shift)
 *
 * Gets `MaplibreNativeFfiMapTileOptions:lod-zoom-shift`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_map_tile_options_get_lod_zoom_shift(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_lod_zoom_shift:
 *
 * Whether `MaplibreNativeFfiMapTileOptions:lod-zoom-shift` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_lod_zoom_shift(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_lod_zoom_shift: (set-property lod-zoom-shift)
 *
 * Sets `MaplibreNativeFfiMapTileOptions:lod-zoom-shift`.
 */
void maplibre_native_ffi_map_tile_options_set_lod_zoom_shift(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiMapTileOptions:lod-mode: (setter set_lod_mode) (getter get_lod_mode)
 *
 * `MAPLIBRE_NATIVE_FFI_TILE_LOD_MODE_DISTANCE` ignores `lod_min_radius`.
 */
/**
 * maplibre_native_ffi_map_tile_options_get_lod_mode: (get-property lod-mode)
 *
 * Gets `MaplibreNativeFfiMapTileOptions:lod-mode`.
 *
 * Returns: the value
 */
MaplibreNativeFfiTileLodMode maplibre_native_ffi_map_tile_options_get_lod_mode(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_lod_mode:
 *
 * Whether `MaplibreNativeFfiMapTileOptions:lod-mode` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_lod_mode(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_lod_mode: (set-property lod-mode)
 *
 * Sets `MaplibreNativeFfiMapTileOptions:lod-mode`.
 */
void maplibre_native_ffi_map_tile_options_set_lod_mode(
  MaplibreNativeFfiMapTileOptions* self, MaplibreNativeFfiTileLodMode value
);

/**
 * MaplibreNativeFfiMapViewportOptions:
 *
 * How the map orients and constrains its view.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_map_viewport_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_map_viewport_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMapViewportOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMapViewportOptions*
maplibre_native_ffi_map_viewport_options_new(void);

/**
 * MaplibreNativeFfiMapViewportOptions:north-orientation: (setter set_north_orientation) (getter get_north_orientation)
 *
 * The `north_orientation` member.
 */
/**
 * maplibre_native_ffi_map_viewport_options_get_north_orientation: (get-property north-orientation)
 *
 * Gets `MaplibreNativeFfiMapViewportOptions:north-orientation`.
 *
 * Returns: the value
 */
MaplibreNativeFfiNorthOrientation
maplibre_native_ffi_map_viewport_options_get_north_orientation(
  MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_has_north_orientation:
 *
 * Whether `MaplibreNativeFfiMapViewportOptions:north-orientation` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_viewport_options_has_north_orientation(
  MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_set_north_orientation: (set-property north-orientation)
 *
 * Sets `MaplibreNativeFfiMapViewportOptions:north-orientation`.
 */
void maplibre_native_ffi_map_viewport_options_set_north_orientation(
  MaplibreNativeFfiMapViewportOptions* self,
  MaplibreNativeFfiNorthOrientation value
);

/**
 * MaplibreNativeFfiMapViewportOptions:constrain-mode: (setter set_constrain_mode) (getter get_constrain_mode)
 *
 * The `constrain_mode` member.
 */
/**
 * maplibre_native_ffi_map_viewport_options_get_constrain_mode: (get-property constrain-mode)
 *
 * Gets `MaplibreNativeFfiMapViewportOptions:constrain-mode`.
 *
 * Returns: the value
 */
MaplibreNativeFfiConstrainMode
maplibre_native_ffi_map_viewport_options_get_constrain_mode(
  MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_has_constrain_mode:
 *
 * Whether `MaplibreNativeFfiMapViewportOptions:constrain-mode` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_viewport_options_has_constrain_mode(
  MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_set_constrain_mode: (set-property constrain-mode)
 *
 * Sets `MaplibreNativeFfiMapViewportOptions:constrain-mode`.
 */
void maplibre_native_ffi_map_viewport_options_set_constrain_mode(
  MaplibreNativeFfiMapViewportOptions* self,
  MaplibreNativeFfiConstrainMode value
);

/**
 * MaplibreNativeFfiMapViewportOptions:viewport-mode: (setter set_viewport_mode) (getter get_viewport_mode)
 *
 * The `viewport_mode` member.
 */
/**
 * maplibre_native_ffi_map_viewport_options_get_viewport_mode: (get-property viewport-mode)
 *
 * Gets `MaplibreNativeFfiMapViewportOptions:viewport-mode`.
 *
 * Returns: the value
 */
MaplibreNativeFfiViewportMode
maplibre_native_ffi_map_viewport_options_get_viewport_mode(
  MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_has_viewport_mode:
 *
 * Whether `MaplibreNativeFfiMapViewportOptions:viewport-mode` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_viewport_options_has_viewport_mode(
  MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_set_viewport_mode: (set-property viewport-mode)
 *
 * Sets `MaplibreNativeFfiMapViewportOptions:viewport-mode`.
 */
void maplibre_native_ffi_map_viewport_options_set_viewport_mode(
  MaplibreNativeFfiMapViewportOptions* self, MaplibreNativeFfiViewportMode value
);

/**
 * MaplibreNativeFfiMapViewportOptions:frustum-offset: (setter set_frustum_offset) (getter get_frustum_offset)
 *
 * The `frustum_offset` member.
 */
/**
 * maplibre_native_ffi_map_viewport_options_get_frustum_offset: (get-property frustum-offset)
 *
 * Gets `MaplibreNativeFfiMapViewportOptions:frustum-offset`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiEdgeInsets*
maplibre_native_ffi_map_viewport_options_get_frustum_offset(
  MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_has_frustum_offset:
 *
 * Whether `MaplibreNativeFfiMapViewportOptions:frustum-offset` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_viewport_options_has_frustum_offset(
  MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_set_frustum_offset: (set-property frustum-offset)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiMapViewportOptions:frustum-offset`.
 */
void maplibre_native_ffi_map_viewport_options_set_frustum_offset(
  MaplibreNativeFfiMapViewportOptions* self,
  const MaplibreNativeFfiEdgeInsets* value
);

/**
 * MaplibreNativeFfiMetalBorrowedTexture:
 *
 * A Metal texture of a borrowed ring.
 *
 * See `mln_metal_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
/**
 * maplibre_native_ffi_metal_borrowed_texture_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMetalBorrowedTexture` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMetalBorrowedTexture*
maplibre_native_ffi_metal_borrowed_texture_new(void);

/**
 * MaplibreNativeFfiMetalBorrowedTexture:texture: (setter set_texture) (getter get_texture)
 *
 * Your MTLTexture.
 */
/**
 * maplibre_native_ffi_metal_borrowed_texture_get_texture: (get-property texture)
 *
 * Gets `MaplibreNativeFfiMetalBorrowedTexture:texture`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_metal_borrowed_texture_get_texture(
  MaplibreNativeFfiMetalBorrowedTexture* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_set_texture: (set-property texture)
 *
 * Sets `MaplibreNativeFfiMetalBorrowedTexture:texture`.
 */
void maplibre_native_ffi_metal_borrowed_texture_set_texture(
  MaplibreNativeFfiMetalBorrowedTexture* self, gpointer value
);

/**
 * MaplibreNativeFfiMetalBorrowedTextureDescriptor:
 *
 * A ring of your Metal textures, for
 * `maplibre_native_ffi_map_attach_metal_borrowed_texture()`.
 *
 * `logical_size` sets the map's size. `physical_width` and `physical_height`
 * give the textures' size in pixels, which is usually `logical_size` times the
 * map's scale factor. The textures must all have that size, one device, one
 * pixel format, one sample, and MTLTextureUsageRenderTarget. The ring holds up
 * to `MAPLIBRE_NATIVE_FFI_TEXTURE_RING_MAX_DEPTH` distinct textures.
 *
 * The session uses the textures until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_metal_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMetalBorrowedTextureDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMetalBorrowedTextureDescriptor*
maplibre_native_ffi_metal_borrowed_texture_descriptor_new(void);

/**
 * MaplibreNativeFfiMetalBorrowedTextureDescriptor:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiMetalBorrowedTextureDescriptor:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiMetalBorrowedTextureDescriptor:logical-size`.
 */
void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiMetalBorrowedTextureDescriptor:physical-width: (setter set_physical_width) (getter get_physical_width)
 *
 * The `physical_width` member.
 */
/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_width: (get-property physical-width)
 *
 * Gets `MaplibreNativeFfiMetalBorrowedTextureDescriptor:physical-width`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_width: (set-property physical-width)
 *
 * Sets `MaplibreNativeFfiMetalBorrowedTextureDescriptor:physical-width`.
 */
void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, guint32 value
);

/**
 * MaplibreNativeFfiMetalBorrowedTextureDescriptor:physical-height: (setter set_physical_height) (getter get_physical_height)
 *
 * The `physical_height` member.
 */
/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_height: (get-property physical-height)
 *
 * Gets `MaplibreNativeFfiMetalBorrowedTextureDescriptor:physical-height`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_height: (set-property physical-height)
 *
 * Sets `MaplibreNativeFfiMetalBorrowedTextureDescriptor:physical-height`.
 */
void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_get_textures:
 *
 * Gets the value that `maplibre_native_ffi_metal_borrowed_texture_descriptor_set_textures` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiMetalBorrowedTexture): the items
 */
GPtrArray* maplibre_native_ffi_metal_borrowed_texture_descriptor_get_textures(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_set_textures:
 * @value: (array length=value_count): the items
 *
 * The `textures` member.
 */
void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_textures(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self,
  MaplibreNativeFfiMetalBorrowedTexture* const* value, gsize value_count
);

/**
 * MaplibreNativeFfiMetalBorrowedTextureDescriptor:command-queue: (setter set_command_queue) (getter get_command_queue)
 *
 * Your MTLCommandQueue on the textures' device, or null for a queue of the
 * session's own. With your queue, frames can use
 * `MAPLIBRE_NATIVE_FFI_GPU_SYNC_QUEUE_ORDER`.
 */
/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_get_command_queue: (get-property command-queue)
 *
 * Gets `MaplibreNativeFfiMetalBorrowedTextureDescriptor:command-queue`.
 *
 * Returns: the value
 */
gpointer
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_command_queue(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_set_command_queue: (set-property command-queue)
 *
 * Sets `MaplibreNativeFfiMetalBorrowedTextureDescriptor:command-queue`.
 */
void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_command_queue(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiMetalBorrowedTextureReplacement:
 *
 * New Metal textures for a borrowed ring, for
 * `maplibre_native_ffi_render_session_set_metal_borrowed_texture_target_async()`.
 *
 * The members and the textures follow the rules of
 * `MaplibreNativeFfiMetalBorrowedTextureDescriptor`. The ring keeps its depth,
 * device, and command queue, so give it as many textures as it attached with,
 * on the same device.
 *
 * See `mln_metal_borrowed_texture_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMetalBorrowedTextureReplacement` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMetalBorrowedTextureReplacement*
maplibre_native_ffi_metal_borrowed_texture_replacement_new(void);

/**
 * MaplibreNativeFfiMetalBorrowedTextureReplacement:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiMetalBorrowedTextureReplacement:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_borrowed_texture_replacement_get_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiMetalBorrowedTextureReplacement:logical-size`.
 */
void maplibre_native_ffi_metal_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiMetalBorrowedTextureReplacement:physical-width: (setter set_physical_width) (getter get_physical_width)
 *
 * The `physical_width` member.
 */
/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_width: (get-property physical-width)
 *
 * Gets `MaplibreNativeFfiMetalBorrowedTextureReplacement:physical-width`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_width: (set-property physical-width)
 *
 * Sets `MaplibreNativeFfiMetalBorrowedTextureReplacement:physical-width`.
 */
void maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self, guint32 value
);

/**
 * MaplibreNativeFfiMetalBorrowedTextureReplacement:physical-height: (setter set_physical_height) (getter get_physical_height)
 *
 * The `physical_height` member.
 */
/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_height: (get-property physical-height)
 *
 * Gets `MaplibreNativeFfiMetalBorrowedTextureReplacement:physical-height`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_height: (set-property physical-height)
 *
 * Sets `MaplibreNativeFfiMetalBorrowedTextureReplacement:physical-height`.
 */
void maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self, guint32 value
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_get_textures:
 *
 * Gets the value that `maplibre_native_ffi_metal_borrowed_texture_replacement_set_textures` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiMetalBorrowedTexture): the items
 */
GPtrArray* maplibre_native_ffi_metal_borrowed_texture_replacement_get_textures(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_set_textures:
 * @value: (array length=value_count): the items
 *
 * The `textures` member.
 */
void maplibre_native_ffi_metal_borrowed_texture_replacement_set_textures(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self,
  MaplibreNativeFfiMetalBorrowedTexture* const* value, gsize value_count
);

/**
 * MaplibreNativeFfiMetalSurfaceDescriptor:
 *
 * A Metal layer to render to, for
 * `maplibre_native_ffi_map_attach_metal_surface()`.
 *
 * The session sets the layer's device, pixel format, and drawable size. It uses
 * the layer until a replacement or release of the target completes, a detach or
 * release of the session completes, or an abandon of it returns.
 *
 * See `mln_metal_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
/**
 * maplibre_native_ffi_metal_surface_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMetalSurfaceDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMetalSurfaceDescriptor*
maplibre_native_ffi_metal_surface_descriptor_new(void);

/**
 * MaplibreNativeFfiMetalSurfaceDescriptor:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_metal_surface_descriptor_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiMetalSurfaceDescriptor:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_surface_descriptor_get_logical_size(
  MaplibreNativeFfiMetalSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_metal_surface_descriptor_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiMetalSurfaceDescriptor:logical-size`.
 */
void maplibre_native_ffi_metal_surface_descriptor_set_logical_size(
  MaplibreNativeFfiMetalSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiMetalSurfaceDescriptor:context: (setter set_context) (getter get_context)
 *
 * The MTLDevice to render with. A null device picks the system's default.
 */
/**
 * maplibre_native_ffi_metal_surface_descriptor_get_context: (get-property context)
 *
 * Gets `MaplibreNativeFfiMetalSurfaceDescriptor:context`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_surface_descriptor_get_context(
  MaplibreNativeFfiMetalSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_metal_surface_descriptor_set_context: (set-property context)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiMetalSurfaceDescriptor:context`.
 */
void maplibre_native_ffi_metal_surface_descriptor_set_context(
  MaplibreNativeFfiMetalSurfaceDescriptor* self,
  MaplibreNativeFfiMetalContextDescriptor* value
);

/**
 * MaplibreNativeFfiMetalSurfaceDescriptor:layer: (setter set_layer) (getter get_layer)
 *
 * Your CAMetalLayer.
 */
/**
 * maplibre_native_ffi_metal_surface_descriptor_get_layer: (get-property layer)
 *
 * Gets `MaplibreNativeFfiMetalSurfaceDescriptor:layer`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_metal_surface_descriptor_get_layer(
  MaplibreNativeFfiMetalSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_metal_surface_descriptor_set_layer: (set-property layer)
 *
 * Sets `MaplibreNativeFfiMetalSurfaceDescriptor:layer`.
 */
void maplibre_native_ffi_metal_surface_descriptor_set_layer(
  MaplibreNativeFfiMetalSurfaceDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiMetalSurfaceReplacement:
 *
 * A new Metal layer for
 * `maplibre_native_ffi_render_session_set_metal_surface_target_async()`.
 *
 * The session renders to it with the device that it attached with. It uses the
 * layer until a replacement or release of the target completes, a detach or
 * release of the session completes, or an abandon of it returns.
 *
 * See `mln_metal_surface_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
/**
 * maplibre_native_ffi_metal_surface_replacement_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMetalSurfaceReplacement` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMetalSurfaceReplacement*
maplibre_native_ffi_metal_surface_replacement_new(void);

/**
 * MaplibreNativeFfiMetalSurfaceReplacement:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_metal_surface_replacement_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiMetalSurfaceReplacement:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_surface_replacement_get_logical_size(
  MaplibreNativeFfiMetalSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_metal_surface_replacement_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiMetalSurfaceReplacement:logical-size`.
 */
void maplibre_native_ffi_metal_surface_replacement_set_logical_size(
  MaplibreNativeFfiMetalSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiMetalSurfaceReplacement:layer: (setter set_layer) (getter get_layer)
 *
 * Your CAMetalLayer.
 */
/**
 * maplibre_native_ffi_metal_surface_replacement_get_layer: (get-property layer)
 *
 * Gets `MaplibreNativeFfiMetalSurfaceReplacement:layer`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_metal_surface_replacement_get_layer(
  MaplibreNativeFfiMetalSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_metal_surface_replacement_set_layer: (set-property layer)
 *
 * Sets `MaplibreNativeFfiMetalSurfaceReplacement:layer`.
 */
void maplibre_native_ffi_metal_surface_replacement_set_layer(
  MaplibreNativeFfiMetalSurfaceReplacement* self, gpointer value
);

/**
 * MaplibreNativeFfiMetalTextureFrame:
 *
 * The Metal texture of an acquired frame.
 *
 * See `mln_metal_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
GType maplibre_native_ffi_metal_texture_frame_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_TEXTURE_FRAME \
  (maplibre_native_ffi_metal_texture_frame_get_type())

/**
 * maplibre_native_ffi_metal_texture_frame_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMetalTextureFrame` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMetalTextureFrame* maplibre_native_ffi_metal_texture_frame_new(
  void
);

/**
 * maplibre_native_ffi_metal_texture_frame_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMetalTextureFrame*
maplibre_native_ffi_metal_texture_frame_copy(
  const MaplibreNativeFfiMetalTextureFrame* self
);

/**
 * maplibre_native_ffi_metal_texture_frame_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_metal_texture_frame_free(
  MaplibreNativeFfiMetalTextureFrame* self
);

/**
 * maplibre_native_ffi_metal_texture_frame_get_width:
 *
 * The `width` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_metal_texture_frame_get_width(
  const MaplibreNativeFfiMetalTextureFrame* self
);

/**
 * maplibre_native_ffi_metal_texture_frame_get_height:
 *
 * The `height` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_metal_texture_frame_get_height(
  const MaplibreNativeFfiMetalTextureFrame* self
);

/**
 * maplibre_native_ffi_metal_texture_frame_get_scale_factor:
 *
 * The `scale_factor` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_metal_texture_frame_get_scale_factor(
  const MaplibreNativeFfiMetalTextureFrame* self
);

/**
 * maplibre_native_ffi_metal_texture_frame_get_slot:
 *
 * The texture's index in the descriptor's `textures`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_metal_texture_frame_get_slot(
  const MaplibreNativeFfiMetalTextureFrame* self
);

/**
 * maplibre_native_ffi_metal_texture_frame_get_texture:
 *
 * The MTLTexture.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_metal_texture_frame_get_texture(
  const MaplibreNativeFfiMetalTextureFrame* self
);

/**
 * maplibre_native_ffi_metal_texture_frame_get_device:
 *
 * The texture's MTLDevice.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_metal_texture_frame_get_device(
  const MaplibreNativeFfiMetalTextureFrame* self
);

/**
 * maplibre_native_ffi_metal_texture_frame_get_pixel_format:
 *
 * The texture's MTLPixelFormat.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_metal_texture_frame_get_pixel_format(
  const MaplibreNativeFfiMetalTextureFrame* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMetalTextureFrame,
  maplibre_native_ffi_metal_texture_frame_free
)

/**
 * MaplibreNativeFfiOfflineGeometryRegionDefinition:
 *
 * An offline region that covers a GeoJSON geometry.
 *
 * The members other than `geometry` mean the same as those of
 * `MaplibreNativeFfiOfflineTilePyramidRegionDefinition`.
 *
 * See `mln_offline_geometry_region_definition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_offline_geometry_region_definition_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiOfflineGeometryRegionDefinition` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOfflineGeometryRegionDefinition*
maplibre_native_ffi_offline_geometry_region_definition_new(void);

/**
 * MaplibreNativeFfiOfflineGeometryRegionDefinition:style-url: (setter set_style_url) (getter get_style_url)
 *
 * The `style_url` member.
 */
/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_style_url: (get-property style-url)
 *
 * Gets `MaplibreNativeFfiOfflineGeometryRegionDefinition:style-url`.
 *
 * Returns: (transfer none): the text
 */
const char*
maplibre_native_ffi_offline_geometry_region_definition_get_style_url(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_style_url: (set-property style-url)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiOfflineGeometryRegionDefinition:style-url`.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_style_url(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, const char* value
);

/**
 * MaplibreNativeFfiOfflineGeometryRegionDefinition:geometry: (setter set_geometry) (getter get_geometry)
 *
 * The `geometry` member.
 */
/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_geometry: (get-property geometry)
 *
 * Gets `MaplibreNativeFfiOfflineGeometryRegionDefinition:geometry`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_offline_geometry_region_definition_get_geometry(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_geometry: (set-property geometry)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiOfflineGeometryRegionDefinition:geometry`.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_geometry(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, const char* value
);

/**
 * MaplibreNativeFfiOfflineGeometryRegionDefinition:min-zoom: (setter set_min_zoom) (getter get_min_zoom)
 *
 * The `min_zoom` member.
 */
/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_min_zoom: (get-property min-zoom)
 *
 * Gets `MaplibreNativeFfiOfflineGeometryRegionDefinition:min-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_offline_geometry_region_definition_get_min_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_min_zoom: (set-property min-zoom)
 *
 * Sets `MaplibreNativeFfiOfflineGeometryRegionDefinition:min-zoom`.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_min_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gdouble value
);

/**
 * MaplibreNativeFfiOfflineGeometryRegionDefinition:max-zoom: (setter set_max_zoom) (getter get_max_zoom)
 *
 * The `max_zoom` member.
 */
/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_max_zoom: (get-property max-zoom)
 *
 * Gets `MaplibreNativeFfiOfflineGeometryRegionDefinition:max-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_offline_geometry_region_definition_get_max_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_max_zoom: (set-property max-zoom)
 *
 * Sets `MaplibreNativeFfiOfflineGeometryRegionDefinition:max-zoom`.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_max_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gdouble value
);

/**
 * MaplibreNativeFfiOfflineGeometryRegionDefinition:pixel-ratio: (setter set_pixel_ratio) (getter get_pixel_ratio)
 *
 * The `pixel_ratio` member.
 */
/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_pixel_ratio: (get-property pixel-ratio)
 *
 * Gets `MaplibreNativeFfiOfflineGeometryRegionDefinition:pixel-ratio`.
 *
 * Returns: the value
 */
gfloat maplibre_native_ffi_offline_geometry_region_definition_get_pixel_ratio(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_pixel_ratio: (set-property pixel-ratio)
 *
 * Sets `MaplibreNativeFfiOfflineGeometryRegionDefinition:pixel-ratio`.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_pixel_ratio(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gfloat value
);

/**
 * MaplibreNativeFfiOfflineGeometryRegionDefinition:include-ideographs: (setter set_include_ideographs) (getter get_include_ideographs)
 *
 * The `include_ideographs` member.
 */
/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_include_ideographs: (get-property include-ideographs)
 *
 * Gets `MaplibreNativeFfiOfflineGeometryRegionDefinition:include-ideographs`.
 *
 * Returns: the value
 */
gboolean
maplibre_native_ffi_offline_geometry_region_definition_get_include_ideographs(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_include_ideographs: (set-property include-ideographs)
 *
 * Sets `MaplibreNativeFfiOfflineGeometryRegionDefinition:include-ideographs`.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_include_ideographs(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gboolean value
);

/**
 * MaplibreNativeFfiOfflineTilePyramidRegionDefinition:
 *
 * An offline region that covers a rectangle of the map.
 *
 * See `mln_offline_tile_pyramid_region_definition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiOfflineTilePyramidRegionDefinition` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOfflineTilePyramidRegionDefinition*
maplibre_native_ffi_offline_tile_pyramid_region_definition_new(void);

/**
 * MaplibreNativeFfiOfflineTilePyramidRegionDefinition:style-url: (setter set_style_url) (getter get_style_url)
 *
 * The style whose resources the region holds. Maps use the region when they
 * load the same style URL.
 */
/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_style_url: (get-property style-url)
 *
 * Gets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:style-url`.
 *
 * Returns: (transfer none): the text
 */
const char*
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_style_url(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_style_url: (set-property style-url)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:style-url`.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_style_url(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, const char* value
);

/**
 * MaplibreNativeFfiOfflineTilePyramidRegionDefinition:bounds: (setter set_bounds) (getter get_bounds)
 *
 * The `bounds` member.
 */
/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_bounds: (get-property bounds)
 *
 * Gets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:bounds`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLatLngBounds*
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_bounds(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_bounds: (set-property bounds)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:bounds`.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_bounds(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self,
  const MaplibreNativeFfiLatLngBounds* value
);

/**
 * MaplibreNativeFfiOfflineTilePyramidRegionDefinition:min-zoom: (setter set_min_zoom) (getter get_min_zoom)
 *
 * The `min_zoom` member.
 */
/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_min_zoom: (get-property min-zoom)
 *
 * Gets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:min-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_offline_tile_pyramid_region_definition_get_min_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_min_zoom: (set-property min-zoom)
 *
 * Sets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:min-zoom`.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_min_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gdouble value
);

/**
 * MaplibreNativeFfiOfflineTilePyramidRegionDefinition:max-zoom: (setter set_max_zoom) (getter get_max_zoom)
 *
 * INFINITY covers every zoom level that the style's sources have.
 */
/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_max_zoom: (get-property max-zoom)
 *
 * Gets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:max-zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_offline_tile_pyramid_region_definition_get_max_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_max_zoom: (set-property max-zoom)
 *
 * Sets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:max-zoom`.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_max_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gdouble value
);

/**
 * MaplibreNativeFfiOfflineTilePyramidRegionDefinition:pixel-ratio: (setter set_pixel_ratio) (getter get_pixel_ratio)
 *
 * The pixel ratio of the images to download, such as 2 for dense screens.
 */
/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_pixel_ratio: (get-property pixel-ratio)
 *
 * Gets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:pixel-ratio`.
 *
 * Returns: the value
 */
gfloat
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_pixel_ratio(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_pixel_ratio: (set-property pixel-ratio)
 *
 * Sets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:pixel-ratio`.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_pixel_ratio(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gfloat value
);

/**
 * MaplibreNativeFfiOfflineTilePyramidRegionDefinition:include-ideographs: (setter set_include_ideographs) (getter get_include_ideographs)
 *
 * Whether the region holds the glyphs of CJK ideographs.
 */
/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_include_ideographs: (get-property include-ideographs)
 *
 * Gets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:include-ideographs`.
 *
 * Returns: the value
 */
gboolean
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_include_ideographs(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_include_ideographs: (set-property include-ideographs)
 *
 * Sets `MaplibreNativeFfiOfflineTilePyramidRegionDefinition:include-ideographs`.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_include_ideographs(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gboolean value
);

/**
 * MaplibreNativeFfiOfflineRegionDefinition:
 *
 * The area, zoom range, and style that an offline region covers.
 *
 * See `mln_offline_region_definition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_offline_region_definition_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFLINE_REGION_DEFINITION \
  (maplibre_native_ffi_offline_region_definition_get_type())

/**
 * maplibre_native_ffi_offline_region_definition_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOfflineRegionDefinition*
maplibre_native_ffi_offline_region_definition_copy(
  const MaplibreNativeFfiOfflineRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_region_definition_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_offline_region_definition_free(
  MaplibreNativeFfiOfflineRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_region_definition_new_tile_pyramid: (constructor)
 * @value: the value
 *
 * The `tile_pyramid` variant.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOfflineRegionDefinition*
maplibre_native_ffi_offline_region_definition_new_tile_pyramid(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* value
);

/**
 * maplibre_native_ffi_offline_region_definition_is_tile_pyramid:
 *
 * Whether the value holds `maplibre_native_ffi_offline_region_definition_get_tile_pyramid`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_offline_region_definition_is_tile_pyramid(
  const MaplibreNativeFfiOfflineRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_region_definition_get_tile_pyramid:
 *
 * The `tile_pyramid` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiOfflineTilePyramidRegionDefinition*
maplibre_native_ffi_offline_region_definition_get_tile_pyramid(
  const MaplibreNativeFfiOfflineRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_region_definition_new_geometry: (constructor)
 * @value: the value
 *
 * The `geometry` variant.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOfflineRegionDefinition*
maplibre_native_ffi_offline_region_definition_new_geometry(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* value
);

/**
 * maplibre_native_ffi_offline_region_definition_is_geometry:
 *
 * Whether the value holds `maplibre_native_ffi_offline_region_definition_get_geometry`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_offline_region_definition_is_geometry(
  const MaplibreNativeFfiOfflineRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_region_definition_get_geometry:
 *
 * The `geometry` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiOfflineGeometryRegionDefinition*
maplibre_native_ffi_offline_region_definition_get_geometry(
  const MaplibreNativeFfiOfflineRegionDefinition* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOfflineRegionDefinition,
  maplibre_native_ffi_offline_region_definition_free
)

/**
 * MaplibreNativeFfiOfflineRegionInfo:
 *
 * An offline region in the runtime's cache file.
 *
 * See `mln_offline_region_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_offline_region_info_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFLINE_REGION_INFO \
  (maplibre_native_ffi_offline_region_info_get_type())

/**
 * maplibre_native_ffi_offline_region_info_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiOfflineRegionInfo` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOfflineRegionInfo* maplibre_native_ffi_offline_region_info_new(
  void
);

/**
 * maplibre_native_ffi_offline_region_info_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOfflineRegionInfo*
maplibre_native_ffi_offline_region_info_copy(
  const MaplibreNativeFfiOfflineRegionInfo* self
);

/**
 * maplibre_native_ffi_offline_region_info_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_offline_region_info_free(
  MaplibreNativeFfiOfflineRegionInfo* self
);

/**
 * maplibre_native_ffi_offline_region_info_get_id:
 *
 * The `id` member.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_offline_region_info_get_id(
  const MaplibreNativeFfiOfflineRegionInfo* self
);

/**
 * maplibre_native_ffi_offline_region_info_get_definition:
 *
 * The `definition` member.
 *
 * Returns: (transfer none) (nullable): the value
 */
MaplibreNativeFfiOfflineRegionDefinition*
maplibre_native_ffi_offline_region_info_get_definition(
  const MaplibreNativeFfiOfflineRegionInfo* self
);

/**
 * maplibre_native_ffi_offline_region_info_get_metadata:
 *
 * The `metadata` member.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_offline_region_info_get_metadata(
  const MaplibreNativeFfiOfflineRegionInfo* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOfflineRegionInfo,
  maplibre_native_ffi_offline_region_info_free
)

GType maplibre_native_ffi_offline_region_status_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFLINE_REGION_STATUS \
  (maplibre_native_ffi_offline_region_status_get_type())

/**
 * maplibre_native_ffi_offline_region_status_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOfflineRegionStatus*
maplibre_native_ffi_offline_region_status_copy(
  const MaplibreNativeFfiOfflineRegionStatus* self
);

/**
 * maplibre_native_ffi_offline_region_status_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_offline_region_status_free(
  MaplibreNativeFfiOfflineRegionStatus* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOfflineRegionStatus,
  maplibre_native_ffi_offline_region_status_free
)

GType maplibre_native_ffi_offscreen_descriptor_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFSCREEN_DESCRIPTOR \
  (maplibre_native_ffi_offscreen_descriptor_get_type())

/**
 * maplibre_native_ffi_offscreen_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOffscreenDescriptor*
maplibre_native_ffi_offscreen_descriptor_copy(
  const MaplibreNativeFfiOffscreenDescriptor* self
);

/**
 * maplibre_native_ffi_offscreen_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_offscreen_descriptor_free(
  MaplibreNativeFfiOffscreenDescriptor* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOffscreenDescriptor,
  maplibre_native_ffi_offscreen_descriptor_free
)

/**
 * MaplibreNativeFfiOffscreenDeviceOptions:
 *
 * Options for `maplibre_native_ffi_offscreen_device_create()`.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_offscreen_device_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
 */
/**
 * maplibre_native_ffi_offscreen_device_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiOffscreenDeviceOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOffscreenDeviceOptions*
maplibre_native_ffi_offscreen_device_options_new(void);

/**
 * MaplibreNativeFfiOffscreenDeviceOptions:adapter-index: (setter set_adapter_index) (getter get_adapter_index)
 *
 * The GPU to use, as an index into the platform's list of GPUs.
 */
/**
 * maplibre_native_ffi_offscreen_device_options_get_adapter_index: (get-property adapter-index)
 *
 * Gets `MaplibreNativeFfiOffscreenDeviceOptions:adapter-index`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_offscreen_device_options_get_adapter_index(
  MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_has_adapter_index:
 *
 * Whether `MaplibreNativeFfiOffscreenDeviceOptions:adapter-index` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_offscreen_device_options_has_adapter_index(
  MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_set_adapter_index: (set-property adapter-index)
 *
 * Sets `MaplibreNativeFfiOffscreenDeviceOptions:adapter-index`.
 */
void maplibre_native_ffi_offscreen_device_options_set_adapter_index(
  MaplibreNativeFfiOffscreenDeviceOptions* self, guint32 value
);

/**
 * MaplibreNativeFfiOffscreenDeviceOptions:single-context: (setter set_single_context) (getter get_single_context)
 *
 * Whether the device's sessions take turns in one OpenGL context instead of
 * having one each. Only an EGL device reads it.
 */
/**
 * maplibre_native_ffi_offscreen_device_options_get_single_context: (get-property single-context)
 *
 * Gets `MaplibreNativeFfiOffscreenDeviceOptions:single-context`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_offscreen_device_options_get_single_context(
  MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_has_single_context:
 *
 * Whether `MaplibreNativeFfiOffscreenDeviceOptions:single-context` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_offscreen_device_options_has_single_context(
  MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_set_single_context: (set-property single-context)
 *
 * Sets `MaplibreNativeFfiOffscreenDeviceOptions:single-context`.
 */
void maplibre_native_ffi_offscreen_device_options_set_single_context(
  MaplibreNativeFfiOffscreenDeviceOptions* self, gboolean value
);

/**
 * MaplibreNativeFfiOffscreenDeviceOptions:client-api: (setter set_client_api) (getter get_client_api)
 *
 * The OpenGL API that the device's sessions render with. An EGL device defaults
 * to `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GLES`, and takes
 * `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GL` too on Linux and FreeBSD. A WGL
 * device takes only `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GL`, its default,
 * and a WebGL device only `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GLES`, its
 * default. Other devices take neither.
 */
/**
 * maplibre_native_ffi_offscreen_device_options_get_client_api: (get-property client-api)
 *
 * Gets `MaplibreNativeFfiOffscreenDeviceOptions:client-api`.
 *
 * Returns: the value
 */
MaplibreNativeFfiOpenGlClientApi
maplibre_native_ffi_offscreen_device_options_get_client_api(
  MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_has_client_api:
 *
 * Whether `MaplibreNativeFfiOffscreenDeviceOptions:client-api` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_offscreen_device_options_has_client_api(
  MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_set_client_api: (set-property client-api)
 *
 * Sets `MaplibreNativeFfiOffscreenDeviceOptions:client-api`.
 */
void maplibre_native_ffi_offscreen_device_options_set_client_api(
  MaplibreNativeFfiOffscreenDeviceOptions* self,
  MaplibreNativeFfiOpenGlClientApi value
);

GType maplibre_native_ffi_open_gl_borrowed_texture_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_BORROWED_TEXTURE \
  (maplibre_native_ffi_open_gl_borrowed_texture_get_type())

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOpenGlBorrowedTexture*
maplibre_native_ffi_open_gl_borrowed_texture_copy(
  const MaplibreNativeFfiOpenGlBorrowedTexture* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_free(
  MaplibreNativeFfiOpenGlBorrowedTexture* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOpenGlBorrowedTexture,
  maplibre_native_ffi_open_gl_borrowed_texture_free
)

/**
 * MaplibreNativeFfiWglContextDescriptor:
 *
 * The WGL objects that a render session makes its OpenGL context from.
 *
 * See `mln_wgl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
/**
 * maplibre_native_ffi_wgl_context_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiWglContextDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiWglContextDescriptor*
maplibre_native_ffi_wgl_context_descriptor_new(void);

/**
 * MaplibreNativeFfiWglContextDescriptor:device-context: (setter set_device_context) (getter get_device_context)
 *
 * An `HDC`.
 */
/**
 * maplibre_native_ffi_wgl_context_descriptor_get_device_context: (get-property device-context)
 *
 * Gets `MaplibreNativeFfiWglContextDescriptor:device-context`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wgl_context_descriptor_get_device_context(
  MaplibreNativeFfiWglContextDescriptor* self
);

/**
 * maplibre_native_ffi_wgl_context_descriptor_set_device_context: (set-property device-context)
 *
 * Sets `MaplibreNativeFfiWglContextDescriptor:device-context`.
 */
void maplibre_native_ffi_wgl_context_descriptor_set_device_context(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiWglContextDescriptor:share-context: (setter set_share_context) (getter get_share_context)
 *
 * Your `HGLRC`, for a shared context.
 */
/**
 * maplibre_native_ffi_wgl_context_descriptor_get_share_context: (get-property share-context)
 *
 * Gets `MaplibreNativeFfiWglContextDescriptor:share-context`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wgl_context_descriptor_get_share_context(
  MaplibreNativeFfiWglContextDescriptor* self
);

/**
 * maplibre_native_ffi_wgl_context_descriptor_set_share_context: (set-property share-context)
 *
 * Sets `MaplibreNativeFfiWglContextDescriptor:share-context`.
 */
void maplibre_native_ffi_wgl_context_descriptor_set_share_context(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiWglContextDescriptor:get-proc-address: (setter set_get_proc_address) (getter get_get_proc_address)
 *
 * Your `wglGetProcAddress`, or null to use the system's.
 */
/**
 * maplibre_native_ffi_wgl_context_descriptor_get_get_proc_address: (get-property get-proc-address)
 *
 * Gets `MaplibreNativeFfiWglContextDescriptor:get-proc-address`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wgl_context_descriptor_get_get_proc_address(
  MaplibreNativeFfiWglContextDescriptor* self
);

/**
 * maplibre_native_ffi_wgl_context_descriptor_set_get_proc_address: (set-property get-proc-address)
 *
 * Sets `MaplibreNativeFfiWglContextDescriptor:get-proc-address`.
 */
void maplibre_native_ffi_wgl_context_descriptor_set_get_proc_address(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiWebGlContextDescriptor:
 *
 * The WebGL context or canvas that a render session uses.
 *
 * See `mln_web_gl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
/**
 * maplibre_native_ffi_web_gl_context_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiWebGlContextDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiWebGlContextDescriptor*
maplibre_native_ffi_web_gl_context_descriptor_new(void);

/**
 * MaplibreNativeFfiWebGlContextDescriptor:kind: (setter set_kind) (getter get_kind)
 *
 * The `kind` member.
 */
/**
 * maplibre_native_ffi_web_gl_context_descriptor_get_kind: (get-property kind)
 *
 * Gets `MaplibreNativeFfiWebGlContextDescriptor:kind`.
 *
 * Returns: the value
 */
MaplibreNativeFfiWebGlContextKind
maplibre_native_ffi_web_gl_context_descriptor_get_kind(
  MaplibreNativeFfiWebGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_web_gl_context_descriptor_set_kind: (set-property kind)
 *
 * Sets `MaplibreNativeFfiWebGlContextDescriptor:kind`.
 */
void maplibre_native_ffi_web_gl_context_descriptor_set_kind(
  MaplibreNativeFfiWebGlContextDescriptor* self,
  MaplibreNativeFfiWebGlContextKind value
);

/**
 * MaplibreNativeFfiWebGlContextDescriptor:context: (setter set_context) (getter get_context)
 *
 * Your `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, for an existing context.
 */
/**
 * maplibre_native_ffi_web_gl_context_descriptor_get_context: (get-property context)
 *
 * Gets `MaplibreNativeFfiWebGlContextDescriptor:context`.
 *
 * Returns: the value
 */
gint32 maplibre_native_ffi_web_gl_context_descriptor_get_context(
  MaplibreNativeFfiWebGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_web_gl_context_descriptor_set_context: (set-property context)
 *
 * Sets `MaplibreNativeFfiWebGlContextDescriptor:context`.
 */
void maplibre_native_ffi_web_gl_context_descriptor_set_context(
  MaplibreNativeFfiWebGlContextDescriptor* self, gint32 value
);

/**
 * MaplibreNativeFfiWebGlContextDescriptor:canvas-selector: (setter set_canvas_selector) (getter get_canvas_selector)
 *
 * The canvas's CSS selector, for a transferred canvas.
 */
/**
 * maplibre_native_ffi_web_gl_context_descriptor_get_canvas_selector: (get-property canvas-selector)
 *
 * Gets `MaplibreNativeFfiWebGlContextDescriptor:canvas-selector`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_web_gl_context_descriptor_get_canvas_selector(
  MaplibreNativeFfiWebGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_web_gl_context_descriptor_set_canvas_selector: (set-property canvas-selector)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiWebGlContextDescriptor:canvas-selector`.
 */
void maplibre_native_ffi_web_gl_context_descriptor_set_canvas_selector(
  MaplibreNativeFfiWebGlContextDescriptor* self, const char* value
);

/**
 * MaplibreNativeFfiOpenGlContextDescriptorData:
 *
 * The WGL, EGL, or WebGL part of an OpenGL context descriptor.
 *
 * See `mln_open_gl_context_descriptor_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
GType maplibre_native_ffi_open_gl_context_descriptor_data_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CONTEXT_DESCRIPTOR_DATA \
  (maplibre_native_ffi_open_gl_context_descriptor_data_get_type())

/**
 * maplibre_native_ffi_open_gl_context_descriptor_data_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_data_copy(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_data_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_open_gl_context_descriptor_data_free(
  MaplibreNativeFfiOpenGlContextDescriptorData* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_data_new_wgl: (constructor)
 * @value: the value
 *
 * The `wgl` variant.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_data_new_wgl(
  MaplibreNativeFfiWglContextDescriptor* value
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_data_is_wgl:
 *
 * Whether the value holds `maplibre_native_ffi_open_gl_context_descriptor_data_get_wgl`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_open_gl_context_descriptor_data_is_wgl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_data_get_wgl:
 *
 * The `wgl` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiWglContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_data_get_wgl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_data_new_egl: (constructor)
 * @value: the value
 *
 * The `egl` variant.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_data_new_egl(
  MaplibreNativeFfiEglContextDescriptor* value
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_data_is_egl:
 *
 * Whether the value holds `maplibre_native_ffi_open_gl_context_descriptor_data_get_egl`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_open_gl_context_descriptor_data_is_egl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_data_get_egl:
 *
 * The `egl` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiEglContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_data_get_egl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_data_new_web_gl: (constructor)
 * @value: the value
 *
 * The `web_gl` variant.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_data_new_web_gl(
  MaplibreNativeFfiWebGlContextDescriptor* value
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_data_is_web_gl:
 *
 * Whether the value holds `maplibre_native_ffi_open_gl_context_descriptor_data_get_web_gl`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_open_gl_context_descriptor_data_is_web_gl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_data_get_web_gl:
 *
 * The `web_gl` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiWebGlContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_data_get_web_gl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOpenGlContextDescriptorData,
  maplibre_native_ffi_open_gl_context_descriptor_data_free
)

/**
 * MaplibreNativeFfiOpenGlContextDescriptor:
 *
 * How a render session gets the OpenGL context that it renders with.
 *
 * Keep the objects that it names alive until a detach or release of the session
 * completes, or an abandon of it returns.
 *
 * See `mln_open_gl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
/**
 * maplibre_native_ffi_open_gl_context_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiOpenGlContextDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_new(void);

/**
 * MaplibreNativeFfiOpenGlContextDescriptor:ownership: (setter set_ownership) (getter get_ownership)
 *
 * The `ownership` member.
 */
/**
 * maplibre_native_ffi_open_gl_context_descriptor_get_ownership: (get-property ownership)
 *
 * Gets `MaplibreNativeFfiOpenGlContextDescriptor:ownership`.
 *
 * Returns: the value
 */
MaplibreNativeFfiOpenGlContextOwnership
maplibre_native_ffi_open_gl_context_descriptor_get_ownership(
  MaplibreNativeFfiOpenGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_set_ownership: (set-property ownership)
 *
 * Sets `MaplibreNativeFfiOpenGlContextDescriptor:ownership`.
 */
void maplibre_native_ffi_open_gl_context_descriptor_set_ownership(
  MaplibreNativeFfiOpenGlContextDescriptor* self,
  MaplibreNativeFfiOpenGlContextOwnership value
);

/**
 * MaplibreNativeFfiOpenGlContextDescriptor:data: (setter set_data) (getter get_data)
 *
 * The `data` member.
 */
/**
 * maplibre_native_ffi_open_gl_context_descriptor_get_data: (get-property data)
 *
 * Gets `MaplibreNativeFfiOpenGlContextDescriptor:data`.
 *
 * Returns: (transfer none) (nullable): the value
 */
MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_get_data(
  MaplibreNativeFfiOpenGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_set_data: (set-property data)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiOpenGlContextDescriptor:data`.
 */
void maplibre_native_ffi_open_gl_context_descriptor_set_data(
  MaplibreNativeFfiOpenGlContextDescriptor* self,
  const MaplibreNativeFfiOpenGlContextDescriptorData* value
);

/**
 * MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:
 *
 * A ring of your OpenGL textures, for
 * `maplibre_native_ffi_map_attach_open_gl_borrowed_texture()`.
 *
 * `logical_size` sets the map's size. `physical_width` and `physical_height`
 * give the textures' size in pixels, which is usually `logical_size` times the
 * map's scale factor. The context must use
 * `MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_OWNERSHIP_SHARED`. Each texture must be
 * in your context's share group, with storage of that size. The ring holds up
 * to `MAPLIBRE_NATIVE_FFI_TEXTURE_RING_MAX_DEPTH` distinct textures.
 *
 * The session uses the textures until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_open_gl_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOpenGlBorrowedTextureDescriptor*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_new(void);

/**
 * MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:logical-size`.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:physical-width: (setter set_physical_width) (getter get_physical_width)
 *
 * The `physical_width` member.
 */
/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_width: (get-property physical-width)
 *
 * Gets `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:physical-width`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_width: (set-property physical-width)
 *
 * Sets `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:physical-width`.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
);

/**
 * MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:physical-height: (setter set_physical_height) (getter get_physical_height)
 *
 * The `physical_height` member.
 */
/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_height: (get-property physical-height)
 *
 * Gets `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:physical-height`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_height: (set-property physical-height)
 *
 * Sets `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:physical-height`.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
);

/**
 * MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:context: (setter set_context) (getter get_context)
 *
 * The `context` member.
 */
/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_context: (get-property context)
 *
 * Gets `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:context`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_context(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_context: (set-property context)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:context`.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_context(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  MaplibreNativeFfiOpenGlContextDescriptor* value
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_textures:
 *
 * Gets the value that `maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_textures` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiOpenGlBorrowedTexture): the items
 */
GPtrArray* maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_textures(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_textures:
 * @value: (array length=value_count): the items
 *
 * The `textures` member.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_textures(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiOpenGlBorrowedTexture* value, gsize value_count
);

/**
 * MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:target: (setter set_target) (getter get_target)
 *
 * The textures' target. It must be GL_TEXTURE_2D.
 */
/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_target: (get-property target)
 *
 * Gets `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:target`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_target(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_target: (set-property target)
 *
 * Sets `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor:target`.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_target(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
);

/**
 * MaplibreNativeFfiOpenGlBorrowedTextureReplacement:
 *
 * New OpenGL textures for a borrowed ring, for
 * `maplibre_native_ffi_render_session_set_open_gl_borrowed_texture_target_async()`.
 *
 * The members and the textures follow the rules of
 * `MaplibreNativeFfiOpenGlBorrowedTextureDescriptor`. The ring keeps its depth,
 * context, and texture target, so give it as many textures as it attached with,
 * in the same share group.
 *
 * See `mln_open_gl_borrowed_texture_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiOpenGlBorrowedTextureReplacement` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOpenGlBorrowedTextureReplacement*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_new(void);

/**
 * MaplibreNativeFfiOpenGlBorrowedTextureReplacement:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiOpenGlBorrowedTextureReplacement:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiOpenGlBorrowedTextureReplacement:logical-size`.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiOpenGlBorrowedTextureReplacement:physical-width: (setter set_physical_width) (getter get_physical_width)
 *
 * The `physical_width` member.
 */
/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_width: (get-property physical-width)
 *
 * Gets `MaplibreNativeFfiOpenGlBorrowedTextureReplacement:physical-width`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_width: (set-property physical-width)
 *
 * Sets `MaplibreNativeFfiOpenGlBorrowedTextureReplacement:physical-width`.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self, guint32 value
);

/**
 * MaplibreNativeFfiOpenGlBorrowedTextureReplacement:physical-height: (setter set_physical_height) (getter get_physical_height)
 *
 * The `physical_height` member.
 */
/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_height: (get-property physical-height)
 *
 * Gets `MaplibreNativeFfiOpenGlBorrowedTextureReplacement:physical-height`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_height: (set-property physical-height)
 *
 * Sets `MaplibreNativeFfiOpenGlBorrowedTextureReplacement:physical-height`.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self, guint32 value
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_textures:
 *
 * Gets the value that `maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_textures` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiOpenGlBorrowedTexture): the items
 */
GPtrArray*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_textures(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_textures:
 * @value: (array length=value_count): the items
 *
 * The `textures` member.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_textures(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self,
  const MaplibreNativeFfiOpenGlBorrowedTexture* value, gsize value_count
);

/**
 * MaplibreNativeFfiOpenGlSurfaceDescriptor:
 *
 * An OpenGL surface to present to, for
 * `maplibre_native_ffi_map_attach_open_gl_surface()`.
 *
 * The session uses the surface until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_open_gl_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
/**
 * maplibre_native_ffi_open_gl_surface_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiOpenGlSurfaceDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOpenGlSurfaceDescriptor*
maplibre_native_ffi_open_gl_surface_descriptor_new(void);

/**
 * MaplibreNativeFfiOpenGlSurfaceDescriptor:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_open_gl_surface_descriptor_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiOpenGlSurfaceDescriptor:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_surface_descriptor_get_logical_size(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_surface_descriptor_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiOpenGlSurfaceDescriptor:logical-size`.
 */
void maplibre_native_ffi_open_gl_surface_descriptor_set_logical_size(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiOpenGlSurfaceDescriptor:context: (setter set_context) (getter get_context)
 *
 * The `context` member.
 */
/**
 * maplibre_native_ffi_open_gl_surface_descriptor_get_context: (get-property context)
 *
 * Gets `MaplibreNativeFfiOpenGlSurfaceDescriptor:context`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_surface_descriptor_get_context(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_surface_descriptor_set_context: (set-property context)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiOpenGlSurfaceDescriptor:context`.
 */
void maplibre_native_ffi_open_gl_surface_descriptor_set_context(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self,
  MaplibreNativeFfiOpenGlContextDescriptor* value
);

/**
 * MaplibreNativeFfiOpenGlSurfaceDescriptor:surface: (setter set_surface) (getter get_surface)
 *
 * Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it.
 */
/**
 * maplibre_native_ffi_open_gl_surface_descriptor_get_surface: (get-property surface)
 *
 * Gets `MaplibreNativeFfiOpenGlSurfaceDescriptor:surface`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_open_gl_surface_descriptor_get_surface(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_surface_descriptor_set_surface: (set-property surface)
 *
 * Sets `MaplibreNativeFfiOpenGlSurfaceDescriptor:surface`.
 */
void maplibre_native_ffi_open_gl_surface_descriptor_set_surface(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiOpenGlSurfaceReplacement:
 *
 * A new OpenGL surface for
 * `maplibre_native_ffi_render_session_set_open_gl_surface_target_async()`.
 *
 * The session presents to it with the context that it attached with. It uses
 * the surface until a replacement or release of the target completes, a detach
 * or release of the session completes, or an abandon of it returns.
 *
 * See `mln_open_gl_surface_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
/**
 * maplibre_native_ffi_open_gl_surface_replacement_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiOpenGlSurfaceReplacement` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOpenGlSurfaceReplacement*
maplibre_native_ffi_open_gl_surface_replacement_new(void);

/**
 * MaplibreNativeFfiOpenGlSurfaceReplacement:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_open_gl_surface_replacement_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiOpenGlSurfaceReplacement:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_surface_replacement_get_logical_size(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_surface_replacement_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiOpenGlSurfaceReplacement:logical-size`.
 */
void maplibre_native_ffi_open_gl_surface_replacement_set_logical_size(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiOpenGlSurfaceReplacement:surface: (setter set_surface) (getter get_surface)
 *
 * Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it, so
 * leave it null.
 */
/**
 * maplibre_native_ffi_open_gl_surface_replacement_get_surface: (get-property surface)
 *
 * Gets `MaplibreNativeFfiOpenGlSurfaceReplacement:surface`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_open_gl_surface_replacement_get_surface(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_surface_replacement_set_surface: (set-property surface)
 *
 * Sets `MaplibreNativeFfiOpenGlSurfaceReplacement:surface`.
 */
void maplibre_native_ffi_open_gl_surface_replacement_set_surface(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self, gpointer value
);

GType maplibre_native_ffi_open_gl_texture_frame_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_TEXTURE_FRAME \
  (maplibre_native_ffi_open_gl_texture_frame_get_type())

/**
 * maplibre_native_ffi_open_gl_texture_frame_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOpenGlTextureFrame*
maplibre_native_ffi_open_gl_texture_frame_copy(
  const MaplibreNativeFfiOpenGlTextureFrame* self
);

/**
 * maplibre_native_ffi_open_gl_texture_frame_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_open_gl_texture_frame_free(
  MaplibreNativeFfiOpenGlTextureFrame* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOpenGlTextureFrame,
  maplibre_native_ffi_open_gl_texture_frame_free
)

/**
 * MaplibreNativeFfiPremultipliedRgba8Image:
 *
 * An image of 8-bit RGBA pixels whose colors are multiplied by their alpha.
 *
 * See `mln_premultiplied_rgba8_image` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
/**
 * maplibre_native_ffi_premultiplied_rgba8_image_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiPremultipliedRgba8Image` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiPremultipliedRgba8Image*
maplibre_native_ffi_premultiplied_rgba8_image_new(void);

/**
 * MaplibreNativeFfiPremultipliedRgba8Image:width: (setter set_width) (getter get_width)
 *
 * The `width` member.
 */
/**
 * maplibre_native_ffi_premultiplied_rgba8_image_get_width: (get-property width)
 *
 * Gets `MaplibreNativeFfiPremultipliedRgba8Image:width`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_width(
  MaplibreNativeFfiPremultipliedRgba8Image* self
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_set_width: (set-property width)
 *
 * Sets `MaplibreNativeFfiPremultipliedRgba8Image:width`.
 */
void maplibre_native_ffi_premultiplied_rgba8_image_set_width(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
);

/**
 * MaplibreNativeFfiPremultipliedRgba8Image:height: (setter set_height) (getter get_height)
 *
 * The `height` member.
 */
/**
 * maplibre_native_ffi_premultiplied_rgba8_image_get_height: (get-property height)
 *
 * Gets `MaplibreNativeFfiPremultipliedRgba8Image:height`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_height(
  MaplibreNativeFfiPremultipliedRgba8Image* self
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_set_height: (set-property height)
 *
 * Sets `MaplibreNativeFfiPremultipliedRgba8Image:height`.
 */
void maplibre_native_ffi_premultiplied_rgba8_image_set_height(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
);

/**
 * MaplibreNativeFfiPremultipliedRgba8Image:stride: (setter set_stride) (getter get_stride)
 *
 * The number of bytes from the start of one row to the start of the next, at
 * least `width` \* 4.
 */
/**
 * maplibre_native_ffi_premultiplied_rgba8_image_get_stride: (get-property stride)
 *
 * Gets `MaplibreNativeFfiPremultipliedRgba8Image:stride`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_stride(
  MaplibreNativeFfiPremultipliedRgba8Image* self
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_set_stride: (set-property stride)
 *
 * Sets `MaplibreNativeFfiPremultipliedRgba8Image:stride`.
 */
void maplibre_native_ffi_premultiplied_rgba8_image_set_stride(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
);

/**
 * MaplibreNativeFfiPremultipliedRgba8Image:pixels: (setter set_pixels) (getter get_pixels)
 *
 * The rows of pixels, `stride` bytes apart, from the top row down.
 */
/**
 * maplibre_native_ffi_premultiplied_rgba8_image_get_pixels: (get-property pixels)
 *
 * Gets `MaplibreNativeFfiPremultipliedRgba8Image:pixels`.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_premultiplied_rgba8_image_get_pixels(
  MaplibreNativeFfiPremultipliedRgba8Image* self
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_set_pixels: (set-property pixels)
 * @value: (nullable): the bytes
 *
 * Sets `MaplibreNativeFfiPremultipliedRgba8Image:pixels`.
 */
void maplibre_native_ffi_premultiplied_rgba8_image_set_pixels(
  MaplibreNativeFfiPremultipliedRgba8Image* self, GBytes* value
);

GType maplibre_native_ffi_projected_meters_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_PROJECTED_METERS \
  (maplibre_native_ffi_projected_meters_get_type())

/**
 * maplibre_native_ffi_projected_meters_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiProjectedMeters* maplibre_native_ffi_projected_meters_copy(
  const MaplibreNativeFfiProjectedMeters* self
);

/**
 * maplibre_native_ffi_projected_meters_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_projected_meters_free(
  MaplibreNativeFfiProjectedMeters* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiProjectedMeters, maplibre_native_ffi_projected_meters_free
)

/**
 * MaplibreNativeFfiProjectionModeOptions:
 *
 * Settings that skew the map or draw it without perspective.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_projection_mode_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
/**
 * maplibre_native_ffi_projection_mode_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiProjectionModeOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiProjectionModeOptions*
maplibre_native_ffi_projection_mode_options_new(void);

/**
 * MaplibreNativeFfiProjectionModeOptions:axonometric: (setter set_axonometric) (getter get_axonometric)
 *
 * Whether the map draws without perspective.
 */
/**
 * maplibre_native_ffi_projection_mode_options_get_axonometric: (get-property axonometric)
 *
 * Gets `MaplibreNativeFfiProjectionModeOptions:axonometric`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_projection_mode_options_get_axonometric(
  MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_has_axonometric:
 *
 * Whether `MaplibreNativeFfiProjectionModeOptions:axonometric` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_projection_mode_options_has_axonometric(
  MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_set_axonometric: (set-property axonometric)
 *
 * Sets `MaplibreNativeFfiProjectionModeOptions:axonometric`.
 */
void maplibre_native_ffi_projection_mode_options_set_axonometric(
  MaplibreNativeFfiProjectionModeOptions* self, gboolean value
);

/**
 * MaplibreNativeFfiProjectionModeOptions:x-skew: (setter set_x_skew) (getter get_x_skew)
 *
 * The `x_skew` member.
 */
/**
 * maplibre_native_ffi_projection_mode_options_get_x_skew: (get-property x-skew)
 *
 * Gets `MaplibreNativeFfiProjectionModeOptions:x-skew`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_projection_mode_options_get_x_skew(
  MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_has_x_skew:
 *
 * Whether `MaplibreNativeFfiProjectionModeOptions:x-skew` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_projection_mode_options_has_x_skew(
  MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_set_x_skew: (set-property x-skew)
 *
 * Sets `MaplibreNativeFfiProjectionModeOptions:x-skew`.
 */
void maplibre_native_ffi_projection_mode_options_set_x_skew(
  MaplibreNativeFfiProjectionModeOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiProjectionModeOptions:y-skew: (setter set_y_skew) (getter get_y_skew)
 *
 * The `y_skew` member.
 */
/**
 * maplibre_native_ffi_projection_mode_options_get_y_skew: (get-property y-skew)
 *
 * Gets `MaplibreNativeFfiProjectionModeOptions:y-skew`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_projection_mode_options_get_y_skew(
  MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_has_y_skew:
 *
 * Whether `MaplibreNativeFfiProjectionModeOptions:y-skew` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_projection_mode_options_has_y_skew(
  MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_set_y_skew: (set-property y-skew)
 *
 * Sets `MaplibreNativeFfiProjectionModeOptions:y-skew`.
 */
void maplibre_native_ffi_projection_mode_options_set_y_skew(
  MaplibreNativeFfiProjectionModeOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiQueueLock:
 *
 * Your lock on a Vulkan queue that you and a render session both submit to.
 *
 * The session holds the lock only while it submits or presents to the queue.
 * Set both `lock` and `unlock`, or neither when you don't submit to the queue
 * yourself.
 *
 * It can run on a library thread, and it can't call the library.
 *
 * See `mln_queue_lock` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
GType maplibre_native_ffi_queue_lock_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_QUEUE_LOCK \
  (maplibre_native_ffi_queue_lock_get_type())

/**
 * maplibre_native_ffi_queue_lock_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiQueueLock` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiQueueLock* maplibre_native_ffi_queue_lock_new(void);

/**
 * maplibre_native_ffi_queue_lock_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiQueueLock* maplibre_native_ffi_queue_lock_copy(
  const MaplibreNativeFfiQueueLock* self
);

/**
 * maplibre_native_ffi_queue_lock_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_queue_lock_free(MaplibreNativeFfiQueueLock* self);

/**
 * maplibre_native_ffi_queue_lock_set_lock: (skip)
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Locks or unlocks the Vulkan queue that you share with a render session.
 *
 * It can run on a library thread, and it can't call the library.
 *
 * See `mln_queue_lock_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
void maplibre_native_ffi_queue_lock_set_lock(
  MaplibreNativeFfiQueueLock* self, MaplibreNativeFfiQueueLockCallback callback,
  gpointer user_data, GDestroyNotify notify
);

/**
 * maplibre_native_ffi_queue_lock_set_unlock: (skip)
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Locks or unlocks the Vulkan queue that you share with a render session.
 *
 * It can run on a library thread, and it can't call the library.
 *
 * See `mln_queue_lock_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
void maplibre_native_ffi_queue_lock_set_unlock(
  MaplibreNativeFfiQueueLock* self, MaplibreNativeFfiQueueLockCallback callback,
  gpointer user_data, GDestroyNotify notify
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiQueueLock, maplibre_native_ffi_queue_lock_free
)

GType maplibre_native_ffi_render_abandon_result_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_ABANDON_RESULT \
  (maplibre_native_ffi_render_abandon_result_get_type())

/**
 * maplibre_native_ffi_render_abandon_result_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRenderAbandonResult*
maplibre_native_ffi_render_abandon_result_copy(
  const MaplibreNativeFfiRenderAbandonResult* self
);

/**
 * maplibre_native_ffi_render_abandon_result_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_render_abandon_result_free(
  MaplibreNativeFfiRenderAbandonResult* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRenderAbandonResult,
  maplibre_native_ffi_render_abandon_result_free
)

GType maplibre_native_ffi_render_frame_result_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_FRAME_RESULT \
  (maplibre_native_ffi_render_frame_result_get_type())

/**
 * maplibre_native_ffi_render_frame_result_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRenderFrameResult*
maplibre_native_ffi_render_frame_result_copy(
  const MaplibreNativeFfiRenderFrameResult* self
);

/**
 * maplibre_native_ffi_render_frame_result_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_render_frame_result_free(
  MaplibreNativeFfiRenderFrameResult* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRenderFrameResult,
  maplibre_native_ffi_render_frame_result_free
)

/**
 * MaplibreNativeFfiWake:
 *
 * A callback that tells the caller when work is ready.
 *
 * Runs on the thread-default main context of the thread that started the call.
 *
 * See `mln_wake` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
 */
GType maplibre_native_ffi_wake_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_WAKE (maplibre_native_ffi_wake_get_type())

/**
 * maplibre_native_ffi_wake_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiWake` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiWake* maplibre_native_ffi_wake_new(void);

/**
 * maplibre_native_ffi_wake_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiWake* maplibre_native_ffi_wake_copy(
  const MaplibreNativeFfiWake* self
);

/**
 * maplibre_native_ffi_wake_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_wake_free(MaplibreNativeFfiWake* self);

/**
 * maplibre_native_ffi_wake_set_callback:
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Null to turn the wake off.
 */
void maplibre_native_ffi_wake_set_callback(
  MaplibreNativeFfiWake* self, MaplibreNativeFfiWakeCallback callback,
  gpointer user_data, GDestroyNotify notify
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiWake, maplibre_native_ffi_wake_free
)

/**
 * MaplibreNativeFfiRenderSessionAttachOptions:
 *
 * Options for attaching a render session to a map.
 *
 * See `mln_render_session_attach_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
/**
 * maplibre_native_ffi_render_session_attach_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRenderSessionAttachOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRenderSessionAttachOptions*
maplibre_native_ffi_render_session_attach_options_new(void);

/**
 * MaplibreNativeFfiRenderSessionAttachOptions:driver: (setter set_driver) (getter get_driver)
 *
 * The thread that runs the session's graphics work, which depends on the
 * target:
 *
 * - `MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CALLER_GRAPHICS_THREAD` for a shared or
 *   dedicated EGL or WGL context, an existing WebGL context, or a browser's
 *   WebGPU device.
 * - `MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CORE_WORKER` for a transferred WebGL
 *   canvas, an Android, OpenHarmony, or Win32 window, a Wayland surface, a ring
 *   of Android hardware buffers, dmabufs, DXGI shared textures, or IOSurfaces,
 *   or an offscreen session.
 * - Either one for another Metal or Vulkan target.
 *
 * An attach with another driver fails with
 * `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED`.
 *
 * Until it is set, it holds 0, which names no value and which native code
 * rejects, whatever the property's default says.
 */
/**
 * maplibre_native_ffi_render_session_attach_options_get_driver: (get-property driver)
 *
 * Gets `MaplibreNativeFfiRenderSessionAttachOptions:driver`.
 *
 * Returns: the value
 */
MaplibreNativeFfiRenderDriverKind
maplibre_native_ffi_render_session_attach_options_get_driver(
  MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_driver: (set-property driver)
 *
 * Sets `MaplibreNativeFfiRenderSessionAttachOptions:driver`.
 */
void maplibre_native_ffi_render_session_attach_options_set_driver(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  MaplibreNativeFfiRenderDriverKind value
);

/**
 * MaplibreNativeFfiRenderSessionAttachOptions:producer-sync-kinds: (setter set_producer_sync_kinds) (getter get_producer_sync_kinds)
 *
 * The kinds of `MaplibreNativeFfiGpuSync` that you can wait on before you read
 * an acquired frame. The session uses one of them that its target supports.
 * Zero lets the session pick a kind that names no object to wait on.
 */
/**
 * maplibre_native_ffi_render_session_attach_options_get_producer_sync_kinds: (get-property producer-sync-kinds)
 *
 * Gets `MaplibreNativeFfiRenderSessionAttachOptions:producer-sync-kinds`.
 *
 * Returns: the value
 */
MaplibreNativeFfiGpuSyncKindFlag
maplibre_native_ffi_render_session_attach_options_get_producer_sync_kinds(
  MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_producer_sync_kinds: (set-property producer-sync-kinds)
 *
 * Sets `MaplibreNativeFfiRenderSessionAttachOptions:producer-sync-kinds`.
 */
void maplibre_native_ffi_render_session_attach_options_set_producer_sync_kinds(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  MaplibreNativeFfiGpuSyncKindFlag value
);

/**
 * MaplibreNativeFfiRenderSessionAttachOptions:frame-wake: (setter set_frame_wake) (getter get_frame_wake)
 *
 * Called when frame results are waiting for
 * `maplibre_native_ffi_render_session_drain_frame_results()`. It isn't called
 * again until a drain takes them.
 */
/**
 * maplibre_native_ffi_render_session_attach_options_get_frame_wake: (get-property frame-wake)
 *
 * Gets `MaplibreNativeFfiRenderSessionAttachOptions:frame-wake`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_frame_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_frame_wake: (set-property frame-wake)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiRenderSessionAttachOptions:frame-wake`.
 */
void maplibre_native_ffi_render_session_attach_options_set_frame_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
);

/**
 * MaplibreNativeFfiRenderSessionAttachOptions:redraw-wake: (setter set_redraw_wake) (getter get_redraw_wake)
 *
 * Called when the session has something new to draw. Demand a frame at your
 * next display refresh. It isn't called again until you demand a frame, or
 * while the session has no target.
 */
/**
 * maplibre_native_ffi_render_session_attach_options_get_redraw_wake: (get-property redraw-wake)
 *
 * Gets `MaplibreNativeFfiRenderSessionAttachOptions:redraw-wake`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_redraw_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_redraw_wake: (set-property redraw-wake)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiRenderSessionAttachOptions:redraw-wake`.
 */
void maplibre_native_ffi_render_session_attach_options_set_redraw_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
);

/**
 * MaplibreNativeFfiRenderSessionAttachOptions:driver-work-wake: (setter set_driver_work_wake) (getter get_driver_work_wake)
 *
 * Called when a session driven by
 * `MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CALLER_GRAPHICS_THREAD` has work for
 * `maplibre_native_ffi_render_session_service_driver_work()`. It isn't called
 * again until a service runs.
 */
/**
 * maplibre_native_ffi_render_session_attach_options_get_driver_work_wake: (get-property driver-work-wake)
 *
 * Gets `MaplibreNativeFfiRenderSessionAttachOptions:driver-work-wake`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_driver_work_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_driver_work_wake: (set-property driver-work-wake)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiRenderSessionAttachOptions:driver-work-wake`.
 */
void maplibre_native_ffi_render_session_attach_options_set_driver_work_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
);

/**
 * MaplibreNativeFfiRenderSessionAttachOptions:queue-lock: (setter set_queue_lock) (getter get_queue_lock)
 *
 * Your lock on the Vulkan queue, when you submit to it yourself.
 */
/**
 * maplibre_native_ffi_render_session_attach_options_get_queue_lock: (get-property queue-lock)
 *
 * Gets `MaplibreNativeFfiRenderSessionAttachOptions:queue-lock`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiQueueLock*
maplibre_native_ffi_render_session_attach_options_get_queue_lock(
  MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_queue_lock: (set-property queue-lock)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiRenderSessionAttachOptions:queue-lock`.
 */
void maplibre_native_ffi_render_session_attach_options_set_queue_lock(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiQueueLock* value
);

GType maplibre_native_ffi_render_session_capabilities_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_SESSION_CAPABILITIES \
  (maplibre_native_ffi_render_session_capabilities_get_type())

/**
 * maplibre_native_ffi_render_session_capabilities_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRenderSessionCapabilities*
maplibre_native_ffi_render_session_capabilities_copy(
  const MaplibreNativeFfiRenderSessionCapabilities* self
);

/**
 * maplibre_native_ffi_render_session_capabilities_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_render_session_capabilities_free(
  MaplibreNativeFfiRenderSessionCapabilities* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRenderSessionCapabilities,
  maplibre_native_ffi_render_session_capabilities_free
)

GType maplibre_native_ffi_render_session_snapshot_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_SESSION_SNAPSHOT \
  (maplibre_native_ffi_render_session_snapshot_get_type())

/**
 * maplibre_native_ffi_render_session_snapshot_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRenderSessionSnapshot*
maplibre_native_ffi_render_session_snapshot_copy(
  const MaplibreNativeFfiRenderSessionSnapshot* self
);

/**
 * maplibre_native_ffi_render_session_snapshot_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_render_session_snapshot_free(
  MaplibreNativeFfiRenderSessionSnapshot* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRenderSessionSnapshot,
  maplibre_native_ffi_render_session_snapshot_free
)

/**
 * MaplibreNativeFfiRenderedFeature:
 *
 * A feature that a rendered feature query found.
 *
 * See `mln_rendered_feature` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
GType maplibre_native_ffi_rendered_feature_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDERED_FEATURE \
  (maplibre_native_ffi_rendered_feature_get_type())

/**
 * maplibre_native_ffi_rendered_feature_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRenderedFeature` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRenderedFeature* maplibre_native_ffi_rendered_feature_new(
  void
);

/**
 * maplibre_native_ffi_rendered_feature_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRenderedFeature* maplibre_native_ffi_rendered_feature_copy(
  const MaplibreNativeFfiRenderedFeature* self
);

/**
 * maplibre_native_ffi_rendered_feature_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_rendered_feature_free(
  MaplibreNativeFfiRenderedFeature* self
);

/**
 * maplibre_native_ffi_rendered_feature_get_feature:
 *
 * The feature, as a GeoJSON feature.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_rendered_feature_get_feature(
  const MaplibreNativeFfiRenderedFeature* self
);

/**
 * maplibre_native_ffi_rendered_feature_get_layer_id:
 *
 * The layer that drew the feature.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_rendered_feature_get_layer_id(
  const MaplibreNativeFfiRenderedFeature* self
);

/**
 * maplibre_native_ffi_rendered_feature_get_source_id:
 *
 * The `source_id` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_rendered_feature_get_source_id(
  const MaplibreNativeFfiRenderedFeature* self
);

/**
 * maplibre_native_ffi_rendered_feature_get_source_layer_id:
 *
 * The feature's source layer, or empty when its source has none.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_rendered_feature_get_source_layer_id(
  const MaplibreNativeFfiRenderedFeature* self
);

/**
 * maplibre_native_ffi_rendered_feature_get_state:
 *
 * The feature's state, as a JSON object.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_rendered_feature_get_state(
  const MaplibreNativeFfiRenderedFeature* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRenderedFeature, maplibre_native_ffi_rendered_feature_free
)

/**
 * MaplibreNativeFfiRenderedFeatureQueryOptions:
 *
 * Options for a rendered feature query.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_rendered_feature_query_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
/**
 * maplibre_native_ffi_rendered_feature_query_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRenderedFeatureQueryOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRenderedFeatureQueryOptions*
maplibre_native_ffi_rendered_feature_query_options_new(void);

/**
 * MaplibreNativeFfiRenderedFeatureQueryOptions:layer-ids: (setter set_layer_ids) (getter get_layer_ids)
 *
 * The layers to query. Without them, the query covers every layer.
 */
/**
 * maplibre_native_ffi_rendered_feature_query_options_get_layer_ids: (get-property layer-ids)
 *
 * Gets `MaplibreNativeFfiRenderedFeatureQueryOptions:layer-ids`.
 *
 * Returns: (transfer none) (array zero-terminated=1): the texts
 */
const char* const*
maplibre_native_ffi_rendered_feature_query_options_get_layer_ids(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_rendered_feature_query_options_has_layer_ids:
 *
 * Whether `MaplibreNativeFfiRenderedFeatureQueryOptions:layer-ids` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_rendered_feature_query_options_has_layer_ids(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_rendered_feature_query_options_set_layer_ids: (set-property layer-ids)
 * @value: (array zero-terminated=1): the texts
 *
 * Sets `MaplibreNativeFfiRenderedFeatureQueryOptions:layer-ids`.
 */
void maplibre_native_ffi_rendered_feature_query_options_set_layer_ids(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self, const char* const* value
);

/**
 * MaplibreNativeFfiRenderedFeatureQueryOptions:filter: (setter set_filter) (getter get_filter)
 *
 * A \[filter\](https://maplibre.org/maplibre-style-spec/expressions/) that each
 * feature must match. Empty matches every feature.
 */
/**
 * maplibre_native_ffi_rendered_feature_query_options_get_filter: (get-property filter)
 *
 * Gets `MaplibreNativeFfiRenderedFeatureQueryOptions:filter`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_rendered_feature_query_options_get_filter(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_rendered_feature_query_options_set_filter: (set-property filter)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiRenderedFeatureQueryOptions:filter`.
 */
void maplibre_native_ffi_rendered_feature_query_options_set_filter(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self, const char* value
);

GType maplibre_native_ffi_screen_box_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_SCREEN_BOX \
  (maplibre_native_ffi_screen_box_get_type())

/**
 * maplibre_native_ffi_screen_box_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiScreenBox* maplibre_native_ffi_screen_box_copy(
  const MaplibreNativeFfiScreenBox* self
);

/**
 * maplibre_native_ffi_screen_box_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_screen_box_free(MaplibreNativeFfiScreenBox* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiScreenBox, maplibre_native_ffi_screen_box_free
)

/**
 * MaplibreNativeFfiScreenLineString:
 *
 * A line through points on the screen.
 *
 * See `mln_screen_line_string` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
/**
 * maplibre_native_ffi_screen_line_string_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiScreenLineString` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiScreenLineString* maplibre_native_ffi_screen_line_string_new(
  void
);

/**
 * maplibre_native_ffi_screen_line_string_get_points:
 *
 * Gets the value that `maplibre_native_ffi_screen_line_string_set_points` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiScreenPoint): the items
 */
GPtrArray* maplibre_native_ffi_screen_line_string_get_points(
  MaplibreNativeFfiScreenLineString* self
);

/**
 * maplibre_native_ffi_screen_line_string_set_points:
 * @value: (array length=value_count): the items
 *
 * The `points` member.
 */
void maplibre_native_ffi_screen_line_string_set_points(
  MaplibreNativeFfiScreenLineString* self,
  const MaplibreNativeFfiScreenPoint* value, gsize value_count
);

/**
 * MaplibreNativeFfiRenderedQueryGeometry:
 *
 * The screen area that a rendered feature query covers.
 *
 * See `mln_rendered_query_geometry` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
GType maplibre_native_ffi_rendered_query_geometry_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDERED_QUERY_GEOMETRY \
  (maplibre_native_ffi_rendered_query_geometry_get_type())

/**
 * maplibre_native_ffi_rendered_query_geometry_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRenderedQueryGeometry*
maplibre_native_ffi_rendered_query_geometry_copy(
  const MaplibreNativeFfiRenderedQueryGeometry* self
);

/**
 * maplibre_native_ffi_rendered_query_geometry_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_rendered_query_geometry_free(
  MaplibreNativeFfiRenderedQueryGeometry* self
);

/**
 * maplibre_native_ffi_rendered_query_geometry_new_point: (constructor)
 * @value: the value
 *
 * The `point` variant.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRenderedQueryGeometry*
maplibre_native_ffi_rendered_query_geometry_new_point(
  const MaplibreNativeFfiScreenPoint* value
);

/**
 * maplibre_native_ffi_rendered_query_geometry_is_point:
 *
 * Whether the value holds `maplibre_native_ffi_rendered_query_geometry_get_point`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_rendered_query_geometry_is_point(
  const MaplibreNativeFfiRenderedQueryGeometry* self
);

/**
 * maplibre_native_ffi_rendered_query_geometry_get_point:
 *
 * The `point` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiScreenPoint*
maplibre_native_ffi_rendered_query_geometry_get_point(
  const MaplibreNativeFfiRenderedQueryGeometry* self
);

/**
 * maplibre_native_ffi_rendered_query_geometry_new_box: (constructor)
 * @value: the value
 *
 * The `box` variant.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRenderedQueryGeometry*
maplibre_native_ffi_rendered_query_geometry_new_box(
  const MaplibreNativeFfiScreenBox* value
);

/**
 * maplibre_native_ffi_rendered_query_geometry_is_box:
 *
 * Whether the value holds `maplibre_native_ffi_rendered_query_geometry_get_box`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_rendered_query_geometry_is_box(
  const MaplibreNativeFfiRenderedQueryGeometry* self
);

/**
 * maplibre_native_ffi_rendered_query_geometry_get_box:
 *
 * The `box` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiScreenBox* maplibre_native_ffi_rendered_query_geometry_get_box(
  const MaplibreNativeFfiRenderedQueryGeometry* self
);

/**
 * maplibre_native_ffi_rendered_query_geometry_new_line_string: (constructor)
 * @value: the value
 *
 * The `line_string` variant.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRenderedQueryGeometry*
maplibre_native_ffi_rendered_query_geometry_new_line_string(
  MaplibreNativeFfiScreenLineString* value
);

/**
 * maplibre_native_ffi_rendered_query_geometry_is_line_string:
 *
 * Whether the value holds `maplibre_native_ffi_rendered_query_geometry_get_line_string`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_rendered_query_geometry_is_line_string(
  const MaplibreNativeFfiRenderedQueryGeometry* self
);

/**
 * maplibre_native_ffi_rendered_query_geometry_get_line_string:
 *
 * The `line_string` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiScreenLineString*
maplibre_native_ffi_rendered_query_geometry_get_line_string(
  const MaplibreNativeFfiRenderedQueryGeometry* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRenderedQueryGeometry,
  maplibre_native_ffi_rendered_query_geometry_free
)

GType maplibre_native_ffi_rendering_stats_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDERING_STATS \
  (maplibre_native_ffi_rendering_stats_get_type())

/**
 * maplibre_native_ffi_rendering_stats_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRenderingStats* maplibre_native_ffi_rendering_stats_copy(
  const MaplibreNativeFfiRenderingStats* self
);

/**
 * maplibre_native_ffi_rendering_stats_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_rendering_stats_free(
  MaplibreNativeFfiRenderingStats* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRenderingStats, maplibre_native_ffi_rendering_stats_free
)

/**
 * MaplibreNativeFfiResourceRoute:
 *
 * The requests that a resource provider serves.
 *
 * See `mln_resource_route` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
/**
 * maplibre_native_ffi_resource_route_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiResourceRoute` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiResourceRoute* maplibre_native_ffi_resource_route_new(void);

/**
 * MaplibreNativeFfiResourceRoute:kinds: (setter set_kinds) (getter get_kinds)
 *
 * The kinds of resource that the route covers. Zero covers every kind.
 */
/**
 * maplibre_native_ffi_resource_route_get_kinds: (get-property kinds)
 *
 * Gets `MaplibreNativeFfiResourceRoute:kinds`.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceKindFlag maplibre_native_ffi_resource_route_get_kinds(
  MaplibreNativeFfiResourceRoute* self
);

/**
 * maplibre_native_ffi_resource_route_set_kinds: (set-property kinds)
 *
 * Sets `MaplibreNativeFfiResourceRoute:kinds`.
 */
void maplibre_native_ffi_resource_route_set_kinds(
  MaplibreNativeFfiResourceRoute* self, MaplibreNativeFfiResourceKindFlag value
);

/**
 * MaplibreNativeFfiResourceRoute:flags: (setter set_flags) (getter get_flags)
 *
 * The `flags` member.
 */
/**
 * maplibre_native_ffi_resource_route_get_flags: (get-property flags)
 *
 * Gets `MaplibreNativeFfiResourceRoute:flags`.
 *
 * Returns: the value
 */
MaplibreNativeFfiUrlMatchFlag maplibre_native_ffi_resource_route_get_flags(
  MaplibreNativeFfiResourceRoute* self
);

/**
 * maplibre_native_ffi_resource_route_set_flags: (set-property flags)
 *
 * Sets `MaplibreNativeFfiResourceRoute:flags`.
 */
void maplibre_native_ffi_resource_route_set_flags(
  MaplibreNativeFfiResourceRoute* self, MaplibreNativeFfiUrlMatchFlag value
);

/**
 * MaplibreNativeFfiResourceRoute:url: (setter set_url) (getter get_url)
 *
 * The `url` member.
 */
/**
 * maplibre_native_ffi_resource_route_get_url: (get-property url)
 *
 * Gets `MaplibreNativeFfiResourceRoute:url`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_route_get_url(
  MaplibreNativeFfiResourceRoute* self
);

/**
 * maplibre_native_ffi_resource_route_set_url: (set-property url)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiResourceRoute:url`.
 */
void maplibre_native_ffi_resource_route_set_url(
  MaplibreNativeFfiResourceRoute* self, const char* value
);

/**
 * MaplibreNativeFfiResourceProvider:
 *
 * A resource provider, for
 * `maplibre_native_ffi_runtime_set_resource_provider_async()`.
 *
 * See `mln_resource_provider` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_resource_provider_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_PROVIDER \
  (maplibre_native_ffi_resource_provider_get_type())

/**
 * maplibre_native_ffi_resource_provider_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiResourceProvider` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiResourceProvider* maplibre_native_ffi_resource_provider_new(
  void
);

/**
 * maplibre_native_ffi_resource_provider_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiResourceProvider* maplibre_native_ffi_resource_provider_copy(
  const MaplibreNativeFfiResourceProvider* self
);

/**
 * maplibre_native_ffi_resource_provider_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_resource_provider_free(
  MaplibreNativeFfiResourceProvider* self
);

/**
 * maplibre_native_ffi_resource_provider_get_routes:
 *
 * Gets the value that `maplibre_native_ffi_resource_provider_set_routes` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiResourceRoute): the items
 */
GPtrArray* maplibre_native_ffi_resource_provider_get_routes(
  const MaplibreNativeFfiResourceProvider* self
);

/**
 * maplibre_native_ffi_resource_provider_set_routes:
 * @value: (array length=value_count): the items
 *
 * The requests that the provider serves. Without routes, it serves every
 * network request.
 */
void maplibre_native_ffi_resource_provider_set_routes(
  MaplibreNativeFfiResourceProvider* self,
  MaplibreNativeFfiResourceRoute* const* value, gsize value_count
);

/**
 * maplibre_native_ffi_resource_provider_set_callback:
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Serves a network request in place of the library.
 *
 * Answer with `maplibre_native_ffi_resource_request_complete()`, or hand the
 * request back to the library with
 * `maplibre_native_ffi_resource_request_pass_through()`. Either can come now or
 * later, from any thread. A request released without an answer fails.
 *
 * See `mln_resource_provider_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_resource_provider_set_callback(
  MaplibreNativeFfiResourceProvider* self,
  MaplibreNativeFfiResourceProviderCallback callback, gpointer user_data,
  GDestroyNotify notify
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiResourceProvider, maplibre_native_ffi_resource_provider_free
)

GType maplibre_native_ffi_resource_range_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_RANGE \
  (maplibre_native_ffi_resource_range_get_type())

/**
 * maplibre_native_ffi_resource_range_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiResourceRange* maplibre_native_ffi_resource_range_copy(
  const MaplibreNativeFfiResourceRange* self
);

/**
 * maplibre_native_ffi_resource_range_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_resource_range_free(
  MaplibreNativeFfiResourceRange* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiResourceRange, maplibre_native_ffi_resource_range_free
)

/**
 * MaplibreNativeFfiResourceRequestCancelHandler:
 *
 * A cancel callback, for
 * `maplibre_native_ffi_resource_request_set_cancel_callback()`.
 *
 * See `mln_resource_request_cancel_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_resource_request_cancel_handler_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_REQUEST_CANCEL_HANDLER \
  (maplibre_native_ffi_resource_request_cancel_handler_get_type())

/**
 * maplibre_native_ffi_resource_request_cancel_handler_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiResourceRequestCancelHandler` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiResourceRequestCancelHandler*
maplibre_native_ffi_resource_request_cancel_handler_new(void);

/**
 * maplibre_native_ffi_resource_request_cancel_handler_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiResourceRequestCancelHandler*
maplibre_native_ffi_resource_request_cancel_handler_copy(
  const MaplibreNativeFfiResourceRequestCancelHandler* self
);

/**
 * maplibre_native_ffi_resource_request_cancel_handler_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_resource_request_cancel_handler_free(
  MaplibreNativeFfiResourceRequestCancelHandler* self
);

/**
 * maplibre_native_ffi_resource_request_cancel_handler_set_callback:
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Reports that the library no longer needs a resource request.
 *
 * The request no longer accepts an answer, so stop working on it and release
 * it.
 *
 * See `mln_resource_request_cancel_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_resource_request_cancel_handler_set_callback(
  MaplibreNativeFfiResourceRequestCancelHandler* self,
  MaplibreNativeFfiResourceRequestCancelCallback callback, gpointer user_data,
  GDestroyNotify notify
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiResourceRequestCancelHandler,
  maplibre_native_ffi_resource_request_cancel_handler_free
)

/**
 * MaplibreNativeFfiResourceRequestInfo:
 *
 * A request that a resource provider serves.
 *
 * When the cache holds an earlier copy of the resource, the `prior_` fields
 * describe it.
 *
 * See `mln_resource_request_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_resource_request_info_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_REQUEST_INFO \
  (maplibre_native_ffi_resource_request_info_get_type())

/**
 * maplibre_native_ffi_resource_request_info_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiResourceRequestInfo` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiResourceRequestInfo*
maplibre_native_ffi_resource_request_info_new(void);

/**
 * maplibre_native_ffi_resource_request_info_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiResourceRequestInfo*
maplibre_native_ffi_resource_request_info_copy(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_resource_request_info_free(
  MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_requested_url:
 *
 * The URL as the style names it.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_request_info_get_requested_url(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_resolved_url:
 *
 * The URL to load, after the library expands any shorthand in `requested_url`
 * and before any resource transform. Routes match this URL.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_request_info_get_resolved_url(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_kind:
 *
 * The `kind` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceKind
maplibre_native_ffi_resource_request_info_get_kind(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_loading_method:
 *
 * The `loading_method` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceLoadingMethod
maplibre_native_ffi_resource_request_info_get_loading_method(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_priority:
 *
 * The `priority` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourcePriority
maplibre_native_ffi_resource_request_info_get_priority(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_usage:
 *
 * The `usage` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceUsage
maplibre_native_ffi_resource_request_info_get_usage(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_storage_policy:
 *
 * The `storage_policy` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceStoragePolicy
maplibre_native_ffi_resource_request_info_get_storage_policy(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_range:
 *
 * The only bytes of the resource that the request wants.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiResourceRange*
maplibre_native_ffi_resource_request_info_get_range(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_has_range:
 *
 * Whether the value has `maplibre_native_ffi_resource_request_info_get_range`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_resource_request_info_has_range(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_prior_modified_unix_ms:
 *
 * The `prior_modified_unix_ms` member.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_resource_request_info_get_prior_modified_unix_ms(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_has_prior_modified_unix_ms:
 *
 * Whether the value has `maplibre_native_ffi_resource_request_info_get_prior_modified_unix_ms`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_resource_request_info_has_prior_modified_unix_ms(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_prior_expires_unix_ms:
 *
 * The `prior_expires_unix_ms` member.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_resource_request_info_get_prior_expires_unix_ms(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_has_prior_expires_unix_ms:
 *
 * Whether the value has `maplibre_native_ffi_resource_request_info_get_prior_expires_unix_ms`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_resource_request_info_has_prior_expires_unix_ms(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_prior_etag:
 *
 * Empty when the earlier copy has no ETag.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_request_info_get_prior_etag(
  const MaplibreNativeFfiResourceRequestInfo* self
);

/**
 * maplibre_native_ffi_resource_request_info_get_prior_data:
 *
 * The `prior_data` member.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_resource_request_info_get_prior_data(
  const MaplibreNativeFfiResourceRequestInfo* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiResourceRequestInfo,
  maplibre_native_ffi_resource_request_info_free
)

/**
 * MaplibreNativeFfiResourceResponse:
 *
 * The answer to a resource request.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_resource_response` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
/**
 * maplibre_native_ffi_resource_response_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiResourceResponse` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiResourceResponse* maplibre_native_ffi_resource_response_new(
  void
);

/**
 * MaplibreNativeFfiResourceResponse:status: (setter set_status) (getter get_status)
 *
 * The `status` member.
 */
/**
 * maplibre_native_ffi_resource_response_get_status: (get-property status)
 *
 * Gets `MaplibreNativeFfiResourceResponse:status`.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceResponseStatus
maplibre_native_ffi_resource_response_get_status(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_status: (set-property status)
 *
 * Sets `MaplibreNativeFfiResourceResponse:status`.
 */
void maplibre_native_ffi_resource_response_set_status(
  MaplibreNativeFfiResourceResponse* self,
  MaplibreNativeFfiResourceResponseStatus value
);

/**
 * MaplibreNativeFfiResourceResponse:error-reason: (setter set_error_reason) (getter get_error_reason)
 *
 * Why the request failed, for
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_ERROR`.
 */
/**
 * maplibre_native_ffi_resource_response_get_error_reason: (get-property error-reason)
 *
 * Gets `MaplibreNativeFfiResourceResponse:error-reason`.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceErrorReason
maplibre_native_ffi_resource_response_get_error_reason(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_error_reason: (set-property error-reason)
 *
 * Sets `MaplibreNativeFfiResourceResponse:error-reason`.
 */
void maplibre_native_ffi_resource_response_set_error_reason(
  MaplibreNativeFfiResourceResponse* self,
  MaplibreNativeFfiResourceErrorReason value
);

/**
 * MaplibreNativeFfiResourceResponse:bytes: (setter set_bytes) (getter get_bytes)
 *
 * The resource, for `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_OK`.
 */
/**
 * maplibre_native_ffi_resource_response_get_bytes: (get-property bytes)
 *
 * Gets `MaplibreNativeFfiResourceResponse:bytes`.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_resource_response_get_bytes(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_bytes: (set-property bytes)
 * @value: (nullable): the bytes
 *
 * Sets `MaplibreNativeFfiResourceResponse:bytes`.
 */
void maplibre_native_ffi_resource_response_set_bytes(
  MaplibreNativeFfiResourceResponse* self, GBytes* value
);

/**
 * MaplibreNativeFfiResourceResponse:error-message: (setter set_error_message) (getter get_error_message)
 *
 * A description of the error, for
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_ERROR`.
 */
/**
 * maplibre_native_ffi_resource_response_get_error_message: (get-property error-message)
 *
 * Gets `MaplibreNativeFfiResourceResponse:error-message`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_response_get_error_message(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_error_message: (set-property error-message)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiResourceResponse:error-message`.
 */
void maplibre_native_ffi_resource_response_set_error_message(
  MaplibreNativeFfiResourceResponse* self, const char* value
);

/**
 * MaplibreNativeFfiResourceResponse:must-revalidate: (setter set_must_revalidate) (getter get_must_revalidate)
 *
 * Whether the library must request the resource again before it reuses this
 * copy.
 */
/**
 * maplibre_native_ffi_resource_response_get_must_revalidate: (get-property must-revalidate)
 *
 * Gets `MaplibreNativeFfiResourceResponse:must-revalidate`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_resource_response_get_must_revalidate(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_must_revalidate: (set-property must-revalidate)
 *
 * Sets `MaplibreNativeFfiResourceResponse:must-revalidate`.
 */
void maplibre_native_ffi_resource_response_set_must_revalidate(
  MaplibreNativeFfiResourceResponse* self, gboolean value
);

/**
 * MaplibreNativeFfiResourceResponse:modified-unix-ms: (setter set_modified_unix_ms) (getter get_modified_unix_ms)
 *
 * The `modified_unix_ms` member.
 */
/**
 * maplibre_native_ffi_resource_response_get_modified_unix_ms: (get-property modified-unix-ms)
 *
 * Gets `MaplibreNativeFfiResourceResponse:modified-unix-ms`.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_resource_response_get_modified_unix_ms(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_has_modified_unix_ms:
 *
 * Whether `MaplibreNativeFfiResourceResponse:modified-unix-ms` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_resource_response_has_modified_unix_ms(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_modified_unix_ms: (set-property modified-unix-ms)
 *
 * Sets `MaplibreNativeFfiResourceResponse:modified-unix-ms`.
 */
void maplibre_native_ffi_resource_response_set_modified_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
);

/**
 * MaplibreNativeFfiResourceResponse:expires-unix-ms: (setter set_expires_unix_ms) (getter get_expires_unix_ms)
 *
 * The `expires_unix_ms` member.
 */
/**
 * maplibre_native_ffi_resource_response_get_expires_unix_ms: (get-property expires-unix-ms)
 *
 * Gets `MaplibreNativeFfiResourceResponse:expires-unix-ms`.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_resource_response_get_expires_unix_ms(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_has_expires_unix_ms:
 *
 * Whether `MaplibreNativeFfiResourceResponse:expires-unix-ms` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_resource_response_has_expires_unix_ms(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_expires_unix_ms: (set-property expires-unix-ms)
 *
 * Sets `MaplibreNativeFfiResourceResponse:expires-unix-ms`.
 */
void maplibre_native_ffi_resource_response_set_expires_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
);

/**
 * MaplibreNativeFfiResourceResponse:etag: (setter set_etag) (getter get_etag)
 *
 * Empty when the resource has no ETag.
 */
/**
 * maplibre_native_ffi_resource_response_get_etag: (get-property etag)
 *
 * Gets `MaplibreNativeFfiResourceResponse:etag`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_response_get_etag(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_etag: (set-property etag)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiResourceResponse:etag`.
 */
void maplibre_native_ffi_resource_response_set_etag(
  MaplibreNativeFfiResourceResponse* self, const char* value
);

/**
 * MaplibreNativeFfiResourceResponse:retry-after-unix-ms: (setter set_retry_after_unix_ms) (getter get_retry_after_unix_ms)
 *
 * When to retry, for `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_ERROR`.
 */
/**
 * maplibre_native_ffi_resource_response_get_retry_after_unix_ms: (get-property retry-after-unix-ms)
 *
 * Gets `MaplibreNativeFfiResourceResponse:retry-after-unix-ms`.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_resource_response_get_retry_after_unix_ms(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_has_retry_after_unix_ms:
 *
 * Whether `MaplibreNativeFfiResourceResponse:retry-after-unix-ms` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_resource_response_has_retry_after_unix_ms(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_retry_after_unix_ms: (set-property retry-after-unix-ms)
 *
 * Sets `MaplibreNativeFfiResourceResponse:retry-after-unix-ms`.
 */
void maplibre_native_ffi_resource_response_set_retry_after_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
);

/**
 * MaplibreNativeFfiUrlQueryParameter:
 *
 * A query parameter that a resource transform rule adds to a URL.
 *
 * Pass `name` and `value` unencoded. The library percent-encodes every byte of
 * them except the letters, digits, `-`, `.`, `_`, and `~`, and adds them as
 * `name=value`.
 *
 * See `mln_url_query_parameter` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
/**
 * maplibre_native_ffi_url_query_parameter_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiUrlQueryParameter` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiUrlQueryParameter* maplibre_native_ffi_url_query_parameter_new(
  void
);

/**
 * MaplibreNativeFfiUrlQueryParameter:name: (setter set_name) (getter get_name)
 *
 * The `name` member.
 */
/**
 * maplibre_native_ffi_url_query_parameter_get_name: (get-property name)
 *
 * Gets `MaplibreNativeFfiUrlQueryParameter:name`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_url_query_parameter_get_name(
  MaplibreNativeFfiUrlQueryParameter* self
);

/**
 * maplibre_native_ffi_url_query_parameter_set_name: (set-property name)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiUrlQueryParameter:name`.
 */
void maplibre_native_ffi_url_query_parameter_set_name(
  MaplibreNativeFfiUrlQueryParameter* self, const char* value
);

/**
 * MaplibreNativeFfiUrlQueryParameter:value: (setter set_value) (getter get_value)
 *
 * The `value` member.
 */
/**
 * maplibre_native_ffi_url_query_parameter_get_value: (get-property value)
 *
 * Gets `MaplibreNativeFfiUrlQueryParameter:value`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_url_query_parameter_get_value(
  MaplibreNativeFfiUrlQueryParameter* self
);

/**
 * maplibre_native_ffi_url_query_parameter_set_value: (set-property value)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiUrlQueryParameter:value`.
 */
void maplibre_native_ffi_url_query_parameter_set_value(
  MaplibreNativeFfiUrlQueryParameter* self, const char* value
);

/**
 * MaplibreNativeFfiResourceTransformRule:
 *
 * Query parameters to add to the URLs of requests that match a pattern.
 *
 * The library adds the parameters, in order, at the end of the URL's query and
 * before any fragment. It keeps the parameters that the URL already has, even
 * one with the same name.
 *
 * See `mln_resource_transform_rule` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
/**
 * maplibre_native_ffi_resource_transform_rule_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiResourceTransformRule` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiResourceTransformRule*
maplibre_native_ffi_resource_transform_rule_new(void);

/**
 * MaplibreNativeFfiResourceTransformRule:kinds: (setter set_kinds) (getter get_kinds)
 *
 * The kinds of resource that the rule covers. Zero covers every kind.
 */
/**
 * maplibre_native_ffi_resource_transform_rule_get_kinds: (get-property kinds)
 *
 * Gets `MaplibreNativeFfiResourceTransformRule:kinds`.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceKindFlag
maplibre_native_ffi_resource_transform_rule_get_kinds(
  MaplibreNativeFfiResourceTransformRule* self
);

/**
 * maplibre_native_ffi_resource_transform_rule_set_kinds: (set-property kinds)
 *
 * Sets `MaplibreNativeFfiResourceTransformRule:kinds`.
 */
void maplibre_native_ffi_resource_transform_rule_set_kinds(
  MaplibreNativeFfiResourceTransformRule* self,
  MaplibreNativeFfiResourceKindFlag value
);

/**
 * MaplibreNativeFfiResourceTransformRule:flags: (setter set_flags) (getter get_flags)
 *
 * The `flags` member.
 */
/**
 * maplibre_native_ffi_resource_transform_rule_get_flags: (get-property flags)
 *
 * Gets `MaplibreNativeFfiResourceTransformRule:flags`.
 *
 * Returns: the value
 */
MaplibreNativeFfiUrlMatchFlag
maplibre_native_ffi_resource_transform_rule_get_flags(
  MaplibreNativeFfiResourceTransformRule* self
);

/**
 * maplibre_native_ffi_resource_transform_rule_set_flags: (set-property flags)
 *
 * Sets `MaplibreNativeFfiResourceTransformRule:flags`.
 */
void maplibre_native_ffi_resource_transform_rule_set_flags(
  MaplibreNativeFfiResourceTransformRule* self,
  MaplibreNativeFfiUrlMatchFlag value
);

/**
 * MaplibreNativeFfiResourceTransformRule:url: (setter set_url) (getter get_url)
 *
 * The `url` member.
 */
/**
 * maplibre_native_ffi_resource_transform_rule_get_url: (get-property url)
 *
 * Gets `MaplibreNativeFfiResourceTransformRule:url`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_transform_rule_get_url(
  MaplibreNativeFfiResourceTransformRule* self
);

/**
 * maplibre_native_ffi_resource_transform_rule_set_url: (set-property url)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiResourceTransformRule:url`.
 */
void maplibre_native_ffi_resource_transform_rule_set_url(
  MaplibreNativeFfiResourceTransformRule* self, const char* value
);

/**
 * maplibre_native_ffi_resource_transform_rule_get_parameters:
 *
 * Gets the value that `maplibre_native_ffi_resource_transform_rule_set_parameters` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiUrlQueryParameter): the items
 */
GPtrArray* maplibre_native_ffi_resource_transform_rule_get_parameters(
  MaplibreNativeFfiResourceTransformRule* self
);

/**
 * maplibre_native_ffi_resource_transform_rule_set_parameters:
 * @value: (array length=value_count): the items
 *
 * The `parameters` member.
 */
void maplibre_native_ffi_resource_transform_rule_set_parameters(
  MaplibreNativeFfiResourceTransformRule* self,
  MaplibreNativeFfiUrlQueryParameter* const* value, gsize value_count
);

/**
 * MaplibreNativeFfiResourceTransform:
 *
 * How to rewrite request URLs, for
 * `maplibre_native_ffi_runtime_set_resource_transform_async()`.
 *
 * Set either `callback` or `rules`, not both.
 *
 * See `mln_resource_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_resource_transform_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_TRANSFORM \
  (maplibre_native_ffi_resource_transform_get_type())

/**
 * maplibre_native_ffi_resource_transform_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiResourceTransform` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiResourceTransform* maplibre_native_ffi_resource_transform_new(
  void
);

/**
 * maplibre_native_ffi_resource_transform_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiResourceTransform* maplibre_native_ffi_resource_transform_copy(
  const MaplibreNativeFfiResourceTransform* self
);

/**
 * maplibre_native_ffi_resource_transform_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_resource_transform_free(
  MaplibreNativeFfiResourceTransform* self
);

/**
 * maplibre_native_ffi_resource_transform_get_rules:
 *
 * Gets the value that `maplibre_native_ffi_resource_transform_set_rules` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiResourceTransformRule): the items
 */
GPtrArray* maplibre_native_ffi_resource_transform_get_rules(
  const MaplibreNativeFfiResourceTransform* self
);

/**
 * maplibre_native_ffi_resource_transform_set_rules:
 * @value: (array length=value_count): the items
 *
 * The first rule that matches a request adds its parameters, and no other. A
 * request that no rule matches keeps its URL.
 */
void maplibre_native_ffi_resource_transform_set_rules(
  MaplibreNativeFfiResourceTransform* self,
  MaplibreNativeFfiResourceTransformRule* const* value, gsize value_count
);

/**
 * maplibre_native_ffi_resource_transform_set_callback:
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Rewrites the URL of a network request before the library sends it.
 *
 * The request waits until you answer with
 * `maplibre_native_ffi_resource_transform_request_complete()` or
 * `maplibre_native_ffi_resource_transform_request_fail()`. To keep `url`,
 * complete with it. Releasing `request` without an answer fails the request.
 * You can answer now or later, from any thread.
 *
 * See `mln_resource_transform_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_resource_transform_set_callback(
  MaplibreNativeFfiResourceTransform* self,
  MaplibreNativeFfiResourceTransformCallback callback, gpointer user_data,
  GDestroyNotify notify
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiResourceTransform,
  maplibre_native_ffi_resource_transform_free
)

/**
 * MaplibreNativeFfiResourceTransformError:
 *
 * Why a resource transform fails a request.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_resource_transform_error` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
/**
 * maplibre_native_ffi_resource_transform_error_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiResourceTransformError` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiResourceTransformError*
maplibre_native_ffi_resource_transform_error_new(void);

/**
 * MaplibreNativeFfiResourceTransformError:reason: (setter set_reason) (getter get_reason)
 *
 * The `reason` member.
 */
/**
 * maplibre_native_ffi_resource_transform_error_get_reason: (get-property reason)
 *
 * Gets `MaplibreNativeFfiResourceTransformError:reason`.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceErrorReason
maplibre_native_ffi_resource_transform_error_get_reason(
  MaplibreNativeFfiResourceTransformError* self
);

/**
 * maplibre_native_ffi_resource_transform_error_set_reason: (set-property reason)
 *
 * Sets `MaplibreNativeFfiResourceTransformError:reason`.
 */
void maplibre_native_ffi_resource_transform_error_set_reason(
  MaplibreNativeFfiResourceTransformError* self,
  MaplibreNativeFfiResourceErrorReason value
);

/**
 * MaplibreNativeFfiResourceTransformError:message: (setter set_message) (getter get_message)
 *
 * The `message` member.
 */
/**
 * maplibre_native_ffi_resource_transform_error_get_message: (get-property message)
 *
 * Gets `MaplibreNativeFfiResourceTransformError:message`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_transform_error_get_message(
  MaplibreNativeFfiResourceTransformError* self
);

/**
 * maplibre_native_ffi_resource_transform_error_set_message: (set-property message)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiResourceTransformError:message`.
 */
void maplibre_native_ffi_resource_transform_error_set_message(
  MaplibreNativeFfiResourceTransformError* self, const char* value
);

/**
 * MaplibreNativeFfiResourceTransformError:retry-after-unix-ms: (setter set_retry_after_unix_ms) (getter get_retry_after_unix_ms)
 *
 * When to retry, for `MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_RATE_LIMIT`.
 */
/**
 * maplibre_native_ffi_resource_transform_error_get_retry_after_unix_ms: (get-property retry-after-unix-ms)
 *
 * Gets `MaplibreNativeFfiResourceTransformError:retry-after-unix-ms`.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_resource_transform_error_get_retry_after_unix_ms(
  MaplibreNativeFfiResourceTransformError* self
);

/**
 * maplibre_native_ffi_resource_transform_error_has_retry_after_unix_ms:
 *
 * Whether `MaplibreNativeFfiResourceTransformError:retry-after-unix-ms` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_resource_transform_error_has_retry_after_unix_ms(
  MaplibreNativeFfiResourceTransformError* self
);

/**
 * maplibre_native_ffi_resource_transform_error_set_retry_after_unix_ms: (set-property retry-after-unix-ms)
 *
 * Sets `MaplibreNativeFfiResourceTransformError:retry-after-unix-ms`.
 */
void maplibre_native_ffi_resource_transform_error_set_retry_after_unix_ms(
  MaplibreNativeFfiResourceTransformError* self, gint64 value
);

GType maplibre_native_ffi_runtime_event_camera_change_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_CAMERA_CHANGE \
  (maplibre_native_ffi_runtime_event_camera_change_get_type())

/**
 * maplibre_native_ffi_runtime_event_camera_change_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventCameraChange*
maplibre_native_ffi_runtime_event_camera_change_copy(
  const MaplibreNativeFfiRuntimeEventCameraChange* self
);

/**
 * maplibre_native_ffi_runtime_event_camera_change_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_camera_change_free(
  MaplibreNativeFfiRuntimeEventCameraChange* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventCameraChange,
  maplibre_native_ffi_runtime_event_camera_change_free
)

GType maplibre_native_ffi_runtime_event_loading_failed_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_LOADING_FAILED \
  (maplibre_native_ffi_runtime_event_loading_failed_get_type())

/**
 * maplibre_native_ffi_runtime_event_loading_failed_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventLoadingFailed*
maplibre_native_ffi_runtime_event_loading_failed_copy(
  const MaplibreNativeFfiRuntimeEventLoadingFailed* self
);

/**
 * maplibre_native_ffi_runtime_event_loading_failed_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_loading_failed_free(
  MaplibreNativeFfiRuntimeEventLoadingFailed* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventLoadingFailed,
  maplibre_native_ffi_runtime_event_loading_failed_free
)

GType maplibre_native_ffi_runtime_event_render_frame_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_RENDER_FRAME \
  (maplibre_native_ffi_runtime_event_render_frame_get_type())

/**
 * maplibre_native_ffi_runtime_event_render_frame_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventRenderFrame*
maplibre_native_ffi_runtime_event_render_frame_copy(
  const MaplibreNativeFfiRuntimeEventRenderFrame* self
);

/**
 * maplibre_native_ffi_runtime_event_render_frame_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_render_frame_free(
  MaplibreNativeFfiRuntimeEventRenderFrame* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventRenderFrame,
  maplibre_native_ffi_runtime_event_render_frame_free
)

/**
 * MaplibreNativeFfiRuntimeEventStyleImage:
 *
 * The details of `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING`.
 *
 * See `mln_runtime_event_style_image` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_runtime_event_style_image_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_STYLE_IMAGE \
  (maplibre_native_ffi_runtime_event_style_image_get_type())

/**
 * maplibre_native_ffi_runtime_event_style_image_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRuntimeEventStyleImage` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRuntimeEventStyleImage*
maplibre_native_ffi_runtime_event_style_image_new(void);

/**
 * maplibre_native_ffi_runtime_event_style_image_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventStyleImage*
maplibre_native_ffi_runtime_event_style_image_copy(
  const MaplibreNativeFfiRuntimeEventStyleImage* self
);

/**
 * maplibre_native_ffi_runtime_event_style_image_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_style_image_free(
  MaplibreNativeFfiRuntimeEventStyleImage* self
);

/**
 * maplibre_native_ffi_runtime_event_style_image_get_image_id:
 *
 * The `image_id` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_event_style_image_get_image_id(
  const MaplibreNativeFfiRuntimeEventStyleImage* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventStyleImage,
  maplibre_native_ffi_runtime_event_style_image_free
)

GType maplibre_native_ffi_tile_id_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_TILE_ID \
  (maplibre_native_ffi_tile_id_get_type())

/**
 * maplibre_native_ffi_tile_id_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiTileId* maplibre_native_ffi_tile_id_copy(
  const MaplibreNativeFfiTileId* self
);

/**
 * maplibre_native_ffi_tile_id_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_tile_id_free(MaplibreNativeFfiTileId* self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiTileId, maplibre_native_ffi_tile_id_free
)

/**
 * MaplibreNativeFfiRuntimeEventTileAction:
 *
 * The details of `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_TILE_ACTION`.
 *
 * See `mln_runtime_event_tile_action` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_runtime_event_tile_action_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_TILE_ACTION \
  (maplibre_native_ffi_runtime_event_tile_action_get_type())

/**
 * maplibre_native_ffi_runtime_event_tile_action_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRuntimeEventTileAction` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRuntimeEventTileAction*
maplibre_native_ffi_runtime_event_tile_action_new(void);

/**
 * maplibre_native_ffi_runtime_event_tile_action_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventTileAction*
maplibre_native_ffi_runtime_event_tile_action_copy(
  const MaplibreNativeFfiRuntimeEventTileAction* self
);

/**
 * maplibre_native_ffi_runtime_event_tile_action_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_tile_action_free(
  MaplibreNativeFfiRuntimeEventTileAction* self
);

/**
 * maplibre_native_ffi_runtime_event_tile_action_get_source_id:
 *
 * The source that the tile belongs to.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_event_tile_action_get_source_id(
  const MaplibreNativeFfiRuntimeEventTileAction* self
);

/**
 * maplibre_native_ffi_runtime_event_tile_action_get_operation:
 *
 * The `operation` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiTileOperation
maplibre_native_ffi_runtime_event_tile_action_get_operation(
  const MaplibreNativeFfiRuntimeEventTileAction* self
);

/**
 * maplibre_native_ffi_runtime_event_tile_action_get_tile_id:
 *
 * The `tile_id` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiTileId*
maplibre_native_ffi_runtime_event_tile_action_get_tile_id(
  const MaplibreNativeFfiRuntimeEventTileAction* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventTileAction,
  maplibre_native_ffi_runtime_event_tile_action_free
)

/**
 * MaplibreNativeFfiRuntimeEventSourceChanged:
 *
 * The details of `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_SOURCE_CHANGED`.
 *
 * See `mln_runtime_event_source_changed` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_runtime_event_source_changed_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_SOURCE_CHANGED \
  (maplibre_native_ffi_runtime_event_source_changed_get_type())

/**
 * maplibre_native_ffi_runtime_event_source_changed_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRuntimeEventSourceChanged` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRuntimeEventSourceChanged*
maplibre_native_ffi_runtime_event_source_changed_new(void);

/**
 * maplibre_native_ffi_runtime_event_source_changed_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventSourceChanged*
maplibre_native_ffi_runtime_event_source_changed_copy(
  const MaplibreNativeFfiRuntimeEventSourceChanged* self
);

/**
 * maplibre_native_ffi_runtime_event_source_changed_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_source_changed_free(
  MaplibreNativeFfiRuntimeEventSourceChanged* self
);

/**
 * maplibre_native_ffi_runtime_event_source_changed_get_source_id:
 *
 * The `source_id` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_event_source_changed_get_source_id(
  const MaplibreNativeFfiRuntimeEventSourceChanged* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventSourceChanged,
  maplibre_native_ffi_runtime_event_source_changed_free
)

GType maplibre_native_ffi_runtime_event_offline_region_status_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_OFFLINE_REGION_STATUS \
  (maplibre_native_ffi_runtime_event_offline_region_status_get_type())

/**
 * maplibre_native_ffi_runtime_event_offline_region_status_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventOfflineRegionStatus*
maplibre_native_ffi_runtime_event_offline_region_status_copy(
  const MaplibreNativeFfiRuntimeEventOfflineRegionStatus* self
);

/**
 * maplibre_native_ffi_runtime_event_offline_region_status_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_offline_region_status_free(
  MaplibreNativeFfiRuntimeEventOfflineRegionStatus* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventOfflineRegionStatus,
  maplibre_native_ffi_runtime_event_offline_region_status_free
)

GType maplibre_native_ffi_runtime_event_offline_region_response_error_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR \
  (maplibre_native_ffi_runtime_event_offline_region_response_error_get_type())

/**
 * maplibre_native_ffi_runtime_event_offline_region_response_error_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventOfflineRegionResponseError*
maplibre_native_ffi_runtime_event_offline_region_response_error_copy(
  const MaplibreNativeFfiRuntimeEventOfflineRegionResponseError* self
);

/**
 * maplibre_native_ffi_runtime_event_offline_region_response_error_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_offline_region_response_error_free(
  MaplibreNativeFfiRuntimeEventOfflineRegionResponseError* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventOfflineRegionResponseError,
  maplibre_native_ffi_runtime_event_offline_region_response_error_free
)

GType maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT \
  (maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_get_type())

/**
 * maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit*
maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_copy(
  const MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit* self
);

/**
 * maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_free(
  MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit,
  maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_free
)

/**
 * MaplibreNativeFfiRuntimeEventGlyphs:
 *
 * The details of a glyph event.
 *
 * The event covers the glyphs from `first` to `last`, both included.
 *
 * See `mln_runtime_event_glyphs` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_runtime_event_glyphs_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_GLYPHS \
  (maplibre_native_ffi_runtime_event_glyphs_get_type())

/**
 * maplibre_native_ffi_runtime_event_glyphs_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRuntimeEventGlyphs` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRuntimeEventGlyphs*
maplibre_native_ffi_runtime_event_glyphs_new(void);

/**
 * maplibre_native_ffi_runtime_event_glyphs_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventGlyphs*
maplibre_native_ffi_runtime_event_glyphs_copy(
  const MaplibreNativeFfiRuntimeEventGlyphs* self
);

/**
 * maplibre_native_ffi_runtime_event_glyphs_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_glyphs_free(
  MaplibreNativeFfiRuntimeEventGlyphs* self
);

/**
 * maplibre_native_ffi_runtime_event_glyphs_get_font_stack:
 *
 * The fonts that the glyphs come from, in order of preference.
 *
 * Returns: (transfer none) (array zero-terminated=1): the texts
 */
const char* const* maplibre_native_ffi_runtime_event_glyphs_get_font_stack(
  const MaplibreNativeFfiRuntimeEventGlyphs* self
);

/**
 * maplibre_native_ffi_runtime_event_glyphs_get_first:
 *
 * The `first` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_runtime_event_glyphs_get_first(
  const MaplibreNativeFfiRuntimeEventGlyphs* self
);

/**
 * maplibre_native_ffi_runtime_event_glyphs_get_last:
 *
 * The `last` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_runtime_event_glyphs_get_last(
  const MaplibreNativeFfiRuntimeEventGlyphs* self
);

/**
 * maplibre_native_ffi_runtime_event_glyphs_get_font_face:
 *
 * The `font_face` member.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_runtime_event_glyphs_get_font_face(
  const MaplibreNativeFfiRuntimeEventGlyphs* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventGlyphs,
  maplibre_native_ffi_runtime_event_glyphs_free
)

/**
 * MaplibreNativeFfiRuntimeEventSprite:
 *
 * The details of a sprite event.
 *
 * `id` and `url` are both empty when the style has no sprite.
 *
 * See `mln_runtime_event_sprite` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_runtime_event_sprite_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_SPRITE \
  (maplibre_native_ffi_runtime_event_sprite_get_type())

/**
 * maplibre_native_ffi_runtime_event_sprite_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRuntimeEventSprite` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRuntimeEventSprite*
maplibre_native_ffi_runtime_event_sprite_new(void);

/**
 * maplibre_native_ffi_runtime_event_sprite_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventSprite*
maplibre_native_ffi_runtime_event_sprite_copy(
  const MaplibreNativeFfiRuntimeEventSprite* self
);

/**
 * maplibre_native_ffi_runtime_event_sprite_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_sprite_free(
  MaplibreNativeFfiRuntimeEventSprite* self
);

/**
 * maplibre_native_ffi_runtime_event_sprite_get_id:
 *
 * The `id` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_event_sprite_get_id(
  const MaplibreNativeFfiRuntimeEventSprite* self
);

/**
 * maplibre_native_ffi_runtime_event_sprite_get_url:
 *
 * The `url` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_event_sprite_get_url(
  const MaplibreNativeFfiRuntimeEventSprite* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventSprite,
  maplibre_native_ffi_runtime_event_sprite_free
)

/**
 * MaplibreNativeFfiRuntimeEventPayload:
 *
 * The details of an event, in the member that its `payload_type` names.
 *
 * See `mln_runtime_event_payload` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_runtime_event_payload_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_PAYLOAD \
  (maplibre_native_ffi_runtime_event_payload_get_type())

/**
 * maplibre_native_ffi_runtime_event_payload_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventPayload*
maplibre_native_ffi_runtime_event_payload_copy(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_payload_free(
  MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_camera_change:
 *
 * Whether the value holds `maplibre_native_ffi_runtime_event_payload_get_camera_change`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_runtime_event_payload_is_camera_change(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_get_camera_change:
 *
 * The `camera_change` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiRuntimeEventCameraChange*
maplibre_native_ffi_runtime_event_payload_get_camera_change(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_loading_failed:
 *
 * Whether the value holds `maplibre_native_ffi_runtime_event_payload_get_loading_failed`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_runtime_event_payload_is_loading_failed(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_get_loading_failed:
 *
 * The `loading_failed` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiRuntimeEventLoadingFailed*
maplibre_native_ffi_runtime_event_payload_get_loading_failed(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_render_frame:
 *
 * Whether the value holds `maplibre_native_ffi_runtime_event_payload_get_render_frame`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_runtime_event_payload_is_render_frame(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_get_render_frame:
 *
 * The `render_frame` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiRuntimeEventRenderFrame*
maplibre_native_ffi_runtime_event_payload_get_render_frame(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_style_image:
 *
 * Whether the value holds `maplibre_native_ffi_runtime_event_payload_get_style_image`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_runtime_event_payload_is_style_image(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_get_style_image:
 *
 * The `style_image` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiRuntimeEventStyleImage*
maplibre_native_ffi_runtime_event_payload_get_style_image(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_tile_action:
 *
 * Whether the value holds `maplibre_native_ffi_runtime_event_payload_get_tile_action`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_runtime_event_payload_is_tile_action(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_get_tile_action:
 *
 * The `tile_action` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiRuntimeEventTileAction*
maplibre_native_ffi_runtime_event_payload_get_tile_action(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_source_changed:
 *
 * Whether the value holds `maplibre_native_ffi_runtime_event_payload_get_source_changed`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_runtime_event_payload_is_source_changed(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_get_source_changed:
 *
 * The `source_changed` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiRuntimeEventSourceChanged*
maplibre_native_ffi_runtime_event_payload_get_source_changed(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_offline_region_status:
 *
 * Whether the value holds `maplibre_native_ffi_runtime_event_payload_get_offline_region_status`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_runtime_event_payload_is_offline_region_status(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_get_offline_region_status:
 *
 * The `offline_region_status` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiRuntimeEventOfflineRegionStatus*
maplibre_native_ffi_runtime_event_payload_get_offline_region_status(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_offline_region_response_error:
 *
 * Whether the value holds `maplibre_native_ffi_runtime_event_payload_get_offline_region_response_error`.
 *
 * Returns: whether the value holds the variant
 */
gboolean
maplibre_native_ffi_runtime_event_payload_is_offline_region_response_error(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_get_offline_region_response_error:
 *
 * The `offline_region_response_error` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiRuntimeEventOfflineRegionResponseError*
maplibre_native_ffi_runtime_event_payload_get_offline_region_response_error(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_offline_region_tile_count_limit:
 *
 * Whether the value holds `maplibre_native_ffi_runtime_event_payload_get_offline_region_tile_count_limit`.
 *
 * Returns: whether the value holds the variant
 */
gboolean
maplibre_native_ffi_runtime_event_payload_is_offline_region_tile_count_limit(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_get_offline_region_tile_count_limit:
 *
 * The `offline_region_tile_count_limit` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit*
maplibre_native_ffi_runtime_event_payload_get_offline_region_tile_count_limit(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_glyphs:
 *
 * Whether the value holds `maplibre_native_ffi_runtime_event_payload_get_glyphs`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_runtime_event_payload_is_glyphs(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_get_glyphs:
 *
 * The `glyphs` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiRuntimeEventGlyphs*
maplibre_native_ffi_runtime_event_payload_get_glyphs(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_sprite:
 *
 * Whether the value holds `maplibre_native_ffi_runtime_event_payload_get_sprite`.
 *
 * Returns: whether the value holds the variant
 */
gboolean maplibre_native_ffi_runtime_event_payload_is_sprite(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_get_sprite:
 *
 * The `sprite` variant.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiRuntimeEventSprite*
maplibre_native_ffi_runtime_event_payload_get_sprite(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

/**
 * maplibre_native_ffi_runtime_event_payload_is_none:
 *
 * Whether the value is the `none` variant.
 *
 * Returns: whether the value is the variant
 */
gboolean maplibre_native_ffi_runtime_event_payload_is_none(
  const MaplibreNativeFfiRuntimeEventPayload* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventPayload,
  maplibre_native_ffi_runtime_event_payload_free
)

/**
 * MaplibreNativeFfiRuntimeEvent:
 *
 * An event that a runtime or one of its maps raised.
 *
 * See `mln_runtime_event` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_runtime_event_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT \
  (maplibre_native_ffi_runtime_event_get_type())

/**
 * maplibre_native_ffi_runtime_event_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRuntimeEvent` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRuntimeEvent* maplibre_native_ffi_runtime_event_new(void);

/**
 * maplibre_native_ffi_runtime_event_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEvent* maplibre_native_ffi_runtime_event_copy(
  const MaplibreNativeFfiRuntimeEvent* self
);

/**
 * maplibre_native_ffi_runtime_event_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_free(
  MaplibreNativeFfiRuntimeEvent* self
);

/**
 * maplibre_native_ffi_runtime_event_get_event_type:
 *
 * The `type` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiRuntimeEventType
maplibre_native_ffi_runtime_event_get_event_type(
  const MaplibreNativeFfiRuntimeEvent* self
);

/**
 * maplibre_native_ffi_runtime_event_get_source_type:
 *
 * The `source_type` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiRuntimeEventSourceType
maplibre_native_ffi_runtime_event_get_source_type(
  const MaplibreNativeFfiRuntimeEvent* self
);

/**
 * maplibre_native_ffi_runtime_event_get_source:
 *
 * The runtime or map that raised the event, as `source_type` says.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_runtime_event_get_source(
  const MaplibreNativeFfiRuntimeEvent* self
);

/**
 * maplibre_native_ffi_runtime_event_get_generation:
 *
 * For a map event, a map snapshot with this generation or a later one includes
 * what the event reports. Zero for a runtime event.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_runtime_event_get_generation(
  const MaplibreNativeFfiRuntimeEvent* self
);

/**
 * maplibre_native_ffi_runtime_event_get_message:
 *
 * The error's text for an event that reports a failure, or empty.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_event_get_message(
  const MaplibreNativeFfiRuntimeEvent* self
);

/**
 * maplibre_native_ffi_runtime_event_get_payload:
 *
 * The `payload` member.
 *
 * Returns: (transfer none) (nullable): the value
 */
MaplibreNativeFfiRuntimeEventPayload*
maplibre_native_ffi_runtime_event_get_payload(
  const MaplibreNativeFfiRuntimeEvent* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEvent, maplibre_native_ffi_runtime_event_free
)

/**
 * MaplibreNativeFfiRuntimeEventHandler:
 *
 * An event callback, for
 * `maplibre_native_ffi_runtime_set_event_handler_async()`.
 *
 * See `mln_runtime_event_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_runtime_event_handler_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_HANDLER \
  (maplibre_native_ffi_runtime_event_handler_get_type())

/**
 * maplibre_native_ffi_runtime_event_handler_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRuntimeEventHandler` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRuntimeEventHandler*
maplibre_native_ffi_runtime_event_handler_new(void);

/**
 * maplibre_native_ffi_runtime_event_handler_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeEventHandler*
maplibre_native_ffi_runtime_event_handler_copy(
  const MaplibreNativeFfiRuntimeEventHandler* self
);

/**
 * maplibre_native_ffi_runtime_event_handler_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_event_handler_free(
  MaplibreNativeFfiRuntimeEventHandler* self
);

/**
 * maplibre_native_ffi_runtime_event_handler_set_callback:
 * @callback: (scope notified) (closure user_data) (destroy notify) (nullable): the callback
 * @user_data: the data to pass to `callback`
 * @notify: the function that frees `user_data` after the last call
 *
 * Receives a batch of events.
 *
 * See `mln_runtime_event_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_runtime_event_handler_set_callback(
  MaplibreNativeFfiRuntimeEventHandler* self,
  MaplibreNativeFfiRuntimeEventCallback callback, gpointer user_data,
  GDestroyNotify notify
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeEventHandler,
  maplibre_native_ffi_runtime_event_handler_free
)

/**
 * MaplibreNativeFfiRuntimeOptions:
 *
 * Options for `maplibre_native_ffi_runtime_create()`.
 *
 * See `mln_runtime_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
/**
 * maplibre_native_ffi_runtime_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRuntimeOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRuntimeOptions* maplibre_native_ffi_runtime_options_new(void);

/**
 * MaplibreNativeFfiRuntimeOptions:asset-path: (setter set_asset_path) (getter get_asset_path)
 *
 * The directory that `asset://` URLs load from. Empty uses the working
 * directory, or the app's assets on Android.
 */
/**
 * maplibre_native_ffi_runtime_options_get_asset_path: (get-property asset-path)
 *
 * Gets `MaplibreNativeFfiRuntimeOptions:asset-path`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_options_get_asset_path(
  MaplibreNativeFfiRuntimeOptions* self
);

/**
 * maplibre_native_ffi_runtime_options_set_asset_path: (set-property asset-path)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiRuntimeOptions:asset-path`.
 */
void maplibre_native_ffi_runtime_options_set_asset_path(
  MaplibreNativeFfiRuntimeOptions* self, const char* value
);

/**
 * MaplibreNativeFfiRuntimeOptions:cache-path: (setter set_cache_path) (getter get_cache_path)
 *
 * The path of the cache database file, which holds the ambient cache and
 * offline regions. Its directory must be writable. Empty keeps the cache in
 * memory.
 */
/**
 * maplibre_native_ffi_runtime_options_get_cache_path: (get-property cache-path)
 *
 * Gets `MaplibreNativeFfiRuntimeOptions:cache-path`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_options_get_cache_path(
  MaplibreNativeFfiRuntimeOptions* self
);

/**
 * maplibre_native_ffi_runtime_options_set_cache_path: (set-property cache-path)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiRuntimeOptions:cache-path`.
 */
void maplibre_native_ffi_runtime_options_set_cache_path(
  MaplibreNativeFfiRuntimeOptions* self, const char* value
);

/**
 * MaplibreNativeFfiSourceFeature:
 *
 * A feature that a source feature query found.
 *
 * See `mln_source_feature` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
GType maplibre_native_ffi_source_feature_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_SOURCE_FEATURE \
  (maplibre_native_ffi_source_feature_get_type())

/**
 * maplibre_native_ffi_source_feature_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiSourceFeature` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiSourceFeature* maplibre_native_ffi_source_feature_new(void);

/**
 * maplibre_native_ffi_source_feature_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiSourceFeature* maplibre_native_ffi_source_feature_copy(
  const MaplibreNativeFfiSourceFeature* self
);

/**
 * maplibre_native_ffi_source_feature_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_source_feature_free(
  MaplibreNativeFfiSourceFeature* self
);

/**
 * maplibre_native_ffi_source_feature_get_feature:
 *
 * The feature, as a GeoJSON feature.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_source_feature_get_feature(
  const MaplibreNativeFfiSourceFeature* self
);

/**
 * maplibre_native_ffi_source_feature_get_source_layer_id:
 *
 * The feature's source layer, or empty when its source has none.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_source_feature_get_source_layer_id(
  const MaplibreNativeFfiSourceFeature* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiSourceFeature, maplibre_native_ffi_source_feature_free
)

/**
 * MaplibreNativeFfiSourceFeatureQueryOptions:
 *
 * Options for a source feature query.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_source_feature_query_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
/**
 * maplibre_native_ffi_source_feature_query_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiSourceFeatureQueryOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiSourceFeatureQueryOptions*
maplibre_native_ffi_source_feature_query_options_new(void);

/**
 * MaplibreNativeFfiSourceFeatureQueryOptions:source-layer-ids: (setter set_source_layer_ids) (getter get_source_layer_ids)
 *
 * The source layers to query. A vector source finds features only in the source
 * layers named here.
 */
/**
 * maplibre_native_ffi_source_feature_query_options_get_source_layer_ids: (get-property source-layer-ids)
 *
 * Gets `MaplibreNativeFfiSourceFeatureQueryOptions:source-layer-ids`.
 *
 * Returns: (transfer none) (array zero-terminated=1): the texts
 */
const char* const*
maplibre_native_ffi_source_feature_query_options_get_source_layer_ids(
  MaplibreNativeFfiSourceFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_source_feature_query_options_has_source_layer_ids:
 *
 * Whether `MaplibreNativeFfiSourceFeatureQueryOptions:source-layer-ids` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_source_feature_query_options_has_source_layer_ids(
  MaplibreNativeFfiSourceFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_source_feature_query_options_set_source_layer_ids: (set-property source-layer-ids)
 * @value: (array zero-terminated=1): the texts
 *
 * Sets `MaplibreNativeFfiSourceFeatureQueryOptions:source-layer-ids`.
 */
void maplibre_native_ffi_source_feature_query_options_set_source_layer_ids(
  MaplibreNativeFfiSourceFeatureQueryOptions* self, const char* const* value
);

/**
 * MaplibreNativeFfiSourceFeatureQueryOptions:filter: (setter set_filter) (getter get_filter)
 *
 * A \[filter\](https://maplibre.org/maplibre-style-spec/expressions/) that each
 * feature must match. Empty matches every feature.
 */
/**
 * maplibre_native_ffi_source_feature_query_options_get_filter: (get-property filter)
 *
 * Gets `MaplibreNativeFfiSourceFeatureQueryOptions:filter`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_source_feature_query_options_get_filter(
  MaplibreNativeFfiSourceFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_source_feature_query_options_set_filter: (set-property filter)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiSourceFeatureQueryOptions:filter`.
 */
void maplibre_native_ffi_source_feature_query_options_set_filter(
  MaplibreNativeFfiSourceFeatureQueryOptions* self, const char* value
);

GType maplibre_native_ffi_style_expression_info_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION_INFO \
  (maplibre_native_ffi_style_expression_info_get_type())

/**
 * maplibre_native_ffi_style_expression_info_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiStyleExpressionInfo*
maplibre_native_ffi_style_expression_info_copy(
  const MaplibreNativeFfiStyleExpressionInfo* self
);

/**
 * maplibre_native_ffi_style_expression_info_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_style_expression_info_free(
  MaplibreNativeFfiStyleExpressionInfo* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiStyleExpressionInfo,
  maplibre_native_ffi_style_expression_info_free
)

/**
 * MaplibreNativeFfiStyleExpressionInputs:
 *
 * The inputs that a style expression reads while it evaluates.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_style_expression_inputs` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
/**
 * maplibre_native_ffi_style_expression_inputs_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleExpressionInputs` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleExpressionInputs*
maplibre_native_ffi_style_expression_inputs_new(void);

/**
 * MaplibreNativeFfiStyleExpressionInputs:zoom: (setter set_zoom) (getter get_zoom)
 *
 * The `zoom` member.
 */
/**
 * maplibre_native_ffi_style_expression_inputs_get_zoom: (get-property zoom)
 *
 * Gets `MaplibreNativeFfiStyleExpressionInputs:zoom`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_expression_inputs_get_zoom(
  MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_has_zoom:
 *
 * Whether `MaplibreNativeFfiStyleExpressionInputs:zoom` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_expression_inputs_has_zoom(
  MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_zoom: (set-property zoom)
 *
 * Sets `MaplibreNativeFfiStyleExpressionInputs:zoom`.
 */
void maplibre_native_ffi_style_expression_inputs_set_zoom(
  MaplibreNativeFfiStyleExpressionInputs* self, gdouble value
);

/**
 * MaplibreNativeFfiStyleExpressionInputs:color-ramp-parameter: (setter set_color_ramp_parameter) (getter get_color_ramp_parameter)
 *
 * The value of `heatmap-density`, `line-progress`, and `elevation`.
 */
/**
 * maplibre_native_ffi_style_expression_inputs_get_color_ramp_parameter: (get-property color-ramp-parameter)
 *
 * Gets `MaplibreNativeFfiStyleExpressionInputs:color-ramp-parameter`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_expression_inputs_get_color_ramp_parameter(
  MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_has_color_ramp_parameter:
 *
 * Whether `MaplibreNativeFfiStyleExpressionInputs:color-ramp-parameter` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_expression_inputs_has_color_ramp_parameter(
  MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_color_ramp_parameter: (set-property color-ramp-parameter)
 *
 * Sets `MaplibreNativeFfiStyleExpressionInputs:color-ramp-parameter`.
 */
void maplibre_native_ffi_style_expression_inputs_set_color_ramp_parameter(
  MaplibreNativeFfiStyleExpressionInputs* self, gdouble value
);

/**
 * MaplibreNativeFfiStyleExpressionInputs:feature: (setter set_feature) (getter get_feature)
 *
 * The feature to evaluate against, as a GeoJSON feature, or empty for none.
 */
/**
 * maplibre_native_ffi_style_expression_inputs_get_feature: (get-property feature)
 *
 * Gets `MaplibreNativeFfiStyleExpressionInputs:feature`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_expression_inputs_get_feature(
  MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_feature: (set-property feature)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiStyleExpressionInputs:feature`.
 */
void maplibre_native_ffi_style_expression_inputs_set_feature(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
);

/**
 * MaplibreNativeFfiStyleExpressionInputs:feature-state: (setter set_feature_state) (getter get_feature_state)
 *
 * The feature's state, as a JSON object, or empty for none.
 */
/**
 * maplibre_native_ffi_style_expression_inputs_get_feature_state: (get-property feature-state)
 *
 * Gets `MaplibreNativeFfiStyleExpressionInputs:feature-state`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_expression_inputs_get_feature_state(
  MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_feature_state: (set-property feature-state)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiStyleExpressionInputs:feature-state`.
 */
void maplibre_native_ffi_style_expression_inputs_set_feature_state(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
);

/**
 * MaplibreNativeFfiStyleExpressionInputs:global-state: (setter set_global_state) (getter get_global_state)
 *
 * The values that `global-state` reads, as a JSON object, or empty for none.
 */
/**
 * maplibre_native_ffi_style_expression_inputs_get_global_state: (get-property global-state)
 *
 * Gets `MaplibreNativeFfiStyleExpressionInputs:global-state`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_expression_inputs_get_global_state(
  MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_global_state: (set-property global-state)
 * @value: the UTF-8 text
 *
 * Sets `MaplibreNativeFfiStyleExpressionInputs:global-state`.
 */
void maplibre_native_ffi_style_expression_inputs_set_global_state(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
);

/**
 * MaplibreNativeFfiStyleExpressionInputs:canonical-tile-id: (setter set_canonical_tile_id) (getter get_canonical_tile_id)
 *
 * The tile that holds the feature. `within` and `distance` need it to read the
 * feature's geometry.
 */
/**
 * maplibre_native_ffi_style_expression_inputs_get_canonical_tile_id: (get-property canonical-tile-id)
 *
 * Gets `MaplibreNativeFfiStyleExpressionInputs:canonical-tile-id`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiCanonicalTileId*
maplibre_native_ffi_style_expression_inputs_get_canonical_tile_id(
  MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_has_canonical_tile_id:
 *
 * Whether `MaplibreNativeFfiStyleExpressionInputs:canonical-tile-id` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_expression_inputs_has_canonical_tile_id(
  MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_canonical_tile_id: (set-property canonical-tile-id)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiStyleExpressionInputs:canonical-tile-id`.
 */
void maplibre_native_ffi_style_expression_inputs_set_canonical_tile_id(
  MaplibreNativeFfiStyleExpressionInputs* self,
  const MaplibreNativeFfiCanonicalTileId* value
);

/**
 * MaplibreNativeFfiStyleExpressionInputs:available-images: (setter set_available_images) (getter get_available_images)
 *
 * The IDs of the images that `image` treats as available.
 */
/**
 * maplibre_native_ffi_style_expression_inputs_get_available_images: (get-property available-images)
 *
 * Gets `MaplibreNativeFfiStyleExpressionInputs:available-images`.
 *
 * Returns: (transfer none) (array zero-terminated=1): the texts
 */
const char* const*
maplibre_native_ffi_style_expression_inputs_get_available_images(
  MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_available_images: (set-property available-images)
 * @value: (array zero-terminated=1): the texts
 *
 * Sets `MaplibreNativeFfiStyleExpressionInputs:available-images`.
 */
void maplibre_native_ffi_style_expression_inputs_set_available_images(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* const* value
);

GType maplibre_native_ffi_style_expression_type_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION_TYPE \
  (maplibre_native_ffi_style_expression_type_get_type())

/**
 * maplibre_native_ffi_style_expression_type_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiStyleExpressionType*
maplibre_native_ffi_style_expression_type_copy(
  const MaplibreNativeFfiStyleExpressionType* self
);

/**
 * maplibre_native_ffi_style_expression_type_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_style_expression_type_free(
  MaplibreNativeFfiStyleExpressionType* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiStyleExpressionType,
  maplibre_native_ffi_style_expression_type_free
)

/**
 * MaplibreNativeFfiStyleExpressionOptions:
 *
 * Options for parsing a style expression.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_style_expression_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
/**
 * maplibre_native_ffi_style_expression_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleExpressionOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleExpressionOptions*
maplibre_native_ffi_style_expression_options_new(void);

/**
 * MaplibreNativeFfiStyleExpressionOptions:expected-type: (setter set_expected_type) (getter get_expected_type)
 *
 * The type that the result must have.
 */
/**
 * maplibre_native_ffi_style_expression_options_get_expected_type: (get-property expected-type)
 *
 * Gets `MaplibreNativeFfiStyleExpressionOptions:expected-type`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiStyleExpressionType*
maplibre_native_ffi_style_expression_options_get_expected_type(
  MaplibreNativeFfiStyleExpressionOptions* self
);

/**
 * maplibre_native_ffi_style_expression_options_has_expected_type:
 *
 * Whether `MaplibreNativeFfiStyleExpressionOptions:expected-type` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_expression_options_has_expected_type(
  MaplibreNativeFfiStyleExpressionOptions* self
);

/**
 * maplibre_native_ffi_style_expression_options_set_expected_type: (set-property expected-type)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiStyleExpressionOptions:expected-type`.
 */
void maplibre_native_ffi_style_expression_options_set_expected_type(
  MaplibreNativeFfiStyleExpressionOptions* self,
  const MaplibreNativeFfiStyleExpressionType* value
);

/**
 * MaplibreNativeFfiStyleImageInfo:
 *
 * An image in the map's style.
 *
 * See `mln_style_image_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
GType maplibre_native_ffi_style_image_info_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_IMAGE_INFO \
  (maplibre_native_ffi_style_image_info_get_type())

/**
 * maplibre_native_ffi_style_image_info_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleImageInfo` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleImageInfo* maplibre_native_ffi_style_image_info_new(void);

/**
 * maplibre_native_ffi_style_image_info_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiStyleImageInfo* maplibre_native_ffi_style_image_info_copy(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_style_image_info_free(
  MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_get_width:
 *
 * The `width` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_style_image_info_get_width(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_get_height:
 *
 * The `height` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_style_image_info_get_height(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_get_pixels:
 *
 * The premultiplied RGBA pixels, in rows of `width` \* 4 bytes, from the top
 * row down.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_style_image_info_get_pixels(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_get_stretch_x:
 *
 * The `stretch_x` member.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiImageStretch): the items
 */
GPtrArray* maplibre_native_ffi_style_image_info_get_stretch_x(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_get_stretch_y:
 *
 * The `stretch_y` member.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiImageStretch): the items
 */
GPtrArray* maplibre_native_ffi_style_image_info_get_stretch_y(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_get_content:
 *
 * The `content` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiImageContent* maplibre_native_ffi_style_image_info_get_content(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_has_content:
 *
 * Whether the value has `maplibre_native_ffi_style_image_info_get_content`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_info_has_content(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_get_text_fit_width:
 *
 * The `text_fit_width` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_info_get_text_fit_width(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_has_text_fit_width:
 *
 * Whether the value has `maplibre_native_ffi_style_image_info_get_text_fit_width`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_info_has_text_fit_width(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_get_text_fit_height:
 *
 * The `text_fit_height` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_info_get_text_fit_height(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_has_text_fit_height:
 *
 * Whether the value has `maplibre_native_ffi_style_image_info_get_text_fit_height`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_info_has_text_fit_height(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_get_pixel_ratio:
 *
 * The `pixel_ratio` member.
 *
 * Returns: the value
 */
gfloat maplibre_native_ffi_style_image_info_get_pixel_ratio(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_get_sdf:
 *
 * The `sdf` member.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_style_image_info_get_sdf(
  const MaplibreNativeFfiStyleImageInfo* self
);

/**
 * maplibre_native_ffi_style_image_info_get_keep_unused:
 *
 * The `keep_unused` member.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_style_image_info_get_keep_unused(
  const MaplibreNativeFfiStyleImageInfo* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiStyleImageInfo, maplibre_native_ffi_style_image_info_free
)

/**
 * MaplibreNativeFfiStyleImageOptions:
 *
 * Options for a style image, as the
 * \[sprite\](https://maplibre.org/maplibre-style-spec/sprite/) spec describes
 * them.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_style_image_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
/**
 * maplibre_native_ffi_style_image_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleImageOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleImageOptions* maplibre_native_ffi_style_image_options_new(
  void
);

/**
 * maplibre_native_ffi_style_image_options_get_stretch_x:
 *
 * Gets the value that `maplibre_native_ffi_style_image_options_set_stretch_x` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiImageStretch): the items
 */
GPtrArray* maplibre_native_ffi_style_image_options_get_stretch_x(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_stretch_x:
 *
 * Whether the value sets `maplibre_native_ffi_style_image_options_set_stretch_x`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_stretch_x(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_stretch_x:
 * @value: (array length=value_count): the items
 *
 * The ranges of columns that stretch, in increasing order without overlap.
 */
void maplibre_native_ffi_style_image_options_set_stretch_x(
  MaplibreNativeFfiStyleImageOptions* self,
  const MaplibreNativeFfiImageStretch* value, gsize value_count
);

/**
 * maplibre_native_ffi_style_image_options_get_stretch_y:
 *
 * Gets the value that `maplibre_native_ffi_style_image_options_set_stretch_y` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiImageStretch): the items
 */
GPtrArray* maplibre_native_ffi_style_image_options_get_stretch_y(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_stretch_y:
 *
 * Whether the value sets `maplibre_native_ffi_style_image_options_set_stretch_y`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_stretch_y(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_stretch_y:
 * @value: (array length=value_count): the items
 *
 * The ranges of rows that stretch, in increasing order without overlap.
 */
void maplibre_native_ffi_style_image_options_set_stretch_y(
  MaplibreNativeFfiStyleImageOptions* self,
  const MaplibreNativeFfiImageStretch* value, gsize value_count
);

/**
 * MaplibreNativeFfiStyleImageOptions:content: (setter set_content) (getter get_content)
 *
 * The `content` member.
 */
/**
 * maplibre_native_ffi_style_image_options_get_content: (get-property content)
 *
 * Gets `MaplibreNativeFfiStyleImageOptions:content`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiImageContent*
maplibre_native_ffi_style_image_options_get_content(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_content:
 *
 * Whether `MaplibreNativeFfiStyleImageOptions:content` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_content(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_content: (set-property content)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiStyleImageOptions:content`.
 */
void maplibre_native_ffi_style_image_options_set_content(
  MaplibreNativeFfiStyleImageOptions* self,
  const MaplibreNativeFfiImageContent* value
);

/**
 * MaplibreNativeFfiStyleImageOptions:text-fit-width: (setter set_text_fit_width) (getter get_text_fit_width)
 *
 * The `text_fit_width` member.
 */
/**
 * maplibre_native_ffi_style_image_options_get_text_fit_width: (get-property text-fit-width)
 *
 * Gets `MaplibreNativeFfiStyleImageOptions:text-fit-width`.
 *
 * Returns: the value
 */
MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_options_get_text_fit_width(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_text_fit_width:
 *
 * Whether `MaplibreNativeFfiStyleImageOptions:text-fit-width` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_text_fit_width(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_text_fit_width: (set-property text-fit-width)
 *
 * Sets `MaplibreNativeFfiStyleImageOptions:text-fit-width`.
 */
void maplibre_native_ffi_style_image_options_set_text_fit_width(
  MaplibreNativeFfiStyleImageOptions* self,
  MaplibreNativeFfiStyleImageTextFit value
);

/**
 * MaplibreNativeFfiStyleImageOptions:text-fit-height: (setter set_text_fit_height) (getter get_text_fit_height)
 *
 * The `text_fit_height` member.
 */
/**
 * maplibre_native_ffi_style_image_options_get_text_fit_height: (get-property text-fit-height)
 *
 * Gets `MaplibreNativeFfiStyleImageOptions:text-fit-height`.
 *
 * Returns: the value
 */
MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_options_get_text_fit_height(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_text_fit_height:
 *
 * Whether `MaplibreNativeFfiStyleImageOptions:text-fit-height` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_text_fit_height(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_text_fit_height: (set-property text-fit-height)
 *
 * Sets `MaplibreNativeFfiStyleImageOptions:text-fit-height`.
 */
void maplibre_native_ffi_style_image_options_set_text_fit_height(
  MaplibreNativeFfiStyleImageOptions* self,
  MaplibreNativeFfiStyleImageTextFit value
);

/**
 * MaplibreNativeFfiStyleImageOptions:pixel-ratio: (setter set_pixel_ratio) (getter get_pixel_ratio)
 *
 * The `pixel_ratio` member.
 */
/**
 * maplibre_native_ffi_style_image_options_get_pixel_ratio: (get-property pixel-ratio)
 *
 * Gets `MaplibreNativeFfiStyleImageOptions:pixel-ratio`.
 *
 * Returns: the value
 */
gfloat maplibre_native_ffi_style_image_options_get_pixel_ratio(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_pixel_ratio:
 *
 * Whether `MaplibreNativeFfiStyleImageOptions:pixel-ratio` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_pixel_ratio(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_pixel_ratio: (set-property pixel-ratio)
 *
 * Sets `MaplibreNativeFfiStyleImageOptions:pixel-ratio`.
 */
void maplibre_native_ffi_style_image_options_set_pixel_ratio(
  MaplibreNativeFfiStyleImageOptions* self, gfloat value
);

/**
 * MaplibreNativeFfiStyleImageOptions:sdf: (setter set_sdf) (getter get_sdf)
 *
 * The `sdf` member.
 */
/**
 * maplibre_native_ffi_style_image_options_get_sdf: (get-property sdf)
 *
 * Gets `MaplibreNativeFfiStyleImageOptions:sdf`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_style_image_options_get_sdf(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_sdf:
 *
 * Whether `MaplibreNativeFfiStyleImageOptions:sdf` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_sdf(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_sdf: (set-property sdf)
 *
 * Sets `MaplibreNativeFfiStyleImageOptions:sdf`.
 */
void maplibre_native_ffi_style_image_options_set_sdf(
  MaplibreNativeFfiStyleImageOptions* self, gboolean value
);

/**
 * MaplibreNativeFfiStyleImageOptions:keep-unused: (setter set_keep_unused) (getter get_keep_unused)
 *
 * Keeps the image after no layer uses it. Without this, the map can remove an
 * image that it asked for through
 * `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING`.
 */
/**
 * maplibre_native_ffi_style_image_options_get_keep_unused: (get-property keep-unused)
 *
 * Gets `MaplibreNativeFfiStyleImageOptions:keep-unused`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_style_image_options_get_keep_unused(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_keep_unused:
 *
 * Whether `MaplibreNativeFfiStyleImageOptions:keep-unused` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_keep_unused(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_keep_unused: (set-property keep-unused)
 *
 * Sets `MaplibreNativeFfiStyleImageOptions:keep-unused`.
 */
void maplibre_native_ffi_style_image_options_set_keep_unused(
  MaplibreNativeFfiStyleImageOptions* self, gboolean value
);

/**
 * MaplibreNativeFfiStyleLayerInfo:
 *
 * A layer in the map's style.
 *
 * See `mln_style_layer_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
GType maplibre_native_ffi_style_layer_info_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_LAYER_INFO \
  (maplibre_native_ffi_style_layer_info_get_type())

/**
 * maplibre_native_ffi_style_layer_info_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleLayerInfo` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleLayerInfo* maplibre_native_ffi_style_layer_info_new(void);

/**
 * maplibre_native_ffi_style_layer_info_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiStyleLayerInfo* maplibre_native_ffi_style_layer_info_copy(
  const MaplibreNativeFfiStyleLayerInfo* self
);

/**
 * maplibre_native_ffi_style_layer_info_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_style_layer_info_free(
  MaplibreNativeFfiStyleLayerInfo* self
);

/**
 * maplibre_native_ffi_style_layer_info_get_id:
 *
 * The `id` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_layer_info_get_id(
  const MaplibreNativeFfiStyleLayerInfo* self
);

/**
 * maplibre_native_ffi_style_layer_info_get_layer_type:
 *
 * The layer's \[type\](https://maplibre.org/maplibre-style-spec/layers/#type),
 * such as `fill`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_layer_info_get_layer_type(
  const MaplibreNativeFfiStyleLayerInfo* self
);

/**
 * maplibre_native_ffi_style_layer_info_get_source_id:
 *
 * The `source_id` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_layer_info_get_source_id(
  const MaplibreNativeFfiStyleLayerInfo* self
);

/**
 * maplibre_native_ffi_style_layer_info_get_source_layer:
 *
 * The `source_layer` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_layer_info_get_source_layer(
  const MaplibreNativeFfiStyleLayerInfo* self
);

/**
 * maplibre_native_ffi_style_layer_info_get_min_zoom:
 *
 * The zoom level where the layer starts to show, or negative infinity when the
 * layer doesn't set one.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_layer_info_get_min_zoom(
  const MaplibreNativeFfiStyleLayerInfo* self
);

/**
 * maplibre_native_ffi_style_layer_info_get_max_zoom:
 *
 * The zoom level where the layer stops showing, or infinity when the layer
 * doesn't set one.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_layer_info_get_max_zoom(
  const MaplibreNativeFfiStyleLayerInfo* self
);

/**
 * maplibre_native_ffi_style_layer_info_get_visibility:
 *
 * The `visibility` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiStyleLayerVisibility
maplibre_native_ffi_style_layer_info_get_visibility(
  const MaplibreNativeFfiStyleLayerInfo* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiStyleLayerInfo, maplibre_native_ffi_style_layer_info_free
)

/**
 * MaplibreNativeFfiStyleSourceTileInfo:
 *
 * The tiles of a source, as the style lists them.
 *
 * See `mln_style_source_tile_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
GType maplibre_native_ffi_style_source_tile_info_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_SOURCE_TILE_INFO \
  (maplibre_native_ffi_style_source_tile_info_get_type())

/**
 * maplibre_native_ffi_style_source_tile_info_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleSourceTileInfo` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleSourceTileInfo*
maplibre_native_ffi_style_source_tile_info_new(void);

/**
 * maplibre_native_ffi_style_source_tile_info_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiStyleSourceTileInfo*
maplibre_native_ffi_style_source_tile_info_copy(
  const MaplibreNativeFfiStyleSourceTileInfo* self
);

/**
 * maplibre_native_ffi_style_source_tile_info_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_style_source_tile_info_free(
  MaplibreNativeFfiStyleSourceTileInfo* self
);

/**
 * maplibre_native_ffi_style_source_tile_info_get_tile_urls:
 *
 * The `tile_urls` member.
 *
 * Returns: (transfer none) (array zero-terminated=1): the texts
 */
const char* const* maplibre_native_ffi_style_source_tile_info_get_tile_urls(
  const MaplibreNativeFfiStyleSourceTileInfo* self
);

/**
 * maplibre_native_ffi_style_source_tile_info_get_min_zoom:
 *
 * The `min_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_source_tile_info_get_min_zoom(
  const MaplibreNativeFfiStyleSourceTileInfo* self
);

/**
 * maplibre_native_ffi_style_source_tile_info_get_max_zoom:
 *
 * The `max_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_source_tile_info_get_max_zoom(
  const MaplibreNativeFfiStyleSourceTileInfo* self
);

/**
 * maplibre_native_ffi_style_source_tile_info_get_scheme:
 *
 * The `scheme` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiStyleTileScheme
maplibre_native_ffi_style_source_tile_info_get_scheme(
  const MaplibreNativeFfiStyleSourceTileInfo* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiStyleSourceTileInfo,
  maplibre_native_ffi_style_source_tile_info_free
)

/**
 * MaplibreNativeFfiStyleSourceInfo:
 *
 * A source in the map's style.
 *
 * See `mln_style_source_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
GType maplibre_native_ffi_style_source_info_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_SOURCE_INFO \
  (maplibre_native_ffi_style_source_info_get_type())

/**
 * maplibre_native_ffi_style_source_info_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleSourceInfo` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleSourceInfo* maplibre_native_ffi_style_source_info_new(
  void
);

/**
 * maplibre_native_ffi_style_source_info_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiStyleSourceInfo* maplibre_native_ffi_style_source_info_copy(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_style_source_info_free(
  MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_get_id:
 *
 * The `id` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_source_info_get_id(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_get_source_type:
 *
 * The `type` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiStyleSourceType
maplibre_native_ffi_style_source_info_get_source_type(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_get_is_volatile:
 *
 * The `is_volatile` member.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_style_source_info_get_is_volatile(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_get_attribution:
 *
 * The `attribution` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_source_info_get_attribution(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_get_url:
 *
 * The `url` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_source_info_get_url(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_get_tile_json:
 *
 * The tiles that the style lists in the source itself, instead of at a URL.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiStyleSourceTileInfo*
maplibre_native_ffi_style_source_info_get_tile_json(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_has_tile_json:
 *
 * Whether the value has `maplibre_native_ffi_style_source_info_get_tile_json`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_source_info_has_tile_json(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_get_bounds:
 *
 * The `bounds` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLatLngBounds* maplibre_native_ffi_style_source_info_get_bounds(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_has_bounds:
 *
 * Whether the value has `maplibre_native_ffi_style_source_info_get_bounds`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_source_info_has_bounds(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_get_tile_size:
 *
 * The `tile_size` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_style_source_info_get_tile_size(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_has_tile_size:
 *
 * Whether the value has `maplibre_native_ffi_style_source_info_get_tile_size`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_source_info_has_tile_size(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_get_vector_encoding:
 *
 * The `vector_encoding` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiStyleVectorTileEncoding
maplibre_native_ffi_style_source_info_get_vector_encoding(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_has_vector_encoding:
 *
 * Whether the value has `maplibre_native_ffi_style_source_info_get_vector_encoding`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_source_info_has_vector_encoding(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_get_raster_encoding:
 *
 * The `raster_encoding` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiStyleRasterDemEncoding
maplibre_native_ffi_style_source_info_get_raster_encoding(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_has_raster_encoding:
 *
 * Whether the value has `maplibre_native_ffi_style_source_info_get_raster_encoding`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_source_info_has_raster_encoding(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_get_geo_json:
 *
 * A GeoJSON source's options, with every field set.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_style_source_info_get_geo_json(
  const MaplibreNativeFfiStyleSourceInfo* self
);

/**
 * maplibre_native_ffi_style_source_info_has_geo_json:
 *
 * Whether the value has `maplibre_native_ffi_style_source_info_get_geo_json`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_source_info_has_geo_json(
  const MaplibreNativeFfiStyleSourceInfo* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiStyleSourceInfo, maplibre_native_ffi_style_source_info_free
)

/**
 * MaplibreNativeFfiStyleTransitionOptions:
 *
 * The default
 * \[transition\](https://maplibre.org/maplibre-style-spec/transition/) for
 * changes to a style's paint properties.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_style_transition_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
/**
 * maplibre_native_ffi_style_transition_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleTransitionOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleTransitionOptions*
maplibre_native_ffi_style_transition_options_new(void);

/**
 * MaplibreNativeFfiStyleTransitionOptions:duration-ms: (setter set_duration_ms) (getter get_duration_ms)
 *
 * The `duration_ms` member.
 */
/**
 * maplibre_native_ffi_style_transition_options_get_duration_ms: (get-property duration-ms)
 *
 * Gets `MaplibreNativeFfiStyleTransitionOptions:duration-ms`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_transition_options_get_duration_ms(
  MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_has_duration_ms:
 *
 * Whether `MaplibreNativeFfiStyleTransitionOptions:duration-ms` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_transition_options_has_duration_ms(
  MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_set_duration_ms: (set-property duration-ms)
 *
 * Sets `MaplibreNativeFfiStyleTransitionOptions:duration-ms`.
 */
void maplibre_native_ffi_style_transition_options_set_duration_ms(
  MaplibreNativeFfiStyleTransitionOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiStyleTransitionOptions:delay-ms: (setter set_delay_ms) (getter get_delay_ms)
 *
 * The `delay_ms` member.
 */
/**
 * maplibre_native_ffi_style_transition_options_get_delay_ms: (get-property delay-ms)
 *
 * Gets `MaplibreNativeFfiStyleTransitionOptions:delay-ms`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_transition_options_get_delay_ms(
  MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_has_delay_ms:
 *
 * Whether `MaplibreNativeFfiStyleTransitionOptions:delay-ms` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_transition_options_has_delay_ms(
  MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_set_delay_ms: (set-property delay-ms)
 *
 * Sets `MaplibreNativeFfiStyleTransitionOptions:delay-ms`.
 */
void maplibre_native_ffi_style_transition_options_set_delay_ms(
  MaplibreNativeFfiStyleTransitionOptions* self, gdouble value
);

/**
 * MaplibreNativeFfiStyleTransitionOptions:enable-placement-transitions: (setter set_enable_placement_transitions) (getter get_enable_placement_transitions)
 *
 * Whether symbols fade in and out as the map places them.
 */
/**
 * maplibre_native_ffi_style_transition_options_get_enable_placement_transitions: (get-property enable-placement-transitions)
 *
 * Gets `MaplibreNativeFfiStyleTransitionOptions:enable-placement-transitions`.
 *
 * Returns: the value
 */
gboolean
maplibre_native_ffi_style_transition_options_get_enable_placement_transitions(
  MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_has_enable_placement_transitions:
 *
 * Whether `MaplibreNativeFfiStyleTransitionOptions:enable-placement-transitions` is set.
 *
 * Returns: whether the member is set
 */
gboolean
maplibre_native_ffi_style_transition_options_has_enable_placement_transitions(
  MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_set_enable_placement_transitions: (set-property enable-placement-transitions)
 *
 * Sets `MaplibreNativeFfiStyleTransitionOptions:enable-placement-transitions`.
 */
void maplibre_native_ffi_style_transition_options_set_enable_placement_transitions(
  MaplibreNativeFfiStyleTransitionOptions* self, gboolean value
);

GType maplibre_native_ffi_texture_image_info_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_TEXTURE_IMAGE_INFO \
  (maplibre_native_ffi_texture_image_info_get_type())

/**
 * maplibre_native_ffi_texture_image_info_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiTextureImageInfo* maplibre_native_ffi_texture_image_info_copy(
  const MaplibreNativeFfiTextureImageInfo* self
);

/**
 * maplibre_native_ffi_texture_image_info_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_texture_image_info_free(
  MaplibreNativeFfiTextureImageInfo* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiTextureImageInfo, maplibre_native_ffi_texture_image_info_free
)

/**
 * MaplibreNativeFfiTextureReadbackResult:
 *
 * The pixels that `maplibre_native_ffi_render_session_read_texture_async()`
 * reads.
 *
 * See `mln_texture_readback_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
GType maplibre_native_ffi_texture_readback_result_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_TEXTURE_READBACK_RESULT \
  (maplibre_native_ffi_texture_readback_result_get_type())

/**
 * maplibre_native_ffi_texture_readback_result_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiTextureReadbackResult` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiTextureReadbackResult*
maplibre_native_ffi_texture_readback_result_new(void);

/**
 * maplibre_native_ffi_texture_readback_result_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiTextureReadbackResult*
maplibre_native_ffi_texture_readback_result_copy(
  const MaplibreNativeFfiTextureReadbackResult* self
);

/**
 * maplibre_native_ffi_texture_readback_result_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_texture_readback_result_free(
  MaplibreNativeFfiTextureReadbackResult* self
);

/**
 * maplibre_native_ffi_texture_readback_result_get_data:
 *
 * Premultiplied RGBA pixels with 8 bits per channel, in rows from top to
 * bottom.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_texture_readback_result_get_data(
  const MaplibreNativeFfiTextureReadbackResult* self
);

/**
 * maplibre_native_ffi_texture_readback_result_get_info:
 *
 * The `info` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiTextureImageInfo*
maplibre_native_ffi_texture_readback_result_get_info(
  const MaplibreNativeFfiTextureReadbackResult* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiTextureReadbackResult,
  maplibre_native_ffi_texture_readback_result_free
)

GType maplibre_native_ffi_vulkan_borrowed_texture_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_BORROWED_TEXTURE \
  (maplibre_native_ffi_vulkan_borrowed_texture_get_type())

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiVulkanBorrowedTexture*
maplibre_native_ffi_vulkan_borrowed_texture_copy(
  const MaplibreNativeFfiVulkanBorrowedTexture* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_free(
  MaplibreNativeFfiVulkanBorrowedTexture* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiVulkanBorrowedTexture,
  maplibre_native_ffi_vulkan_borrowed_texture_free
)

/**
 * MaplibreNativeFfiVulkanContextDescriptor:
 *
 * The Vulkan device and queue that a render session uses.
 *
 * Keep these objects alive until a detach or release of the session completes,
 * or an abandon of it returns.
 *
 * See `mln_vulkan_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
/**
 * maplibre_native_ffi_vulkan_context_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiVulkanContextDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_context_descriptor_new(void);

/**
 * MaplibreNativeFfiVulkanContextDescriptor:instance: (setter set_instance) (getter get_instance)
 *
 * A `VkInstance`.
 */
/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_instance: (get-property instance)
 *
 * Gets `MaplibreNativeFfiVulkanContextDescriptor:instance`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_vulkan_context_descriptor_get_instance(
  MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_instance: (set-property instance)
 *
 * Sets `MaplibreNativeFfiVulkanContextDescriptor:instance`.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_instance(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiVulkanContextDescriptor:physical-device: (setter set_physical_device) (getter get_physical_device)
 *
 * A `VkPhysicalDevice`.
 */
/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_physical_device: (get-property physical-device)
 *
 * Gets `MaplibreNativeFfiVulkanContextDescriptor:physical-device`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_vulkan_context_descriptor_get_physical_device(
  MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_physical_device: (set-property physical-device)
 *
 * Sets `MaplibreNativeFfiVulkanContextDescriptor:physical-device`.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_physical_device(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiVulkanContextDescriptor:device: (setter set_device) (getter get_device)
 *
 * A `VkDevice`.
 */
/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_device: (get-property device)
 *
 * Gets `MaplibreNativeFfiVulkanContextDescriptor:device`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_vulkan_context_descriptor_get_device(
  MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_device: (set-property device)
 *
 * Sets `MaplibreNativeFfiVulkanContextDescriptor:device`.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_device(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiVulkanContextDescriptor:graphics-queue: (setter set_graphics_queue) (getter get_graphics_queue)
 *
 * A `VkQueue` from `graphics_queue_family_index` that supports graphics. If you
 * submit to it too, set
 * `MaplibreNativeFfiRenderSessionAttachOptions:queue-lock`.
 */
/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue: (get-property graphics-queue)
 *
 * Gets `MaplibreNativeFfiVulkanContextDescriptor:graphics-queue`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue(
  MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue: (set-property graphics-queue)
 *
 * Sets `MaplibreNativeFfiVulkanContextDescriptor:graphics-queue`.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiVulkanContextDescriptor:graphics-queue-family-index: (setter set_graphics_queue_family_index) (getter get_graphics_queue_family_index)
 *
 * The `graphics_queue_family_index` member.
 */
/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue_family_index: (get-property graphics-queue-family-index)
 *
 * Gets `MaplibreNativeFfiVulkanContextDescriptor:graphics-queue-family-index`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue_family_index(
  MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue_family_index: (set-property graphics-queue-family-index)
 *
 * Sets `MaplibreNativeFfiVulkanContextDescriptor:graphics-queue-family-index`.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue_family_index(
  MaplibreNativeFfiVulkanContextDescriptor* self, guint32 value
);

/**
 * MaplibreNativeFfiVulkanContextDescriptor:get-instance-proc-addr: (setter set_get_instance_proc_addr) (getter get_get_instance_proc_addr)
 *
 * Your `vkGetInstanceProcAddr`, or null to use the Vulkan loader's.
 */
/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_get_instance_proc_addr: (get-property get-instance-proc-addr)
 *
 * Gets `MaplibreNativeFfiVulkanContextDescriptor:get-instance-proc-addr`.
 *
 * Returns: the value
 */
gpointer
maplibre_native_ffi_vulkan_context_descriptor_get_get_instance_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_get_instance_proc_addr: (set-property get-instance-proc-addr)
 *
 * Sets `MaplibreNativeFfiVulkanContextDescriptor:get-instance-proc-addr`.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_get_instance_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiVulkanContextDescriptor:get-device-proc-addr: (setter set_get_device_proc_addr) (getter get_get_device_proc_addr)
 *
 * Your `vkGetDeviceProcAddr`, or null to look it up from the instance.
 */
/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_get_device_proc_addr: (get-property get-device-proc-addr)
 *
 * Gets `MaplibreNativeFfiVulkanContextDescriptor:get-device-proc-addr`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_vulkan_context_descriptor_get_get_device_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_get_device_proc_addr: (set-property get-device-proc-addr)
 *
 * Sets `MaplibreNativeFfiVulkanContextDescriptor:get-device-proc-addr`.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_get_device_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureDescriptor:
 *
 * A ring of your Vulkan images, for
 * `maplibre_native_ffi_map_attach_vulkan_borrowed_texture()`.
 *
 * `logical_size` sets the map's size. `physical_width` and `physical_height`
 * give the images' size in pixels, which is usually `logical_size` times the
 * map's scale factor. Each image must have that size, `format`, and
 * VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, and belong to the context's device. The
 * ring holds up to `MAPLIBRE_NATIVE_FFI_TEXTURE_RING_MAX_DEPTH` distinct
 * images.
 *
 * The session uses the images until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_vulkan_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiVulkanBorrowedTextureDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiVulkanBorrowedTextureDescriptor*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_new(void);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureDescriptor:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:logical-size`.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureDescriptor:physical-width: (setter set_physical_width) (getter get_physical_width)
 *
 * The `physical_width` member.
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_width: (get-property physical-width)
 *
 * Gets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:physical-width`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_width: (set-property physical-width)
 *
 * Sets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:physical-width`.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureDescriptor:physical-height: (setter set_physical_height) (getter get_physical_height)
 *
 * The `physical_height` member.
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_height: (get-property physical-height)
 *
 * Gets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:physical-height`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_height: (set-property physical-height)
 *
 * Sets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:physical-height`.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureDescriptor:context: (setter set_context) (getter get_context)
 *
 * The `context` member.
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_context: (get-property context)
 *
 * Gets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:context`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_context(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_context: (set-property context)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:context`.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_context(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  MaplibreNativeFfiVulkanContextDescriptor* value
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_textures:
 *
 * Gets the value that `maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_textures` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiVulkanBorrowedTexture): the items
 */
GPtrArray* maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_textures(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_textures:
 * @value: (array length=value_count): the items
 *
 * The `textures` member.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_textures(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiVulkanBorrowedTexture* value, gsize value_count
);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureDescriptor:format: (setter set_format) (getter get_format)
 *
 * The images' VkFormat.
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_format: (get-property format)
 *
 * Gets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:format`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_format(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_format: (set-property format)
 *
 * Sets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:format`.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_format(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureDescriptor:final-layout: (setter set_final_layout) (getter get_final_layout)
 *
 * The VkImageLayout that each frame leaves its image in.
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_final_layout: (get-property final-layout)
 *
 * Gets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:final-layout`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_final_layout(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_final_layout: (set-property final-layout)
 *
 * Sets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:final-layout`.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_final_layout(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureDescriptor:owner: (setter set_owner) (getter get_owner)
 *
 * The `owner` member.
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_owner: (get-property owner)
 *
 * Gets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:owner`.
 *
 * Returns: the value
 */
MaplibreNativeFfiVulkanImageOwner
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_owner(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_owner: (set-property owner)
 *
 * Sets `MaplibreNativeFfiVulkanBorrowedTextureDescriptor:owner`.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_owner(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  MaplibreNativeFfiVulkanImageOwner value
);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureReplacement:
 *
 * New Vulkan images for a borrowed ring, for
 * `maplibre_native_ffi_render_session_set_vulkan_borrowed_texture_target_async()`.
 *
 * The members and the images follow the rules of
 * `MaplibreNativeFfiVulkanBorrowedTextureDescriptor`. The ring keeps its depth,
 * context, `format`, `final_layout`, and `owner`, so give it as many images as
 * it attached with, in that format, on the same device.
 *
 * See `mln_vulkan_borrowed_texture_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiVulkanBorrowedTextureReplacement` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiVulkanBorrowedTextureReplacement*
maplibre_native_ffi_vulkan_borrowed_texture_replacement_new(void);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureReplacement:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiVulkanBorrowedTextureReplacement:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiVulkanBorrowedTextureReplacement:logical-size`.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureReplacement:physical-width: (setter set_physical_width) (getter get_physical_width)
 *
 * The `physical_width` member.
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_width: (get-property physical-width)
 *
 * Gets `MaplibreNativeFfiVulkanBorrowedTextureReplacement:physical-width`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_width: (set-property physical-width)
 *
 * Sets `MaplibreNativeFfiVulkanBorrowedTextureReplacement:physical-width`.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self, guint32 value
);

/**
 * MaplibreNativeFfiVulkanBorrowedTextureReplacement:physical-height: (setter set_physical_height) (getter get_physical_height)
 *
 * The `physical_height` member.
 */
/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_height: (get-property physical-height)
 *
 * Gets `MaplibreNativeFfiVulkanBorrowedTextureReplacement:physical-height`.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_height: (set-property physical-height)
 *
 * Sets `MaplibreNativeFfiVulkanBorrowedTextureReplacement:physical-height`.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self, guint32 value
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_textures:
 *
 * Gets the value that `maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_textures` sets.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiVulkanBorrowedTexture): the items
 */
GPtrArray* maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_textures(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_textures:
 * @value: (array length=value_count): the items
 *
 * The `textures` member.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_textures(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self,
  const MaplibreNativeFfiVulkanBorrowedTexture* value, gsize value_count
);

/**
 * MaplibreNativeFfiVulkanDeviceContextResult:
 *
 * A Vulkan device's context, as
 * `maplibre_native_ffi_vulkan_device_get_context()` reports it.
 *
 * See `mln_vulkan_device_context_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
 */
GType maplibre_native_ffi_vulkan_device_context_result_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_DEVICE_CONTEXT_RESULT \
  (maplibre_native_ffi_vulkan_device_context_result_get_type())

/**
 * maplibre_native_ffi_vulkan_device_context_result_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiVulkanDeviceContextResult` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiVulkanDeviceContextResult*
maplibre_native_ffi_vulkan_device_context_result_new(void);

/**
 * maplibre_native_ffi_vulkan_device_context_result_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiVulkanDeviceContextResult*
maplibre_native_ffi_vulkan_device_context_result_copy(
  const MaplibreNativeFfiVulkanDeviceContextResult* self
);

/**
 * maplibre_native_ffi_vulkan_device_context_result_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_vulkan_device_context_result_free(
  MaplibreNativeFfiVulkanDeviceContextResult* self
);

/**
 * maplibre_native_ffi_vulkan_device_context_result_get_context:
 *
 * The `context` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_device_context_result_get_context(
  const MaplibreNativeFfiVulkanDeviceContextResult* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiVulkanDeviceContextResult,
  maplibre_native_ffi_vulkan_device_context_result_free
)

/**
 * MaplibreNativeFfiVulkanDeviceOptions:
 *
 * Options for `maplibre_native_ffi_vulkan_device_create()`.
 *
 * A member that a `has_` method reports on is unset until its property or
 * setter sets it.
 *
 * See `mln_vulkan_device_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
 */
/**
 * maplibre_native_ffi_vulkan_device_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiVulkanDeviceOptions` whose members are zero, empty, or unset.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiVulkanDeviceOptions*
maplibre_native_ffi_vulkan_device_options_new(void);

/**
 * MaplibreNativeFfiVulkanDeviceOptions:presentation: (setter set_presentation) (getter get_presentation)
 *
 * Whether the device can present to a window. The instance then enables
 * VK_KHR_surface and the platform's surface extensions, and the device enables
 * VK_KHR_swapchain. Defaults to false.
 */
/**
 * maplibre_native_ffi_vulkan_device_options_get_presentation: (get-property presentation)
 *
 * Gets `MaplibreNativeFfiVulkanDeviceOptions:presentation`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_vulkan_device_options_get_presentation(
  MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_has_presentation:
 *
 * Whether `MaplibreNativeFfiVulkanDeviceOptions:presentation` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_vulkan_device_options_has_presentation(
  MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_set_presentation: (set-property presentation)
 *
 * Sets `MaplibreNativeFfiVulkanDeviceOptions:presentation`.
 */
void maplibre_native_ffi_vulkan_device_options_set_presentation(
  MaplibreNativeFfiVulkanDeviceOptions* self, gboolean value
);

/**
 * MaplibreNativeFfiVulkanDeviceOptions:device-uuid: (setter set_device_uuid) (getter get_device_uuid)
 *
 * The 16-byte `deviceUUID` of the GPU to use, as `VkPhysicalDeviceIDProperties`
 * reports it.
 */
/**
 * maplibre_native_ffi_vulkan_device_options_get_device_uuid: (get-property device-uuid)
 *
 * Gets `MaplibreNativeFfiVulkanDeviceOptions:device-uuid`.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_vulkan_device_options_get_device_uuid(
  MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_has_device_uuid:
 *
 * Whether `MaplibreNativeFfiVulkanDeviceOptions:device-uuid` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_vulkan_device_options_has_device_uuid(
  MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_set_device_uuid: (set-property device-uuid)
 * @value: (nullable): the bytes
 *
 * Sets `MaplibreNativeFfiVulkanDeviceOptions:device-uuid`.
 */
void maplibre_native_ffi_vulkan_device_options_set_device_uuid(
  MaplibreNativeFfiVulkanDeviceOptions* self, GBytes* value
);

/**
 * MaplibreNativeFfiVulkanDeviceOptions:device-luid: (setter set_device_luid) (getter get_device_luid)
 *
 * The 8-byte `deviceLUID` of the GPU to use, as `VkPhysicalDeviceIDProperties`
 * reports it. On Windows, this is the GPU's DXGI adapter LUID.
 */
/**
 * maplibre_native_ffi_vulkan_device_options_get_device_luid: (get-property device-luid)
 *
 * Gets `MaplibreNativeFfiVulkanDeviceOptions:device-luid`.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_vulkan_device_options_get_device_luid(
  MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_has_device_luid:
 *
 * Whether `MaplibreNativeFfiVulkanDeviceOptions:device-luid` is set.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_vulkan_device_options_has_device_luid(
  MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_set_device_luid: (set-property device-luid)
 * @value: (nullable): the bytes
 *
 * Sets `MaplibreNativeFfiVulkanDeviceOptions:device-luid`.
 */
void maplibre_native_ffi_vulkan_device_options_set_device_luid(
  MaplibreNativeFfiVulkanDeviceOptions* self, GBytes* value
);

/**
 * MaplibreNativeFfiVulkanSurfaceDescriptor:
 *
 * A Vulkan surface to present to, for
 * `maplibre_native_ffi_map_attach_vulkan_surface()`.
 *
 * The device must have VK_KHR_swapchain enabled, and the queue family of
 * `graphics_queue_family_index` must support presenting to the surface. The
 * session creates the swapchain. It uses the surface until a replacement or
 * release of the target completes, a detach or release of the session
 * completes, or an abandon of it returns.
 *
 * See `mln_vulkan_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
/**
 * maplibre_native_ffi_vulkan_surface_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiVulkanSurfaceDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiVulkanSurfaceDescriptor*
maplibre_native_ffi_vulkan_surface_descriptor_new(void);

/**
 * MaplibreNativeFfiVulkanSurfaceDescriptor:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_vulkan_surface_descriptor_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiVulkanSurfaceDescriptor:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_surface_descriptor_get_logical_size(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_surface_descriptor_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiVulkanSurfaceDescriptor:logical-size`.
 */
void maplibre_native_ffi_vulkan_surface_descriptor_set_logical_size(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiVulkanSurfaceDescriptor:context: (setter set_context) (getter get_context)
 *
 * The `context` member.
 */
/**
 * maplibre_native_ffi_vulkan_surface_descriptor_get_context: (get-property context)
 *
 * Gets `MaplibreNativeFfiVulkanSurfaceDescriptor:context`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_surface_descriptor_get_context(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_surface_descriptor_set_context: (set-property context)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiVulkanSurfaceDescriptor:context`.
 */
void maplibre_native_ffi_vulkan_surface_descriptor_set_context(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self,
  MaplibreNativeFfiVulkanContextDescriptor* value
);

/**
 * MaplibreNativeFfiVulkanSurfaceDescriptor:surface: (setter set_surface) (getter get_surface)
 *
 * Your VkSurfaceKHR.
 */
/**
 * maplibre_native_ffi_vulkan_surface_descriptor_get_surface: (get-property surface)
 *
 * Gets `MaplibreNativeFfiVulkanSurfaceDescriptor:surface`.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_vulkan_surface_descriptor_get_surface(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_surface_descriptor_set_surface: (set-property surface)
 *
 * Sets `MaplibreNativeFfiVulkanSurfaceDescriptor:surface`.
 */
void maplibre_native_ffi_vulkan_surface_descriptor_set_surface(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self, guint64 value
);

GType maplibre_native_ffi_vulkan_surface_replacement_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_SURFACE_REPLACEMENT \
  (maplibre_native_ffi_vulkan_surface_replacement_get_type())

/**
 * maplibre_native_ffi_vulkan_surface_replacement_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiVulkanSurfaceReplacement*
maplibre_native_ffi_vulkan_surface_replacement_copy(
  const MaplibreNativeFfiVulkanSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_vulkan_surface_replacement_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_vulkan_surface_replacement_free(
  MaplibreNativeFfiVulkanSurfaceReplacement* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiVulkanSurfaceReplacement,
  maplibre_native_ffi_vulkan_surface_replacement_free
)

/**
 * MaplibreNativeFfiVulkanTextureFrame:
 *
 * The Vulkan image of an acquired frame.
 *
 * See `mln_vulkan_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
GType maplibre_native_ffi_vulkan_texture_frame_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_TEXTURE_FRAME \
  (maplibre_native_ffi_vulkan_texture_frame_get_type())

/**
 * maplibre_native_ffi_vulkan_texture_frame_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiVulkanTextureFrame` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiVulkanTextureFrame*
maplibre_native_ffi_vulkan_texture_frame_new(void);

/**
 * maplibre_native_ffi_vulkan_texture_frame_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiVulkanTextureFrame*
maplibre_native_ffi_vulkan_texture_frame_copy(
  const MaplibreNativeFfiVulkanTextureFrame* self
);

/**
 * maplibre_native_ffi_vulkan_texture_frame_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_vulkan_texture_frame_free(
  MaplibreNativeFfiVulkanTextureFrame* self
);

/**
 * maplibre_native_ffi_vulkan_texture_frame_get_width:
 *
 * The `width` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_vulkan_texture_frame_get_width(
  const MaplibreNativeFfiVulkanTextureFrame* self
);

/**
 * maplibre_native_ffi_vulkan_texture_frame_get_height:
 *
 * The `height` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_vulkan_texture_frame_get_height(
  const MaplibreNativeFfiVulkanTextureFrame* self
);

/**
 * maplibre_native_ffi_vulkan_texture_frame_get_scale_factor:
 *
 * The `scale_factor` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_vulkan_texture_frame_get_scale_factor(
  const MaplibreNativeFfiVulkanTextureFrame* self
);

/**
 * maplibre_native_ffi_vulkan_texture_frame_get_slot:
 *
 * The image's index in the descriptor's `textures`.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_vulkan_texture_frame_get_slot(
  const MaplibreNativeFfiVulkanTextureFrame* self
);

/**
 * maplibre_native_ffi_vulkan_texture_frame_get_image:
 *
 * The VkImage.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_vulkan_texture_frame_get_image(
  const MaplibreNativeFfiVulkanTextureFrame* self
);

/**
 * maplibre_native_ffi_vulkan_texture_frame_get_image_view:
 *
 * The VkImageView.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_vulkan_texture_frame_get_image_view(
  const MaplibreNativeFfiVulkanTextureFrame* self
);

/**
 * maplibre_native_ffi_vulkan_texture_frame_get_device:
 *
 * The image's VkDevice.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_vulkan_texture_frame_get_device(
  const MaplibreNativeFfiVulkanTextureFrame* self
);

/**
 * maplibre_native_ffi_vulkan_texture_frame_get_format:
 *
 * The image's VkFormat.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_vulkan_texture_frame_get_format(
  const MaplibreNativeFfiVulkanTextureFrame* self
);

/**
 * maplibre_native_ffi_vulkan_texture_frame_get_layout:
 *
 * The VkImageLayout that the frame left the image in.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_vulkan_texture_frame_get_layout(
  const MaplibreNativeFfiVulkanTextureFrame* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiVulkanTextureFrame,
  maplibre_native_ffi_vulkan_texture_frame_free
)

/**
 * MaplibreNativeFfiWaylandSurfaceDescriptor:
 *
 * A Wayland surface to present to, for
 * `maplibre_native_ffi_map_attach_wayland_surface()`.
 *
 * See `mln_wayland_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).
 */
/**
 * maplibre_native_ffi_wayland_surface_descriptor_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiWaylandSurfaceDescriptor` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiWaylandSurfaceDescriptor*
maplibre_native_ffi_wayland_surface_descriptor_new(void);

/**
 * MaplibreNativeFfiWaylandSurfaceDescriptor:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_wayland_surface_descriptor_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiWaylandSurfaceDescriptor:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_wayland_surface_descriptor_get_logical_size(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_wayland_surface_descriptor_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiWaylandSurfaceDescriptor:logical-size`.
 */
void maplibre_native_ffi_wayland_surface_descriptor_set_logical_size(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiWaylandSurfaceDescriptor:display: (setter set_display) (getter get_display)
 *
 * Your `wl_display*`.
 */
/**
 * maplibre_native_ffi_wayland_surface_descriptor_get_display: (get-property display)
 *
 * Gets `MaplibreNativeFfiWaylandSurfaceDescriptor:display`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wayland_surface_descriptor_get_display(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_wayland_surface_descriptor_set_display: (set-property display)
 *
 * Sets `MaplibreNativeFfiWaylandSurfaceDescriptor:display`.
 */
void maplibre_native_ffi_wayland_surface_descriptor_set_display(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiWaylandSurfaceDescriptor:surface: (setter set_surface) (getter get_surface)
 *
 * Your `wl_surface*`, on `display`.
 */
/**
 * maplibre_native_ffi_wayland_surface_descriptor_get_surface: (get-property surface)
 *
 * Gets `MaplibreNativeFfiWaylandSurfaceDescriptor:surface`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wayland_surface_descriptor_get_surface(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_wayland_surface_descriptor_set_surface: (set-property surface)
 *
 * Sets `MaplibreNativeFfiWaylandSurfaceDescriptor:surface`.
 */
void maplibre_native_ffi_wayland_surface_descriptor_set_surface(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self, gpointer value
);

/**
 * MaplibreNativeFfiWaylandSurfaceReplacement:
 *
 * A new Wayland surface for
 * `maplibre_native_ffi_render_session_set_wayland_surface_target_async()`.
 *
 * See `mln_wayland_surface_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).
 */
/**
 * maplibre_native_ffi_wayland_surface_replacement_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiWaylandSurfaceReplacement` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiWaylandSurfaceReplacement*
maplibre_native_ffi_wayland_surface_replacement_new(void);

/**
 * MaplibreNativeFfiWaylandSurfaceReplacement:logical-size: (setter set_logical_size) (getter get_logical_size)
 *
 * The `logical_size` member.
 */
/**
 * maplibre_native_ffi_wayland_surface_replacement_get_logical_size: (get-property logical-size)
 *
 * Gets `MaplibreNativeFfiWaylandSurfaceReplacement:logical-size`.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_wayland_surface_replacement_get_logical_size(
  MaplibreNativeFfiWaylandSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_wayland_surface_replacement_set_logical_size: (set-property logical-size)
 * @value: the value
 *
 * Sets `MaplibreNativeFfiWaylandSurfaceReplacement:logical-size`.
 */
void maplibre_native_ffi_wayland_surface_replacement_set_logical_size(
  MaplibreNativeFfiWaylandSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * MaplibreNativeFfiWaylandSurfaceReplacement:surface: (setter set_surface) (getter get_surface)
 *
 * Your `wl_surface*`, on the display that the session attached with.
 */
/**
 * maplibre_native_ffi_wayland_surface_replacement_get_surface: (get-property surface)
 *
 * Gets `MaplibreNativeFfiWaylandSurfaceReplacement:surface`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wayland_surface_replacement_get_surface(
  MaplibreNativeFfiWaylandSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_wayland_surface_replacement_set_surface: (set-property surface)
 *
 * Sets `MaplibreNativeFfiWaylandSurfaceReplacement:surface`.
 */
void maplibre_native_ffi_wayland_surface_replacement_set_surface(
  MaplibreNativeFfiWaylandSurfaceReplacement* self, gpointer value
);

G_END_DECLS

#endif
