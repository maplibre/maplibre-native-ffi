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
typedef struct _MaplibreNativeFfiGeoJsonSourceData
  MaplibreNativeFfiGeoJsonSourceData;
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
typedef struct _MaplibreNativeFfiCameraTransitionHandler
  MaplibreNativeFfiCameraTransitionHandler;
typedef struct _MaplibreNativeFfiAnimationOptions
  MaplibreNativeFfiAnimationOptions;
typedef struct _MaplibreNativeFfiBoundOptions MaplibreNativeFfiBoundOptions;
typedef struct _MaplibreNativeFfiCameraDelta MaplibreNativeFfiCameraDelta;
typedef struct _MaplibreNativeFfiCameraFitOptions
  MaplibreNativeFfiCameraFitOptions;
typedef struct _MaplibreNativeFfiCameraFitUpdate
  MaplibreNativeFfiCameraFitUpdate;
typedef struct _MaplibreNativeFfiCameraOptions MaplibreNativeFfiCameraOptions;
typedef struct _MaplibreNativeFfiCameraUpdate MaplibreNativeFfiCameraUpdate;
typedef struct _MaplibreNativeFfiCustomGeometrySourceOptions
  MaplibreNativeFfiCustomGeometrySourceOptions;
typedef struct _MaplibreNativeFfiCustomVectorSourceOptions
  MaplibreNativeFfiCustomVectorSourceOptions;
typedef struct _MaplibreNativeFfiDmaBufFrame MaplibreNativeFfiDmaBufFrame;
typedef struct _MaplibreNativeFfiDmaBufRingDescriptor
  MaplibreNativeFfiDmaBufRingDescriptor;
typedef struct _MaplibreNativeFfiEglContextDescriptor
  MaplibreNativeFfiEglContextDescriptor;
typedef struct _MaplibreNativeFfiFeatureId MaplibreNativeFfiFeatureId;
typedef struct _MaplibreNativeFfiFeatureStateSelector
  MaplibreNativeFfiFeatureStateSelector;
typedef struct _MaplibreNativeFfiFramePacerOptions
  MaplibreNativeFfiFramePacerOptions;
typedef struct _MaplibreNativeFfiFreeCameraOptions
  MaplibreNativeFfiFreeCameraOptions;
typedef struct _MaplibreNativeFfiGeoJsonSourceOptions
  MaplibreNativeFfiGeoJsonSourceOptions;
typedef struct _MaplibreNativeFfiHttpHeader MaplibreNativeFfiHttpHeader;
typedef struct _MaplibreNativeFfiHttpHeaderRule MaplibreNativeFfiHttpHeaderRule;
typedef struct _MaplibreNativeFfiHttpHeaderTransform
  MaplibreNativeFfiHttpHeaderTransform;
typedef struct _MaplibreNativeFfiIoSurfaceFrame MaplibreNativeFfiIoSurfaceFrame;
typedef struct _MaplibreNativeFfiMetalContextDescriptor
  MaplibreNativeFfiMetalContextDescriptor;
typedef struct _MaplibreNativeFfiIoSurfaceRingDescriptor
  MaplibreNativeFfiIoSurfaceRingDescriptor;
typedef struct _MaplibreNativeFfiLogHandler MaplibreNativeFfiLogHandler;
typedef struct _MaplibreNativeFfiMapOptions MaplibreNativeFfiMapOptions;
typedef struct _MaplibreNativeFfiMapTileOptions MaplibreNativeFfiMapTileOptions;
typedef struct _MaplibreNativeFfiMapViewportOptions
  MaplibreNativeFfiMapViewportOptions;
typedef struct _MaplibreNativeFfiMetalBorrowedTexture
  MaplibreNativeFfiMetalBorrowedTexture;
typedef struct _MaplibreNativeFfiMetalBorrowedTextureDescriptor
  MaplibreNativeFfiMetalBorrowedTextureDescriptor;
typedef struct _MaplibreNativeFfiMetalBorrowedTextureReplacement
  MaplibreNativeFfiMetalBorrowedTextureReplacement;
typedef struct _MaplibreNativeFfiMetalSurfaceDescriptor
  MaplibreNativeFfiMetalSurfaceDescriptor;
typedef struct _MaplibreNativeFfiMetalSurfaceReplacement
  MaplibreNativeFfiMetalSurfaceReplacement;
typedef struct _MaplibreNativeFfiMetalTextureFrame
  MaplibreNativeFfiMetalTextureFrame;
typedef struct _MaplibreNativeFfiOfflineGeometryRegionDefinition
  MaplibreNativeFfiOfflineGeometryRegionDefinition;
typedef struct _MaplibreNativeFfiOfflineTilePyramidRegionDefinition
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition;
typedef struct _MaplibreNativeFfiOfflineRegionDefinition
  MaplibreNativeFfiOfflineRegionDefinition;
typedef struct _MaplibreNativeFfiOfflineRegionInfo
  MaplibreNativeFfiOfflineRegionInfo;
typedef struct _MaplibreNativeFfiOffscreenDeviceOptions
  MaplibreNativeFfiOffscreenDeviceOptions;
typedef struct _MaplibreNativeFfiWglContextDescriptor
  MaplibreNativeFfiWglContextDescriptor;
typedef struct _MaplibreNativeFfiWebGlContextDescriptor
  MaplibreNativeFfiWebGlContextDescriptor;
typedef struct _MaplibreNativeFfiOpenGlContextDescriptorData
  MaplibreNativeFfiOpenGlContextDescriptorData;
typedef struct _MaplibreNativeFfiOpenGlContextDescriptor
  MaplibreNativeFfiOpenGlContextDescriptor;
typedef struct _MaplibreNativeFfiOpenGlBorrowedTextureDescriptor
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor;
typedef struct _MaplibreNativeFfiOpenGlBorrowedTextureReplacement
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement;
typedef struct _MaplibreNativeFfiOpenGlSurfaceDescriptor
  MaplibreNativeFfiOpenGlSurfaceDescriptor;
typedef struct _MaplibreNativeFfiOpenGlSurfaceReplacement
  MaplibreNativeFfiOpenGlSurfaceReplacement;
typedef struct _MaplibreNativeFfiPremultipliedRgba8Image
  MaplibreNativeFfiPremultipliedRgba8Image;
typedef struct _MaplibreNativeFfiProjectionModeOptions
  MaplibreNativeFfiProjectionModeOptions;
typedef struct _MaplibreNativeFfiQueueLock MaplibreNativeFfiQueueLock;
typedef struct _MaplibreNativeFfiWake MaplibreNativeFfiWake;
typedef struct _MaplibreNativeFfiRenderSessionAttachOptions
  MaplibreNativeFfiRenderSessionAttachOptions;
typedef struct _MaplibreNativeFfiRenderedFeature
  MaplibreNativeFfiRenderedFeature;
typedef struct _MaplibreNativeFfiRenderedFeatureQueryOptions
  MaplibreNativeFfiRenderedFeatureQueryOptions;
typedef struct _MaplibreNativeFfiScreenLineString
  MaplibreNativeFfiScreenLineString;
typedef struct _MaplibreNativeFfiRenderedQueryGeometry
  MaplibreNativeFfiRenderedQueryGeometry;
typedef struct _MaplibreNativeFfiResourceRoute MaplibreNativeFfiResourceRoute;
typedef struct _MaplibreNativeFfiResourceProvider
  MaplibreNativeFfiResourceProvider;
typedef struct _MaplibreNativeFfiResourceRequestCancelHandler
  MaplibreNativeFfiResourceRequestCancelHandler;
typedef struct _MaplibreNativeFfiResourceRequestInfo
  MaplibreNativeFfiResourceRequestInfo;
typedef struct _MaplibreNativeFfiResourceResponse
  MaplibreNativeFfiResourceResponse;
typedef struct _MaplibreNativeFfiUrlQueryParameter
  MaplibreNativeFfiUrlQueryParameter;
typedef struct _MaplibreNativeFfiResourceTransformRule
  MaplibreNativeFfiResourceTransformRule;
typedef struct _MaplibreNativeFfiResourceTransform
  MaplibreNativeFfiResourceTransform;
typedef struct _MaplibreNativeFfiResourceTransformError
  MaplibreNativeFfiResourceTransformError;
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
typedef struct _MaplibreNativeFfiRuntimeOptions MaplibreNativeFfiRuntimeOptions;
typedef struct _MaplibreNativeFfiSourceFeature MaplibreNativeFfiSourceFeature;
typedef struct _MaplibreNativeFfiSourceFeatureQueryOptions
  MaplibreNativeFfiSourceFeatureQueryOptions;
typedef struct _MaplibreNativeFfiStyleExpressionInputs
  MaplibreNativeFfiStyleExpressionInputs;
typedef struct _MaplibreNativeFfiStyleExpressionOptions
  MaplibreNativeFfiStyleExpressionOptions;
typedef struct _MaplibreNativeFfiStyleImageInfo MaplibreNativeFfiStyleImageInfo;
typedef struct _MaplibreNativeFfiStyleImageOptions
  MaplibreNativeFfiStyleImageOptions;
typedef struct _MaplibreNativeFfiStyleLayerInfo MaplibreNativeFfiStyleLayerInfo;
typedef struct _MaplibreNativeFfiStyleSourceTileInfo
  MaplibreNativeFfiStyleSourceTileInfo;
typedef struct _MaplibreNativeFfiStyleSourceInfo
  MaplibreNativeFfiStyleSourceInfo;
typedef struct _MaplibreNativeFfiStyleTransitionOptions
  MaplibreNativeFfiStyleTransitionOptions;
typedef struct _MaplibreNativeFfiTextureReadbackResult
  MaplibreNativeFfiTextureReadbackResult;
typedef struct _MaplibreNativeFfiVulkanContextDescriptor
  MaplibreNativeFfiVulkanContextDescriptor;
typedef struct _MaplibreNativeFfiVulkanBorrowedTextureDescriptor
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor;
typedef struct _MaplibreNativeFfiVulkanBorrowedTextureReplacement
  MaplibreNativeFfiVulkanBorrowedTextureReplacement;
typedef struct _MaplibreNativeFfiVulkanDeviceContextResult
  MaplibreNativeFfiVulkanDeviceContextResult;
typedef struct _MaplibreNativeFfiVulkanDeviceOptions
  MaplibreNativeFfiVulkanDeviceOptions;
typedef struct _MaplibreNativeFfiVulkanSurfaceDescriptor
  MaplibreNativeFfiVulkanSurfaceDescriptor;
typedef struct _MaplibreNativeFfiVulkanTextureFrame
  MaplibreNativeFfiVulkanTextureFrame;
typedef struct _MaplibreNativeFfiWaylandSurfaceDescriptor
  MaplibreNativeFfiWaylandSurfaceDescriptor;
typedef struct _MaplibreNativeFfiWaylandSurfaceReplacement
  MaplibreNativeFfiWaylandSurfaceReplacement;

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
 * MaplibreNativeFfiCameraTransitionEnd:
 * @outcome: outcome
 * @generation: The generation of the first map snapshot that shows where the
 *   change left the camera, or 0 when the map closed first.
 *
 * The end of a camera change.
 *
 * See `mln_camera_transition_end` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef struct {
  MaplibreNativeFfiCameraTransitionOutcome outcome;
  guint64 generation;
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
 *
 * The details of `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE` and
 * `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE`.
 *
 * See `mln_runtime_event_camera_change` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct {
  MaplibreNativeFfiCameraChangeMode mode;
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
 * See `mln_animation_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_animation_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_ANIMATION_OPTIONS \
  (maplibre_native_ffi_animation_options_get_type())

/**
 * maplibre_native_ffi_animation_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiAnimationOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiAnimationOptions* maplibre_native_ffi_animation_options_new(
  void
);

/**
 * maplibre_native_ffi_animation_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiAnimationOptions* maplibre_native_ffi_animation_options_copy(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_animation_options_free(
  MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_get_duration_ms:
 *
 * The `duration_ms` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_animation_options_get_duration_ms(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_has_duration_ms:
 *
 * Whether the value sets `maplibre_native_ffi_animation_options_set_duration_ms`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_animation_options_has_duration_ms(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_duration_ms:
 *
 * The `duration_ms` member.
 */
void maplibre_native_ffi_animation_options_set_duration_ms(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_animation_options_get_velocity:
 *
 * The average speed of a fly animation, in screens per second. `duration_ms`
 * overrides it.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_animation_options_get_velocity(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_has_velocity:
 *
 * Whether the value sets `maplibre_native_ffi_animation_options_set_velocity`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_animation_options_has_velocity(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_velocity:
 *
 * The average speed of a fly animation, in screens per second. `duration_ms`
 * overrides it.
 */
void maplibre_native_ffi_animation_options_set_velocity(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_animation_options_get_min_zoom:
 *
 * The zoom level at the peak of a fly animation.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_animation_options_get_min_zoom(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_has_min_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_animation_options_set_min_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_animation_options_has_min_zoom(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_min_zoom:
 *
 * The zoom level at the peak of a fly animation.
 */
void maplibre_native_ffi_animation_options_set_min_zoom(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_animation_options_get_easing:
 *
 * The `easing` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiUnitBezier* maplibre_native_ffi_animation_options_get_easing(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_has_easing:
 *
 * Whether the value sets `maplibre_native_ffi_animation_options_set_easing`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_animation_options_has_easing(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_easing:
 * @value: the value
 *
 * The `easing` member.
 */
void maplibre_native_ffi_animation_options_set_easing(
  MaplibreNativeFfiAnimationOptions* self,
  const MaplibreNativeFfiUnitBezier* value
);

/**
 * maplibre_native_ffi_animation_options_get_transition_id:
 *
 * An ID that
 * `maplibre_native_ffi_map_cancel_camera_transitions_with_id_async()` cancels
 * the change by. Several changes can share one.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_animation_options_get_transition_id(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_has_transition_id:
 *
 * Whether the value sets `maplibre_native_ffi_animation_options_set_transition_id`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_animation_options_has_transition_id(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_transition_id:
 *
 * An ID that
 * `maplibre_native_ffi_map_cancel_camera_transitions_with_id_async()` cancels
 * the change by. Several changes can share one.
 */
void maplibre_native_ffi_animation_options_set_transition_id(
  MaplibreNativeFfiAnimationOptions* self, guint64 value
);

/**
 * maplibre_native_ffi_animation_options_get_end_handler:
 *
 * The `end_handler` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiCameraTransitionHandler*
maplibre_native_ffi_animation_options_get_end_handler(
  const MaplibreNativeFfiAnimationOptions* self
);

/**
 * maplibre_native_ffi_animation_options_set_end_handler:
 * @value: the value
 *
 * The `end_handler` member.
 */
void maplibre_native_ffi_animation_options_set_end_handler(
  MaplibreNativeFfiAnimationOptions* self,
  const MaplibreNativeFfiCameraTransitionHandler* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiAnimationOptions, maplibre_native_ffi_animation_options_free
)

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
 * See `mln_bound_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_bound_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_BOUND_OPTIONS \
  (maplibre_native_ffi_bound_options_get_type())

/**
 * maplibre_native_ffi_bound_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiBoundOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiBoundOptions* maplibre_native_ffi_bound_options_new(void);

/**
 * maplibre_native_ffi_bound_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiBoundOptions* maplibre_native_ffi_bound_options_copy(
  const MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_bound_options_free(
  MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_get_bounds:
 *
 * The area that the camera's center stays within.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLatLngBounds* maplibre_native_ffi_bound_options_get_bounds(
  const MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_has_bounds:
 *
 * Whether the value sets `maplibre_native_ffi_bound_options_set_bounds`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_bound_options_has_bounds(
  const MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_set_bounds:
 * @value: the value
 *
 * The area that the camera's center stays within.
 */
void maplibre_native_ffi_bound_options_set_bounds(
  MaplibreNativeFfiBoundOptions* self,
  const MaplibreNativeFfiLatLngBounds* value
);

/**
 * maplibre_native_ffi_bound_options_get_min_zoom:
 *
 * The `min_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_bound_options_get_min_zoom(
  const MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_has_min_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_bound_options_set_min_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_bound_options_has_min_zoom(
  const MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_set_min_zoom:
 *
 * The `min_zoom` member.
 */
void maplibre_native_ffi_bound_options_set_min_zoom(
  MaplibreNativeFfiBoundOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_bound_options_get_max_zoom:
 *
 * The `max_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_bound_options_get_max_zoom(
  const MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_has_max_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_bound_options_set_max_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_bound_options_has_max_zoom(
  const MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_set_max_zoom:
 *
 * The `max_zoom` member.
 */
void maplibre_native_ffi_bound_options_set_max_zoom(
  MaplibreNativeFfiBoundOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_bound_options_get_min_pitch:
 *
 * The lowest pitch, in degrees.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_bound_options_get_min_pitch(
  const MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_has_min_pitch:
 *
 * Whether the value sets `maplibre_native_ffi_bound_options_set_min_pitch`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_bound_options_has_min_pitch(
  const MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_set_min_pitch:
 *
 * The lowest pitch, in degrees.
 */
void maplibre_native_ffi_bound_options_set_min_pitch(
  MaplibreNativeFfiBoundOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_bound_options_get_max_pitch:
 *
 * The highest pitch, in degrees.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_bound_options_get_max_pitch(
  const MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_has_max_pitch:
 *
 * Whether the value sets `maplibre_native_ffi_bound_options_set_max_pitch`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_bound_options_has_max_pitch(
  const MaplibreNativeFfiBoundOptions* self
);

/**
 * maplibre_native_ffi_bound_options_set_max_pitch:
 *
 * The highest pitch, in degrees.
 */
void maplibre_native_ffi_bound_options_set_max_pitch(
  MaplibreNativeFfiBoundOptions* self, gdouble value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiBoundOptions, maplibre_native_ffi_bound_options_free
)

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

/**
 * MaplibreNativeFfiCameraDelta:
 *
 * A change to the camera from where it is now, for gestures.
 *
 * See `mln_camera_delta` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_camera_delta_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_DELTA \
  (maplibre_native_ffi_camera_delta_get_type())

/**
 * maplibre_native_ffi_camera_delta_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCameraDelta` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCameraDelta* maplibre_native_ffi_camera_delta_new(void);

/**
 * maplibre_native_ffi_camera_delta_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCameraDelta* maplibre_native_ffi_camera_delta_copy(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_camera_delta_free(MaplibreNativeFfiCameraDelta* self);

/**
 * maplibre_native_ffi_camera_delta_get_offset:
 *
 * Moves the map's content this far on the screen, as a drag does.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_delta_get_offset(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_has_offset:
 *
 * Whether the value sets `maplibre_native_ffi_camera_delta_set_offset`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_delta_has_offset(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_offset:
 * @value: the value
 *
 * Moves the map's content this far on the screen, as a drag does.
 */
void maplibre_native_ffi_camera_delta_set_offset(
  MaplibreNativeFfiCameraDelta* self, const MaplibreNativeFfiScreenPoint* value
);

/**
 * maplibre_native_ffi_camera_delta_get_scale:
 *
 * Multiplies the map's scale. 2 zooms in by one level.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_delta_get_scale(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_has_scale:
 *
 * Whether the value sets `maplibre_native_ffi_camera_delta_set_scale`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_delta_has_scale(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_scale:
 *
 * Multiplies the map's scale. 2 zooms in by one level.
 */
void maplibre_native_ffi_camera_delta_set_scale(
  MaplibreNativeFfiCameraDelta* self, gdouble value
);

/**
 * maplibre_native_ffi_camera_delta_get_bearing:
 *
 * Degrees to add to the bearing.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_delta_get_bearing(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_has_bearing:
 *
 * Whether the value sets `maplibre_native_ffi_camera_delta_set_bearing`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_delta_has_bearing(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_bearing:
 *
 * Degrees to add to the bearing.
 */
void maplibre_native_ffi_camera_delta_set_bearing(
  MaplibreNativeFfiCameraDelta* self, gdouble value
);

/**
 * maplibre_native_ffi_camera_delta_get_pitch:
 *
 * Degrees to add to the pitch.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_delta_get_pitch(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_has_pitch:
 *
 * Whether the value sets `maplibre_native_ffi_camera_delta_set_pitch`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_delta_has_pitch(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_pitch:
 *
 * Degrees to add to the pitch.
 */
void maplibre_native_ffi_camera_delta_set_pitch(
  MaplibreNativeFfiCameraDelta* self, gdouble value
);

/**
 * maplibre_native_ffi_camera_delta_get_anchor:
 *
 * The screen point that `scale`, `bearing`, and `pitch` keep in place. It needs
 * one of them, and it combines with `offset` only in a delta without a
 * duration.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_delta_get_anchor(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_has_anchor:
 *
 * Whether the value sets `maplibre_native_ffi_camera_delta_set_anchor`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_delta_has_anchor(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_anchor:
 * @value: the value
 *
 * The screen point that `scale`, `bearing`, and `pitch` keep in place. It needs
 * one of them, and it combines with `offset` only in a delta without a
 * duration.
 */
void maplibre_native_ffi_camera_delta_set_anchor(
  MaplibreNativeFfiCameraDelta* self, const MaplibreNativeFfiScreenPoint* value
);

/**
 * maplibre_native_ffi_camera_delta_get_animation:
 *
 * The `animation` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_delta_get_animation(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_animation:
 * @value: the value
 *
 * The `animation` member.
 */
void maplibre_native_ffi_camera_delta_set_animation(
  MaplibreNativeFfiCameraDelta* self,
  const MaplibreNativeFfiAnimationOptions* value
);

/**
 * maplibre_native_ffi_camera_delta_get_gesture_phase:
 *
 * The `gesture_phase` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiGesturePhase
maplibre_native_ffi_camera_delta_get_gesture_phase(
  const MaplibreNativeFfiCameraDelta* self
);

/**
 * maplibre_native_ffi_camera_delta_set_gesture_phase:
 *
 * The `gesture_phase` member.
 */
void maplibre_native_ffi_camera_delta_set_gesture_phase(
  MaplibreNativeFfiCameraDelta* self, MaplibreNativeFfiGesturePhase value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCameraDelta, maplibre_native_ffi_camera_delta_free
)

/**
 * MaplibreNativeFfiCameraFitOptions:
 *
 * Options for finding the camera that fits an area.
 *
 * See `mln_camera_fit_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_camera_fit_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_FIT_OPTIONS \
  (maplibre_native_ffi_camera_fit_options_get_type())

/**
 * maplibre_native_ffi_camera_fit_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCameraFitOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCameraFitOptions* maplibre_native_ffi_camera_fit_options_new(
  void
);

/**
 * maplibre_native_ffi_camera_fit_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCameraFitOptions* maplibre_native_ffi_camera_fit_options_copy(
  const MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_camera_fit_options_free(
  MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_get_padding:
 *
 * Space to leave between the area and each edge of the map. The left and right
 * insets must add up to less than the map's width, and the top and bottom to
 * less than its height.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiEdgeInsets* maplibre_native_ffi_camera_fit_options_get_padding(
  const MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_has_padding:
 *
 * Whether the value sets `maplibre_native_ffi_camera_fit_options_set_padding`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_fit_options_has_padding(
  const MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_set_padding:
 * @value: the value
 *
 * Space to leave between the area and each edge of the map. The left and right
 * insets must add up to less than the map's width, and the top and bottom to
 * less than its height.
 */
void maplibre_native_ffi_camera_fit_options_set_padding(
  MaplibreNativeFfiCameraFitOptions* self,
  const MaplibreNativeFfiEdgeInsets* value
);

/**
 * maplibre_native_ffi_camera_fit_options_get_bearing:
 *
 * The rotation from north, in degrees.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_fit_options_get_bearing(
  const MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_has_bearing:
 *
 * Whether the value sets `maplibre_native_ffi_camera_fit_options_set_bearing`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_fit_options_has_bearing(
  const MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_set_bearing:
 *
 * The rotation from north, in degrees.
 */
void maplibre_native_ffi_camera_fit_options_set_bearing(
  MaplibreNativeFfiCameraFitOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_camera_fit_options_get_pitch:
 *
 * The tilt toward the horizon, in degrees.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_fit_options_get_pitch(
  const MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_has_pitch:
 *
 * Whether the value sets `maplibre_native_ffi_camera_fit_options_set_pitch`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_fit_options_has_pitch(
  const MaplibreNativeFfiCameraFitOptions* self
);

/**
 * maplibre_native_ffi_camera_fit_options_set_pitch:
 *
 * The tilt toward the horizon, in degrees.
 */
void maplibre_native_ffi_camera_fit_options_set_pitch(
  MaplibreNativeFfiCameraFitOptions* self, gdouble value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCameraFitOptions, maplibre_native_ffi_camera_fit_options_free
)

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
GType maplibre_native_ffi_camera_fit_update_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_FIT_UPDATE \
  (maplibre_native_ffi_camera_fit_update_get_type())

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
 * maplibre_native_ffi_camera_fit_update_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCameraFitUpdate* maplibre_native_ffi_camera_fit_update_copy(
  const MaplibreNativeFfiCameraFitUpdate* self
);

/**
 * maplibre_native_ffi_camera_fit_update_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_camera_fit_update_free(
  MaplibreNativeFfiCameraFitUpdate* self
);

/**
 * maplibre_native_ffi_camera_fit_update_get_mode:
 *
 * The `mode` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiCameraUpdateMode
maplibre_native_ffi_camera_fit_update_get_mode(
  const MaplibreNativeFfiCameraFitUpdate* self
);

/**
 * maplibre_native_ffi_camera_fit_update_set_mode:
 *
 * The `mode` member.
 */
void maplibre_native_ffi_camera_fit_update_set_mode(
  MaplibreNativeFfiCameraFitUpdate* self,
  MaplibreNativeFfiCameraUpdateMode value
);

/**
 * maplibre_native_ffi_camera_fit_update_get_fit:
 *
 * The `fit` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiCameraFitOptions*
maplibre_native_ffi_camera_fit_update_get_fit(
  const MaplibreNativeFfiCameraFitUpdate* self
);

/**
 * maplibre_native_ffi_camera_fit_update_set_fit:
 * @value: the value
 *
 * The `fit` member.
 */
void maplibre_native_ffi_camera_fit_update_set_fit(
  MaplibreNativeFfiCameraFitUpdate* self,
  const MaplibreNativeFfiCameraFitOptions* value
);

/**
 * maplibre_native_ffi_camera_fit_update_get_animation:
 *
 * The `animation` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_fit_update_get_animation(
  const MaplibreNativeFfiCameraFitUpdate* self
);

/**
 * maplibre_native_ffi_camera_fit_update_set_animation:
 * @value: the value
 *
 * The `animation` member.
 */
void maplibre_native_ffi_camera_fit_update_set_animation(
  MaplibreNativeFfiCameraFitUpdate* self,
  const MaplibreNativeFfiAnimationOptions* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCameraFitUpdate, maplibre_native_ffi_camera_fit_update_free
)

/**
 * MaplibreNativeFfiCameraOptions:
 *
 * A camera, with only the members that `fields` selects.
 *
 * See `mln_camera_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_camera_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_OPTIONS \
  (maplibre_native_ffi_camera_options_get_type())

/**
 * maplibre_native_ffi_camera_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCameraOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_options_new(void);

/**
 * maplibre_native_ffi_camera_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_options_copy(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_camera_options_free(
  MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_get_center:
 *
 * The `center` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLatLng* maplibre_native_ffi_camera_options_get_center(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_center:
 *
 * Whether the value sets `maplibre_native_ffi_camera_options_set_center`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_center(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_center:
 * @value: the value
 *
 * The `center` member.
 */
void maplibre_native_ffi_camera_options_set_center(
  MaplibreNativeFfiCameraOptions* self, const MaplibreNativeFfiLatLng* value
);

/**
 * maplibre_native_ffi_camera_options_get_center_altitude:
 *
 * The altitude of the center, in meters above sea level.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_center_altitude(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_center_altitude:
 *
 * Whether the value sets `maplibre_native_ffi_camera_options_set_center_altitude`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_center_altitude(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_center_altitude:
 *
 * The altitude of the center, in meters above sea level.
 */
void maplibre_native_ffi_camera_options_set_center_altitude(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_camera_options_get_padding:
 *
 * Insets from the map's edges. The camera centers within the rest.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiEdgeInsets* maplibre_native_ffi_camera_options_get_padding(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_padding:
 *
 * Whether the value sets `maplibre_native_ffi_camera_options_set_padding`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_padding(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_padding:
 * @value: the value
 *
 * Insets from the map's edges. The camera centers within the rest.
 */
void maplibre_native_ffi_camera_options_set_padding(
  MaplibreNativeFfiCameraOptions* self, const MaplibreNativeFfiEdgeInsets* value
);

/**
 * maplibre_native_ffi_camera_options_get_anchor:
 *
 * The screen point that a change to the zoom, bearing, or pitch keeps in place.
 * A `center` overrides it.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_options_get_anchor(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_anchor:
 *
 * Whether the value sets `maplibre_native_ffi_camera_options_set_anchor`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_anchor(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_anchor:
 * @value: the value
 *
 * The screen point that a change to the zoom, bearing, or pitch keeps in place.
 * A `center` overrides it.
 */
void maplibre_native_ffi_camera_options_set_anchor(
  MaplibreNativeFfiCameraOptions* self,
  const MaplibreNativeFfiScreenPoint* value
);

/**
 * maplibre_native_ffi_camera_options_get_zoom:
 *
 * The `zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_zoom(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_camera_options_set_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_zoom(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_zoom:
 *
 * The `zoom` member.
 */
void maplibre_native_ffi_camera_options_set_zoom(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_camera_options_get_bearing:
 *
 * The rotation from north, in degrees.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_bearing(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_bearing:
 *
 * Whether the value sets `maplibre_native_ffi_camera_options_set_bearing`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_bearing(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_bearing:
 *
 * The rotation from north, in degrees.
 */
void maplibre_native_ffi_camera_options_set_bearing(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_camera_options_get_pitch:
 *
 * The tilt toward the horizon, in degrees. 0 looks straight down.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_pitch(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_pitch:
 *
 * Whether the value sets `maplibre_native_ffi_camera_options_set_pitch`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_pitch(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_pitch:
 *
 * The tilt toward the horizon, in degrees. 0 looks straight down.
 */
void maplibre_native_ffi_camera_options_set_pitch(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_camera_options_get_roll:
 *
 * The roll, in degrees.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_roll(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_roll:
 *
 * Whether the value sets `maplibre_native_ffi_camera_options_set_roll`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_roll(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_roll:
 *
 * The roll, in degrees.
 */
void maplibre_native_ffi_camera_options_set_roll(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_camera_options_get_field_of_view:
 *
 * The vertical field of view, in degrees.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_camera_options_get_field_of_view(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_has_field_of_view:
 *
 * Whether the value sets `maplibre_native_ffi_camera_options_set_field_of_view`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_camera_options_has_field_of_view(
  const MaplibreNativeFfiCameraOptions* self
);

/**
 * maplibre_native_ffi_camera_options_set_field_of_view:
 *
 * The vertical field of view, in degrees.
 */
void maplibre_native_ffi_camera_options_set_field_of_view(
  MaplibreNativeFfiCameraOptions* self, gdouble value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCameraOptions, maplibre_native_ffi_camera_options_free
)

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
GType maplibre_native_ffi_camera_update_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_UPDATE \
  (maplibre_native_ffi_camera_update_get_type())

/**
 * maplibre_native_ffi_camera_update_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCameraUpdate` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCameraUpdate* maplibre_native_ffi_camera_update_new(void);

/**
 * maplibre_native_ffi_camera_update_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCameraUpdate* maplibre_native_ffi_camera_update_copy(
  const MaplibreNativeFfiCameraUpdate* self
);

/**
 * maplibre_native_ffi_camera_update_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_camera_update_free(
  MaplibreNativeFfiCameraUpdate* self
);

/**
 * maplibre_native_ffi_camera_update_get_mode:
 *
 * The `mode` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiCameraUpdateMode maplibre_native_ffi_camera_update_get_mode(
  const MaplibreNativeFfiCameraUpdate* self
);

/**
 * maplibre_native_ffi_camera_update_set_mode:
 *
 * The `mode` member.
 */
void maplibre_native_ffi_camera_update_set_mode(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiCameraUpdateMode value
);

/**
 * maplibre_native_ffi_camera_update_get_camera:
 *
 * The `camera` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_update_get_camera(
  const MaplibreNativeFfiCameraUpdate* self
);

/**
 * maplibre_native_ffi_camera_update_set_camera:
 * @value: the value
 *
 * The `camera` member.
 */
void maplibre_native_ffi_camera_update_set_camera(
  MaplibreNativeFfiCameraUpdate* self,
  const MaplibreNativeFfiCameraOptions* value
);

/**
 * maplibre_native_ffi_camera_update_get_animation:
 *
 * The `animation` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_update_get_animation(
  const MaplibreNativeFfiCameraUpdate* self
);

/**
 * maplibre_native_ffi_camera_update_set_animation:
 * @value: the value
 *
 * The `animation` member.
 */
void maplibre_native_ffi_camera_update_set_animation(
  MaplibreNativeFfiCameraUpdate* self,
  const MaplibreNativeFfiAnimationOptions* value
);

/**
 * maplibre_native_ffi_camera_update_get_gesture_phase:
 *
 * The `gesture_phase` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiGesturePhase
maplibre_native_ffi_camera_update_get_gesture_phase(
  const MaplibreNativeFfiCameraUpdate* self
);

/**
 * maplibre_native_ffi_camera_update_set_gesture_phase:
 *
 * The `gesture_phase` member.
 */
void maplibre_native_ffi_camera_update_set_gesture_phase(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiGesturePhase value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCameraUpdate, maplibre_native_ffi_camera_update_free
)

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
 * See `mln_custom_geometry_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
GType maplibre_native_ffi_custom_geometry_source_options_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CUSTOM_GEOMETRY_SOURCE_OPTIONS \
  (maplibre_native_ffi_custom_geometry_source_options_get_type())

/**
 * maplibre_native_ffi_custom_geometry_source_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCustomGeometrySourceOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCustomGeometrySourceOptions*
maplibre_native_ffi_custom_geometry_source_options_new(void);

/**
 * maplibre_native_ffi_custom_geometry_source_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCustomGeometrySourceOptions*
maplibre_native_ffi_custom_geometry_source_options_copy(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_custom_geometry_source_options_free(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_get_min_zoom:
 *
 * The `min_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_custom_geometry_source_options_get_min_zoom(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_min_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_custom_geometry_source_options_set_min_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_min_zoom(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_min_zoom:
 *
 * The `min_zoom` member.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_min_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_get_max_zoom:
 *
 * The `max_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_custom_geometry_source_options_get_max_zoom(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_max_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_custom_geometry_source_options_set_max_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_max_zoom(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_max_zoom:
 *
 * The `max_zoom` member.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_max_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_get_tolerance:
 *
 * How far, in pixels, simplified geometry can stray from the original.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_custom_geometry_source_options_get_tolerance(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_tolerance:
 *
 * Whether the value sets `maplibre_native_ffi_custom_geometry_source_options_set_tolerance`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_tolerance(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_tolerance:
 *
 * How far, in pixels, simplified geometry can stray from the original.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_tolerance(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_get_tile_size:
 *
 * The width of a tile in pixels, a power of two no larger than 8192.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_custom_geometry_source_options_get_tile_size(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_tile_size:
 *
 * Whether the value sets `maplibre_native_ffi_custom_geometry_source_options_set_tile_size`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_tile_size(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_tile_size:
 *
 * The width of a tile in pixels, a power of two no larger than 8192.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_tile_size(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, guint32 value
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_get_buffer:
 *
 * The width in pixels of the margin around a tile that holds geometry from
 * beyond its edges. `buffer` times 8192 / `tile_size` must be at most 65535.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_custom_geometry_source_options_get_buffer(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_buffer:
 *
 * Whether the value sets `maplibre_native_ffi_custom_geometry_source_options_set_buffer`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_buffer(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_buffer:
 *
 * The width in pixels of the margin around a tile that holds geometry from
 * beyond its edges. `buffer` times 8192 / `tile_size` must be at most 65535.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_buffer(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, guint32 value
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_get_clip:
 *
 * Whether to cut geometry at the outer edge of a tile's margin.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_get_clip(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_clip:
 *
 * Whether the value sets `maplibre_native_ffi_custom_geometry_source_options_set_clip`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_clip(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_clip:
 *
 * Whether to cut geometry at the outer edge of a tile's margin.
 */
void maplibre_native_ffi_custom_geometry_source_options_set_clip(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gboolean value
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_get_wrap:
 *
 * Whether to wrap geometry that crosses the antimeridian.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_get_wrap(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_has_wrap:
 *
 * Whether the value sets `maplibre_native_ffi_custom_geometry_source_options_set_wrap`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_geometry_source_options_has_wrap(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
);

/**
 * maplibre_native_ffi_custom_geometry_source_options_set_wrap:
 *
 * Whether to wrap geometry that crosses the antimeridian.
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCustomGeometrySourceOptions,
  maplibre_native_ffi_custom_geometry_source_options_free
)

/**
 * MaplibreNativeFfiCustomVectorSourceOptions:
 *
 * The callbacks and options of a custom vector source.
 *
 * Zoom levels are whole numbers from 0 to 32.
 *
 * See `mln_custom_vector_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
GType maplibre_native_ffi_custom_vector_source_options_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CUSTOM_VECTOR_SOURCE_OPTIONS \
  (maplibre_native_ffi_custom_vector_source_options_get_type())

/**
 * maplibre_native_ffi_custom_vector_source_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiCustomVectorSourceOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiCustomVectorSourceOptions*
maplibre_native_ffi_custom_vector_source_options_new(void);

/**
 * maplibre_native_ffi_custom_vector_source_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCustomVectorSourceOptions*
maplibre_native_ffi_custom_vector_source_options_copy(
  const MaplibreNativeFfiCustomVectorSourceOptions* self
);

/**
 * maplibre_native_ffi_custom_vector_source_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_custom_vector_source_options_free(
  MaplibreNativeFfiCustomVectorSourceOptions* self
);

/**
 * maplibre_native_ffi_custom_vector_source_options_get_min_zoom:
 *
 * The `min_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_custom_vector_source_options_get_min_zoom(
  const MaplibreNativeFfiCustomVectorSourceOptions* self
);

/**
 * maplibre_native_ffi_custom_vector_source_options_has_min_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_custom_vector_source_options_set_min_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_vector_source_options_has_min_zoom(
  const MaplibreNativeFfiCustomVectorSourceOptions* self
);

/**
 * maplibre_native_ffi_custom_vector_source_options_set_min_zoom:
 *
 * The `min_zoom` member.
 */
void maplibre_native_ffi_custom_vector_source_options_set_min_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_custom_vector_source_options_get_max_zoom:
 *
 * The `max_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_custom_vector_source_options_get_max_zoom(
  const MaplibreNativeFfiCustomVectorSourceOptions* self
);

/**
 * maplibre_native_ffi_custom_vector_source_options_has_max_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_custom_vector_source_options_set_max_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_custom_vector_source_options_has_max_zoom(
  const MaplibreNativeFfiCustomVectorSourceOptions* self
);

/**
 * maplibre_native_ffi_custom_vector_source_options_set_max_zoom:
 *
 * The `max_zoom` member.
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCustomVectorSourceOptions,
  maplibre_native_ffi_custom_vector_source_options_free
)

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
GType maplibre_native_ffi_dma_buf_ring_descriptor_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_DMA_BUF_RING_DESCRIPTOR \
  (maplibre_native_ffi_dma_buf_ring_descriptor_get_type())

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
 * maplibre_native_ffi_dma_buf_ring_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiDmaBufRingDescriptor*
maplibre_native_ffi_dma_buf_ring_descriptor_copy(
  const MaplibreNativeFfiDmaBufRingDescriptor* self
);

/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_dma_buf_ring_descriptor_free(
  MaplibreNativeFfiDmaBufRingDescriptor* self
);

/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_dma_buf_ring_descriptor_get_logical_size(
  const MaplibreNativeFfiDmaBufRingDescriptor* self
);

/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_dma_buf_ring_descriptor_set_logical_size(
  MaplibreNativeFfiDmaBufRingDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_get_buffer_count:
 *
 * How many buffers the ring has, from 1 to 8.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_dma_buf_ring_descriptor_get_buffer_count(
  const MaplibreNativeFfiDmaBufRingDescriptor* self
);

/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_set_buffer_count:
 *
 * How many buffers the ring has, from 1 to 8.
 */
void maplibre_native_ffi_dma_buf_ring_descriptor_set_buffer_count(
  MaplibreNativeFfiDmaBufRingDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_dma_buf_ring_descriptor_get_formats:
 *
 * The formats and modifiers that your reader imports, most preferred first,
 * such as those that eglQueryDmaBufModifiersEXT() reports.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiDmaBufFormat): the items
 */
GPtrArray* maplibre_native_ffi_dma_buf_ring_descriptor_get_formats(
  const MaplibreNativeFfiDmaBufRingDescriptor* self
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiDmaBufRingDescriptor,
  maplibre_native_ffi_dma_buf_ring_descriptor_free
)

/**
 * MaplibreNativeFfiEglContextDescriptor:
 *
 * The EGL objects that a render session makes its OpenGL context from.
 *
 * See `mln_egl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
GType maplibre_native_ffi_egl_context_descriptor_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_EGL_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_egl_context_descriptor_get_type())

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
 * maplibre_native_ffi_egl_context_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiEglContextDescriptor*
maplibre_native_ffi_egl_context_descriptor_copy(
  const MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_egl_context_descriptor_free(
  MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_get_display:
 *
 * An `EGLDisplay`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_egl_context_descriptor_get_display(
  const MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_set_display:
 *
 * An `EGLDisplay`.
 */
void maplibre_native_ffi_egl_context_descriptor_set_display(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
);

/**
 * maplibre_native_ffi_egl_context_descriptor_get_config:
 *
 * An `EGLConfig`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_egl_context_descriptor_get_config(
  const MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_set_config:
 *
 * An `EGLConfig`.
 */
void maplibre_native_ffi_egl_context_descriptor_set_config(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
);

/**
 * maplibre_native_ffi_egl_context_descriptor_get_share_context:
 *
 * Your `EGLContext`, for a shared context.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_egl_context_descriptor_get_share_context(
  const MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_set_share_context:
 *
 * Your `EGLContext`, for a shared context.
 */
void maplibre_native_ffi_egl_context_descriptor_set_share_context(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
);

/**
 * maplibre_native_ffi_egl_context_descriptor_get_client_api:
 *
 * The API of a dedicated context. A shared context uses the API of yours.
 *
 * Returns: the value
 */
MaplibreNativeFfiOpenGlClientApi
maplibre_native_ffi_egl_context_descriptor_get_client_api(
  const MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_set_client_api:
 *
 * The API of a dedicated context. A shared context uses the API of yours.
 */
void maplibre_native_ffi_egl_context_descriptor_set_client_api(
  MaplibreNativeFfiEglContextDescriptor* self,
  MaplibreNativeFfiOpenGlClientApi value
);

/**
 * maplibre_native_ffi_egl_context_descriptor_get_get_proc_address:
 *
 * Your `eglGetProcAddress`, or null to use the system's.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_egl_context_descriptor_get_get_proc_address(
  const MaplibreNativeFfiEglContextDescriptor* self
);

/**
 * maplibre_native_ffi_egl_context_descriptor_set_get_proc_address:
 *
 * Your `eglGetProcAddress`, or null to use the system's.
 */
void maplibre_native_ffi_egl_context_descriptor_set_get_proc_address(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiEglContextDescriptor,
  maplibre_native_ffi_egl_context_descriptor_free
)

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
 * See `mln_feature_state_selector` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_feature_state_selector_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_FEATURE_STATE_SELECTOR \
  (maplibre_native_ffi_feature_state_selector_get_type())

/**
 * maplibre_native_ffi_feature_state_selector_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiFeatureStateSelector` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiFeatureStateSelector*
maplibre_native_ffi_feature_state_selector_new(void);

/**
 * maplibre_native_ffi_feature_state_selector_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiFeatureStateSelector*
maplibre_native_ffi_feature_state_selector_copy(
  const MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_feature_state_selector_free(
  MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_get_source_id:
 *
 * The `source_id` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_feature_state_selector_get_source_id(
  const MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_set_source_id:
 * @value: the UTF-8 text
 *
 * The `source_id` member.
 */
void maplibre_native_ffi_feature_state_selector_set_source_id(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
);

/**
 * maplibre_native_ffi_feature_state_selector_get_source_layer_id:
 *
 * The source layer, for a vector source. Empty for other sources.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_feature_state_selector_get_source_layer_id(
  const MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_set_source_layer_id:
 * @value: the UTF-8 text
 *
 * The source layer, for a vector source. Empty for other sources.
 */
void maplibre_native_ffi_feature_state_selector_set_source_layer_id(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
);

/**
 * maplibre_native_ffi_feature_state_selector_get_feature_id:
 *
 * The feature's ID. Setting or reading state needs it. Without it,
 * `maplibre_native_ffi_map_remove_feature_state_async()` removes the state of
 * every feature in the source layer.
 *
 * Returns: (transfer none) (nullable): the value
 */
MaplibreNativeFfiFeatureId*
maplibre_native_ffi_feature_state_selector_get_feature_id(
  const MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_has_feature_id:
 *
 * Whether the value sets `maplibre_native_ffi_feature_state_selector_set_feature_id`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_feature_state_selector_has_feature_id(
  const MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_set_feature_id:
 * @value: the value
 *
 * The feature's ID. Setting or reading state needs it. Without it,
 * `maplibre_native_ffi_map_remove_feature_state_async()` removes the state of
 * every feature in the source layer.
 */
void maplibre_native_ffi_feature_state_selector_set_feature_id(
  MaplibreNativeFfiFeatureStateSelector* self,
  const MaplibreNativeFfiFeatureId* value
);

/**
 * maplibre_native_ffi_feature_state_selector_get_state_key:
 *
 * The one key that `maplibre_native_ffi_map_remove_feature_state_async()`
 * removes. It needs `feature_id`. Empty removes every key.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_feature_state_selector_get_state_key(
  const MaplibreNativeFfiFeatureStateSelector* self
);

/**
 * maplibre_native_ffi_feature_state_selector_set_state_key:
 * @value: the UTF-8 text
 *
 * The one key that `maplibre_native_ffi_map_remove_feature_state_async()`
 * removes. It needs `feature_id`. Empty removes every key.
 */
void maplibre_native_ffi_feature_state_selector_set_state_key(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiFeatureStateSelector,
  maplibre_native_ffi_feature_state_selector_free
)

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
 * See `mln_frame_pacer_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
GType maplibre_native_ffi_frame_pacer_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_FRAME_PACER_OPTIONS \
  (maplibre_native_ffi_frame_pacer_options_get_type())

/**
 * maplibre_native_ffi_frame_pacer_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiFramePacerOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiFramePacerOptions* maplibre_native_ffi_frame_pacer_options_new(
  void
);

/**
 * maplibre_native_ffi_frame_pacer_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiFramePacerOptions*
maplibre_native_ffi_frame_pacer_options_copy(
  const MaplibreNativeFfiFramePacerOptions* self
);

/**
 * maplibre_native_ffi_frame_pacer_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_frame_pacer_options_free(
  MaplibreNativeFfiFramePacerOptions* self
);

/**
 * maplibre_native_ffi_frame_pacer_options_get_maximum_frames_per_second:
 *
 * The most frames per second that the pacer demands, greater than zero. The
 * pacer skips display refreshes to stay within it. By default, it can demand
 * frames at every refresh.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_frame_pacer_options_get_maximum_frames_per_second(
  const MaplibreNativeFfiFramePacerOptions* self
);

/**
 * maplibre_native_ffi_frame_pacer_options_has_maximum_frames_per_second:
 *
 * Whether the value sets `maplibre_native_ffi_frame_pacer_options_set_maximum_frames_per_second`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_frame_pacer_options_has_maximum_frames_per_second(
  const MaplibreNativeFfiFramePacerOptions* self
);

/**
 * maplibre_native_ffi_frame_pacer_options_set_maximum_frames_per_second:
 *
 * The most frames per second that the pacer demands, greater than zero. The
 * pacer skips display refreshes to stay within it. By default, it can demand
 * frames at every refresh.
 */
void maplibre_native_ffi_frame_pacer_options_set_maximum_frames_per_second(
  MaplibreNativeFfiFramePacerOptions* self, guint32 value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiFramePacerOptions,
  maplibre_native_ffi_frame_pacer_options_free
)

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
 * See `mln_free_camera_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_free_camera_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_FREE_CAMERA_OPTIONS \
  (maplibre_native_ffi_free_camera_options_get_type())

/**
 * maplibre_native_ffi_free_camera_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiFreeCameraOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiFreeCameraOptions* maplibre_native_ffi_free_camera_options_new(
  void
);

/**
 * maplibre_native_ffi_free_camera_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiFreeCameraOptions*
maplibre_native_ffi_free_camera_options_copy(
  const MaplibreNativeFfiFreeCameraOptions* self
);

/**
 * maplibre_native_ffi_free_camera_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_free_camera_options_free(
  MaplibreNativeFfiFreeCameraOptions* self
);

/**
 * maplibre_native_ffi_free_camera_options_get_position:
 *
 * The camera's position in Web Mercator units, where the world is 1 wide, (0,
 * 0) is its northwest corner, and (1, 1) is its southeast corner. `z` is the
 * altitude in the same units.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiVec3* maplibre_native_ffi_free_camera_options_get_position(
  const MaplibreNativeFfiFreeCameraOptions* self
);

/**
 * maplibre_native_ffi_free_camera_options_has_position:
 *
 * Whether the value sets `maplibre_native_ffi_free_camera_options_set_position`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_free_camera_options_has_position(
  const MaplibreNativeFfiFreeCameraOptions* self
);

/**
 * maplibre_native_ffi_free_camera_options_set_position:
 * @value: the value
 *
 * The camera's position in Web Mercator units, where the world is 1 wide, (0,
 * 0) is its northwest corner, and (1, 1) is its southeast corner. `z` is the
 * altitude in the same units.
 */
void maplibre_native_ffi_free_camera_options_set_position(
  MaplibreNativeFfiFreeCameraOptions* self, const MaplibreNativeFfiVec3* value
);

/**
 * maplibre_native_ffi_free_camera_options_get_orientation:
 *
 * The camera's rotation from looking straight down with north at the top of the
 * screen.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiQuaternion*
maplibre_native_ffi_free_camera_options_get_orientation(
  const MaplibreNativeFfiFreeCameraOptions* self
);

/**
 * maplibre_native_ffi_free_camera_options_has_orientation:
 *
 * Whether the value sets `maplibre_native_ffi_free_camera_options_set_orientation`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_free_camera_options_has_orientation(
  const MaplibreNativeFfiFreeCameraOptions* self
);

/**
 * maplibre_native_ffi_free_camera_options_set_orientation:
 * @value: the value
 *
 * The camera's rotation from looking straight down with north at the top of the
 * screen.
 */
void maplibre_native_ffi_free_camera_options_set_orientation(
  MaplibreNativeFfiFreeCameraOptions* self,
  const MaplibreNativeFfiQuaternion* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiFreeCameraOptions,
  maplibre_native_ffi_free_camera_options_free
)

/**
 * MaplibreNativeFfiGeoJsonSourceOptions:
 *
 * Options for a GeoJSON
 * \[source\](https://maplibre.org/maplibre-style-spec/sources/#geojson).
 *
 * Zoom levels are whole numbers.
 *
 * See `mln_geo_json_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
GType maplibre_native_ffi_geo_json_source_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_GEO_JSON_SOURCE_OPTIONS \
  (maplibre_native_ffi_geo_json_source_options_get_type())

/**
 * maplibre_native_ffi_geo_json_source_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiGeoJsonSourceOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_geo_json_source_options_new(void);

/**
 * maplibre_native_ffi_geo_json_source_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_geo_json_source_options_copy(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_geo_json_source_options_free(
  MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_get_min_zoom:
 *
 * The `min_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_geo_json_source_options_get_min_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_min_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_geo_json_source_options_set_min_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_min_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_min_zoom:
 *
 * The `min_zoom` member.
 */
void maplibre_native_ffi_geo_json_source_options_set_min_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_geo_json_source_options_get_max_zoom:
 *
 * The `max_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_geo_json_source_options_get_max_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_max_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_geo_json_source_options_set_max_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_max_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_max_zoom:
 *
 * The `max_zoom` member.
 */
void maplibre_native_ffi_geo_json_source_options_set_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_geo_json_source_options_get_tolerance:
 *
 * The `tolerance` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_geo_json_source_options_get_tolerance(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_tolerance:
 *
 * Whether the value sets `maplibre_native_ffi_geo_json_source_options_set_tolerance`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_tolerance(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_tolerance:
 *
 * The `tolerance` member.
 */
void maplibre_native_ffi_geo_json_source_options_set_tolerance(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_geo_json_source_options_get_cluster_max_zoom:
 *
 * The last zoom level that has clusters, at most 30.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_geo_json_source_options_get_cluster_max_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_cluster_max_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_geo_json_source_options_set_cluster_max_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_max_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_cluster_max_zoom:
 *
 * The last zoom level that has clusters, at most 30.
 */
void maplibre_native_ffi_geo_json_source_options_set_cluster_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_geo_json_source_options_get_cluster_properties:
 *
 * The `clusterProperties` object, or empty for none.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_geo_json_source_options_get_cluster_properties(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_cluster_properties:
 * @value: the UTF-8 text
 *
 * The `clusterProperties` object, or empty for none.
 */
void maplibre_native_ffi_geo_json_source_options_set_cluster_properties(
  MaplibreNativeFfiGeoJsonSourceOptions* self, const char* value
);

/**
 * maplibre_native_ffi_geo_json_source_options_get_buffer:
 *
 * The `buffer` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_geo_json_source_options_get_buffer(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_buffer:
 *
 * Whether the value sets `maplibre_native_ffi_geo_json_source_options_set_buffer`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_buffer(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_buffer:
 *
 * The `buffer` member.
 */
void maplibre_native_ffi_geo_json_source_options_set_buffer(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint32 value
);

/**
 * maplibre_native_ffi_geo_json_source_options_get_cluster_radius:
 *
 * The `cluster_radius` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_geo_json_source_options_get_cluster_radius(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_cluster_radius:
 *
 * Whether the value sets `maplibre_native_ffi_geo_json_source_options_set_cluster_radius`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_radius(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_cluster_radius:
 *
 * The `cluster_radius` member.
 */
void maplibre_native_ffi_geo_json_source_options_set_cluster_radius(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint32 value
);

/**
 * maplibre_native_ffi_geo_json_source_options_get_cluster_min_points:
 *
 * At most 4294967295 on a 32-bit target.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_geo_json_source_options_get_cluster_min_points(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_cluster_min_points:
 *
 * Whether the value sets `maplibre_native_ffi_geo_json_source_options_set_cluster_min_points`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_min_points(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_cluster_min_points:
 *
 * At most 4294967295 on a 32-bit target.
 */
void maplibre_native_ffi_geo_json_source_options_set_cluster_min_points(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint64 value
);

/**
 * maplibre_native_ffi_geo_json_source_options_get_line_metrics:
 *
 * The `line_metrics` member.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_geo_json_source_options_get_line_metrics(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_line_metrics:
 *
 * Whether the value sets `maplibre_native_ffi_geo_json_source_options_set_line_metrics`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_line_metrics(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_line_metrics:
 *
 * The `line_metrics` member.
 */
void maplibre_native_ffi_geo_json_source_options_set_line_metrics(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gboolean value
);

/**
 * maplibre_native_ffi_geo_json_source_options_get_cluster:
 *
 * The `cluster` member.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_geo_json_source_options_get_cluster(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_has_cluster:
 *
 * Whether the value sets `maplibre_native_ffi_geo_json_source_options_set_cluster`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_geo_json_source_options_has_cluster(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
);

/**
 * maplibre_native_ffi_geo_json_source_options_set_cluster:
 *
 * The `cluster` member.
 */
void maplibre_native_ffi_geo_json_source_options_set_cluster(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gboolean value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiGeoJsonSourceOptions,
  maplibre_native_ffi_geo_json_source_options_free
)

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
GType maplibre_native_ffi_http_header_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_HTTP_HEADER \
  (maplibre_native_ffi_http_header_get_type())

/**
 * maplibre_native_ffi_http_header_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiHttpHeader` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiHttpHeader* maplibre_native_ffi_http_header_new(void);

/**
 * maplibre_native_ffi_http_header_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiHttpHeader* maplibre_native_ffi_http_header_copy(
  const MaplibreNativeFfiHttpHeader* self
);

/**
 * maplibre_native_ffi_http_header_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_http_header_free(MaplibreNativeFfiHttpHeader* self);

/**
 * maplibre_native_ffi_http_header_get_name:
 *
 * The `name` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_http_header_get_name(
  const MaplibreNativeFfiHttpHeader* self
);

/**
 * maplibre_native_ffi_http_header_set_name:
 * @value: the UTF-8 text
 *
 * The `name` member.
 */
void maplibre_native_ffi_http_header_set_name(
  MaplibreNativeFfiHttpHeader* self, const char* value
);

/**
 * maplibre_native_ffi_http_header_get_value:
 *
 * The `value` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_http_header_get_value(
  const MaplibreNativeFfiHttpHeader* self
);

/**
 * maplibre_native_ffi_http_header_set_value:
 * @value: the UTF-8 text
 *
 * The `value` member.
 */
void maplibre_native_ffi_http_header_set_value(
  MaplibreNativeFfiHttpHeader* self, const char* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiHttpHeader, maplibre_native_ffi_http_header_free
)

/**
 * MaplibreNativeFfiHttpHeaderRule:
 *
 * Headers to add to the HTTP requests whose URL matches a pattern.
 *
 * See `mln_http_header_rule` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_http_header_rule_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_HTTP_HEADER_RULE \
  (maplibre_native_ffi_http_header_rule_get_type())

/**
 * maplibre_native_ffi_http_header_rule_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiHttpHeaderRule` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiHttpHeaderRule* maplibre_native_ffi_http_header_rule_new(void);

/**
 * maplibre_native_ffi_http_header_rule_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiHttpHeaderRule* maplibre_native_ffi_http_header_rule_copy(
  const MaplibreNativeFfiHttpHeaderRule* self
);

/**
 * maplibre_native_ffi_http_header_rule_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_http_header_rule_free(
  MaplibreNativeFfiHttpHeaderRule* self
);

/**
 * maplibre_native_ffi_http_header_rule_get_kinds:
 *
 * The kinds of resource that the rule covers. Zero covers every kind.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceKindFlag
maplibre_native_ffi_http_header_rule_get_kinds(
  const MaplibreNativeFfiHttpHeaderRule* self
);

/**
 * maplibre_native_ffi_http_header_rule_set_kinds:
 *
 * The kinds of resource that the rule covers. Zero covers every kind.
 */
void maplibre_native_ffi_http_header_rule_set_kinds(
  MaplibreNativeFfiHttpHeaderRule* self, MaplibreNativeFfiResourceKindFlag value
);

/**
 * maplibre_native_ffi_http_header_rule_get_flags:
 *
 * The `flags` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiUrlMatchFlag maplibre_native_ffi_http_header_rule_get_flags(
  const MaplibreNativeFfiHttpHeaderRule* self
);

/**
 * maplibre_native_ffi_http_header_rule_set_flags:
 *
 * The `flags` member.
 */
void maplibre_native_ffi_http_header_rule_set_flags(
  MaplibreNativeFfiHttpHeaderRule* self, MaplibreNativeFfiUrlMatchFlag value
);

/**
 * maplibre_native_ffi_http_header_rule_get_url:
 *
 * The `url` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_http_header_rule_get_url(
  const MaplibreNativeFfiHttpHeaderRule* self
);

/**
 * maplibre_native_ffi_http_header_rule_set_url:
 * @value: the UTF-8 text
 *
 * The `url` member.
 */
void maplibre_native_ffi_http_header_rule_set_url(
  MaplibreNativeFfiHttpHeaderRule* self, const char* value
);

/**
 * maplibre_native_ffi_http_header_rule_get_headers:
 *
 * The `headers` member.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiHttpHeader): the items
 */
GPtrArray* maplibre_native_ffi_http_header_rule_get_headers(
  const MaplibreNativeFfiHttpHeaderRule* self
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiHttpHeaderRule, maplibre_native_ffi_http_header_rule_free
)

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
 * The first rule that matches a request adds its headers, and no other.
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
GType maplibre_native_ffi_metal_context_descriptor_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_metal_context_descriptor_get_type())

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
 * maplibre_native_ffi_metal_context_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_context_descriptor_copy(
  const MaplibreNativeFfiMetalContextDescriptor* self
);

/**
 * maplibre_native_ffi_metal_context_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_metal_context_descriptor_free(
  MaplibreNativeFfiMetalContextDescriptor* self
);

/**
 * maplibre_native_ffi_metal_context_descriptor_get_device:
 *
 * An `MTLDevice`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_metal_context_descriptor_get_device(
  const MaplibreNativeFfiMetalContextDescriptor* self
);

/**
 * maplibre_native_ffi_metal_context_descriptor_set_device:
 *
 * An `MTLDevice`.
 */
void maplibre_native_ffi_metal_context_descriptor_set_device(
  MaplibreNativeFfiMetalContextDescriptor* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMetalContextDescriptor,
  maplibre_native_ffi_metal_context_descriptor_free
)

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
GType maplibre_native_ffi_io_surface_ring_descriptor_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_IO_SURFACE_RING_DESCRIPTOR \
  (maplibre_native_ffi_io_surface_ring_descriptor_get_type())

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
 * maplibre_native_ffi_io_surface_ring_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiIoSurfaceRingDescriptor*
maplibre_native_ffi_io_surface_ring_descriptor_copy(
  const MaplibreNativeFfiIoSurfaceRingDescriptor* self
);

/**
 * maplibre_native_ffi_io_surface_ring_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_io_surface_ring_descriptor_free(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self
);

/**
 * maplibre_native_ffi_io_surface_ring_descriptor_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_io_surface_ring_descriptor_get_logical_size(
  const MaplibreNativeFfiIoSurfaceRingDescriptor* self
);

/**
 * maplibre_native_ffi_io_surface_ring_descriptor_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_io_surface_ring_descriptor_set_logical_size(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_io_surface_ring_descriptor_get_buffer_count:
 *
 * How many surfaces the ring has, from 1 to 8.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_io_surface_ring_descriptor_get_buffer_count(
  const MaplibreNativeFfiIoSurfaceRingDescriptor* self
);

/**
 * maplibre_native_ffi_io_surface_ring_descriptor_set_buffer_count:
 *
 * How many surfaces the ring has, from 1 to 8.
 */
void maplibre_native_ffi_io_surface_ring_descriptor_set_buffer_count(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_io_surface_ring_descriptor_get_context:
 *
 * The MTLDevice to render with. A null device picks the system's default.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_io_surface_ring_descriptor_get_context(
  const MaplibreNativeFfiIoSurfaceRingDescriptor* self
);

/**
 * maplibre_native_ffi_io_surface_ring_descriptor_set_context:
 * @value: the value
 *
 * The MTLDevice to render with. A null device picks the system's default.
 */
void maplibre_native_ffi_io_surface_ring_descriptor_set_context(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self,
  const MaplibreNativeFfiMetalContextDescriptor* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiIoSurfaceRingDescriptor,
  maplibre_native_ffi_io_surface_ring_descriptor_free
)

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
GType maplibre_native_ffi_log_handler_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_LOG_HANDLER \
  (maplibre_native_ffi_log_handler_get_type())

/**
 * maplibre_native_ffi_log_handler_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiLogHandler` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiLogHandler* maplibre_native_ffi_log_handler_new(void);

/**
 * maplibre_native_ffi_log_handler_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiLogHandler* maplibre_native_ffi_log_handler_copy(
  const MaplibreNativeFfiLogHandler* self
);

/**
 * maplibre_native_ffi_log_handler_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_log_handler_free(MaplibreNativeFfiLogHandler* self);

/**
 * maplibre_native_ffi_log_handler_get_platform_severities:
 *
 * The severities that also go to the platform's log, as they do without a
 * callback. 0 sends none. A message of these severities goes to the platform's
 * log even when the handler's queue can't accept it.
 *
 * Returns: the value
 */
MaplibreNativeFfiLogSeverityFlag
maplibre_native_ffi_log_handler_get_platform_severities(
  const MaplibreNativeFfiLogHandler* self
);

/**
 * maplibre_native_ffi_log_handler_set_platform_severities:
 *
 * The severities that also go to the platform's log, as they do without a
 * callback. 0 sends none. A message of these severities goes to the platform's
 * log even when the handler's queue can't accept it.
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiLogHandler, maplibre_native_ffi_log_handler_free
)

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
 * See `mln_map_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_map_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_OPTIONS \
  (maplibre_native_ffi_map_options_get_type())

/**
 * maplibre_native_ffi_map_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMapOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMapOptions* maplibre_native_ffi_map_options_new(void);

/**
 * maplibre_native_ffi_map_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMapOptions* maplibre_native_ffi_map_options_copy(
  const MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_map_options_free(MaplibreNativeFfiMapOptions* self);

/**
 * maplibre_native_ffi_map_options_get_initial_extent:
 *
 * The map's first size, and its scale factor. The scale factor stays fixed for
 * the map's life, and every render session of the map renders at it.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalExtent*
maplibre_native_ffi_map_options_get_initial_extent(
  const MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_set_initial_extent:
 * @value: the value
 *
 * The map's first size, and its scale factor. The scale factor stays fixed for
 * the map's life, and every render session of the map renders at it.
 */
void maplibre_native_ffi_map_options_set_initial_extent(
  MaplibreNativeFfiMapOptions* self, const MaplibreNativeFfiLogicalExtent* value
);

/**
 * maplibre_native_ffi_map_options_get_map_mode:
 *
 * The `map_mode` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiMapMode maplibre_native_ffi_map_options_get_map_mode(
  const MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_set_map_mode:
 *
 * The `map_mode` member.
 */
void maplibre_native_ffi_map_options_set_map_mode(
  MaplibreNativeFfiMapOptions* self, MaplibreNativeFfiMapMode value
);

/**
 * maplibre_native_ffi_map_options_get_fast_pfor_enabled:
 *
 * Whether the map decodes MapLibre Tiles (MLT) that use FastPFOR integer
 * encoding. Without it, such tiles fail to load.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_map_options_get_fast_pfor_enabled(
  const MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_set_fast_pfor_enabled:
 *
 * Whether the map decodes MapLibre Tiles (MLT) that use FastPFOR integer
 * encoding. Without it, such tiles fail to load.
 */
void maplibre_native_ffi_map_options_set_fast_pfor_enabled(
  MaplibreNativeFfiMapOptions* self, gboolean value
);

/**
 * maplibre_native_ffi_map_options_get_event_mask:
 *
 * The events that the map reports. Without it, the map reports them all.
 *
 * Returns: the value
 */
MaplibreNativeFfiRuntimeEventFlag
maplibre_native_ffi_map_options_get_event_mask(
  const MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_has_event_mask:
 *
 * Whether the value sets `maplibre_native_ffi_map_options_set_event_mask`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_options_has_event_mask(
  const MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_set_event_mask:
 *
 * The events that the map reports. Without it, the map reports them all.
 */
void maplibre_native_ffi_map_options_set_event_mask(
  MaplibreNativeFfiMapOptions* self, MaplibreNativeFfiRuntimeEventFlag value
);

/**
 * maplibre_native_ffi_map_options_get_tile_cache_enabled:
 *
 * Whether the map keeps tiles in memory after they leave the view, so they show
 * again without loading. Without it, the map keeps them.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_map_options_get_tile_cache_enabled(
  const MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_has_tile_cache_enabled:
 *
 * Whether the value sets `maplibre_native_ffi_map_options_set_tile_cache_enabled`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_options_has_tile_cache_enabled(
  const MaplibreNativeFfiMapOptions* self
);

/**
 * maplibre_native_ffi_map_options_set_tile_cache_enabled:
 *
 * Whether the map keeps tiles in memory after they leave the view, so they show
 * again without loading. Without it, the map keeps them.
 */
void maplibre_native_ffi_map_options_set_tile_cache_enabled(
  MaplibreNativeFfiMapOptions* self, gboolean value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMapOptions, maplibre_native_ffi_map_options_free
)

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
 * See `mln_map_tile_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_map_tile_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_TILE_OPTIONS \
  (maplibre_native_ffi_map_tile_options_get_type())

/**
 * maplibre_native_ffi_map_tile_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMapTileOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMapTileOptions* maplibre_native_ffi_map_tile_options_new(void);

/**
 * maplibre_native_ffi_map_tile_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMapTileOptions* maplibre_native_ffi_map_tile_options_copy(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_map_tile_options_free(
  MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_get_prefetch_zoom_delta:
 *
 * How many zoom levels lower the map also loads tiles for, so it can show a
 * coarse map while the current tiles load. 0 loads only the current tiles.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_map_tile_options_get_prefetch_zoom_delta(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_prefetch_zoom_delta:
 *
 * Whether the value sets `maplibre_native_ffi_map_tile_options_set_prefetch_zoom_delta`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_prefetch_zoom_delta(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_prefetch_zoom_delta:
 *
 * How many zoom levels lower the map also loads tiles for, so it can show a
 * coarse map while the current tiles load. 0 loads only the current tiles.
 */
void maplibre_native_ffi_map_tile_options_set_prefetch_zoom_delta(
  MaplibreNativeFfiMapTileOptions* self, guint32 value
);

/**
 * maplibre_native_ffi_map_tile_options_get_lod_min_radius:
 *
 * The radius around the view point, in tiles, that keeps full detail.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_map_tile_options_get_lod_min_radius(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_lod_min_radius:
 *
 * Whether the value sets `maplibre_native_ffi_map_tile_options_set_lod_min_radius`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_lod_min_radius(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_lod_min_radius:
 *
 * The radius around the view point, in tiles, that keeps full detail.
 */
void maplibre_native_ffi_map_tile_options_set_lod_min_radius(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_map_tile_options_get_lod_scale:
 *
 * Scales the distance to the view point. Above 1 lowers the detail.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_map_tile_options_get_lod_scale(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_lod_scale:
 *
 * Whether the value sets `maplibre_native_ffi_map_tile_options_set_lod_scale`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_lod_scale(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_lod_scale:
 *
 * Scales the distance to the view point. Above 1 lowers the detail.
 */
void maplibre_native_ffi_map_tile_options_set_lod_scale(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_map_tile_options_get_lod_pitch_threshold:
 *
 * The pitch, in degrees, above which the map lowers the detail.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_map_tile_options_get_lod_pitch_threshold(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_lod_pitch_threshold:
 *
 * Whether the value sets `maplibre_native_ffi_map_tile_options_set_lod_pitch_threshold`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_lod_pitch_threshold(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_lod_pitch_threshold:
 *
 * The pitch, in degrees, above which the map lowers the detail.
 */
void maplibre_native_ffi_map_tile_options_set_lod_pitch_threshold(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_map_tile_options_get_lod_zoom_shift:
 *
 * Zoom levels to add for picking the detail. Below 0 loads fewer tiles.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_map_tile_options_get_lod_zoom_shift(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_lod_zoom_shift:
 *
 * Whether the value sets `maplibre_native_ffi_map_tile_options_set_lod_zoom_shift`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_lod_zoom_shift(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_lod_zoom_shift:
 *
 * Zoom levels to add for picking the detail. Below 0 loads fewer tiles.
 */
void maplibre_native_ffi_map_tile_options_set_lod_zoom_shift(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_map_tile_options_get_lod_mode:
 *
 * `MAPLIBRE_NATIVE_FFI_TILE_LOD_MODE_DISTANCE` ignores `lod_min_radius`.
 *
 * Returns: the value
 */
MaplibreNativeFfiTileLodMode maplibre_native_ffi_map_tile_options_get_lod_mode(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_has_lod_mode:
 *
 * Whether the value sets `maplibre_native_ffi_map_tile_options_set_lod_mode`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_tile_options_has_lod_mode(
  const MaplibreNativeFfiMapTileOptions* self
);

/**
 * maplibre_native_ffi_map_tile_options_set_lod_mode:
 *
 * `MAPLIBRE_NATIVE_FFI_TILE_LOD_MODE_DISTANCE` ignores `lod_min_radius`.
 */
void maplibre_native_ffi_map_tile_options_set_lod_mode(
  MaplibreNativeFfiMapTileOptions* self, MaplibreNativeFfiTileLodMode value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMapTileOptions, maplibre_native_ffi_map_tile_options_free
)

/**
 * MaplibreNativeFfiMapViewportOptions:
 *
 * How the map orients and constrains its view.
 *
 * See `mln_map_viewport_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_map_viewport_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_VIEWPORT_OPTIONS \
  (maplibre_native_ffi_map_viewport_options_get_type())

/**
 * maplibre_native_ffi_map_viewport_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiMapViewportOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiMapViewportOptions*
maplibre_native_ffi_map_viewport_options_new(void);

/**
 * maplibre_native_ffi_map_viewport_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMapViewportOptions*
maplibre_native_ffi_map_viewport_options_copy(
  const MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_map_viewport_options_free(
  MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_get_north_orientation:
 *
 * The `north_orientation` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiNorthOrientation
maplibre_native_ffi_map_viewport_options_get_north_orientation(
  const MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_has_north_orientation:
 *
 * Whether the value sets `maplibre_native_ffi_map_viewport_options_set_north_orientation`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_viewport_options_has_north_orientation(
  const MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_set_north_orientation:
 *
 * The `north_orientation` member.
 */
void maplibre_native_ffi_map_viewport_options_set_north_orientation(
  MaplibreNativeFfiMapViewportOptions* self,
  MaplibreNativeFfiNorthOrientation value
);

/**
 * maplibre_native_ffi_map_viewport_options_get_constrain_mode:
 *
 * The `constrain_mode` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiConstrainMode
maplibre_native_ffi_map_viewport_options_get_constrain_mode(
  const MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_has_constrain_mode:
 *
 * Whether the value sets `maplibre_native_ffi_map_viewport_options_set_constrain_mode`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_viewport_options_has_constrain_mode(
  const MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_set_constrain_mode:
 *
 * The `constrain_mode` member.
 */
void maplibre_native_ffi_map_viewport_options_set_constrain_mode(
  MaplibreNativeFfiMapViewportOptions* self,
  MaplibreNativeFfiConstrainMode value
);

/**
 * maplibre_native_ffi_map_viewport_options_get_viewport_mode:
 *
 * The `viewport_mode` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiViewportMode
maplibre_native_ffi_map_viewport_options_get_viewport_mode(
  const MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_has_viewport_mode:
 *
 * Whether the value sets `maplibre_native_ffi_map_viewport_options_set_viewport_mode`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_viewport_options_has_viewport_mode(
  const MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_set_viewport_mode:
 *
 * The `viewport_mode` member.
 */
void maplibre_native_ffi_map_viewport_options_set_viewport_mode(
  MaplibreNativeFfiMapViewportOptions* self, MaplibreNativeFfiViewportMode value
);

/**
 * maplibre_native_ffi_map_viewport_options_get_frustum_offset:
 *
 * The `frustum_offset` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiEdgeInsets*
maplibre_native_ffi_map_viewport_options_get_frustum_offset(
  const MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_has_frustum_offset:
 *
 * Whether the value sets `maplibre_native_ffi_map_viewport_options_set_frustum_offset`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_map_viewport_options_has_frustum_offset(
  const MaplibreNativeFfiMapViewportOptions* self
);

/**
 * maplibre_native_ffi_map_viewport_options_set_frustum_offset:
 * @value: the value
 *
 * The `frustum_offset` member.
 */
void maplibre_native_ffi_map_viewport_options_set_frustum_offset(
  MaplibreNativeFfiMapViewportOptions* self,
  const MaplibreNativeFfiEdgeInsets* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMapViewportOptions,
  maplibre_native_ffi_map_viewport_options_free
)

/**
 * MaplibreNativeFfiMetalBorrowedTexture:
 *
 * A Metal texture of a borrowed ring.
 *
 * See `mln_metal_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
GType maplibre_native_ffi_metal_borrowed_texture_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_BORROWED_TEXTURE \
  (maplibre_native_ffi_metal_borrowed_texture_get_type())

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
 * maplibre_native_ffi_metal_borrowed_texture_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMetalBorrowedTexture*
maplibre_native_ffi_metal_borrowed_texture_copy(
  const MaplibreNativeFfiMetalBorrowedTexture* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_metal_borrowed_texture_free(
  MaplibreNativeFfiMetalBorrowedTexture* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_get_texture:
 *
 * Your MTLTexture.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_metal_borrowed_texture_get_texture(
  const MaplibreNativeFfiMetalBorrowedTexture* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_set_texture:
 *
 * Your MTLTexture.
 */
void maplibre_native_ffi_metal_borrowed_texture_set_texture(
  MaplibreNativeFfiMetalBorrowedTexture* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMetalBorrowedTexture,
  maplibre_native_ffi_metal_borrowed_texture_free
)

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
GType maplibre_native_ffi_metal_borrowed_texture_descriptor_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_BORROWED_TEXTURE_DESCRIPTOR \
  (maplibre_native_ffi_metal_borrowed_texture_descriptor_get_type())

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
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMetalBorrowedTextureDescriptor*
maplibre_native_ffi_metal_borrowed_texture_descriptor_copy(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_metal_borrowed_texture_descriptor_free(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_logical_size(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_width:
 *
 * The `physical_width` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_width(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_width:
 *
 * The `physical_width` member.
 */
void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_height:
 *
 * The `physical_height` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_height(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_height:
 *
 * The `physical_height` member.
 */
void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_get_textures:
 *
 * The `textures` member.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiMetalBorrowedTexture): the items
 */
GPtrArray* maplibre_native_ffi_metal_borrowed_texture_descriptor_get_textures(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
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
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_get_command_queue:
 *
 * Your MTLCommandQueue on the textures' device, or null for a queue of the
 * session's own. With your queue, frames can use
 * `MAPLIBRE_NATIVE_FFI_GPU_SYNC_QUEUE_ORDER`.
 *
 * Returns: the value
 */
gpointer
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_command_queue(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_descriptor_set_command_queue:
 *
 * Your MTLCommandQueue on the textures' device, or null for a queue of the
 * session's own. With your queue, frames can use
 * `MAPLIBRE_NATIVE_FFI_GPU_SYNC_QUEUE_ORDER`.
 */
void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_command_queue(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor,
  maplibre_native_ffi_metal_borrowed_texture_descriptor_free
)

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
GType maplibre_native_ffi_metal_borrowed_texture_replacement_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_BORROWED_TEXTURE_REPLACEMENT \
  (maplibre_native_ffi_metal_borrowed_texture_replacement_get_type())

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
 * maplibre_native_ffi_metal_borrowed_texture_replacement_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMetalBorrowedTextureReplacement*
maplibre_native_ffi_metal_borrowed_texture_replacement_copy(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_metal_borrowed_texture_replacement_free(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_borrowed_texture_replacement_get_logical_size(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_metal_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_width:
 *
 * The `physical_width` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_width(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_width:
 *
 * The `physical_width` member.
 */
void maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self, guint32 value
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_height:
 *
 * The `physical_height` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_height(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_height:
 *
 * The `physical_height` member.
 */
void maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self, guint32 value
);

/**
 * maplibre_native_ffi_metal_borrowed_texture_replacement_get_textures:
 *
 * The `textures` member.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiMetalBorrowedTexture): the items
 */
GPtrArray* maplibre_native_ffi_metal_borrowed_texture_replacement_get_textures(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMetalBorrowedTextureReplacement,
  maplibre_native_ffi_metal_borrowed_texture_replacement_free
)

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
GType maplibre_native_ffi_metal_surface_descriptor_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_SURFACE_DESCRIPTOR \
  (maplibre_native_ffi_metal_surface_descriptor_get_type())

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
 * maplibre_native_ffi_metal_surface_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMetalSurfaceDescriptor*
maplibre_native_ffi_metal_surface_descriptor_copy(
  const MaplibreNativeFfiMetalSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_metal_surface_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_metal_surface_descriptor_free(
  MaplibreNativeFfiMetalSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_metal_surface_descriptor_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_surface_descriptor_get_logical_size(
  const MaplibreNativeFfiMetalSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_metal_surface_descriptor_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_metal_surface_descriptor_set_logical_size(
  MaplibreNativeFfiMetalSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_metal_surface_descriptor_get_context:
 *
 * The MTLDevice to render with. A null device picks the system's default.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_surface_descriptor_get_context(
  const MaplibreNativeFfiMetalSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_metal_surface_descriptor_set_context:
 * @value: the value
 *
 * The MTLDevice to render with. A null device picks the system's default.
 */
void maplibre_native_ffi_metal_surface_descriptor_set_context(
  MaplibreNativeFfiMetalSurfaceDescriptor* self,
  const MaplibreNativeFfiMetalContextDescriptor* value
);

/**
 * maplibre_native_ffi_metal_surface_descriptor_get_layer:
 *
 * Your CAMetalLayer.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_metal_surface_descriptor_get_layer(
  const MaplibreNativeFfiMetalSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_metal_surface_descriptor_set_layer:
 *
 * Your CAMetalLayer.
 */
void maplibre_native_ffi_metal_surface_descriptor_set_layer(
  MaplibreNativeFfiMetalSurfaceDescriptor* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMetalSurfaceDescriptor,
  maplibre_native_ffi_metal_surface_descriptor_free
)

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
GType maplibre_native_ffi_metal_surface_replacement_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_METAL_SURFACE_REPLACEMENT \
  (maplibre_native_ffi_metal_surface_replacement_get_type())

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
 * maplibre_native_ffi_metal_surface_replacement_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiMetalSurfaceReplacement*
maplibre_native_ffi_metal_surface_replacement_copy(
  const MaplibreNativeFfiMetalSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_metal_surface_replacement_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_metal_surface_replacement_free(
  MaplibreNativeFfiMetalSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_metal_surface_replacement_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_surface_replacement_get_logical_size(
  const MaplibreNativeFfiMetalSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_metal_surface_replacement_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_metal_surface_replacement_set_logical_size(
  MaplibreNativeFfiMetalSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_metal_surface_replacement_get_layer:
 *
 * Your CAMetalLayer.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_metal_surface_replacement_get_layer(
  const MaplibreNativeFfiMetalSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_metal_surface_replacement_set_layer:
 *
 * Your CAMetalLayer.
 */
void maplibre_native_ffi_metal_surface_replacement_set_layer(
  MaplibreNativeFfiMetalSurfaceReplacement* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiMetalSurfaceReplacement,
  maplibre_native_ffi_metal_surface_replacement_free
)

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
GType maplibre_native_ffi_offline_geometry_region_definition_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFLINE_GEOMETRY_REGION_DEFINITION \
  (maplibre_native_ffi_offline_geometry_region_definition_get_type())

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
 * maplibre_native_ffi_offline_geometry_region_definition_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOfflineGeometryRegionDefinition*
maplibre_native_ffi_offline_geometry_region_definition_copy(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_offline_geometry_region_definition_free(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_style_url:
 *
 * The `style_url` member.
 *
 * Returns: (transfer none): the text
 */
const char*
maplibre_native_ffi_offline_geometry_region_definition_get_style_url(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_style_url:
 * @value: the UTF-8 text
 *
 * The `style_url` member.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_style_url(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, const char* value
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_geometry:
 *
 * The `geometry` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_offline_geometry_region_definition_get_geometry(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_geometry:
 * @value: the UTF-8 text
 *
 * The `geometry` member.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_geometry(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, const char* value
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_min_zoom:
 *
 * The `min_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_offline_geometry_region_definition_get_min_zoom(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_min_zoom:
 *
 * The `min_zoom` member.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_min_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gdouble value
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_max_zoom:
 *
 * The `max_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_offline_geometry_region_definition_get_max_zoom(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_max_zoom:
 *
 * The `max_zoom` member.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_max_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gdouble value
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_pixel_ratio:
 *
 * The `pixel_ratio` member.
 *
 * Returns: the value
 */
gfloat maplibre_native_ffi_offline_geometry_region_definition_get_pixel_ratio(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_pixel_ratio:
 *
 * The `pixel_ratio` member.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_pixel_ratio(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gfloat value
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_get_include_ideographs:
 *
 * The `include_ideographs` member.
 *
 * Returns: the value
 */
gboolean
maplibre_native_ffi_offline_geometry_region_definition_get_include_ideographs(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_geometry_region_definition_set_include_ideographs:
 *
 * The `include_ideographs` member.
 */
void maplibre_native_ffi_offline_geometry_region_definition_set_include_ideographs(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gboolean value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOfflineGeometryRegionDefinition,
  maplibre_native_ffi_offline_geometry_region_definition_free
)

/**
 * MaplibreNativeFfiOfflineTilePyramidRegionDefinition:
 *
 * An offline region that covers a rectangle of the map.
 *
 * See `mln_offline_tile_pyramid_region_definition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_offline_tile_pyramid_region_definition_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFLINE_TILE_PYRAMID_REGION_DEFINITION \
  (maplibre_native_ffi_offline_tile_pyramid_region_definition_get_type())

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
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOfflineTilePyramidRegionDefinition*
maplibre_native_ffi_offline_tile_pyramid_region_definition_copy(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_free(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_style_url:
 *
 * The style whose resources the region holds. Maps use the region when they
 * load the same style URL.
 *
 * Returns: (transfer none): the text
 */
const char*
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_style_url(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_style_url:
 * @value: the UTF-8 text
 *
 * The style whose resources the region holds. Maps use the region when they
 * load the same style URL.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_style_url(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, const char* value
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_bounds:
 *
 * The `bounds` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLatLngBounds*
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_bounds(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_bounds:
 * @value: the value
 *
 * The `bounds` member.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_bounds(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self,
  const MaplibreNativeFfiLatLngBounds* value
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_min_zoom:
 *
 * The `min_zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_offline_tile_pyramid_region_definition_get_min_zoom(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_min_zoom:
 *
 * The `min_zoom` member.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_min_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gdouble value
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_max_zoom:
 *
 * INFINITY covers every zoom level that the style's sources have.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_offline_tile_pyramid_region_definition_get_max_zoom(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_max_zoom:
 *
 * INFINITY covers every zoom level that the style's sources have.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_max_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gdouble value
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_pixel_ratio:
 *
 * The pixel ratio of the images to download, such as 2 for dense screens.
 *
 * Returns: the value
 */
gfloat
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_pixel_ratio(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_pixel_ratio:
 *
 * The pixel ratio of the images to download, such as 2 for dense screens.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_pixel_ratio(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gfloat value
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_get_include_ideographs:
 *
 * Whether the region holds the glyphs of CJK ideographs.
 *
 * Returns: the value
 */
gboolean
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_include_ideographs(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
);

/**
 * maplibre_native_ffi_offline_tile_pyramid_region_definition_set_include_ideographs:
 *
 * Whether the region holds the glyphs of CJK ideographs.
 */
void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_include_ideographs(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gboolean value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition,
  maplibre_native_ffi_offline_tile_pyramid_region_definition_free
)

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
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* value
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
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* value
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
 * See `mln_offscreen_device_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
 */
GType maplibre_native_ffi_offscreen_device_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFSCREEN_DEVICE_OPTIONS \
  (maplibre_native_ffi_offscreen_device_options_get_type())

/**
 * maplibre_native_ffi_offscreen_device_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiOffscreenDeviceOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiOffscreenDeviceOptions*
maplibre_native_ffi_offscreen_device_options_new(void);

/**
 * maplibre_native_ffi_offscreen_device_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOffscreenDeviceOptions*
maplibre_native_ffi_offscreen_device_options_copy(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_offscreen_device_options_free(
  MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_get_adapter_index:
 *
 * The GPU to use, as an index into the platform's list of GPUs.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_offscreen_device_options_get_adapter_index(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_has_adapter_index:
 *
 * Whether the value sets `maplibre_native_ffi_offscreen_device_options_set_adapter_index`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_offscreen_device_options_has_adapter_index(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_set_adapter_index:
 *
 * The GPU to use, as an index into the platform's list of GPUs.
 */
void maplibre_native_ffi_offscreen_device_options_set_adapter_index(
  MaplibreNativeFfiOffscreenDeviceOptions* self, guint32 value
);

/**
 * maplibre_native_ffi_offscreen_device_options_get_single_context:
 *
 * Whether the device's sessions take turns in one OpenGL context instead of
 * having one each. Only an EGL device reads it.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_offscreen_device_options_get_single_context(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_has_single_context:
 *
 * Whether the value sets `maplibre_native_ffi_offscreen_device_options_set_single_context`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_offscreen_device_options_has_single_context(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_set_single_context:
 *
 * Whether the device's sessions take turns in one OpenGL context instead of
 * having one each. Only an EGL device reads it.
 */
void maplibre_native_ffi_offscreen_device_options_set_single_context(
  MaplibreNativeFfiOffscreenDeviceOptions* self, gboolean value
);

/**
 * maplibre_native_ffi_offscreen_device_options_get_client_api:
 *
 * The OpenGL API that the device's sessions render with. An EGL device defaults
 * to `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GLES`, and takes
 * `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GL` too on Linux and FreeBSD. A WGL
 * device takes only `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GL`, its default,
 * and a WebGL device only `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GLES`, its
 * default. Other devices take neither.
 *
 * Returns: the value
 */
MaplibreNativeFfiOpenGlClientApi
maplibre_native_ffi_offscreen_device_options_get_client_api(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_has_client_api:
 *
 * Whether the value sets `maplibre_native_ffi_offscreen_device_options_set_client_api`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_offscreen_device_options_has_client_api(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
);

/**
 * maplibre_native_ffi_offscreen_device_options_set_client_api:
 *
 * The OpenGL API that the device's sessions render with. An EGL device defaults
 * to `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GLES`, and takes
 * `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GL` too on Linux and FreeBSD. A WGL
 * device takes only `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GL`, its default,
 * and a WebGL device only `MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GLES`, its
 * default. Other devices take neither.
 */
void maplibre_native_ffi_offscreen_device_options_set_client_api(
  MaplibreNativeFfiOffscreenDeviceOptions* self,
  MaplibreNativeFfiOpenGlClientApi value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOffscreenDeviceOptions,
  maplibre_native_ffi_offscreen_device_options_free
)

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
GType maplibre_native_ffi_wgl_context_descriptor_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_WGL_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_wgl_context_descriptor_get_type())

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
 * maplibre_native_ffi_wgl_context_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiWglContextDescriptor*
maplibre_native_ffi_wgl_context_descriptor_copy(
  const MaplibreNativeFfiWglContextDescriptor* self
);

/**
 * maplibre_native_ffi_wgl_context_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_wgl_context_descriptor_free(
  MaplibreNativeFfiWglContextDescriptor* self
);

/**
 * maplibre_native_ffi_wgl_context_descriptor_get_device_context:
 *
 * An `HDC`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wgl_context_descriptor_get_device_context(
  const MaplibreNativeFfiWglContextDescriptor* self
);

/**
 * maplibre_native_ffi_wgl_context_descriptor_set_device_context:
 *
 * An `HDC`.
 */
void maplibre_native_ffi_wgl_context_descriptor_set_device_context(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
);

/**
 * maplibre_native_ffi_wgl_context_descriptor_get_share_context:
 *
 * Your `HGLRC`, for a shared context.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wgl_context_descriptor_get_share_context(
  const MaplibreNativeFfiWglContextDescriptor* self
);

/**
 * maplibre_native_ffi_wgl_context_descriptor_set_share_context:
 *
 * Your `HGLRC`, for a shared context.
 */
void maplibre_native_ffi_wgl_context_descriptor_set_share_context(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
);

/**
 * maplibre_native_ffi_wgl_context_descriptor_get_get_proc_address:
 *
 * Your `wglGetProcAddress`, or null to use the system's.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wgl_context_descriptor_get_get_proc_address(
  const MaplibreNativeFfiWglContextDescriptor* self
);

/**
 * maplibre_native_ffi_wgl_context_descriptor_set_get_proc_address:
 *
 * Your `wglGetProcAddress`, or null to use the system's.
 */
void maplibre_native_ffi_wgl_context_descriptor_set_get_proc_address(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiWglContextDescriptor,
  maplibre_native_ffi_wgl_context_descriptor_free
)

/**
 * MaplibreNativeFfiWebGlContextDescriptor:
 *
 * The WebGL context or canvas that a render session uses.
 *
 * See `mln_web_gl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
GType maplibre_native_ffi_web_gl_context_descriptor_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_WEB_GL_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_web_gl_context_descriptor_get_type())

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
 * maplibre_native_ffi_web_gl_context_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiWebGlContextDescriptor*
maplibre_native_ffi_web_gl_context_descriptor_copy(
  const MaplibreNativeFfiWebGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_web_gl_context_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_web_gl_context_descriptor_free(
  MaplibreNativeFfiWebGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_web_gl_context_descriptor_get_kind:
 *
 * The `kind` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiWebGlContextKind
maplibre_native_ffi_web_gl_context_descriptor_get_kind(
  const MaplibreNativeFfiWebGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_web_gl_context_descriptor_set_kind:
 *
 * The `kind` member.
 */
void maplibre_native_ffi_web_gl_context_descriptor_set_kind(
  MaplibreNativeFfiWebGlContextDescriptor* self,
  MaplibreNativeFfiWebGlContextKind value
);

/**
 * maplibre_native_ffi_web_gl_context_descriptor_get_context:
 *
 * Your `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, for an existing context.
 *
 * Returns: the value
 */
gint32 maplibre_native_ffi_web_gl_context_descriptor_get_context(
  const MaplibreNativeFfiWebGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_web_gl_context_descriptor_set_context:
 *
 * Your `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, for an existing context.
 */
void maplibre_native_ffi_web_gl_context_descriptor_set_context(
  MaplibreNativeFfiWebGlContextDescriptor* self, gint32 value
);

/**
 * maplibre_native_ffi_web_gl_context_descriptor_get_canvas_selector:
 *
 * The canvas's CSS selector, for a transferred canvas.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_web_gl_context_descriptor_get_canvas_selector(
  const MaplibreNativeFfiWebGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_web_gl_context_descriptor_set_canvas_selector:
 * @value: the UTF-8 text
 *
 * The canvas's CSS selector, for a transferred canvas.
 */
void maplibre_native_ffi_web_gl_context_descriptor_set_canvas_selector(
  MaplibreNativeFfiWebGlContextDescriptor* self, const char* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiWebGlContextDescriptor,
  maplibre_native_ffi_web_gl_context_descriptor_free
)

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
  const MaplibreNativeFfiWglContextDescriptor* value
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
  const MaplibreNativeFfiEglContextDescriptor* value
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
  const MaplibreNativeFfiWebGlContextDescriptor* value
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
GType maplibre_native_ffi_open_gl_context_descriptor_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_open_gl_context_descriptor_get_type())

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
 * maplibre_native_ffi_open_gl_context_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_copy(
  const MaplibreNativeFfiOpenGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_open_gl_context_descriptor_free(
  MaplibreNativeFfiOpenGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_get_ownership:
 *
 * The `ownership` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiOpenGlContextOwnership
maplibre_native_ffi_open_gl_context_descriptor_get_ownership(
  const MaplibreNativeFfiOpenGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_set_ownership:
 *
 * The `ownership` member.
 */
void maplibre_native_ffi_open_gl_context_descriptor_set_ownership(
  MaplibreNativeFfiOpenGlContextDescriptor* self,
  MaplibreNativeFfiOpenGlContextOwnership value
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_get_data:
 *
 * The `data` member.
 *
 * Returns: (transfer none) (nullable): the value
 */
MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_get_data(
  const MaplibreNativeFfiOpenGlContextDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_context_descriptor_set_data:
 * @value: the value
 *
 * The `data` member.
 */
void maplibre_native_ffi_open_gl_context_descriptor_set_data(
  MaplibreNativeFfiOpenGlContextDescriptor* self,
  const MaplibreNativeFfiOpenGlContextDescriptorData* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOpenGlContextDescriptor,
  maplibre_native_ffi_open_gl_context_descriptor_free
)

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
GType maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR \
  (maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_type())

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
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOpenGlBorrowedTextureDescriptor*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_copy(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_free(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_logical_size(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_width:
 *
 * The `physical_width` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_width(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_width:
 *
 * The `physical_width` member.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_height:
 *
 * The `physical_height` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_height(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_height:
 *
 * The `physical_height` member.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_context:
 *
 * The `context` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_context(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_context:
 * @value: the value
 *
 * The `context` member.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_context(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiOpenGlContextDescriptor* value
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_textures:
 *
 * The `textures` member.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiOpenGlBorrowedTexture): the items
 */
GPtrArray* maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_textures(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
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
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_target:
 *
 * The textures' target. It must be GL_TEXTURE_2D.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_target(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_target:
 *
 * The textures' target. It must be GL_TEXTURE_2D.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_target(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor,
  maplibre_native_ffi_open_gl_borrowed_texture_descriptor_free
)

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
GType maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT \
  (maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_type())

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
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOpenGlBorrowedTextureReplacement*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_copy(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_replacement_free(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_logical_size(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_width:
 *
 * The `physical_width` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_width(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_width:
 *
 * The `physical_width` member.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self, guint32 value
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_height:
 *
 * The `physical_height` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_height(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_height:
 *
 * The `physical_height` member.
 */
void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self, guint32 value
);

/**
 * maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_textures:
 *
 * The `textures` member.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiOpenGlBorrowedTexture): the items
 */
GPtrArray*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_textures(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement,
  maplibre_native_ffi_open_gl_borrowed_texture_replacement_free
)

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
GType maplibre_native_ffi_open_gl_surface_descriptor_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_SURFACE_DESCRIPTOR \
  (maplibre_native_ffi_open_gl_surface_descriptor_get_type())

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
 * maplibre_native_ffi_open_gl_surface_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOpenGlSurfaceDescriptor*
maplibre_native_ffi_open_gl_surface_descriptor_copy(
  const MaplibreNativeFfiOpenGlSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_surface_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_open_gl_surface_descriptor_free(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_surface_descriptor_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_surface_descriptor_get_logical_size(
  const MaplibreNativeFfiOpenGlSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_surface_descriptor_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_open_gl_surface_descriptor_set_logical_size(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_open_gl_surface_descriptor_get_context:
 *
 * The `context` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_surface_descriptor_get_context(
  const MaplibreNativeFfiOpenGlSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_surface_descriptor_set_context:
 * @value: the value
 *
 * The `context` member.
 */
void maplibre_native_ffi_open_gl_surface_descriptor_set_context(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self,
  const MaplibreNativeFfiOpenGlContextDescriptor* value
);

/**
 * maplibre_native_ffi_open_gl_surface_descriptor_get_surface:
 *
 * Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_open_gl_surface_descriptor_get_surface(
  const MaplibreNativeFfiOpenGlSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_open_gl_surface_descriptor_set_surface:
 *
 * Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it.
 */
void maplibre_native_ffi_open_gl_surface_descriptor_set_surface(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOpenGlSurfaceDescriptor,
  maplibre_native_ffi_open_gl_surface_descriptor_free
)

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
GType maplibre_native_ffi_open_gl_surface_replacement_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_SURFACE_REPLACEMENT \
  (maplibre_native_ffi_open_gl_surface_replacement_get_type())

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
 * maplibre_native_ffi_open_gl_surface_replacement_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiOpenGlSurfaceReplacement*
maplibre_native_ffi_open_gl_surface_replacement_copy(
  const MaplibreNativeFfiOpenGlSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_surface_replacement_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_open_gl_surface_replacement_free(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_surface_replacement_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_surface_replacement_get_logical_size(
  const MaplibreNativeFfiOpenGlSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_surface_replacement_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_open_gl_surface_replacement_set_logical_size(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_open_gl_surface_replacement_get_surface:
 *
 * Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it, so
 * leave it null.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_open_gl_surface_replacement_get_surface(
  const MaplibreNativeFfiOpenGlSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_open_gl_surface_replacement_set_surface:
 *
 * Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it, so
 * leave it null.
 */
void maplibre_native_ffi_open_gl_surface_replacement_set_surface(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiOpenGlSurfaceReplacement,
  maplibre_native_ffi_open_gl_surface_replacement_free
)

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
GType maplibre_native_ffi_premultiplied_rgba8_image_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_PREMULTIPLIED_RGBA8_IMAGE \
  (maplibre_native_ffi_premultiplied_rgba8_image_get_type())

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
 * maplibre_native_ffi_premultiplied_rgba8_image_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiPremultipliedRgba8Image*
maplibre_native_ffi_premultiplied_rgba8_image_copy(
  const MaplibreNativeFfiPremultipliedRgba8Image* self
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_premultiplied_rgba8_image_free(
  MaplibreNativeFfiPremultipliedRgba8Image* self
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_get_width:
 *
 * The `width` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_width(
  const MaplibreNativeFfiPremultipliedRgba8Image* self
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_set_width:
 *
 * The `width` member.
 */
void maplibre_native_ffi_premultiplied_rgba8_image_set_width(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_get_height:
 *
 * The `height` member.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_height(
  const MaplibreNativeFfiPremultipliedRgba8Image* self
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_set_height:
 *
 * The `height` member.
 */
void maplibre_native_ffi_premultiplied_rgba8_image_set_height(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_get_stride:
 *
 * The number of bytes from the start of one row to the start of the next, at
 * least `width` \* 4.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_stride(
  const MaplibreNativeFfiPremultipliedRgba8Image* self
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_set_stride:
 *
 * The number of bytes from the start of one row to the start of the next, at
 * least `width` \* 4.
 */
void maplibre_native_ffi_premultiplied_rgba8_image_set_stride(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_get_pixels:
 *
 * The rows of pixels, `stride` bytes apart, from the top row down.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_premultiplied_rgba8_image_get_pixels(
  const MaplibreNativeFfiPremultipliedRgba8Image* self
);

/**
 * maplibre_native_ffi_premultiplied_rgba8_image_set_pixels:
 * @value: (nullable): the bytes
 *
 * The rows of pixels, `stride` bytes apart, from the top row down.
 */
void maplibre_native_ffi_premultiplied_rgba8_image_set_pixels(
  MaplibreNativeFfiPremultipliedRgba8Image* self, GBytes* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiPremultipliedRgba8Image,
  maplibre_native_ffi_premultiplied_rgba8_image_free
)

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
 * See `mln_projection_mode_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
GType maplibre_native_ffi_projection_mode_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_PROJECTION_MODE_OPTIONS \
  (maplibre_native_ffi_projection_mode_options_get_type())

/**
 * maplibre_native_ffi_projection_mode_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiProjectionModeOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiProjectionModeOptions*
maplibre_native_ffi_projection_mode_options_new(void);

/**
 * maplibre_native_ffi_projection_mode_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiProjectionModeOptions*
maplibre_native_ffi_projection_mode_options_copy(
  const MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_projection_mode_options_free(
  MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_get_axonometric:
 *
 * Whether the map draws without perspective.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_projection_mode_options_get_axonometric(
  const MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_has_axonometric:
 *
 * Whether the value sets `maplibre_native_ffi_projection_mode_options_set_axonometric`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_projection_mode_options_has_axonometric(
  const MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_set_axonometric:
 *
 * Whether the map draws without perspective.
 */
void maplibre_native_ffi_projection_mode_options_set_axonometric(
  MaplibreNativeFfiProjectionModeOptions* self, gboolean value
);

/**
 * maplibre_native_ffi_projection_mode_options_get_x_skew:
 *
 * The `x_skew` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_projection_mode_options_get_x_skew(
  const MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_has_x_skew:
 *
 * Whether the value sets `maplibre_native_ffi_projection_mode_options_set_x_skew`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_projection_mode_options_has_x_skew(
  const MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_set_x_skew:
 *
 * The `x_skew` member.
 */
void maplibre_native_ffi_projection_mode_options_set_x_skew(
  MaplibreNativeFfiProjectionModeOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_projection_mode_options_get_y_skew:
 *
 * The `y_skew` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_projection_mode_options_get_y_skew(
  const MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_has_y_skew:
 *
 * Whether the value sets `maplibre_native_ffi_projection_mode_options_set_y_skew`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_projection_mode_options_has_y_skew(
  const MaplibreNativeFfiProjectionModeOptions* self
);

/**
 * maplibre_native_ffi_projection_mode_options_set_y_skew:
 *
 * The `y_skew` member.
 */
void maplibre_native_ffi_projection_mode_options_set_y_skew(
  MaplibreNativeFfiProjectionModeOptions* self, gdouble value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiProjectionModeOptions,
  maplibre_native_ffi_projection_mode_options_free
)

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
GType maplibre_native_ffi_render_session_attach_options_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_SESSION_ATTACH_OPTIONS \
  (maplibre_native_ffi_render_session_attach_options_get_type())

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
 * maplibre_native_ffi_render_session_attach_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRenderSessionAttachOptions*
maplibre_native_ffi_render_session_attach_options_copy(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_render_session_attach_options_free(
  MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_get_driver:
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
 * Returns: the value
 */
MaplibreNativeFfiRenderDriverKind
maplibre_native_ffi_render_session_attach_options_get_driver(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_driver:
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
 */
void maplibre_native_ffi_render_session_attach_options_set_driver(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  MaplibreNativeFfiRenderDriverKind value
);

/**
 * maplibre_native_ffi_render_session_attach_options_get_producer_sync_kinds:
 *
 * The kinds of `MaplibreNativeFfiGpuSync` that you can wait on before you read
 * an acquired frame. The session uses one of them that its target supports.
 * Zero lets the session pick a kind that names no object to wait on.
 *
 * Returns: the value
 */
MaplibreNativeFfiGpuSyncKindFlag
maplibre_native_ffi_render_session_attach_options_get_producer_sync_kinds(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_producer_sync_kinds:
 *
 * The kinds of `MaplibreNativeFfiGpuSync` that you can wait on before you read
 * an acquired frame. The session uses one of them that its target supports.
 * Zero lets the session pick a kind that names no object to wait on.
 */
void maplibre_native_ffi_render_session_attach_options_set_producer_sync_kinds(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  MaplibreNativeFfiGpuSyncKindFlag value
);

/**
 * maplibre_native_ffi_render_session_attach_options_get_frame_wake:
 *
 * Called when frame results are waiting for
 * `maplibre_native_ffi_render_session_drain_frame_results()`. It isn't called
 * again until a drain takes them.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_frame_wake(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_frame_wake:
 * @value: the value
 *
 * Called when frame results are waiting for
 * `maplibre_native_ffi_render_session_drain_frame_results()`. It isn't called
 * again until a drain takes them.
 */
void maplibre_native_ffi_render_session_attach_options_set_frame_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
);

/**
 * maplibre_native_ffi_render_session_attach_options_get_redraw_wake:
 *
 * Called when the session has something new to draw. Demand a frame at your
 * next display refresh. It isn't called again until you demand a frame, or
 * while the session has no target.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_redraw_wake(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_redraw_wake:
 * @value: the value
 *
 * Called when the session has something new to draw. Demand a frame at your
 * next display refresh. It isn't called again until you demand a frame, or
 * while the session has no target.
 */
void maplibre_native_ffi_render_session_attach_options_set_redraw_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
);

/**
 * maplibre_native_ffi_render_session_attach_options_get_driver_work_wake:
 *
 * Called when a session driven by
 * `MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CALLER_GRAPHICS_THREAD` has work for
 * `maplibre_native_ffi_render_session_service_driver_work()`. It isn't called
 * again until a service runs.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_driver_work_wake(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_driver_work_wake:
 * @value: the value
 *
 * Called when a session driven by
 * `MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CALLER_GRAPHICS_THREAD` has work for
 * `maplibre_native_ffi_render_session_service_driver_work()`. It isn't called
 * again until a service runs.
 */
void maplibre_native_ffi_render_session_attach_options_set_driver_work_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
);

/**
 * maplibre_native_ffi_render_session_attach_options_get_queue_lock:
 *
 * Your lock on the Vulkan queue, when you submit to it yourself.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiQueueLock*
maplibre_native_ffi_render_session_attach_options_get_queue_lock(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
);

/**
 * maplibre_native_ffi_render_session_attach_options_set_queue_lock:
 * @value: the value
 *
 * Your lock on the Vulkan queue, when you submit to it yourself.
 */
void maplibre_native_ffi_render_session_attach_options_set_queue_lock(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiQueueLock* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRenderSessionAttachOptions,
  maplibre_native_ffi_render_session_attach_options_free
)

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
 * See `mln_rendered_feature_query_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
GType maplibre_native_ffi_rendered_feature_query_options_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDERED_FEATURE_QUERY_OPTIONS \
  (maplibre_native_ffi_rendered_feature_query_options_get_type())

/**
 * maplibre_native_ffi_rendered_feature_query_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRenderedFeatureQueryOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRenderedFeatureQueryOptions*
maplibre_native_ffi_rendered_feature_query_options_new(void);

/**
 * maplibre_native_ffi_rendered_feature_query_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRenderedFeatureQueryOptions*
maplibre_native_ffi_rendered_feature_query_options_copy(
  const MaplibreNativeFfiRenderedFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_rendered_feature_query_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_rendered_feature_query_options_free(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_rendered_feature_query_options_get_layer_ids:
 *
 * The layers to query. Without them, the query covers every layer.
 *
 * Returns: (transfer none) (array zero-terminated=1): the texts
 */
const char* const*
maplibre_native_ffi_rendered_feature_query_options_get_layer_ids(
  const MaplibreNativeFfiRenderedFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_rendered_feature_query_options_has_layer_ids:
 *
 * Whether the value sets `maplibre_native_ffi_rendered_feature_query_options_set_layer_ids`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_rendered_feature_query_options_has_layer_ids(
  const MaplibreNativeFfiRenderedFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_rendered_feature_query_options_set_layer_ids:
 * @value: (array zero-terminated=1): the texts
 *
 * The layers to query. Without them, the query covers every layer.
 */
void maplibre_native_ffi_rendered_feature_query_options_set_layer_ids(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self, const char* const* value
);

/**
 * maplibre_native_ffi_rendered_feature_query_options_get_filter:
 *
 * A \[filter\](https://maplibre.org/maplibre-style-spec/expressions/) that each
 * feature must match. Empty matches every feature.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_rendered_feature_query_options_get_filter(
  const MaplibreNativeFfiRenderedFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_rendered_feature_query_options_set_filter:
 * @value: the UTF-8 text
 *
 * A \[filter\](https://maplibre.org/maplibre-style-spec/expressions/) that each
 * feature must match. Empty matches every feature.
 */
void maplibre_native_ffi_rendered_feature_query_options_set_filter(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self, const char* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRenderedFeatureQueryOptions,
  maplibre_native_ffi_rendered_feature_query_options_free
)

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
GType maplibre_native_ffi_screen_line_string_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_SCREEN_LINE_STRING \
  (maplibre_native_ffi_screen_line_string_get_type())

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
 * maplibre_native_ffi_screen_line_string_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiScreenLineString* maplibre_native_ffi_screen_line_string_copy(
  const MaplibreNativeFfiScreenLineString* self
);

/**
 * maplibre_native_ffi_screen_line_string_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_screen_line_string_free(
  MaplibreNativeFfiScreenLineString* self
);

/**
 * maplibre_native_ffi_screen_line_string_get_points:
 *
 * The `points` member.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiScreenPoint): the items
 */
GPtrArray* maplibre_native_ffi_screen_line_string_get_points(
  const MaplibreNativeFfiScreenLineString* self
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiScreenLineString, maplibre_native_ffi_screen_line_string_free
)

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
  const MaplibreNativeFfiScreenLineString* value
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
GType maplibre_native_ffi_resource_route_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_ROUTE \
  (maplibre_native_ffi_resource_route_get_type())

/**
 * maplibre_native_ffi_resource_route_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiResourceRoute` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiResourceRoute* maplibre_native_ffi_resource_route_new(void);

/**
 * maplibre_native_ffi_resource_route_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiResourceRoute* maplibre_native_ffi_resource_route_copy(
  const MaplibreNativeFfiResourceRoute* self
);

/**
 * maplibre_native_ffi_resource_route_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_resource_route_free(
  MaplibreNativeFfiResourceRoute* self
);

/**
 * maplibre_native_ffi_resource_route_get_kinds:
 *
 * The kinds of resource that the route covers. Zero covers every kind.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceKindFlag maplibre_native_ffi_resource_route_get_kinds(
  const MaplibreNativeFfiResourceRoute* self
);

/**
 * maplibre_native_ffi_resource_route_set_kinds:
 *
 * The kinds of resource that the route covers. Zero covers every kind.
 */
void maplibre_native_ffi_resource_route_set_kinds(
  MaplibreNativeFfiResourceRoute* self, MaplibreNativeFfiResourceKindFlag value
);

/**
 * maplibre_native_ffi_resource_route_get_flags:
 *
 * The `flags` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiUrlMatchFlag maplibre_native_ffi_resource_route_get_flags(
  const MaplibreNativeFfiResourceRoute* self
);

/**
 * maplibre_native_ffi_resource_route_set_flags:
 *
 * The `flags` member.
 */
void maplibre_native_ffi_resource_route_set_flags(
  MaplibreNativeFfiResourceRoute* self, MaplibreNativeFfiUrlMatchFlag value
);

/**
 * maplibre_native_ffi_resource_route_get_url:
 *
 * The `url` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_route_get_url(
  const MaplibreNativeFfiResourceRoute* self
);

/**
 * maplibre_native_ffi_resource_route_set_url:
 * @value: the UTF-8 text
 *
 * The `url` member.
 */
void maplibre_native_ffi_resource_route_set_url(
  MaplibreNativeFfiResourceRoute* self, const char* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiResourceRoute, maplibre_native_ffi_resource_route_free
)

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
 * The requests that the provider serves. Without routes, it serves every
 * network request.
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
 * See `mln_resource_response` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_resource_response_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_RESPONSE \
  (maplibre_native_ffi_resource_response_get_type())

/**
 * maplibre_native_ffi_resource_response_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiResourceResponse` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiResourceResponse* maplibre_native_ffi_resource_response_new(
  void
);

/**
 * maplibre_native_ffi_resource_response_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiResourceResponse* maplibre_native_ffi_resource_response_copy(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_resource_response_free(
  MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_get_status:
 *
 * The `status` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceResponseStatus
maplibre_native_ffi_resource_response_get_status(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_status:
 *
 * The `status` member.
 */
void maplibre_native_ffi_resource_response_set_status(
  MaplibreNativeFfiResourceResponse* self,
  MaplibreNativeFfiResourceResponseStatus value
);

/**
 * maplibre_native_ffi_resource_response_get_error_reason:
 *
 * Why the request failed, for
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_ERROR`.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceErrorReason
maplibre_native_ffi_resource_response_get_error_reason(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_error_reason:
 *
 * Why the request failed, for
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_ERROR`.
 */
void maplibre_native_ffi_resource_response_set_error_reason(
  MaplibreNativeFfiResourceResponse* self,
  MaplibreNativeFfiResourceErrorReason value
);

/**
 * maplibre_native_ffi_resource_response_get_bytes:
 *
 * The resource, for `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_OK`.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_resource_response_get_bytes(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_bytes:
 * @value: (nullable): the bytes
 *
 * The resource, for `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_OK`.
 */
void maplibre_native_ffi_resource_response_set_bytes(
  MaplibreNativeFfiResourceResponse* self, GBytes* value
);

/**
 * maplibre_native_ffi_resource_response_get_error_message:
 *
 * A description of the error, for
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_ERROR`.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_response_get_error_message(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_error_message:
 * @value: the UTF-8 text
 *
 * A description of the error, for
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_ERROR`.
 */
void maplibre_native_ffi_resource_response_set_error_message(
  MaplibreNativeFfiResourceResponse* self, const char* value
);

/**
 * maplibre_native_ffi_resource_response_get_must_revalidate:
 *
 * Whether the library must request the resource again before it reuses this
 * copy.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_resource_response_get_must_revalidate(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_must_revalidate:
 *
 * Whether the library must request the resource again before it reuses this
 * copy.
 */
void maplibre_native_ffi_resource_response_set_must_revalidate(
  MaplibreNativeFfiResourceResponse* self, gboolean value
);

/**
 * maplibre_native_ffi_resource_response_get_modified_unix_ms:
 *
 * The `modified_unix_ms` member.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_resource_response_get_modified_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_has_modified_unix_ms:
 *
 * Whether the value sets `maplibre_native_ffi_resource_response_set_modified_unix_ms`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_resource_response_has_modified_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_modified_unix_ms:
 *
 * The `modified_unix_ms` member.
 */
void maplibre_native_ffi_resource_response_set_modified_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
);

/**
 * maplibre_native_ffi_resource_response_get_expires_unix_ms:
 *
 * The `expires_unix_ms` member.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_resource_response_get_expires_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_has_expires_unix_ms:
 *
 * Whether the value sets `maplibre_native_ffi_resource_response_set_expires_unix_ms`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_resource_response_has_expires_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_expires_unix_ms:
 *
 * The `expires_unix_ms` member.
 */
void maplibre_native_ffi_resource_response_set_expires_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
);

/**
 * maplibre_native_ffi_resource_response_get_etag:
 *
 * Empty when the resource has no ETag.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_response_get_etag(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_etag:
 * @value: the UTF-8 text
 *
 * Empty when the resource has no ETag.
 */
void maplibre_native_ffi_resource_response_set_etag(
  MaplibreNativeFfiResourceResponse* self, const char* value
);

/**
 * maplibre_native_ffi_resource_response_get_retry_after_unix_ms:
 *
 * When to retry, for `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_ERROR`.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_resource_response_get_retry_after_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_has_retry_after_unix_ms:
 *
 * Whether the value sets `maplibre_native_ffi_resource_response_set_retry_after_unix_ms`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_resource_response_has_retry_after_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
);

/**
 * maplibre_native_ffi_resource_response_set_retry_after_unix_ms:
 *
 * When to retry, for `MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_ERROR`.
 */
void maplibre_native_ffi_resource_response_set_retry_after_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiResourceResponse, maplibre_native_ffi_resource_response_free
)

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
GType maplibre_native_ffi_url_query_parameter_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_URL_QUERY_PARAMETER \
  (maplibre_native_ffi_url_query_parameter_get_type())

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
 * maplibre_native_ffi_url_query_parameter_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiUrlQueryParameter*
maplibre_native_ffi_url_query_parameter_copy(
  const MaplibreNativeFfiUrlQueryParameter* self
);

/**
 * maplibre_native_ffi_url_query_parameter_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_url_query_parameter_free(
  MaplibreNativeFfiUrlQueryParameter* self
);

/**
 * maplibre_native_ffi_url_query_parameter_get_name:
 *
 * The `name` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_url_query_parameter_get_name(
  const MaplibreNativeFfiUrlQueryParameter* self
);

/**
 * maplibre_native_ffi_url_query_parameter_set_name:
 * @value: the UTF-8 text
 *
 * The `name` member.
 */
void maplibre_native_ffi_url_query_parameter_set_name(
  MaplibreNativeFfiUrlQueryParameter* self, const char* value
);

/**
 * maplibre_native_ffi_url_query_parameter_get_value:
 *
 * The `value` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_url_query_parameter_get_value(
  const MaplibreNativeFfiUrlQueryParameter* self
);

/**
 * maplibre_native_ffi_url_query_parameter_set_value:
 * @value: the UTF-8 text
 *
 * The `value` member.
 */
void maplibre_native_ffi_url_query_parameter_set_value(
  MaplibreNativeFfiUrlQueryParameter* self, const char* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiUrlQueryParameter,
  maplibre_native_ffi_url_query_parameter_free
)

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
GType maplibre_native_ffi_resource_transform_rule_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_TRANSFORM_RULE \
  (maplibre_native_ffi_resource_transform_rule_get_type())

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
 * maplibre_native_ffi_resource_transform_rule_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiResourceTransformRule*
maplibre_native_ffi_resource_transform_rule_copy(
  const MaplibreNativeFfiResourceTransformRule* self
);

/**
 * maplibre_native_ffi_resource_transform_rule_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_resource_transform_rule_free(
  MaplibreNativeFfiResourceTransformRule* self
);

/**
 * maplibre_native_ffi_resource_transform_rule_get_kinds:
 *
 * The kinds of resource that the rule covers. Zero covers every kind.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceKindFlag
maplibre_native_ffi_resource_transform_rule_get_kinds(
  const MaplibreNativeFfiResourceTransformRule* self
);

/**
 * maplibre_native_ffi_resource_transform_rule_set_kinds:
 *
 * The kinds of resource that the rule covers. Zero covers every kind.
 */
void maplibre_native_ffi_resource_transform_rule_set_kinds(
  MaplibreNativeFfiResourceTransformRule* self,
  MaplibreNativeFfiResourceKindFlag value
);

/**
 * maplibre_native_ffi_resource_transform_rule_get_flags:
 *
 * The `flags` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiUrlMatchFlag
maplibre_native_ffi_resource_transform_rule_get_flags(
  const MaplibreNativeFfiResourceTransformRule* self
);

/**
 * maplibre_native_ffi_resource_transform_rule_set_flags:
 *
 * The `flags` member.
 */
void maplibre_native_ffi_resource_transform_rule_set_flags(
  MaplibreNativeFfiResourceTransformRule* self,
  MaplibreNativeFfiUrlMatchFlag value
);

/**
 * maplibre_native_ffi_resource_transform_rule_get_url:
 *
 * The `url` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_transform_rule_get_url(
  const MaplibreNativeFfiResourceTransformRule* self
);

/**
 * maplibre_native_ffi_resource_transform_rule_set_url:
 * @value: the UTF-8 text
 *
 * The `url` member.
 */
void maplibre_native_ffi_resource_transform_rule_set_url(
  MaplibreNativeFfiResourceTransformRule* self, const char* value
);

/**
 * maplibre_native_ffi_resource_transform_rule_get_parameters:
 *
 * The `parameters` member.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiUrlQueryParameter): the items
 */
GPtrArray* maplibre_native_ffi_resource_transform_rule_get_parameters(
  const MaplibreNativeFfiResourceTransformRule* self
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiResourceTransformRule,
  maplibre_native_ffi_resource_transform_rule_free
)

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
 * The first rule that matches a request adds its parameters, and no other. A
 * request that no rule matches keeps its URL.
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
 * See `mln_resource_transform_error` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
GType maplibre_native_ffi_resource_transform_error_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_TRANSFORM_ERROR \
  (maplibre_native_ffi_resource_transform_error_get_type())

/**
 * maplibre_native_ffi_resource_transform_error_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiResourceTransformError` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiResourceTransformError*
maplibre_native_ffi_resource_transform_error_new(void);

/**
 * maplibre_native_ffi_resource_transform_error_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiResourceTransformError*
maplibre_native_ffi_resource_transform_error_copy(
  const MaplibreNativeFfiResourceTransformError* self
);

/**
 * maplibre_native_ffi_resource_transform_error_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_resource_transform_error_free(
  MaplibreNativeFfiResourceTransformError* self
);

/**
 * maplibre_native_ffi_resource_transform_error_get_reason:
 *
 * The `reason` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiResourceErrorReason
maplibre_native_ffi_resource_transform_error_get_reason(
  const MaplibreNativeFfiResourceTransformError* self
);

/**
 * maplibre_native_ffi_resource_transform_error_set_reason:
 *
 * The `reason` member.
 */
void maplibre_native_ffi_resource_transform_error_set_reason(
  MaplibreNativeFfiResourceTransformError* self,
  MaplibreNativeFfiResourceErrorReason value
);

/**
 * maplibre_native_ffi_resource_transform_error_get_message:
 *
 * The `message` member.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_resource_transform_error_get_message(
  const MaplibreNativeFfiResourceTransformError* self
);

/**
 * maplibre_native_ffi_resource_transform_error_set_message:
 * @value: the UTF-8 text
 *
 * The `message` member.
 */
void maplibre_native_ffi_resource_transform_error_set_message(
  MaplibreNativeFfiResourceTransformError* self, const char* value
);

/**
 * maplibre_native_ffi_resource_transform_error_get_retry_after_unix_ms:
 *
 * When to retry, for `MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_RATE_LIMIT`.
 *
 * Returns: the value
 */
gint64 maplibre_native_ffi_resource_transform_error_get_retry_after_unix_ms(
  const MaplibreNativeFfiResourceTransformError* self
);

/**
 * maplibre_native_ffi_resource_transform_error_has_retry_after_unix_ms:
 *
 * Whether the value sets `maplibre_native_ffi_resource_transform_error_set_retry_after_unix_ms`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_resource_transform_error_has_retry_after_unix_ms(
  const MaplibreNativeFfiResourceTransformError* self
);

/**
 * maplibre_native_ffi_resource_transform_error_set_retry_after_unix_ms:
 *
 * When to retry, for `MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_RATE_LIMIT`.
 */
void maplibre_native_ffi_resource_transform_error_set_retry_after_unix_ms(
  MaplibreNativeFfiResourceTransformError* self, gint64 value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiResourceTransformError,
  maplibre_native_ffi_resource_transform_error_free
)

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
GType maplibre_native_ffi_runtime_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_OPTIONS \
  (maplibre_native_ffi_runtime_options_get_type())

/**
 * maplibre_native_ffi_runtime_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRuntimeOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRuntimeOptions* maplibre_native_ffi_runtime_options_new(void);

/**
 * maplibre_native_ffi_runtime_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeOptions* maplibre_native_ffi_runtime_options_copy(
  const MaplibreNativeFfiRuntimeOptions* self
);

/**
 * maplibre_native_ffi_runtime_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_options_free(
  MaplibreNativeFfiRuntimeOptions* self
);

/**
 * maplibre_native_ffi_runtime_options_get_asset_path:
 *
 * The directory that `asset://` URLs load from. Empty uses the working
 * directory, or the app's assets on Android.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_options_get_asset_path(
  const MaplibreNativeFfiRuntimeOptions* self
);

/**
 * maplibre_native_ffi_runtime_options_set_asset_path:
 * @value: the UTF-8 text
 *
 * The directory that `asset://` URLs load from. Empty uses the working
 * directory, or the app's assets on Android.
 */
void maplibre_native_ffi_runtime_options_set_asset_path(
  MaplibreNativeFfiRuntimeOptions* self, const char* value
);

/**
 * maplibre_native_ffi_runtime_options_get_cache_path:
 *
 * The cache database file, which holds the ambient cache and offline regions.
 * Empty keeps them in memory.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_options_get_cache_path(
  const MaplibreNativeFfiRuntimeOptions* self
);

/**
 * maplibre_native_ffi_runtime_options_set_cache_path:
 * @value: the UTF-8 text
 *
 * The cache database file, which holds the ambient cache and offline regions.
 * Empty keeps them in memory.
 */
void maplibre_native_ffi_runtime_options_set_cache_path(
  MaplibreNativeFfiRuntimeOptions* self, const char* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiRuntimeOptions, maplibre_native_ffi_runtime_options_free
)

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
 * See `mln_source_feature_query_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
GType maplibre_native_ffi_source_feature_query_options_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_SOURCE_FEATURE_QUERY_OPTIONS \
  (maplibre_native_ffi_source_feature_query_options_get_type())

/**
 * maplibre_native_ffi_source_feature_query_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiSourceFeatureQueryOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiSourceFeatureQueryOptions*
maplibre_native_ffi_source_feature_query_options_new(void);

/**
 * maplibre_native_ffi_source_feature_query_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiSourceFeatureQueryOptions*
maplibre_native_ffi_source_feature_query_options_copy(
  const MaplibreNativeFfiSourceFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_source_feature_query_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_source_feature_query_options_free(
  MaplibreNativeFfiSourceFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_source_feature_query_options_get_source_layer_ids:
 *
 * The source layers to query. A vector source finds features only in the source
 * layers named here.
 *
 * Returns: (transfer none) (array zero-terminated=1): the texts
 */
const char* const*
maplibre_native_ffi_source_feature_query_options_get_source_layer_ids(
  const MaplibreNativeFfiSourceFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_source_feature_query_options_has_source_layer_ids:
 *
 * Whether the value sets `maplibre_native_ffi_source_feature_query_options_set_source_layer_ids`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_source_feature_query_options_has_source_layer_ids(
  const MaplibreNativeFfiSourceFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_source_feature_query_options_set_source_layer_ids:
 * @value: (array zero-terminated=1): the texts
 *
 * The source layers to query. A vector source finds features only in the source
 * layers named here.
 */
void maplibre_native_ffi_source_feature_query_options_set_source_layer_ids(
  MaplibreNativeFfiSourceFeatureQueryOptions* self, const char* const* value
);

/**
 * maplibre_native_ffi_source_feature_query_options_get_filter:
 *
 * A \[filter\](https://maplibre.org/maplibre-style-spec/expressions/) that each
 * feature must match. Empty matches every feature.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_source_feature_query_options_get_filter(
  const MaplibreNativeFfiSourceFeatureQueryOptions* self
);

/**
 * maplibre_native_ffi_source_feature_query_options_set_filter:
 * @value: the UTF-8 text
 *
 * A \[filter\](https://maplibre.org/maplibre-style-spec/expressions/) that each
 * feature must match. Empty matches every feature.
 */
void maplibre_native_ffi_source_feature_query_options_set_filter(
  MaplibreNativeFfiSourceFeatureQueryOptions* self, const char* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiSourceFeatureQueryOptions,
  maplibre_native_ffi_source_feature_query_options_free
)

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
 * See `mln_style_expression_inputs` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
GType maplibre_native_ffi_style_expression_inputs_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION_INPUTS \
  (maplibre_native_ffi_style_expression_inputs_get_type())

/**
 * maplibre_native_ffi_style_expression_inputs_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleExpressionInputs` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleExpressionInputs*
maplibre_native_ffi_style_expression_inputs_new(void);

/**
 * maplibre_native_ffi_style_expression_inputs_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiStyleExpressionInputs*
maplibre_native_ffi_style_expression_inputs_copy(
  const MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_style_expression_inputs_free(
  MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_get_zoom:
 *
 * The `zoom` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_expression_inputs_get_zoom(
  const MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_has_zoom:
 *
 * Whether the value sets `maplibre_native_ffi_style_expression_inputs_set_zoom`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_expression_inputs_has_zoom(
  const MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_zoom:
 *
 * The `zoom` member.
 */
void maplibre_native_ffi_style_expression_inputs_set_zoom(
  MaplibreNativeFfiStyleExpressionInputs* self, gdouble value
);

/**
 * maplibre_native_ffi_style_expression_inputs_get_color_ramp_parameter:
 *
 * The value of `heatmap-density`, `line-progress`, and `elevation`.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_expression_inputs_get_color_ramp_parameter(
  const MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_has_color_ramp_parameter:
 *
 * Whether the value sets `maplibre_native_ffi_style_expression_inputs_set_color_ramp_parameter`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_expression_inputs_has_color_ramp_parameter(
  const MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_color_ramp_parameter:
 *
 * The value of `heatmap-density`, `line-progress`, and `elevation`.
 */
void maplibre_native_ffi_style_expression_inputs_set_color_ramp_parameter(
  MaplibreNativeFfiStyleExpressionInputs* self, gdouble value
);

/**
 * maplibre_native_ffi_style_expression_inputs_get_feature:
 *
 * The feature to evaluate against, as a GeoJSON feature, or empty for none.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_expression_inputs_get_feature(
  const MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_feature:
 * @value: the UTF-8 text
 *
 * The feature to evaluate against, as a GeoJSON feature, or empty for none.
 */
void maplibre_native_ffi_style_expression_inputs_set_feature(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
);

/**
 * maplibre_native_ffi_style_expression_inputs_get_feature_state:
 *
 * The feature's state, as a JSON object, or empty for none.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_expression_inputs_get_feature_state(
  const MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_feature_state:
 * @value: the UTF-8 text
 *
 * The feature's state, as a JSON object, or empty for none.
 */
void maplibre_native_ffi_style_expression_inputs_set_feature_state(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
);

/**
 * maplibre_native_ffi_style_expression_inputs_get_global_state:
 *
 * The values that `global-state` reads, as a JSON object, or empty for none.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_style_expression_inputs_get_global_state(
  const MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_global_state:
 * @value: the UTF-8 text
 *
 * The values that `global-state` reads, as a JSON object, or empty for none.
 */
void maplibre_native_ffi_style_expression_inputs_set_global_state(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
);

/**
 * maplibre_native_ffi_style_expression_inputs_get_canonical_tile_id:
 *
 * The tile that holds the feature. `within` and `distance` need it to read the
 * feature's geometry.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiCanonicalTileId*
maplibre_native_ffi_style_expression_inputs_get_canonical_tile_id(
  const MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_has_canonical_tile_id:
 *
 * Whether the value sets `maplibre_native_ffi_style_expression_inputs_set_canonical_tile_id`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_expression_inputs_has_canonical_tile_id(
  const MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_canonical_tile_id:
 * @value: the value
 *
 * The tile that holds the feature. `within` and `distance` need it to read the
 * feature's geometry.
 */
void maplibre_native_ffi_style_expression_inputs_set_canonical_tile_id(
  MaplibreNativeFfiStyleExpressionInputs* self,
  const MaplibreNativeFfiCanonicalTileId* value
);

/**
 * maplibre_native_ffi_style_expression_inputs_get_available_images:
 *
 * The IDs of the images that `image` treats as available.
 *
 * Returns: (transfer none) (array zero-terminated=1): the texts
 */
const char* const*
maplibre_native_ffi_style_expression_inputs_get_available_images(
  const MaplibreNativeFfiStyleExpressionInputs* self
);

/**
 * maplibre_native_ffi_style_expression_inputs_set_available_images:
 * @value: (array zero-terminated=1): the texts
 *
 * The IDs of the images that `image` treats as available.
 */
void maplibre_native_ffi_style_expression_inputs_set_available_images(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* const* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiStyleExpressionInputs,
  maplibre_native_ffi_style_expression_inputs_free
)

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
 * See `mln_style_expression_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
GType maplibre_native_ffi_style_expression_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION_OPTIONS \
  (maplibre_native_ffi_style_expression_options_get_type())

/**
 * maplibre_native_ffi_style_expression_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleExpressionOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleExpressionOptions*
maplibre_native_ffi_style_expression_options_new(void);

/**
 * maplibre_native_ffi_style_expression_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiStyleExpressionOptions*
maplibre_native_ffi_style_expression_options_copy(
  const MaplibreNativeFfiStyleExpressionOptions* self
);

/**
 * maplibre_native_ffi_style_expression_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_style_expression_options_free(
  MaplibreNativeFfiStyleExpressionOptions* self
);

/**
 * maplibre_native_ffi_style_expression_options_get_expected_type:
 *
 * The type that the result must have.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiStyleExpressionType*
maplibre_native_ffi_style_expression_options_get_expected_type(
  const MaplibreNativeFfiStyleExpressionOptions* self
);

/**
 * maplibre_native_ffi_style_expression_options_has_expected_type:
 *
 * Whether the value sets `maplibre_native_ffi_style_expression_options_set_expected_type`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_expression_options_has_expected_type(
  const MaplibreNativeFfiStyleExpressionOptions* self
);

/**
 * maplibre_native_ffi_style_expression_options_set_expected_type:
 * @value: the value
 *
 * The type that the result must have.
 */
void maplibre_native_ffi_style_expression_options_set_expected_type(
  MaplibreNativeFfiStyleExpressionOptions* self,
  const MaplibreNativeFfiStyleExpressionType* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiStyleExpressionOptions,
  maplibre_native_ffi_style_expression_options_free
)

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
 * See `mln_style_image_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
GType maplibre_native_ffi_style_image_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_IMAGE_OPTIONS \
  (maplibre_native_ffi_style_image_options_get_type())

/**
 * maplibre_native_ffi_style_image_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleImageOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleImageOptions* maplibre_native_ffi_style_image_options_new(
  void
);

/**
 * maplibre_native_ffi_style_image_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiStyleImageOptions*
maplibre_native_ffi_style_image_options_copy(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_style_image_options_free(
  MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_get_stretch_x:
 *
 * The ranges of columns that stretch, in increasing order without overlap.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiImageStretch): the items
 */
GPtrArray* maplibre_native_ffi_style_image_options_get_stretch_x(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_stretch_x:
 *
 * Whether the value sets `maplibre_native_ffi_style_image_options_set_stretch_x`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_stretch_x(
  const MaplibreNativeFfiStyleImageOptions* self
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
 * The ranges of rows that stretch, in increasing order without overlap.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiImageStretch): the items
 */
GPtrArray* maplibre_native_ffi_style_image_options_get_stretch_y(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_stretch_y:
 *
 * Whether the value sets `maplibre_native_ffi_style_image_options_set_stretch_y`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_stretch_y(
  const MaplibreNativeFfiStyleImageOptions* self
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
 * maplibre_native_ffi_style_image_options_get_content:
 *
 * The `content` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiImageContent*
maplibre_native_ffi_style_image_options_get_content(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_content:
 *
 * Whether the value sets `maplibre_native_ffi_style_image_options_set_content`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_content(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_content:
 * @value: the value
 *
 * The `content` member.
 */
void maplibre_native_ffi_style_image_options_set_content(
  MaplibreNativeFfiStyleImageOptions* self,
  const MaplibreNativeFfiImageContent* value
);

/**
 * maplibre_native_ffi_style_image_options_get_text_fit_width:
 *
 * The `text_fit_width` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_options_get_text_fit_width(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_text_fit_width:
 *
 * Whether the value sets `maplibre_native_ffi_style_image_options_set_text_fit_width`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_text_fit_width(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_text_fit_width:
 *
 * The `text_fit_width` member.
 */
void maplibre_native_ffi_style_image_options_set_text_fit_width(
  MaplibreNativeFfiStyleImageOptions* self,
  MaplibreNativeFfiStyleImageTextFit value
);

/**
 * maplibre_native_ffi_style_image_options_get_text_fit_height:
 *
 * The `text_fit_height` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_options_get_text_fit_height(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_text_fit_height:
 *
 * Whether the value sets `maplibre_native_ffi_style_image_options_set_text_fit_height`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_text_fit_height(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_text_fit_height:
 *
 * The `text_fit_height` member.
 */
void maplibre_native_ffi_style_image_options_set_text_fit_height(
  MaplibreNativeFfiStyleImageOptions* self,
  MaplibreNativeFfiStyleImageTextFit value
);

/**
 * maplibre_native_ffi_style_image_options_get_pixel_ratio:
 *
 * The `pixel_ratio` member.
 *
 * Returns: the value
 */
gfloat maplibre_native_ffi_style_image_options_get_pixel_ratio(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_pixel_ratio:
 *
 * Whether the value sets `maplibre_native_ffi_style_image_options_set_pixel_ratio`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_pixel_ratio(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_pixel_ratio:
 *
 * The `pixel_ratio` member.
 */
void maplibre_native_ffi_style_image_options_set_pixel_ratio(
  MaplibreNativeFfiStyleImageOptions* self, gfloat value
);

/**
 * maplibre_native_ffi_style_image_options_get_sdf:
 *
 * The `sdf` member.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_style_image_options_get_sdf(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_sdf:
 *
 * Whether the value sets `maplibre_native_ffi_style_image_options_set_sdf`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_sdf(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_sdf:
 *
 * The `sdf` member.
 */
void maplibre_native_ffi_style_image_options_set_sdf(
  MaplibreNativeFfiStyleImageOptions* self, gboolean value
);

/**
 * maplibre_native_ffi_style_image_options_get_keep_unused:
 *
 * Keeps the image after no layer uses it. Without this, the map can remove an
 * image that it asked for through
 * `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING`.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_style_image_options_get_keep_unused(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_has_keep_unused:
 *
 * Whether the value sets `maplibre_native_ffi_style_image_options_set_keep_unused`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_image_options_has_keep_unused(
  const MaplibreNativeFfiStyleImageOptions* self
);

/**
 * maplibre_native_ffi_style_image_options_set_keep_unused:
 *
 * Keeps the image after no layer uses it. Without this, the map can remove an
 * image that it asked for through
 * `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING`.
 */
void maplibre_native_ffi_style_image_options_set_keep_unused(
  MaplibreNativeFfiStyleImageOptions* self, gboolean value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiStyleImageOptions,
  maplibre_native_ffi_style_image_options_free
)

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
 * A GeoJSON source's options, with every field set. Data prepared with them can
 * replace the source's data.
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
 * See `mln_style_transition_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
GType maplibre_native_ffi_style_transition_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_TRANSITION_OPTIONS \
  (maplibre_native_ffi_style_transition_options_get_type())

/**
 * maplibre_native_ffi_style_transition_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiStyleTransitionOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiStyleTransitionOptions*
maplibre_native_ffi_style_transition_options_new(void);

/**
 * maplibre_native_ffi_style_transition_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiStyleTransitionOptions*
maplibre_native_ffi_style_transition_options_copy(
  const MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_style_transition_options_free(
  MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_get_duration_ms:
 *
 * The `duration_ms` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_transition_options_get_duration_ms(
  const MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_has_duration_ms:
 *
 * Whether the value sets `maplibre_native_ffi_style_transition_options_set_duration_ms`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_transition_options_has_duration_ms(
  const MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_set_duration_ms:
 *
 * The `duration_ms` member.
 */
void maplibre_native_ffi_style_transition_options_set_duration_ms(
  MaplibreNativeFfiStyleTransitionOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_style_transition_options_get_delay_ms:
 *
 * The `delay_ms` member.
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_style_transition_options_get_delay_ms(
  const MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_has_delay_ms:
 *
 * Whether the value sets `maplibre_native_ffi_style_transition_options_set_delay_ms`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_style_transition_options_has_delay_ms(
  const MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_set_delay_ms:
 *
 * The `delay_ms` member.
 */
void maplibre_native_ffi_style_transition_options_set_delay_ms(
  MaplibreNativeFfiStyleTransitionOptions* self, gdouble value
);

/**
 * maplibre_native_ffi_style_transition_options_get_enable_placement_transitions:
 *
 * Whether symbols fade in and out as the map places them.
 *
 * Returns: the value
 */
gboolean
maplibre_native_ffi_style_transition_options_get_enable_placement_transitions(
  const MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_has_enable_placement_transitions:
 *
 * Whether the value sets `maplibre_native_ffi_style_transition_options_set_enable_placement_transitions`.
 *
 * Returns: whether the member is set
 */
gboolean
maplibre_native_ffi_style_transition_options_has_enable_placement_transitions(
  const MaplibreNativeFfiStyleTransitionOptions* self
);

/**
 * maplibre_native_ffi_style_transition_options_set_enable_placement_transitions:
 *
 * Whether symbols fade in and out as the map places them.
 */
void maplibre_native_ffi_style_transition_options_set_enable_placement_transitions(
  MaplibreNativeFfiStyleTransitionOptions* self, gboolean value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiStyleTransitionOptions,
  maplibre_native_ffi_style_transition_options_free
)

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
GType maplibre_native_ffi_vulkan_context_descriptor_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_CONTEXT_DESCRIPTOR \
  (maplibre_native_ffi_vulkan_context_descriptor_get_type())

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
 * maplibre_native_ffi_vulkan_context_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_context_descriptor_copy(
  const MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_vulkan_context_descriptor_free(
  MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_instance:
 *
 * A `VkInstance`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_vulkan_context_descriptor_get_instance(
  const MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_instance:
 *
 * A `VkInstance`.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_instance(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_physical_device:
 *
 * A `VkPhysicalDevice`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_vulkan_context_descriptor_get_physical_device(
  const MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_physical_device:
 *
 * A `VkPhysicalDevice`.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_physical_device(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_device:
 *
 * A `VkDevice`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_vulkan_context_descriptor_get_device(
  const MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_device:
 *
 * A `VkDevice`.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_device(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue:
 *
 * A `VkQueue` from `graphics_queue_family_index` that supports graphics. If you
 * submit to it too, set
 * `maplibre_native_ffi_render_session_attach_options_set_queue_lock`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue(
  const MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue:
 *
 * A `VkQueue` from `graphics_queue_family_index` that supports graphics. If you
 * submit to it too, set
 * `maplibre_native_ffi_render_session_attach_options_set_queue_lock`.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue_family_index:
 *
 * The `graphics_queue_family_index` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue_family_index(
  const MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue_family_index:
 *
 * The `graphics_queue_family_index` member.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue_family_index(
  MaplibreNativeFfiVulkanContextDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_get_instance_proc_addr:
 *
 * Your `vkGetInstanceProcAddr`, or null to use the Vulkan loader's.
 *
 * Returns: the value
 */
gpointer
maplibre_native_ffi_vulkan_context_descriptor_get_get_instance_proc_addr(
  const MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_get_instance_proc_addr:
 *
 * Your `vkGetInstanceProcAddr`, or null to use the Vulkan loader's.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_get_instance_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_get_get_device_proc_addr:
 *
 * Your `vkGetDeviceProcAddr`, or null to look it up from the instance.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_vulkan_context_descriptor_get_get_device_proc_addr(
  const MaplibreNativeFfiVulkanContextDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_context_descriptor_set_get_device_proc_addr:
 *
 * Your `vkGetDeviceProcAddr`, or null to look it up from the instance.
 */
void maplibre_native_ffi_vulkan_context_descriptor_set_get_device_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiVulkanContextDescriptor,
  maplibre_native_ffi_vulkan_context_descriptor_free
)

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
GType maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_BORROWED_TEXTURE_DESCRIPTOR \
  (maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_type())

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
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiVulkanBorrowedTextureDescriptor*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_copy(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_free(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_logical_size(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_width:
 *
 * The `physical_width` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_width(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_width:
 *
 * The `physical_width` member.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_height:
 *
 * The `physical_height` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_height(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_height:
 *
 * The `physical_height` member.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_context:
 *
 * The `context` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_context(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_context:
 * @value: the value
 *
 * The `context` member.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_context(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiVulkanContextDescriptor* value
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_textures:
 *
 * The `textures` member.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiVulkanBorrowedTexture): the items
 */
GPtrArray* maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_textures(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
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
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_format:
 *
 * The images' VkFormat.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_format(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_format:
 *
 * The images' VkFormat.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_format(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_final_layout:
 *
 * The VkImageLayout that each frame leaves its image in.
 *
 * Returns: the value
 */
guint32 maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_final_layout(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_final_layout:
 *
 * The VkImageLayout that each frame leaves its image in.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_final_layout(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_owner:
 *
 * The `owner` member.
 *
 * Returns: the value
 */
MaplibreNativeFfiVulkanImageOwner
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_owner(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_owner:
 *
 * The `owner` member.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_owner(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  MaplibreNativeFfiVulkanImageOwner value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor,
  maplibre_native_ffi_vulkan_borrowed_texture_descriptor_free
)

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
GType maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_BORROWED_TEXTURE_REPLACEMENT \
  (maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_type())

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
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiVulkanBorrowedTextureReplacement*
maplibre_native_ffi_vulkan_borrowed_texture_replacement_copy(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_replacement_free(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_logical_size(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_width:
 *
 * The `physical_width` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_width(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_width:
 *
 * The `physical_width` member.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self, guint32 value
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_height:
 *
 * The `physical_height` member.
 *
 * Returns: the value
 */
guint32
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_height(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_height:
 *
 * The `physical_height` member.
 */
void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self, guint32 value
);

/**
 * maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_textures:
 *
 * The `textures` member.
 *
 * Returns: (transfer none) (element-type MaplibreNativeFfiVulkanBorrowedTexture): the items
 */
GPtrArray* maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_textures(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement,
  maplibre_native_ffi_vulkan_borrowed_texture_replacement_free
)

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
 * See `mln_vulkan_device_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
 */
GType maplibre_native_ffi_vulkan_device_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_DEVICE_OPTIONS \
  (maplibre_native_ffi_vulkan_device_options_get_type())

/**
 * maplibre_native_ffi_vulkan_device_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiVulkanDeviceOptions` whose members are zero or empty.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiVulkanDeviceOptions*
maplibre_native_ffi_vulkan_device_options_new(void);

/**
 * maplibre_native_ffi_vulkan_device_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiVulkanDeviceOptions*
maplibre_native_ffi_vulkan_device_options_copy(
  const MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_vulkan_device_options_free(
  MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_get_presentation:
 *
 * Whether the device can present to a window. The instance then enables
 * VK_KHR_surface and the platform's surface extensions, and the device enables
 * VK_KHR_swapchain. Defaults to false.
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_vulkan_device_options_get_presentation(
  const MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_has_presentation:
 *
 * Whether the value sets `maplibre_native_ffi_vulkan_device_options_set_presentation`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_vulkan_device_options_has_presentation(
  const MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_set_presentation:
 *
 * Whether the device can present to a window. The instance then enables
 * VK_KHR_surface and the platform's surface extensions, and the device enables
 * VK_KHR_swapchain. Defaults to false.
 */
void maplibre_native_ffi_vulkan_device_options_set_presentation(
  MaplibreNativeFfiVulkanDeviceOptions* self, gboolean value
);

/**
 * maplibre_native_ffi_vulkan_device_options_get_device_uuid:
 *
 * The 16-byte `deviceUUID` of the GPU to use, as `VkPhysicalDeviceIDProperties`
 * reports it.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_vulkan_device_options_get_device_uuid(
  const MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_has_device_uuid:
 *
 * Whether the value sets `maplibre_native_ffi_vulkan_device_options_set_device_uuid`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_vulkan_device_options_has_device_uuid(
  const MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_set_device_uuid:
 * @value: (nullable): the bytes
 *
 * The 16-byte `deviceUUID` of the GPU to use, as `VkPhysicalDeviceIDProperties`
 * reports it.
 */
void maplibre_native_ffi_vulkan_device_options_set_device_uuid(
  MaplibreNativeFfiVulkanDeviceOptions* self, GBytes* value
);

/**
 * maplibre_native_ffi_vulkan_device_options_get_device_luid:
 *
 * The 8-byte `deviceLUID` of the GPU to use, as `VkPhysicalDeviceIDProperties`
 * reports it. On Windows, this is the GPU's DXGI adapter LUID.
 *
 * Returns: (transfer none) (nullable): the bytes
 */
GBytes* maplibre_native_ffi_vulkan_device_options_get_device_luid(
  const MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_has_device_luid:
 *
 * Whether the value sets `maplibre_native_ffi_vulkan_device_options_set_device_luid`.
 *
 * Returns: whether the member is set
 */
gboolean maplibre_native_ffi_vulkan_device_options_has_device_luid(
  const MaplibreNativeFfiVulkanDeviceOptions* self
);

/**
 * maplibre_native_ffi_vulkan_device_options_set_device_luid:
 * @value: (nullable): the bytes
 *
 * The 8-byte `deviceLUID` of the GPU to use, as `VkPhysicalDeviceIDProperties`
 * reports it. On Windows, this is the GPU's DXGI adapter LUID.
 */
void maplibre_native_ffi_vulkan_device_options_set_device_luid(
  MaplibreNativeFfiVulkanDeviceOptions* self, GBytes* value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiVulkanDeviceOptions,
  maplibre_native_ffi_vulkan_device_options_free
)

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
GType maplibre_native_ffi_vulkan_surface_descriptor_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_SURFACE_DESCRIPTOR \
  (maplibre_native_ffi_vulkan_surface_descriptor_get_type())

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
 * maplibre_native_ffi_vulkan_surface_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiVulkanSurfaceDescriptor*
maplibre_native_ffi_vulkan_surface_descriptor_copy(
  const MaplibreNativeFfiVulkanSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_surface_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_vulkan_surface_descriptor_free(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_surface_descriptor_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_surface_descriptor_get_logical_size(
  const MaplibreNativeFfiVulkanSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_surface_descriptor_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_vulkan_surface_descriptor_set_logical_size(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_vulkan_surface_descriptor_get_context:
 *
 * The `context` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_surface_descriptor_get_context(
  const MaplibreNativeFfiVulkanSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_surface_descriptor_set_context:
 * @value: the value
 *
 * The `context` member.
 */
void maplibre_native_ffi_vulkan_surface_descriptor_set_context(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self,
  const MaplibreNativeFfiVulkanContextDescriptor* value
);

/**
 * maplibre_native_ffi_vulkan_surface_descriptor_get_surface:
 *
 * Your VkSurfaceKHR.
 *
 * Returns: the value
 */
guint64 maplibre_native_ffi_vulkan_surface_descriptor_get_surface(
  const MaplibreNativeFfiVulkanSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_vulkan_surface_descriptor_set_surface:
 *
 * Your VkSurfaceKHR.
 */
void maplibre_native_ffi_vulkan_surface_descriptor_set_surface(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self, guint64 value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiVulkanSurfaceDescriptor,
  maplibre_native_ffi_vulkan_surface_descriptor_free
)

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
GType maplibre_native_ffi_wayland_surface_descriptor_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_WAYLAND_SURFACE_DESCRIPTOR \
  (maplibre_native_ffi_wayland_surface_descriptor_get_type())

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
 * maplibre_native_ffi_wayland_surface_descriptor_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiWaylandSurfaceDescriptor*
maplibre_native_ffi_wayland_surface_descriptor_copy(
  const MaplibreNativeFfiWaylandSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_wayland_surface_descriptor_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_wayland_surface_descriptor_free(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_wayland_surface_descriptor_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_wayland_surface_descriptor_get_logical_size(
  const MaplibreNativeFfiWaylandSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_wayland_surface_descriptor_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_wayland_surface_descriptor_set_logical_size(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_wayland_surface_descriptor_get_display:
 *
 * Your `wl_display*`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wayland_surface_descriptor_get_display(
  const MaplibreNativeFfiWaylandSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_wayland_surface_descriptor_set_display:
 *
 * Your `wl_display*`.
 */
void maplibre_native_ffi_wayland_surface_descriptor_set_display(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self, gpointer value
);

/**
 * maplibre_native_ffi_wayland_surface_descriptor_get_surface:
 *
 * Your `wl_surface*`, on `display`.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wayland_surface_descriptor_get_surface(
  const MaplibreNativeFfiWaylandSurfaceDescriptor* self
);

/**
 * maplibre_native_ffi_wayland_surface_descriptor_set_surface:
 *
 * Your `wl_surface*`, on `display`.
 */
void maplibre_native_ffi_wayland_surface_descriptor_set_surface(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiWaylandSurfaceDescriptor,
  maplibre_native_ffi_wayland_surface_descriptor_free
)

/**
 * MaplibreNativeFfiWaylandSurfaceReplacement:
 *
 * A new Wayland surface for
 * `maplibre_native_ffi_render_session_set_wayland_surface_target_async()`.
 *
 * See `mln_wayland_surface_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).
 */
GType maplibre_native_ffi_wayland_surface_replacement_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_WAYLAND_SURFACE_REPLACEMENT \
  (maplibre_native_ffi_wayland_surface_replacement_get_type())

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
 * maplibre_native_ffi_wayland_surface_replacement_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiWaylandSurfaceReplacement*
maplibre_native_ffi_wayland_surface_replacement_copy(
  const MaplibreNativeFfiWaylandSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_wayland_surface_replacement_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_wayland_surface_replacement_free(
  MaplibreNativeFfiWaylandSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_wayland_surface_replacement_get_logical_size:
 *
 * The `logical_size` member.
 *
 * Returns: (transfer none): the value
 */
MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_wayland_surface_replacement_get_logical_size(
  const MaplibreNativeFfiWaylandSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_wayland_surface_replacement_set_logical_size:
 * @value: the value
 *
 * The `logical_size` member.
 */
void maplibre_native_ffi_wayland_surface_replacement_set_logical_size(
  MaplibreNativeFfiWaylandSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
);

/**
 * maplibre_native_ffi_wayland_surface_replacement_get_surface:
 *
 * Your `wl_surface*`, on the display that the session attached with.
 *
 * Returns: the value
 */
gpointer maplibre_native_ffi_wayland_surface_replacement_get_surface(
  const MaplibreNativeFfiWaylandSurfaceReplacement* self
);

/**
 * maplibre_native_ffi_wayland_surface_replacement_set_surface:
 *
 * Your `wl_surface*`, on the display that the session attached with.
 */
void maplibre_native_ffi_wayland_surface_replacement_set_surface(
  MaplibreNativeFfiWaylandSurfaceReplacement* self, gpointer value
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiWaylandSurfaceReplacement,
  maplibre_native_ffi_wayland_surface_replacement_free
)

G_END_DECLS

#endif
