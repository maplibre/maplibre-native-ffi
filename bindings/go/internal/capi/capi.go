// Package capi contains the private cgo boundary for the public MapLibre
// Native C API.
package capi

/*
#cgo CFLAGS: -std=c2x
#include <stdlib.h>
#include "maplibre_native_c.h"

static inline void* mln_go_ptr(uintptr_t value) { return (void*)value; }

static inline mln_json_value mln_go_json_null(void) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_NULL;
  return value;
}

static inline mln_json_value mln_go_json_bool(bool raw) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_BOOL;
  value.data.bool_value = raw;
  return value;
}

static inline mln_json_value mln_go_json_uint(uint64_t raw) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_UINT;
  value.data.uint_value = raw;
  return value;
}

static inline mln_json_value mln_go_json_int(int64_t raw) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_INT;
  value.data.int_value = raw;
  return value;
}

static inline mln_json_value mln_go_json_double(double raw) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_DOUBLE;
  value.data.double_value = raw;
  return value;
}

static inline mln_json_value mln_go_json_string(mln_string_view raw) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_STRING;
  value.data.string_value = raw;
  return value;
}

static inline mln_json_value mln_go_json_array(const mln_json_value* values, size_t count) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_ARRAY;
  value.data.array_value.values = values;
  value.data.array_value.value_count = count;
  return value;
}

static inline mln_json_member mln_go_json_member(mln_string_view key, const mln_json_value* raw) {
  mln_json_member member;
  member.key = key;
  member.value = raw;
  return member;
}

static inline mln_json_value mln_go_json_object(const mln_json_member* members, size_t count) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_OBJECT;
  value.data.object_value.members = members;
  value.data.object_value.member_count = count;
  return value;
}

static inline uint32_t mln_go_json_type(const mln_json_value* value) { return value->type; }
static inline bool mln_go_json_bool_value(const mln_json_value* value) { return value->data.bool_value; }
static inline uint64_t mln_go_json_uint_value(const mln_json_value* value) { return value->data.uint_value; }
static inline int64_t mln_go_json_int_value(const mln_json_value* value) { return value->data.int_value; }
static inline double mln_go_json_double_value(const mln_json_value* value) { return value->data.double_value; }
static inline mln_string_view mln_go_json_string_value(const mln_json_value* value) { return value->data.string_value; }
static inline size_t mln_go_json_array_count(const mln_json_value* value) { return value->data.array_value.value_count; }
static inline const mln_json_value* mln_go_json_array_get(const mln_json_value* value, size_t index) { return &value->data.array_value.values[index]; }
static inline size_t mln_go_json_object_count(const mln_json_value* value) { return value->data.object_value.member_count; }
static inline mln_string_view mln_go_json_object_key(const mln_json_value* value, size_t index) { return value->data.object_value.members[index].key; }
static inline const mln_json_value* mln_go_json_object_value(const mln_json_value* value, size_t index) { return value->data.object_value.members[index].value; }

static inline mln_geometry mln_go_geometry_empty(void) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_EMPTY;
  return geometry;
}

static inline mln_geometry mln_go_geometry_point(mln_lat_lng point) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_POINT;
  geometry.data.point = point;
  return geometry;
}

static inline mln_coordinate_span mln_go_coordinate_span(const mln_lat_lng* coordinates, size_t count) {
  mln_coordinate_span span;
  span.coordinates = coordinates;
  span.coordinate_count = count;
  return span;
}

static inline mln_geometry mln_go_geometry_line_string(mln_coordinate_span span) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_LINE_STRING;
  geometry.data.line_string = span;
  return geometry;
}

static inline mln_geometry mln_go_geometry_polygon(const mln_coordinate_span* rings, size_t count) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_POLYGON;
  geometry.data.polygon.rings = rings;
  geometry.data.polygon.ring_count = count;
  return geometry;
}

static inline mln_geometry mln_go_geometry_multi_point(mln_coordinate_span span) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_MULTI_POINT;
  geometry.data.multi_point = span;
  return geometry;
}

static inline mln_geometry mln_go_geometry_multi_line_string(const mln_coordinate_span* lines, size_t count) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_MULTI_LINE_STRING;
  geometry.data.multi_line_string.lines = lines;
  geometry.data.multi_line_string.line_count = count;
  return geometry;
}

static inline mln_geometry mln_go_geometry_multi_polygon(const mln_polygon_geometry* polygons, size_t count) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_MULTI_POLYGON;
  geometry.data.multi_polygon.polygons = polygons;
  geometry.data.multi_polygon.polygon_count = count;
  return geometry;
}

static inline mln_geometry mln_go_geometry_collection(const mln_geometry* geometries, size_t count) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION;
  geometry.data.geometry_collection.geometries = geometries;
  geometry.data.geometry_collection.geometry_count = count;
  return geometry;
}

static inline mln_feature mln_go_feature_null(const mln_geometry* geometry, const mln_json_member* properties, size_t count) {
  mln_feature feature;
  feature.size = sizeof(mln_feature);
  feature.geometry = geometry;
  feature.properties = properties;
  feature.property_count = count;
  feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_NULL;
  return feature;
}

static inline mln_feature mln_go_feature_uint(const mln_geometry* geometry, const mln_json_member* properties, size_t count, uint64_t id) {
  mln_feature feature = mln_go_feature_null(geometry, properties, count);
  feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_UINT;
  feature.identifier.uint_value = id;
  return feature;
}

static inline mln_feature mln_go_feature_int(const mln_geometry* geometry, const mln_json_member* properties, size_t count, int64_t id) {
  mln_feature feature = mln_go_feature_null(geometry, properties, count);
  feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_INT;
  feature.identifier.int_value = id;
  return feature;
}

static inline mln_feature mln_go_feature_double(const mln_geometry* geometry, const mln_json_member* properties, size_t count, double id) {
  mln_feature feature = mln_go_feature_null(geometry, properties, count);
  feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE;
  feature.identifier.double_value = id;
  return feature;
}

static inline mln_feature mln_go_feature_string(const mln_geometry* geometry, const mln_json_member* properties, size_t count, mln_string_view id) {
  mln_feature feature = mln_go_feature_null(geometry, properties, count);
  feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_STRING;
  feature.identifier.string_value = id;
  return feature;
}

static inline mln_geojson mln_go_geojson_geometry(const mln_geometry* geometry) {
  mln_geojson geojson;
  geojson.size = sizeof(mln_geojson);
  geojson.type = MLN_GEOJSON_TYPE_GEOMETRY;
  geojson.data.geometry = geometry;
  return geojson;
}

static inline mln_geojson mln_go_geojson_feature(const mln_feature* feature) {
  mln_geojson geojson;
  geojson.size = sizeof(mln_geojson);
  geojson.type = MLN_GEOJSON_TYPE_FEATURE;
  geojson.data.feature = feature;
  return geojson;
}

static inline mln_geojson mln_go_geojson_feature_collection(const mln_feature* features, size_t count) {
  mln_geojson geojson;
  geojson.size = sizeof(mln_geojson);
  geojson.type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION;
  geojson.data.feature_collection.features = features;
  geojson.data.feature_collection.feature_count = count;
  return geojson;
}

static inline mln_offline_region_definition mln_go_offline_tile_pyramid_region_definition(
  const char* style_url, mln_lat_lng_bounds bounds, double min_zoom, double max_zoom,
  float pixel_ratio, bool include_ideographs
) {
  mln_offline_region_definition definition;
  definition.size = sizeof(mln_offline_region_definition);
  definition.type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
  definition.data.tile_pyramid.size = sizeof(mln_offline_tile_pyramid_region_definition);
  definition.data.tile_pyramid.style_url = style_url;
  definition.data.tile_pyramid.bounds = bounds;
  definition.data.tile_pyramid.min_zoom = min_zoom;
  definition.data.tile_pyramid.max_zoom = max_zoom;
  definition.data.tile_pyramid.pixel_ratio = pixel_ratio;
  definition.data.tile_pyramid.include_ideographs = include_ideographs;
  return definition;
}

static inline uint32_t mln_go_offline_region_info_definition_type(
  const mln_offline_region_info* info
) {
  return info->definition.type;
}

static inline mln_offline_tile_pyramid_region_definition mln_go_offline_region_info_tile_pyramid(
  const mln_offline_region_info* info
) {
  return info->definition.data.tile_pyramid;
}
*/
import "C"
import (
	"fmt"
	"math"
	"unsafe"
)

// Runtime is an opaque native runtime handle.
type Runtime struct{ _ byte }

// Map is an opaque native map handle.
type Map struct{ _ byte }

// Projection is an opaque native map projection handle.
type Projection struct{ _ byte }

// RenderSession is an opaque native render session handle.
type RenderSession struct{ _ byte }

// StyleIDList is an opaque native style ID list handle.
type StyleIDList struct{ _ byte }

// JSONSnapshot is an opaque native JSON snapshot handle.
type JSONSnapshot struct{ _ byte }

// RuntimeOptions contains semantic runtime creation options.
type RuntimeOptions struct {
	AssetPath        string
	CachePath        string
	MaximumCacheSize *uint64
}

// MapOptions contains semantic map creation options.
type MapOptions struct {
	Width       uint32
	Height      uint32
	ScaleFactor float64
	MapMode     uint32
}

// LatLng is a geographic coordinate in degrees.
type LatLng struct {
	Latitude  float64
	Longitude float64
}

// ScreenPoint is a logical pixel coordinate.
type ScreenPoint struct {
	X float64
	Y float64
}

// EdgeInsets is a screen-space inset in logical map pixels.
type EdgeInsets struct {
	Top    float64
	Left   float64
	Bottom float64
	Right  float64
}

// RenderTargetExtent is a logical render target extent.
type RenderTargetExtent struct {
	Width       uint32
	Height      uint32
	ScaleFactor float64
}

// MetalContextDescriptor contains Metal backend context handles.
type MetalContextDescriptor struct {
	Device uintptr
}

// VulkanContextDescriptor contains Vulkan backend context handles.
type VulkanContextDescriptor struct {
	Instance                 uintptr
	PhysicalDevice           uintptr
	Device                   uintptr
	GraphicsQueue            uintptr
	GraphicsQueueFamilyIndex uint32
}

// MetalSurfaceDescriptor contains Metal surface attachment fields.
type MetalSurfaceDescriptor struct {
	Extent  RenderTargetExtent
	Context MetalContextDescriptor
	Layer   uintptr
}

// VulkanSurfaceDescriptor contains Vulkan surface attachment fields.
type VulkanSurfaceDescriptor struct {
	Extent  RenderTargetExtent
	Context VulkanContextDescriptor
	Surface uintptr
}

// MetalOwnedTextureDescriptor contains Metal owned-texture attachment fields.
type MetalOwnedTextureDescriptor struct {
	Extent  RenderTargetExtent
	Context MetalContextDescriptor
}

// MetalBorrowedTextureDescriptor contains Metal borrowed-texture attachment fields.
type MetalBorrowedTextureDescriptor struct {
	Extent  RenderTargetExtent
	Texture uintptr
}

// VulkanOwnedTextureDescriptor contains Vulkan owned-texture attachment fields.
type VulkanOwnedTextureDescriptor struct {
	Extent  RenderTargetExtent
	Context VulkanContextDescriptor
}

// VulkanBorrowedTextureDescriptor contains Vulkan borrowed-texture attachment fields.
type VulkanBorrowedTextureDescriptor struct {
	Extent        RenderTargetExtent
	Context       VulkanContextDescriptor
	Image         uintptr
	ImageView     uintptr
	Format        uint32
	InitialLayout uint32
	FinalLayout   uint32
}

// TextureImageInfo contains CPU readback metadata.
type TextureImageInfo struct {
	Width      uint32
	Height     uint32
	Stride     uint32
	ByteLength uint64
}

// StyleSourceInfo contains fixed metadata for one style source.
type StyleSourceInfo struct {
	Type            uint32
	IDSize          uint64
	IsVolatile      bool
	HasAttribution  bool
	AttributionSize uint64
}

// StyleTileSourceOptions contains semantic vector/raster tile source options.
type StyleTileSourceOptions struct {
	Fields         uint32
	MinZoom        float64
	MaxZoom        float64
	Attribution    string
	Scheme         uint32
	Bounds         LatLngBounds
	TileSize       uint32
	VectorEncoding uint32
	RasterEncoding uint32
}

// CanonicalTileID identifies one canonical tile.
type CanonicalTileID struct {
	Z uint32
	X uint32
	Y uint32
}

// PremultipliedRGBA8Image contains caller-owned premultiplied RGBA8 pixels.
type PremultipliedRGBA8Image struct {
	Width      uint32
	Height     uint32
	Stride     uint32
	Pixels     []byte
	ByteLength uint64
}

// StyleImageOptions contains runtime style image options.
type StyleImageOptions struct {
	Fields     uint32
	PixelRatio float32
	SDF        bool
}

// StyleImageInfo contains runtime style image metadata.
type StyleImageInfo struct {
	Width      uint32
	Height     uint32
	Stride     uint32
	ByteLength uint64
	PixelRatio float32
	SDF        bool
}

// Geometry contains a semantic GeoJSON geometry descriptor.
type Geometry struct {
	Type       uint32
	Point      LatLng
	Points     []LatLng
	Lines      [][]LatLng
	Polygons   [][][]LatLng
	Geometries []Geometry
}

// Feature contains a semantic GeoJSON feature descriptor.
type Feature struct {
	Geometry   Geometry
	Properties map[string]any
	Identifier any
}

// GeoJSON contains a semantic GeoJSON root descriptor.
type GeoJSON struct {
	Type     uint32
	Geometry Geometry
	Feature  Feature
	Features []Feature
}

// MetalOwnedTextureFrame is a copied Metal owned texture frame descriptor.
type MetalOwnedTextureFrame struct {
	Generation  uint64
	Width       uint32
	Height      uint32
	ScaleFactor float64
	FrameID     uint64
	Texture     uintptr
	Device      uintptr
	PixelFormat uint64
}

// VulkanOwnedTextureFrame is a copied Vulkan owned texture frame descriptor.
type VulkanOwnedTextureFrame struct {
	Generation  uint64
	Width       uint32
	Height      uint32
	ScaleFactor float64
	FrameID     uint64
	Image       uintptr
	ImageView   uintptr
	Device      uintptr
	Format      uint32
	Layout      uint32
}

// ProjectedMeters is a spherical Mercator coordinate in meters.
type ProjectedMeters struct {
	Northing float64
	Easting  float64
}

// LatLngBounds is a geographic bounds rectangle in degrees.
type LatLngBounds struct {
	Southwest LatLng
	Northeast LatLng
}

// ViewportOptions contains semantic viewport and render-transform controls.
type ViewportOptions struct {
	Fields           uint32
	NorthOrientation uint32
	ConstrainMode    uint32
	ViewportMode     uint32
	FrustumOffset    EdgeInsets
}

// TileOptions contains semantic tile prefetch and LOD controls.
type TileOptions struct {
	Fields            uint32
	PrefetchZoomDelta uint32
	LODMinRadius      float64
	LODScale          float64
	LODPitchThreshold float64
	LODZoomShift      float64
	LODMode           uint32
}

// ProjectionModeOptions contains semantic axonometric projection controls.
type ProjectionModeOptions struct {
	Fields      uint32
	Axonometric bool
	XSkew       float64
	YSkew       float64
}

// CameraOptions contains semantic camera snapshot and command fields.
type CameraOptions struct {
	Fields         uint32
	Center         LatLng
	CenterAltitude float64
	Padding        EdgeInsets
	Anchor         ScreenPoint
	Zoom           float64
	Bearing        float64
	Pitch          float64
	Roll           float64
	FieldOfView    float64
}

// UnitBezier contains cubic easing curve control points.
type UnitBezier struct {
	X1 float64
	Y1 float64
	X2 float64
	Y2 float64
}

// AnimationOptions contains semantic camera animation controls.
type AnimationOptions struct {
	Fields     uint32
	DurationMS float64
	Velocity   float64
	MinZoom    float64
	Easing     UnitBezier
}

// CameraFitOptions contains semantic camera fit controls.
type CameraFitOptions struct {
	Fields  uint32
	Padding EdgeInsets
	Bearing float64
	Pitch   float64
}

// BoundOptions contains semantic map camera constraint fields.
type BoundOptions struct {
	Fields   uint32
	Bounds   LatLngBounds
	MinZoom  float64
	MaxZoom  float64
	MinPitch float64
	MaxPitch float64
}

// Vec3 is a three-component vector.
type Vec3 struct {
	X float64
	Y float64
	Z float64
}

// Quaternion stores x, y, z, w components.
type Quaternion struct {
	X float64
	Y float64
	Z float64
	W float64
}

// FreeCameraOptions contains semantic free camera fields.
type FreeCameraOptions struct {
	Fields      uint32
	Position    Vec3
	Orientation Quaternion
}

// OfflineTilePyramidRegionDefinition contains tile-pyramid offline region data.
type OfflineTilePyramidRegionDefinition struct {
	StyleURL          string
	Bounds            LatLngBounds
	MinZoom           float64
	MaxZoom           float64
	PixelRatio        float32
	IncludeIdeographs bool
}

// OfflineRegionInfo is a copied native offline region snapshot.
type OfflineRegionInfo struct {
	ID             int64
	DefinitionType uint32
	TilePyramid    OfflineTilePyramidRegionDefinition
	Metadata       []byte
}

// OfflineRegionStatus is a copied native offline region status snapshot.
type OfflineRegionStatus struct {
	DownloadState                  uint32
	CompletedResourceCount         uint64
	CompletedResourceSize          uint64
	CompletedTileCount             uint64
	RequiredTileCount              uint64
	CompletedTileSize              uint64
	RequiredResourceCount          uint64
	RequiredResourceCountIsPrecise bool
	Complete                       bool
}

// RuntimeEvent is a copied native runtime event.
type RuntimeEvent struct {
	Type        uint32
	SourceType  uint32
	Source      uintptr
	Code        int32
	PayloadType uint32
	PayloadSize uintptr
	Message     string
	Payload     any
}

// RuntimeEventOfflineRegionStatusPayload is a copied offline status event payload.
type RuntimeEventOfflineRegionStatusPayload struct {
	RegionID int64
	Status   OfflineRegionStatus
}

// RuntimeEventOfflineRegionResponseErrorPayload is a copied offline response error payload.
type RuntimeEventOfflineRegionResponseErrorPayload struct {
	RegionID int64
	Reason   uint32
}

// RuntimeEventOfflineRegionTileCountLimitPayload is a copied offline tile-count limit payload.
type RuntimeEventOfflineRegionTileCountLimitPayload struct {
	RegionID int64
	Limit    uint64
}

// RuntimeEventOfflineOperationCompletedPayload is a copied offline operation completion payload.
type RuntimeEventOfflineOperationCompletedPayload struct {
	OperationID   uint64
	OperationKind uint32
	ResultKind    uint32
	ResultStatus  int32
	Found         bool
}

// Status is the raw C status value returned by fallible C API calls.
type Status int32

const (
	StatusOK              Status = Status(C.MLN_STATUS_OK)
	StatusInvalidArgument Status = Status(C.MLN_STATUS_INVALID_ARGUMENT)
	StatusInvalidState    Status = Status(C.MLN_STATUS_INVALID_STATE)
	StatusWrongThread     Status = Status(C.MLN_STATUS_WRONG_THREAD)
	StatusUnsupported     Status = Status(C.MLN_STATUS_UNSUPPORTED)
	StatusNativeError     Status = Status(C.MLN_STATUS_NATIVE_ERROR)
)

const (
	RenderBackendFlagMetal  uint32 = uint32(C.MLN_RENDER_BACKEND_FLAG_METAL)
	RenderBackendFlagVulkan uint32 = uint32(C.MLN_RENDER_BACKEND_FLAG_VULKAN)
)

const (
	NetworkStatusOnline  uint32 = uint32(C.MLN_NETWORK_STATUS_ONLINE)
	NetworkStatusOffline uint32 = uint32(C.MLN_NETWORK_STATUS_OFFLINE)
)

const (
	RuntimeOptionMaximumCacheSize uint32 = uint32(C.MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE)
)

const (
	MapModeContinuous uint32 = uint32(C.MLN_MAP_MODE_CONTINUOUS)
	MapModeStatic     uint32 = uint32(C.MLN_MAP_MODE_STATIC)
	MapModeTile       uint32 = uint32(C.MLN_MAP_MODE_TILE)
)

const (
	MapDebugTileBorders uint32 = uint32(C.MLN_MAP_DEBUG_TILE_BORDERS)
	MapDebugParseStatus uint32 = uint32(C.MLN_MAP_DEBUG_PARSE_STATUS)
	MapDebugTimestamps  uint32 = uint32(C.MLN_MAP_DEBUG_TIMESTAMPS)
	MapDebugCollision   uint32 = uint32(C.MLN_MAP_DEBUG_COLLISION)
	MapDebugOverdraw    uint32 = uint32(C.MLN_MAP_DEBUG_OVERDRAW)
	MapDebugStencilClip uint32 = uint32(C.MLN_MAP_DEBUG_STENCIL_CLIP)
	MapDebugDepthBuffer uint32 = uint32(C.MLN_MAP_DEBUG_DEPTH_BUFFER)
)

const (
	NorthOrientationUp    uint32 = uint32(C.MLN_NORTH_ORIENTATION_UP)
	NorthOrientationRight uint32 = uint32(C.MLN_NORTH_ORIENTATION_RIGHT)
	NorthOrientationDown  uint32 = uint32(C.MLN_NORTH_ORIENTATION_DOWN)
	NorthOrientationLeft  uint32 = uint32(C.MLN_NORTH_ORIENTATION_LEFT)
)

const (
	ConstrainModeNone           uint32 = uint32(C.MLN_CONSTRAIN_MODE_NONE)
	ConstrainModeHeightOnly     uint32 = uint32(C.MLN_CONSTRAIN_MODE_HEIGHT_ONLY)
	ConstrainModeWidthAndHeight uint32 = uint32(C.MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT)
	ConstrainModeScreen         uint32 = uint32(C.MLN_CONSTRAIN_MODE_SCREEN)
)

const (
	ViewportModeDefault  uint32 = uint32(C.MLN_VIEWPORT_MODE_DEFAULT)
	ViewportModeFlippedY uint32 = uint32(C.MLN_VIEWPORT_MODE_FLIPPED_Y)
)

const (
	ViewportOptionNorthOrientation uint32 = uint32(C.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION)
	ViewportOptionConstrainMode    uint32 = uint32(C.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE)
	ViewportOptionViewportMode     uint32 = uint32(C.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE)
	ViewportOptionFrustumOffset    uint32 = uint32(C.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET)
)

const (
	TileLODModeDefault  uint32 = uint32(C.MLN_TILE_LOD_MODE_DEFAULT)
	TileLODModeDistance uint32 = uint32(C.MLN_TILE_LOD_MODE_DISTANCE)
)

const (
	TileOptionPrefetchZoomDelta uint32 = uint32(C.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA)
	TileOptionLODMinRadius      uint32 = uint32(C.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS)
	TileOptionLODScale          uint32 = uint32(C.MLN_MAP_TILE_OPTION_LOD_SCALE)
	TileOptionLODPitchThreshold uint32 = uint32(C.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD)
	TileOptionLODZoomShift      uint32 = uint32(C.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT)
	TileOptionLODMode           uint32 = uint32(C.MLN_MAP_TILE_OPTION_LOD_MODE)
)

const (
	ProjectionModeAxonometric uint32 = uint32(C.MLN_PROJECTION_MODE_AXONOMETRIC)
	ProjectionModeXSkew       uint32 = uint32(C.MLN_PROJECTION_MODE_X_SKEW)
	ProjectionModeYSkew       uint32 = uint32(C.MLN_PROJECTION_MODE_Y_SKEW)
)

const (
	CameraOptionCenter         uint32 = uint32(C.MLN_CAMERA_OPTION_CENTER)
	CameraOptionZoom           uint32 = uint32(C.MLN_CAMERA_OPTION_ZOOM)
	CameraOptionBearing        uint32 = uint32(C.MLN_CAMERA_OPTION_BEARING)
	CameraOptionPitch          uint32 = uint32(C.MLN_CAMERA_OPTION_PITCH)
	CameraOptionCenterAltitude uint32 = uint32(C.MLN_CAMERA_OPTION_CENTER_ALTITUDE)
	CameraOptionPadding        uint32 = uint32(C.MLN_CAMERA_OPTION_PADDING)
	CameraOptionAnchor         uint32 = uint32(C.MLN_CAMERA_OPTION_ANCHOR)
	CameraOptionRoll           uint32 = uint32(C.MLN_CAMERA_OPTION_ROLL)
	CameraOptionFOV            uint32 = uint32(C.MLN_CAMERA_OPTION_FOV)
)

const (
	AnimationOptionDuration uint32 = uint32(C.MLN_ANIMATION_OPTION_DURATION)
	AnimationOptionVelocity uint32 = uint32(C.MLN_ANIMATION_OPTION_VELOCITY)
	AnimationOptionMinZoom  uint32 = uint32(C.MLN_ANIMATION_OPTION_MIN_ZOOM)
	AnimationOptionEasing   uint32 = uint32(C.MLN_ANIMATION_OPTION_EASING)
)

const (
	CameraFitOptionPadding uint32 = uint32(C.MLN_CAMERA_FIT_OPTION_PADDING)
	CameraFitOptionBearing uint32 = uint32(C.MLN_CAMERA_FIT_OPTION_BEARING)
	CameraFitOptionPitch   uint32 = uint32(C.MLN_CAMERA_FIT_OPTION_PITCH)
)

const (
	BoundOptionBounds   uint32 = uint32(C.MLN_BOUND_OPTION_BOUNDS)
	BoundOptionMinZoom  uint32 = uint32(C.MLN_BOUND_OPTION_MIN_ZOOM)
	BoundOptionMaxZoom  uint32 = uint32(C.MLN_BOUND_OPTION_MAX_ZOOM)
	BoundOptionMinPitch uint32 = uint32(C.MLN_BOUND_OPTION_MIN_PITCH)
	BoundOptionMaxPitch uint32 = uint32(C.MLN_BOUND_OPTION_MAX_PITCH)
)

const (
	FreeCameraOptionPosition    uint32 = uint32(C.MLN_FREE_CAMERA_OPTION_POSITION)
	FreeCameraOptionOrientation uint32 = uint32(C.MLN_FREE_CAMERA_OPTION_ORIENTATION)
)

const (
	LogSeverityInfo    uint32 = uint32(C.MLN_LOG_SEVERITY_INFO)
	LogSeverityWarning uint32 = uint32(C.MLN_LOG_SEVERITY_WARNING)
	LogSeverityError   uint32 = uint32(C.MLN_LOG_SEVERITY_ERROR)
)

const (
	LogSeverityMaskInfo    uint32 = uint32(C.MLN_LOG_SEVERITY_MASK_INFO)
	LogSeverityMaskWarning uint32 = uint32(C.MLN_LOG_SEVERITY_MASK_WARNING)
	LogSeverityMaskError   uint32 = uint32(C.MLN_LOG_SEVERITY_MASK_ERROR)
	LogSeverityMaskDefault uint32 = uint32(C.MLN_LOG_SEVERITY_MASK_DEFAULT)
	LogSeverityMaskAll     uint32 = uint32(C.MLN_LOG_SEVERITY_MASK_ALL)
)

const (
	LogEventGeneral     uint32 = uint32(C.MLN_LOG_EVENT_GENERAL)
	LogEventSetup       uint32 = uint32(C.MLN_LOG_EVENT_SETUP)
	LogEventShader      uint32 = uint32(C.MLN_LOG_EVENT_SHADER)
	LogEventParseStyle  uint32 = uint32(C.MLN_LOG_EVENT_PARSE_STYLE)
	LogEventParseTile   uint32 = uint32(C.MLN_LOG_EVENT_PARSE_TILE)
	LogEventRender      uint32 = uint32(C.MLN_LOG_EVENT_RENDER)
	LogEventStyle       uint32 = uint32(C.MLN_LOG_EVENT_STYLE)
	LogEventDatabase    uint32 = uint32(C.MLN_LOG_EVENT_DATABASE)
	LogEventHTTPRequest uint32 = uint32(C.MLN_LOG_EVENT_HTTP_REQUEST)
	LogEventSprite      uint32 = uint32(C.MLN_LOG_EVENT_SPRITE)
	LogEventImage       uint32 = uint32(C.MLN_LOG_EVENT_IMAGE)
	LogEventOpenGL      uint32 = uint32(C.MLN_LOG_EVENT_OPENGL)
	LogEventJNI         uint32 = uint32(C.MLN_LOG_EVENT_JNI)
	LogEventAndroid     uint32 = uint32(C.MLN_LOG_EVENT_ANDROID)
	LogEventCrash       uint32 = uint32(C.MLN_LOG_EVENT_CRASH)
	LogEventGlyph       uint32 = uint32(C.MLN_LOG_EVENT_GLYPH)
	LogEventTiming      uint32 = uint32(C.MLN_LOG_EVENT_TIMING)
)

const (
	RuntimeEventMapCameraWillChange                 uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE)
	RuntimeEventMapCameraIsChanging                 uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING)
	RuntimeEventMapCameraDidChange                  uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE)
	RuntimeEventMapStyleLoaded                      uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
	RuntimeEventMapLoadingStarted                   uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_LOADING_STARTED)
	RuntimeEventMapLoadingFinished                  uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED)
	RuntimeEventMapLoadingFailed                    uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_LOADING_FAILED)
	RuntimeEventMapIdle                             uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_IDLE)
	RuntimeEventMapRenderUpdateAvailable            uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE)
	RuntimeEventMapRenderError                      uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_RENDER_ERROR)
	RuntimeEventMapStillImageFinished               uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED)
	RuntimeEventMapStillImageFailed                 uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED)
	RuntimeEventMapRenderFrameStarted               uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED)
	RuntimeEventMapRenderFrameFinished              uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED)
	RuntimeEventMapRenderMapStarted                 uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED)
	RuntimeEventMapRenderMapFinished                uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED)
	RuntimeEventMapStyleImageMissing                uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING)
	RuntimeEventMapTileAction                       uint32 = uint32(C.MLN_RUNTIME_EVENT_MAP_TILE_ACTION)
	RuntimeEventOfflineRegionStatusChanged          uint32 = uint32(C.MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED)
	RuntimeEventOfflineRegionResponseError          uint32 = uint32(C.MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR)
	RuntimeEventOfflineRegionTileCountLimitExceeded uint32 = uint32(C.MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED)
	RuntimeEventOfflineOperationCompleted           uint32 = uint32(C.MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED)
)

const (
	RuntimeEventSourceRuntime uint32 = uint32(C.MLN_RUNTIME_EVENT_SOURCE_RUNTIME)
	RuntimeEventSourceMap     uint32 = uint32(C.MLN_RUNTIME_EVENT_SOURCE_MAP)
)

const (
	RuntimeEventPayloadNone                        uint32 = uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_NONE)
	RuntimeEventPayloadRenderFrame                 uint32 = uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME)
	RuntimeEventPayloadRenderMap                   uint32 = uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP)
	RuntimeEventPayloadStyleImageMissing           uint32 = uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING)
	RuntimeEventPayloadTileAction                  uint32 = uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION)
	RuntimeEventPayloadOfflineRegionStatus         uint32 = uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS)
	RuntimeEventPayloadOfflineRegionResponseError  uint32 = uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR)
	RuntimeEventPayloadOfflineRegionTileCountLimit uint32 = uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT)
	RuntimeEventPayloadOfflineOperationCompleted   uint32 = uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED)
)

const (
	ResourceKindUnknown     uint32 = uint32(C.MLN_RESOURCE_KIND_UNKNOWN)
	ResourceKindStyle       uint32 = uint32(C.MLN_RESOURCE_KIND_STYLE)
	ResourceKindSource      uint32 = uint32(C.MLN_RESOURCE_KIND_SOURCE)
	ResourceKindTile        uint32 = uint32(C.MLN_RESOURCE_KIND_TILE)
	ResourceKindGlyphs      uint32 = uint32(C.MLN_RESOURCE_KIND_GLYPHS)
	ResourceKindSpriteImage uint32 = uint32(C.MLN_RESOURCE_KIND_SPRITE_IMAGE)
	ResourceKindSpriteJSON  uint32 = uint32(C.MLN_RESOURCE_KIND_SPRITE_JSON)
	ResourceKindImage       uint32 = uint32(C.MLN_RESOURCE_KIND_IMAGE)
)

const (
	ResourceLoadingMethodAll         uint32 = uint32(C.MLN_RESOURCE_LOADING_METHOD_ALL)
	ResourceLoadingMethodCacheOnly   uint32 = uint32(C.MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY)
	ResourceLoadingMethodNetworkOnly uint32 = uint32(C.MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY)
)

const (
	ResourcePriorityRegular uint32 = uint32(C.MLN_RESOURCE_PRIORITY_REGULAR)
	ResourcePriorityLow     uint32 = uint32(C.MLN_RESOURCE_PRIORITY_LOW)
)

const (
	ResourceUsageOnline  uint32 = uint32(C.MLN_RESOURCE_USAGE_ONLINE)
	ResourceUsageOffline uint32 = uint32(C.MLN_RESOURCE_USAGE_OFFLINE)
)

const (
	ResourceStoragePolicyPermanent uint32 = uint32(C.MLN_RESOURCE_STORAGE_POLICY_PERMANENT)
	ResourceStoragePolicyVolatile  uint32 = uint32(C.MLN_RESOURCE_STORAGE_POLICY_VOLATILE)
)

const (
	StyleSourceTypeUnknown      uint32 = uint32(C.MLN_STYLE_SOURCE_TYPE_UNKNOWN)
	StyleSourceTypeVector       uint32 = uint32(C.MLN_STYLE_SOURCE_TYPE_VECTOR)
	StyleSourceTypeRaster       uint32 = uint32(C.MLN_STYLE_SOURCE_TYPE_RASTER)
	StyleSourceTypeRasterDEM    uint32 = uint32(C.MLN_STYLE_SOURCE_TYPE_RASTER_DEM)
	StyleSourceTypeGeoJSON      uint32 = uint32(C.MLN_STYLE_SOURCE_TYPE_GEOJSON)
	StyleSourceTypeImage        uint32 = uint32(C.MLN_STYLE_SOURCE_TYPE_IMAGE)
	StyleSourceTypeVideo        uint32 = uint32(C.MLN_STYLE_SOURCE_TYPE_VIDEO)
	StyleSourceTypeAnnotations  uint32 = uint32(C.MLN_STYLE_SOURCE_TYPE_ANNOTATIONS)
	StyleSourceTypeCustomVector uint32 = uint32(C.MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR)
)

const (
	JSONValueTypeNull   uint32 = uint32(C.MLN_JSON_VALUE_TYPE_NULL)
	JSONValueTypeBool   uint32 = uint32(C.MLN_JSON_VALUE_TYPE_BOOL)
	JSONValueTypeUint   uint32 = uint32(C.MLN_JSON_VALUE_TYPE_UINT)
	JSONValueTypeInt    uint32 = uint32(C.MLN_JSON_VALUE_TYPE_INT)
	JSONValueTypeDouble uint32 = uint32(C.MLN_JSON_VALUE_TYPE_DOUBLE)
	JSONValueTypeString uint32 = uint32(C.MLN_JSON_VALUE_TYPE_STRING)
	JSONValueTypeArray  uint32 = uint32(C.MLN_JSON_VALUE_TYPE_ARRAY)
	JSONValueTypeObject uint32 = uint32(C.MLN_JSON_VALUE_TYPE_OBJECT)
)

const (
	GeometryTypeEmpty              uint32 = uint32(C.MLN_GEOMETRY_TYPE_EMPTY)
	GeometryTypePoint              uint32 = uint32(C.MLN_GEOMETRY_TYPE_POINT)
	GeometryTypeLineString         uint32 = uint32(C.MLN_GEOMETRY_TYPE_LINE_STRING)
	GeometryTypePolygon            uint32 = uint32(C.MLN_GEOMETRY_TYPE_POLYGON)
	GeometryTypeMultiPoint         uint32 = uint32(C.MLN_GEOMETRY_TYPE_MULTI_POINT)
	GeometryTypeMultiLineString    uint32 = uint32(C.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING)
	GeometryTypeMultiPolygon       uint32 = uint32(C.MLN_GEOMETRY_TYPE_MULTI_POLYGON)
	GeometryTypeGeometryCollection uint32 = uint32(C.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION)
)

const (
	GeoJSONTypeGeometry          uint32 = uint32(C.MLN_GEOJSON_TYPE_GEOMETRY)
	GeoJSONTypeFeature           uint32 = uint32(C.MLN_GEOJSON_TYPE_FEATURE)
	GeoJSONTypeFeatureCollection uint32 = uint32(C.MLN_GEOJSON_TYPE_FEATURE_COLLECTION)
)

const (
	StyleTileSourceOptionMinZoom        uint32 = uint32(C.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM)
	StyleTileSourceOptionMaxZoom        uint32 = uint32(C.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM)
	StyleTileSourceOptionAttribution    uint32 = uint32(C.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION)
	StyleTileSourceOptionScheme         uint32 = uint32(C.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME)
	StyleTileSourceOptionBounds         uint32 = uint32(C.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS)
	StyleTileSourceOptionTileSize       uint32 = uint32(C.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE)
	StyleTileSourceOptionVectorEncoding uint32 = uint32(C.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING)
	StyleTileSourceOptionRasterEncoding uint32 = uint32(C.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING)
)

const (
	CustomGeometrySourceOptionMinZoom   uint32 = uint32(C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM)
	CustomGeometrySourceOptionMaxZoom   uint32 = uint32(C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM)
	CustomGeometrySourceOptionTolerance uint32 = uint32(C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE)
	CustomGeometrySourceOptionTileSize  uint32 = uint32(C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE)
	CustomGeometrySourceOptionBuffer    uint32 = uint32(C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER)
	CustomGeometrySourceOptionClip      uint32 = uint32(C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP)
	CustomGeometrySourceOptionWrap      uint32 = uint32(C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP)
)

const (
	StyleImageOptionPixelRatio uint32 = uint32(C.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO)
	StyleImageOptionSDF        uint32 = uint32(C.MLN_STYLE_IMAGE_OPTION_SDF)
)

const (
	StyleTileSchemeXYZ uint32 = uint32(C.MLN_STYLE_TILE_SCHEME_XYZ)
	StyleTileSchemeTMS uint32 = uint32(C.MLN_STYLE_TILE_SCHEME_TMS)
)

const (
	StyleVectorTileEncodingMVT uint32 = uint32(C.MLN_STYLE_VECTOR_TILE_ENCODING_MVT)
	StyleVectorTileEncodingMLT uint32 = uint32(C.MLN_STYLE_VECTOR_TILE_ENCODING_MLT)
)

const (
	StyleRasterDEMEncodingMapbox    uint32 = uint32(C.MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX)
	StyleRasterDEMEncodingTerrarium uint32 = uint32(C.MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM)
)

const (
	LocationIndicatorImageKindTop     uint32 = uint32(C.MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP)
	LocationIndicatorImageKindBearing uint32 = uint32(C.MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING)
	LocationIndicatorImageKindShadow  uint32 = uint32(C.MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW)
)

const (
	ResourceResponseStatusOK          uint32 = uint32(C.MLN_RESOURCE_RESPONSE_STATUS_OK)
	ResourceResponseStatusError       uint32 = uint32(C.MLN_RESOURCE_RESPONSE_STATUS_ERROR)
	ResourceResponseStatusNoContent   uint32 = uint32(C.MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT)
	ResourceResponseStatusNotModified uint32 = uint32(C.MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED)
)

const (
	ResourceErrorReasonNone       uint32 = uint32(C.MLN_RESOURCE_ERROR_REASON_NONE)
	ResourceErrorReasonNotFound   uint32 = uint32(C.MLN_RESOURCE_ERROR_REASON_NOT_FOUND)
	ResourceErrorReasonServer     uint32 = uint32(C.MLN_RESOURCE_ERROR_REASON_SERVER)
	ResourceErrorReasonConnection uint32 = uint32(C.MLN_RESOURCE_ERROR_REASON_CONNECTION)
	ResourceErrorReasonRateLimit  uint32 = uint32(C.MLN_RESOURCE_ERROR_REASON_RATE_LIMIT)
	ResourceErrorReasonOther      uint32 = uint32(C.MLN_RESOURCE_ERROR_REASON_OTHER)
)

const (
	ResourceProviderDecisionPassThrough uint32 = uint32(C.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH)
	ResourceProviderDecisionHandle      uint32 = uint32(C.MLN_RESOURCE_PROVIDER_DECISION_HANDLE)
	ResourceProviderDecisionUnknown     uint32 = ^uint32(0)
)

const (
	AmbientCacheOperationResetDatabase uint32 = uint32(C.MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE)
	AmbientCacheOperationPackDatabase  uint32 = uint32(C.MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE)
	AmbientCacheOperationInvalidate    uint32 = uint32(C.MLN_AMBIENT_CACHE_OPERATION_INVALIDATE)
	AmbientCacheOperationClear         uint32 = uint32(C.MLN_AMBIENT_CACHE_OPERATION_CLEAR)
)

const (
	OfflineRegionDefinitionTilePyramid uint32 = uint32(C.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID)
	OfflineRegionDefinitionGeometry    uint32 = uint32(C.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY)
)

const (
	OfflineRegionDownloadInactive uint32 = uint32(C.MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE)
	OfflineRegionDownloadActive   uint32 = uint32(C.MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE)
)

const (
	OfflineOperationAmbientCache           uint32 = uint32(C.MLN_OFFLINE_OPERATION_AMBIENT_CACHE)
	OfflineOperationRegionCreate           uint32 = uint32(C.MLN_OFFLINE_OPERATION_REGION_CREATE)
	OfflineOperationRegionGet              uint32 = uint32(C.MLN_OFFLINE_OPERATION_REGION_GET)
	OfflineOperationRegionsList            uint32 = uint32(C.MLN_OFFLINE_OPERATION_REGIONS_LIST)
	OfflineOperationRegionsMergeDatabase   uint32 = uint32(C.MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE)
	OfflineOperationRegionUpdateMetadata   uint32 = uint32(C.MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA)
	OfflineOperationRegionGetStatus        uint32 = uint32(C.MLN_OFFLINE_OPERATION_REGION_GET_STATUS)
	OfflineOperationRegionSetObserved      uint32 = uint32(C.MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED)
	OfflineOperationRegionSetDownloadState uint32 = uint32(C.MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE)
	OfflineOperationRegionInvalidate       uint32 = uint32(C.MLN_OFFLINE_OPERATION_REGION_INVALIDATE)
	OfflineOperationRegionDelete           uint32 = uint32(C.MLN_OFFLINE_OPERATION_REGION_DELETE)
)

const (
	OfflineOperationResultNone           uint32 = uint32(C.MLN_OFFLINE_OPERATION_RESULT_NONE)
	OfflineOperationResultRegion         uint32 = uint32(C.MLN_OFFLINE_OPERATION_RESULT_REGION)
	OfflineOperationResultOptionalRegion uint32 = uint32(C.MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION)
	OfflineOperationResultRegionList     uint32 = uint32(C.MLN_OFFLINE_OPERATION_RESULT_REGION_LIST)
	OfflineOperationResultRegionStatus   uint32 = uint32(C.MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS)
)

// CVersion returns the linked native C ABI contract version.
func CVersion() uint32 {
	return uint32(C.mln_c_version())
}

// SupportedRenderBackendMask returns the raw backend support mask.
func SupportedRenderBackendMask() uint32 {
	return uint32(C.mln_supported_render_backend_mask())
}

// NetworkStatusGet reads MapLibre Native's process-global network status.
func NetworkStatusGet(out *uint32) Status {
	var raw C.uint32_t
	status := Status(C.mln_network_status_get(&raw))
	if status == StatusOK {
		*out = uint32(raw)
	}
	return status
}

// NetworkStatusSet sets MapLibre Native's process-global network status.
func NetworkStatusSet(status uint32) Status {
	return Status(C.mln_network_status_set(C.uint32_t(status)))
}

// ThreadLastErrorMessage copies the current thread-local C diagnostic.
func ThreadLastErrorMessage() string {
	return C.GoString(C.mln_thread_last_error_message())
}

// RuntimeCreateDefault creates a runtime with native default options.
func RuntimeCreateDefault(out **Runtime) Status {
	var raw *C.mln_runtime
	status := Status(C.mln_runtime_create(nil, &raw))
	if status == StatusOK {
		*out = (*Runtime)(unsafe.Pointer(raw))
	}
	return status
}

// RuntimeCreate creates a runtime with explicit options.
func RuntimeCreate(options RuntimeOptions, out **Runtime) Status {
	rawOptions := C.mln_runtime_options_default()
	assetPath := C.CString(options.AssetPath)
	defer C.free(unsafe.Pointer(assetPath))
	cachePath := C.CString(options.CachePath)
	defer C.free(unsafe.Pointer(cachePath))
	rawOptions.asset_path = assetPath
	rawOptions.cache_path = cachePath
	if options.MaximumCacheSize != nil {
		rawOptions.flags |= C.uint32_t(RuntimeOptionMaximumCacheSize)
		rawOptions.maximum_cache_size = C.uint64_t(*options.MaximumCacheSize)
	}

	var raw *C.mln_runtime
	status := Status(C.mln_runtime_create(&rawOptions, &raw))
	if status == StatusOK {
		*out = (*Runtime)(unsafe.Pointer(raw))
	}
	return status
}

// RuntimeRunAmbientCacheOperationStart starts an ambient cache operation.
func RuntimeRunAmbientCacheOperationStart(runtime *Runtime, operation uint32, out *uint64) Status {
	var raw C.mln_offline_operation_id
	status := Status(C.mln_runtime_run_ambient_cache_operation_start(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.uint32_t(operation),
		&raw,
	))
	if status == StatusOK {
		*out = uint64(raw)
	}
	return status
}

// RuntimeOfflineOperationDiscard discards runtime-owned operation state.
func RuntimeOfflineOperationDiscard(runtime *Runtime, operationID uint64) Status {
	return Status(C.mln_runtime_offline_operation_discard(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_operation_id(operationID),
	))
}

// RuntimeOfflineRegionCreateStart starts creating a tile-pyramid offline region.
func RuntimeOfflineRegionCreateStart(runtime *Runtime, definition OfflineTilePyramidRegionDefinition, metadata []byte, out *uint64) Status {
	styleURL := C.CString(definition.StyleURL)
	defer C.free(unsafe.Pointer(styleURL))
	rawDefinition := C.mln_go_offline_tile_pyramid_region_definition(
		styleURL,
		latLngBoundsToC(definition.Bounds),
		C.double(definition.MinZoom),
		C.double(definition.MaxZoom),
		C.float(definition.PixelRatio),
		C.bool(definition.IncludeIdeographs),
	)
	metadataPointer, metadataSize := bytesToC(metadata)
	defer freeOptional(metadataPointer)
	var rawID C.mln_offline_operation_id
	status := Status(C.mln_runtime_offline_region_create_start(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		&rawDefinition,
		(*C.uint8_t)(metadataPointer),
		metadataSize,
		&rawID,
	))
	if status == StatusOK {
		*out = uint64(rawID)
	}
	return status
}

// RuntimeOfflineRegionGetStart starts getting an offline region by ID.
func RuntimeOfflineRegionGetStart(runtime *Runtime, regionID int64, out *uint64) Status {
	var rawID C.mln_offline_operation_id
	status := Status(C.mln_runtime_offline_region_get_start(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_region_id(regionID),
		&rawID,
	))
	if status == StatusOK {
		*out = uint64(rawID)
	}
	return status
}

// RuntimeOfflineRegionsListStart starts listing offline regions.
func RuntimeOfflineRegionsListStart(runtime *Runtime, out *uint64) Status {
	var rawID C.mln_offline_operation_id
	status := Status(C.mln_runtime_offline_regions_list_start((*C.mln_runtime)(unsafe.Pointer(runtime)), &rawID))
	if status == StatusOK {
		*out = uint64(rawID)
	}
	return status
}

// RuntimeOfflineRegionsMergeDatabaseStart starts merging a side database.
func RuntimeOfflineRegionsMergeDatabaseStart(runtime *Runtime, path string, out *uint64) Status {
	cPath := C.CString(path)
	defer C.free(unsafe.Pointer(cPath))
	var rawID C.mln_offline_operation_id
	status := Status(C.mln_runtime_offline_regions_merge_database_start(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		cPath,
		&rawID,
	))
	if status == StatusOK {
		*out = uint64(rawID)
	}
	return status
}

// RuntimeOfflineRegionUpdateMetadataStart starts updating offline region metadata.
func RuntimeOfflineRegionUpdateMetadataStart(runtime *Runtime, regionID int64, metadata []byte, out *uint64) Status {
	metadataPointer, metadataSize := bytesToC(metadata)
	defer freeOptional(metadataPointer)
	var rawID C.mln_offline_operation_id
	status := Status(C.mln_runtime_offline_region_update_metadata_start(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_region_id(regionID),
		(*C.uint8_t)(metadataPointer),
		metadataSize,
		&rawID,
	))
	if status == StatusOK {
		*out = uint64(rawID)
	}
	return status
}

// RuntimeOfflineRegionGetStatusStart starts getting offline region status.
func RuntimeOfflineRegionGetStatusStart(runtime *Runtime, regionID int64, out *uint64) Status {
	var rawID C.mln_offline_operation_id
	status := Status(C.mln_runtime_offline_region_get_status_start(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_region_id(regionID),
		&rawID,
	))
	if status == StatusOK {
		*out = uint64(rawID)
	}
	return status
}

// RuntimeOfflineRegionSetObservedStart starts setting observation state.
func RuntimeOfflineRegionSetObservedStart(runtime *Runtime, regionID int64, observed bool, out *uint64) Status {
	var rawID C.mln_offline_operation_id
	status := Status(C.mln_runtime_offline_region_set_observed_start(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_region_id(regionID),
		C.bool(observed),
		&rawID,
	))
	if status == StatusOK {
		*out = uint64(rawID)
	}
	return status
}

// RuntimeOfflineRegionSetDownloadStateStart starts setting download state.
func RuntimeOfflineRegionSetDownloadStateStart(runtime *Runtime, regionID int64, state uint32, out *uint64) Status {
	var rawID C.mln_offline_operation_id
	status := Status(C.mln_runtime_offline_region_set_download_state_start(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_region_id(regionID),
		C.uint32_t(state),
		&rawID,
	))
	if status == StatusOK {
		*out = uint64(rawID)
	}
	return status
}

// RuntimeOfflineRegionInvalidateStart starts invalidating an offline region.
func RuntimeOfflineRegionInvalidateStart(runtime *Runtime, regionID int64, out *uint64) Status {
	var rawID C.mln_offline_operation_id
	status := Status(C.mln_runtime_offline_region_invalidate_start(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_region_id(regionID),
		&rawID,
	))
	if status == StatusOK {
		*out = uint64(rawID)
	}
	return status
}

// RuntimeOfflineRegionDeleteStart starts deleting an offline region.
func RuntimeOfflineRegionDeleteStart(runtime *Runtime, regionID int64, out *uint64) Status {
	var rawID C.mln_offline_operation_id
	status := Status(C.mln_runtime_offline_region_delete_start(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_region_id(regionID),
		&rawID,
	))
	if status == StatusOK {
		*out = uint64(rawID)
	}
	return status
}

// RuntimeOfflineRegionCreateTakeResult takes and copies a create result.
func RuntimeOfflineRegionCreateTakeResult(runtime *Runtime, operationID uint64, out *OfflineRegionInfo) Status {
	var snapshot *C.mln_offline_region_snapshot
	status := Status(C.mln_runtime_offline_region_create_take_result(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_operation_id(operationID),
		&snapshot,
	))
	if status != StatusOK {
		return status
	}
	defer C.mln_offline_region_snapshot_destroy(snapshot)
	return offlineRegionSnapshotCopy(snapshot, out)
}

// RuntimeOfflineRegionGetTakeResult takes and copies an optional get result.
func RuntimeOfflineRegionGetTakeResult(runtime *Runtime, operationID uint64, out *OfflineRegionInfo, found *bool) Status {
	var snapshot *C.mln_offline_region_snapshot
	var rawFound C.bool
	status := Status(C.mln_runtime_offline_region_get_take_result(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_operation_id(operationID),
		&snapshot,
		&rawFound,
	))
	if status != StatusOK {
		return status
	}
	*found = bool(rawFound)
	if !bool(rawFound) {
		*out = OfflineRegionInfo{}
		return StatusOK
	}
	defer C.mln_offline_region_snapshot_destroy(snapshot)
	return offlineRegionSnapshotCopy(snapshot, out)
}

// RuntimeOfflineRegionsListTakeResult takes and copies a list result.
func RuntimeOfflineRegionsListTakeResult(runtime *Runtime, operationID uint64, out *[]OfflineRegionInfo) Status {
	var list *C.mln_offline_region_list
	status := Status(C.mln_runtime_offline_regions_list_take_result(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_operation_id(operationID),
		&list,
	))
	if status != StatusOK {
		return status
	}
	defer C.mln_offline_region_list_destroy(list)
	return offlineRegionListCopy(list, out)
}

// RuntimeOfflineRegionsMergeDatabaseTakeResult takes and copies a merge result.
func RuntimeOfflineRegionsMergeDatabaseTakeResult(runtime *Runtime, operationID uint64, out *[]OfflineRegionInfo) Status {
	var list *C.mln_offline_region_list
	status := Status(C.mln_runtime_offline_regions_merge_database_take_result(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_operation_id(operationID),
		&list,
	))
	if status != StatusOK {
		return status
	}
	defer C.mln_offline_region_list_destroy(list)
	return offlineRegionListCopy(list, out)
}

// RuntimeOfflineRegionUpdateMetadataTakeResult takes and copies an update result.
func RuntimeOfflineRegionUpdateMetadataTakeResult(runtime *Runtime, operationID uint64, out *OfflineRegionInfo) Status {
	var snapshot *C.mln_offline_region_snapshot
	status := Status(C.mln_runtime_offline_region_update_metadata_take_result(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_operation_id(operationID),
		&snapshot,
	))
	if status != StatusOK {
		return status
	}
	defer C.mln_offline_region_snapshot_destroy(snapshot)
	return offlineRegionSnapshotCopy(snapshot, out)
}

// RuntimeOfflineRegionGetStatusTakeResult takes and copies a status result.
func RuntimeOfflineRegionGetStatusTakeResult(runtime *Runtime, operationID uint64, out *OfflineRegionStatus) Status {
	rawStatus := C.mln_offline_region_status{size: C.uint32_t(unsafe.Sizeof(C.mln_offline_region_status{}))}
	status := Status(C.mln_runtime_offline_region_get_status_take_result(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		C.mln_offline_operation_id(operationID),
		&rawStatus,
	))
	if status == StatusOK {
		*out = offlineRegionStatusFromC(rawStatus)
	}
	return status
}

func offlineRegionSnapshotCopy(snapshot *C.mln_offline_region_snapshot, out *OfflineRegionInfo) Status {
	rawInfo := C.mln_offline_region_info{size: C.uint32_t(unsafe.Sizeof(C.mln_offline_region_info{}))}
	status := Status(C.mln_offline_region_snapshot_get(snapshot, &rawInfo))
	if status == StatusOK {
		*out = offlineRegionInfoFromC(rawInfo)
	}
	return status
}

func offlineRegionListCopy(list *C.mln_offline_region_list, out *[]OfflineRegionInfo) Status {
	var count C.size_t
	status := Status(C.mln_offline_region_list_count(list, &count))
	if status != StatusOK {
		return status
	}
	regions := make([]OfflineRegionInfo, 0, int(count))
	for index := C.size_t(0); index < count; index++ {
		rawInfo := C.mln_offline_region_info{size: C.uint32_t(unsafe.Sizeof(C.mln_offline_region_info{}))}
		status := Status(C.mln_offline_region_list_get(list, index, &rawInfo))
		if status != StatusOK {
			return status
		}
		regions = append(regions, offlineRegionInfoFromC(rawInfo))
	}
	*out = regions
	return StatusOK
}

func offlineRegionInfoFromC(info C.mln_offline_region_info) OfflineRegionInfo {
	copied := OfflineRegionInfo{
		ID:             int64(info.id),
		DefinitionType: uint32(C.mln_go_offline_region_info_definition_type(&info)),
	}
	if info.metadata != nil && info.metadata_size > 0 {
		copied.Metadata = C.GoBytes(unsafe.Pointer(info.metadata), C.int(info.metadata_size))
	}
	if copied.DefinitionType == OfflineRegionDefinitionTilePyramid {
		copied.TilePyramid = offlineTilePyramidDefinitionFromC(C.mln_go_offline_region_info_tile_pyramid(&info))
	}
	return copied
}

func offlineTilePyramidDefinitionFromC(definition C.mln_offline_tile_pyramid_region_definition) OfflineTilePyramidRegionDefinition {
	return OfflineTilePyramidRegionDefinition{
		StyleURL:          C.GoString(definition.style_url),
		Bounds:            latLngBoundsFromC(definition.bounds),
		MinZoom:           float64(definition.min_zoom),
		MaxZoom:           float64(definition.max_zoom),
		PixelRatio:        float32(definition.pixel_ratio),
		IncludeIdeographs: bool(definition.include_ideographs),
	}
}

func offlineRegionStatusFromC(status C.mln_offline_region_status) OfflineRegionStatus {
	return OfflineRegionStatus{
		DownloadState:                  uint32(status.download_state),
		CompletedResourceCount:         uint64(status.completed_resource_count),
		CompletedResourceSize:          uint64(status.completed_resource_size),
		CompletedTileCount:             uint64(status.completed_tile_count),
		RequiredTileCount:              uint64(status.required_tile_count),
		CompletedTileSize:              uint64(status.completed_tile_size),
		RequiredResourceCount:          uint64(status.required_resource_count),
		RequiredResourceCountIsPrecise: bool(status.required_resource_count_is_precise),
		Complete:                       bool(status.complete),
	}
}

func bytesToC(bytes []byte) (unsafe.Pointer, C.size_t) {
	if len(bytes) == 0 {
		return nil, 0
	}
	return C.CBytes(bytes), C.size_t(len(bytes))
}

func freeOptional(pointer unsafe.Pointer) {
	if pointer != nil {
		C.free(pointer)
	}
}

// RuntimeDestroy destroys a runtime handle.
func RuntimeDestroy(runtime *Runtime) Status {
	return Status(C.mln_runtime_destroy((*C.mln_runtime)(unsafe.Pointer(runtime))))
}

// RuntimeRunOnce runs one pending owner-thread task for a runtime.
func RuntimeRunOnce(runtime *Runtime) Status {
	return Status(C.mln_runtime_run_once((*C.mln_runtime)(unsafe.Pointer(runtime))))
}

// RuntimePollEvent polls and copies one runtime event.
func RuntimePollEvent(runtime *Runtime, out *RuntimeEvent, hasEvent *bool) Status {
	rawEvent := C.mln_runtime_event{size: C.uint32_t(unsafe.Sizeof(C.mln_runtime_event{}))}
	var rawHasEvent C.bool
	status := Status(C.mln_runtime_poll_event(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		&rawEvent,
		&rawHasEvent,
	))
	if status == StatusOK {
		*hasEvent = bool(rawHasEvent)
		if bool(rawHasEvent) {
			*out = runtimeEventFromC(rawEvent)
		} else {
			*out = RuntimeEvent{}
		}
	}
	return status
}

func runtimeEventFromC(event C.mln_runtime_event) RuntimeEvent {
	message := ""
	if event.message != nil && event.message_size > 0 {
		message = C.GoStringN(event.message, C.int(event.message_size))
	}
	copied := RuntimeEvent{
		Type:        uint32(event._type),
		SourceType:  uint32(event.source_type),
		Source:      uintptr(event.source),
		Code:        int32(event.code),
		PayloadType: uint32(event.payload_type),
		PayloadSize: uintptr(event.payload_size),
		Message:     message,
	}
	if event.payload != nil {
		copied.Payload = runtimeEventPayloadFromC(event)
	}
	return copied
}

func runtimeEventPayloadFromC(event C.mln_runtime_event) any {
	switch uint32(event.payload_type) {
	case RuntimeEventPayloadOfflineRegionStatus:
		if event.payload_size < C.size_t(unsafe.Sizeof(C.mln_runtime_event_offline_region_status{})) {
			return nil
		}
		payload := (*C.mln_runtime_event_offline_region_status)(event.payload)
		return RuntimeEventOfflineRegionStatusPayload{
			RegionID: int64(payload.region_id),
			Status:   offlineRegionStatusFromC(payload.status),
		}
	case RuntimeEventPayloadOfflineRegionResponseError:
		if event.payload_size < C.size_t(unsafe.Sizeof(C.mln_runtime_event_offline_region_response_error{})) {
			return nil
		}
		payload := (*C.mln_runtime_event_offline_region_response_error)(event.payload)
		return RuntimeEventOfflineRegionResponseErrorPayload{RegionID: int64(payload.region_id), Reason: uint32(payload.reason)}
	case RuntimeEventPayloadOfflineRegionTileCountLimit:
		if event.payload_size < C.size_t(unsafe.Sizeof(C.mln_runtime_event_offline_region_tile_count_limit{})) {
			return nil
		}
		payload := (*C.mln_runtime_event_offline_region_tile_count_limit)(event.payload)
		return RuntimeEventOfflineRegionTileCountLimitPayload{RegionID: int64(payload.region_id), Limit: uint64(payload.limit)}
	case RuntimeEventPayloadOfflineOperationCompleted:
		if event.payload_size < C.size_t(unsafe.Sizeof(C.mln_runtime_event_offline_operation_completed{})) {
			return nil
		}
		payload := (*C.mln_runtime_event_offline_operation_completed)(event.payload)
		return RuntimeEventOfflineOperationCompletedPayload{
			OperationID:   uint64(payload.operation_id),
			OperationKind: uint32(payload.operation_kind),
			ResultKind:    uint32(payload.result_kind),
			ResultStatus:  int32(payload.result_status),
			Found:         bool(payload.found),
		}
	default:
		return nil
	}
}

// MapCreateDefault creates a map with native default options.
func MapCreateDefault(runtime *Runtime, out **Map) Status {
	options := C.mln_map_options_default()
	return MapCreate(runtime, mapOptionsFromC(options), out)
}

// MapCreate creates a map with explicit options.
func MapCreate(runtime *Runtime, options MapOptions, out **Map) Status {
	rawOptions := C.mln_map_options_default()
	rawOptions.width = C.uint32_t(options.Width)
	rawOptions.height = C.uint32_t(options.Height)
	rawOptions.scale_factor = C.double(options.ScaleFactor)
	rawOptions.map_mode = C.uint32_t(options.MapMode)
	var raw *C.mln_map
	status := Status(C.mln_map_create(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		&rawOptions,
		&raw,
	))
	if status == StatusOK {
		*out = (*Map)(unsafe.Pointer(raw))
	}
	return status
}

func mapOptionsFromC(options C.mln_map_options) MapOptions {
	return MapOptions{
		Width:       uint32(options.width),
		Height:      uint32(options.height),
		ScaleFactor: float64(options.scale_factor),
		MapMode:     uint32(options.map_mode),
	}
}

// MapRequestRepaint requests a repaint for a continuous map.
func MapRequestRepaint(m *Map) Status {
	return Status(C.mln_map_request_repaint((*C.mln_map)(unsafe.Pointer(m))))
}

// MapRequestStillImage requests one still image for a static or tile map.
func MapRequestStillImage(m *Map) Status {
	return Status(C.mln_map_request_still_image((*C.mln_map)(unsafe.Pointer(m))))
}

// MapDestroy destroys a map handle.
func MapDestroy(m *Map) Status {
	return Status(C.mln_map_destroy((*C.mln_map)(unsafe.Pointer(m))))
}

// MetalSurfaceAttach attaches a Metal surface render target to a map.
func MetalSurfaceAttach(m *Map, descriptor MetalSurfaceDescriptor, out **RenderSession) Status {
	rawDescriptor := metalSurfaceDescriptorToC(descriptor)
	var rawSession *C.mln_render_session
	status := Status(C.mln_metal_surface_attach((*C.mln_map)(unsafe.Pointer(m)), &rawDescriptor, &rawSession))
	if status == StatusOK {
		*out = (*RenderSession)(unsafe.Pointer(rawSession))
	}
	return status
}

// VulkanSurfaceAttach attaches a Vulkan surface render target to a map.
func VulkanSurfaceAttach(m *Map, descriptor VulkanSurfaceDescriptor, out **RenderSession) Status {
	rawDescriptor := vulkanSurfaceDescriptorToC(descriptor)
	var rawSession *C.mln_render_session
	status := Status(C.mln_vulkan_surface_attach((*C.mln_map)(unsafe.Pointer(m)), &rawDescriptor, &rawSession))
	if status == StatusOK {
		*out = (*RenderSession)(unsafe.Pointer(rawSession))
	}
	return status
}

// RenderSessionResize resizes a render session target.
func RenderSessionResize(session *RenderSession, extent RenderTargetExtent) Status {
	return Status(C.mln_render_session_resize((*C.mln_render_session)(unsafe.Pointer(session)), C.uint32_t(extent.Width), C.uint32_t(extent.Height), C.double(extent.ScaleFactor)))
}

// RenderSessionRenderUpdate renders one frame/update into the attached target.
func RenderSessionRenderUpdate(session *RenderSession) Status {
	return Status(C.mln_render_session_render_update((*C.mln_render_session)(unsafe.Pointer(session))))
}

// RenderSessionDetach detaches a render session target.
func RenderSessionDetach(session *RenderSession) Status {
	return Status(C.mln_render_session_detach((*C.mln_render_session)(unsafe.Pointer(session))))
}

// RenderSessionDestroy destroys a render session.
func RenderSessionDestroy(session *RenderSession) Status {
	return Status(C.mln_render_session_destroy((*C.mln_render_session)(unsafe.Pointer(session))))
}

// RenderSessionReduceMemoryUse asks the session to reduce memory use.
func RenderSessionReduceMemoryUse(session *RenderSession) Status {
	return Status(C.mln_render_session_reduce_memory_use((*C.mln_render_session)(unsafe.Pointer(session))))
}

// RenderSessionClearData clears render-session data.
func RenderSessionClearData(session *RenderSession) Status {
	return Status(C.mln_render_session_clear_data((*C.mln_render_session)(unsafe.Pointer(session))))
}

// RenderSessionDumpDebugLogs dumps render-session debug logs.
func RenderSessionDumpDebugLogs(session *RenderSession) Status {
	return Status(C.mln_render_session_dump_debug_logs((*C.mln_render_session)(unsafe.Pointer(session))))
}

// MetalOwnedTextureAttach attaches a Metal session-owned texture target to a map.
func MetalOwnedTextureAttach(m *Map, descriptor MetalOwnedTextureDescriptor, out **RenderSession) Status {
	rawDescriptor := metalOwnedTextureDescriptorToC(descriptor)
	var rawSession *C.mln_render_session
	status := Status(C.mln_metal_owned_texture_attach((*C.mln_map)(unsafe.Pointer(m)), &rawDescriptor, &rawSession))
	if status == StatusOK {
		*out = (*RenderSession)(unsafe.Pointer(rawSession))
	}
	return status
}

// MetalBorrowedTextureAttach attaches a Metal caller-owned texture target to a map.
func MetalBorrowedTextureAttach(m *Map, descriptor MetalBorrowedTextureDescriptor, out **RenderSession) Status {
	rawDescriptor := metalBorrowedTextureDescriptorToC(descriptor)
	var rawSession *C.mln_render_session
	status := Status(C.mln_metal_borrowed_texture_attach((*C.mln_map)(unsafe.Pointer(m)), &rawDescriptor, &rawSession))
	if status == StatusOK {
		*out = (*RenderSession)(unsafe.Pointer(rawSession))
	}
	return status
}

// VulkanOwnedTextureAttach attaches a Vulkan session-owned texture target to a map.
func VulkanOwnedTextureAttach(m *Map, descriptor VulkanOwnedTextureDescriptor, out **RenderSession) Status {
	rawDescriptor := vulkanOwnedTextureDescriptorToC(descriptor)
	var rawSession *C.mln_render_session
	status := Status(C.mln_vulkan_owned_texture_attach((*C.mln_map)(unsafe.Pointer(m)), &rawDescriptor, &rawSession))
	if status == StatusOK {
		*out = (*RenderSession)(unsafe.Pointer(rawSession))
	}
	return status
}

// VulkanBorrowedTextureAttach attaches a Vulkan caller-owned texture target to a map.
func VulkanBorrowedTextureAttach(m *Map, descriptor VulkanBorrowedTextureDescriptor, out **RenderSession) Status {
	rawDescriptor := vulkanBorrowedTextureDescriptorToC(descriptor)
	var rawSession *C.mln_render_session
	status := Status(C.mln_vulkan_borrowed_texture_attach((*C.mln_map)(unsafe.Pointer(m)), &rawDescriptor, &rawSession))
	if status == StatusOK {
		*out = (*RenderSession)(unsafe.Pointer(rawSession))
	}
	return status
}

// TextureReadPremultipliedRGBA8 reads texture session pixels into caller storage.
func TextureReadPremultipliedRGBA8(session *RenderSession, buffer []byte, out *TextureImageInfo) Status {
	var rawInfo C.mln_texture_image_info = C.mln_texture_image_info_default()
	var data *C.uint8_t
	if len(buffer) > 0 {
		data = (*C.uint8_t)(unsafe.Pointer(&buffer[0]))
	}
	status := Status(C.mln_texture_read_premultiplied_rgba8((*C.mln_render_session)(unsafe.Pointer(session)), data, C.size_t(len(buffer)), &rawInfo))
	*out = textureImageInfoFromC(rawInfo)
	return status
}

// MetalOwnedTextureAcquireFrame acquires a Metal owned texture frame.
func MetalOwnedTextureAcquireFrame(session *RenderSession, out *MetalOwnedTextureFrame) Status {
	rawFrame := C.mln_metal_owned_texture_frame{size: C.uint32_t(unsafe.Sizeof(C.mln_metal_owned_texture_frame{}))}
	status := Status(C.mln_metal_owned_texture_acquire_frame((*C.mln_render_session)(unsafe.Pointer(session)), &rawFrame))
	if status == StatusOK {
		*out = metalOwnedTextureFrameFromC(rawFrame)
	}
	return status
}

// MetalOwnedTextureReleaseFrame releases a Metal owned texture frame.
func MetalOwnedTextureReleaseFrame(session *RenderSession, frame MetalOwnedTextureFrame) Status {
	rawFrame := metalOwnedTextureFrameToC(frame)
	return Status(C.mln_metal_owned_texture_release_frame((*C.mln_render_session)(unsafe.Pointer(session)), &rawFrame))
}

// VulkanOwnedTextureAcquireFrame acquires a Vulkan owned texture frame.
func VulkanOwnedTextureAcquireFrame(session *RenderSession, out *VulkanOwnedTextureFrame) Status {
	rawFrame := C.mln_vulkan_owned_texture_frame{size: C.uint32_t(unsafe.Sizeof(C.mln_vulkan_owned_texture_frame{}))}
	status := Status(C.mln_vulkan_owned_texture_acquire_frame((*C.mln_render_session)(unsafe.Pointer(session)), &rawFrame))
	if status == StatusOK {
		*out = vulkanOwnedTextureFrameFromC(rawFrame)
	}
	return status
}

// VulkanOwnedTextureReleaseFrame releases a Vulkan owned texture frame.
func VulkanOwnedTextureReleaseFrame(session *RenderSession, frame VulkanOwnedTextureFrame) Status {
	rawFrame := vulkanOwnedTextureFrameToC(frame)
	return Status(C.mln_vulkan_owned_texture_release_frame((*C.mln_render_session)(unsafe.Pointer(session)), &rawFrame))
}

// MapSetStyleURL loads a style URL.
func MapSetStyleURL(m *Map, url string) Status {
	cURL := C.CString(url)
	defer C.free(unsafe.Pointer(cURL))
	return Status(C.mln_map_set_style_url((*C.mln_map)(unsafe.Pointer(m)), cURL))
}

// MapSetStyleJSON loads inline style JSON.
func MapSetStyleJSON(m *Map, json string) Status {
	cJSON := C.CString(json)
	defer C.free(unsafe.Pointer(cJSON))
	return Status(C.mln_map_set_style_json((*C.mln_map)(unsafe.Pointer(m)), cJSON))
}

// MapAddGeoJSONSourceURL adds a GeoJSON source that loads from a URL.
func MapAddGeoJSONSourceURL(m *Map, sourceID string, url string) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	urlView := newStringView(url)
	defer urlView.free()
	return Status(C.mln_map_add_geojson_source_url((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), urlView.raw()))
}

// MapSetGeoJSONSourceURL updates a GeoJSON source to load from a URL.
func MapSetGeoJSONSourceURL(m *Map, sourceID string, url string) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	urlView := newStringView(url)
	defer urlView.free()
	return Status(C.mln_map_set_geojson_source_url((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), urlView.raw()))
}

// MapAddGeoJSONSourceData adds a GeoJSON source with inline data.
func MapAddGeoJSONSourceData(m *Map, sourceID string, data GeoJSON) (Status, error) {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	materializer := newGeoJSONMaterializer()
	defer materializer.free()
	rawData, err := materializer.geoJSON(data)
	if err != nil {
		return StatusInvalidArgument, err
	}
	return Status(C.mln_map_add_geojson_source_data((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), &rawData)), nil
}

// MapSetGeoJSONSourceData updates a GeoJSON source with inline data.
func MapSetGeoJSONSourceData(m *Map, sourceID string, data GeoJSON) (Status, error) {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	materializer := newGeoJSONMaterializer()
	defer materializer.free()
	rawData, err := materializer.geoJSON(data)
	if err != nil {
		return StatusInvalidArgument, err
	}
	return Status(C.mln_map_set_geojson_source_data((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), &rawData)), nil
}

// MapSetCustomGeometrySourceTileData sets custom geometry source data for one tile.
func MapSetCustomGeometrySourceTileData(m *Map, sourceID string, tileID CanonicalTileID, data GeoJSON) (Status, error) {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	materializer := newGeoJSONMaterializer()
	defer materializer.free()
	rawData, err := materializer.geoJSON(data)
	if err != nil {
		return StatusInvalidArgument, err
	}
	return Status(C.mln_map_set_custom_geometry_source_tile_data((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), canonicalTileIDToC(tileID), &rawData)), nil
}

// MapInvalidateCustomGeometrySourceTile invalidates custom geometry source data for one tile.
func MapInvalidateCustomGeometrySourceTile(m *Map, sourceID string, tileID CanonicalTileID) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	return Status(C.mln_map_invalidate_custom_geometry_source_tile((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), canonicalTileIDToC(tileID)))
}

// MapInvalidateCustomGeometrySourceRegion invalidates custom geometry source data inside one region.
func MapInvalidateCustomGeometrySourceRegion(m *Map, sourceID string, bounds LatLngBounds) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	return Status(C.mln_map_invalidate_custom_geometry_source_region((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), latLngBoundsToC(bounds)))
}

// MapSetStyleImage sets one runtime style image.
func MapSetStyleImage(m *Map, imageID string, image PremultipliedRGBA8Image, options StyleImageOptions) Status {
	imageView := newStringView(imageID)
	defer imageView.free()
	rawImage, imageAllocation := premultipliedRGBA8ImageToC(image)
	defer C.free(imageAllocation)
	rawOptions := styleImageOptionsToC(options)
	return Status(C.mln_map_set_style_image((*C.mln_map)(unsafe.Pointer(m)), imageView.raw(), &rawImage, &rawOptions))
}

// MapRemoveStyleImage removes one runtime style image.
func MapRemoveStyleImage(m *Map, imageID string, outRemoved *bool) Status {
	imageView := newStringView(imageID)
	defer imageView.free()
	var rawRemoved C.bool
	status := Status(C.mln_map_remove_style_image((*C.mln_map)(unsafe.Pointer(m)), imageView.raw(), &rawRemoved))
	if status == StatusOK {
		*outRemoved = bool(rawRemoved)
	}
	return status
}

// MapStyleImageExists reports whether one runtime style image exists.
func MapStyleImageExists(m *Map, imageID string, outExists *bool) Status {
	imageView := newStringView(imageID)
	defer imageView.free()
	var rawExists C.bool
	status := Status(C.mln_map_style_image_exists((*C.mln_map)(unsafe.Pointer(m)), imageView.raw(), &rawExists))
	if status == StatusOK {
		*outExists = bool(rawExists)
	}
	return status
}

// MapGetStyleImageInfo copies fixed metadata for one runtime style image.
func MapGetStyleImageInfo(m *Map, imageID string, outInfo *StyleImageInfo, outFound *bool) Status {
	imageView := newStringView(imageID)
	defer imageView.free()
	rawInfo := C.mln_style_image_info_default()
	var rawFound C.bool
	status := Status(C.mln_map_get_style_image_info((*C.mln_map)(unsafe.Pointer(m)), imageView.raw(), &rawInfo, &rawFound))
	if status == StatusOK {
		*outInfo = styleImageInfoFromC(rawInfo)
		*outFound = bool(rawFound)
	}
	return status
}

// MapCopyStyleImagePremultipliedRGBA8 copies tightly packed premultiplied RGBA8 pixels.
func MapCopyStyleImagePremultipliedRGBA8(m *Map, imageID string, buffer []byte, outByteLength *uint64, outFound *bool) Status {
	imageView := newStringView(imageID)
	defer imageView.free()
	var data *C.uint8_t
	if len(buffer) > 0 {
		data = (*C.uint8_t)(unsafe.Pointer(&buffer[0]))
	}
	var rawByteLength C.size_t
	var rawFound C.bool
	status := Status(C.mln_map_copy_style_image_premultiplied_rgba8((*C.mln_map)(unsafe.Pointer(m)), imageView.raw(), data, C.size_t(len(buffer)), &rawByteLength, &rawFound))
	if status == StatusOK || status == StatusInvalidArgument {
		*outByteLength = uint64(rawByteLength)
		*outFound = bool(rawFound)
	}
	return status
}

// MapAddImageSourceURL adds an image source that loads from a URL.
func MapAddImageSourceURL(m *Map, sourceID string, coordinates []LatLng, url string) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	urlView := newStringView(url)
	defer urlView.free()
	rawCoordinates, coordinateCount := latLngSliceToCPointer(coordinates)
	defer C.free(unsafe.Pointer(rawCoordinates))
	return Status(C.mln_map_add_image_source_url((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), rawCoordinates, coordinateCount, urlView.raw()))
}

// MapAddImageSourceImage adds an image source with inline image pixels.
func MapAddImageSourceImage(m *Map, sourceID string, coordinates []LatLng, image PremultipliedRGBA8Image) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	rawCoordinates, coordinateCount := latLngSliceToCPointer(coordinates)
	defer C.free(unsafe.Pointer(rawCoordinates))
	rawImage, imageAllocation := premultipliedRGBA8ImageToC(image)
	defer C.free(imageAllocation)
	return Status(C.mln_map_add_image_source_image((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), rawCoordinates, coordinateCount, &rawImage))
}

// MapSetImageSourceURL updates an image source to load from a URL.
func MapSetImageSourceURL(m *Map, sourceID string, url string) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	urlView := newStringView(url)
	defer urlView.free()
	return Status(C.mln_map_set_image_source_url((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), urlView.raw()))
}

// MapSetImageSourceImage updates an image source with inline image pixels.
func MapSetImageSourceImage(m *Map, sourceID string, image PremultipliedRGBA8Image) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	rawImage, imageAllocation := premultipliedRGBA8ImageToC(image)
	defer C.free(imageAllocation)
	return Status(C.mln_map_set_image_source_image((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), &rawImage))
}

// MapSetImageSourceCoordinates updates image source coordinates.
func MapSetImageSourceCoordinates(m *Map, sourceID string, coordinates []LatLng) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	rawCoordinates, coordinateCount := latLngSliceToCPointer(coordinates)
	defer C.free(unsafe.Pointer(rawCoordinates))
	return Status(C.mln_map_set_image_source_coordinates((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), rawCoordinates, coordinateCount))
}

// MapGetImageSourceCoordinates copies image source coordinates.
func MapGetImageSourceCoordinates(m *Map, sourceID string, outCoordinates *[]LatLng, outFound *bool) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	var rawCoordinates [4]C.mln_lat_lng
	var rawCount C.size_t
	var rawFound C.bool
	status := Status(C.mln_map_get_image_source_coordinates((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), &rawCoordinates[0], C.size_t(len(rawCoordinates)), &rawCount, &rawFound))
	if status == StatusOK {
		*outFound = bool(rawFound)
		*outCoordinates = latLngSliceFromC(&rawCoordinates[0], int(rawCount))
	}
	return status
}

// MapAddVectorSourceURL adds a vector source with a TileJSON URL.
func MapAddVectorSourceURL(m *Map, sourceID string, url string, options *StyleTileSourceOptions) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	urlView := newStringView(url)
	defer urlView.free()
	rawOptions, rawOptionsPointer := styleTileSourceOptionsToCPointer(options)
	defer rawOptions.free()
	return Status(C.mln_map_add_vector_source_url((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), urlView.raw(), rawOptionsPointer))
}

// MapAddVectorSourceTiles adds a vector source with inline tile URLs.
func MapAddVectorSourceTiles(m *Map, sourceID string, tiles []string, options *StyleTileSourceOptions) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	rawTiles := newStringViewArray(tiles)
	defer rawTiles.free()
	rawOptions, rawOptionsPointer := styleTileSourceOptionsToCPointer(options)
	defer rawOptions.free()
	return Status(C.mln_map_add_vector_source_tiles((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), rawTiles.data, C.size_t(rawTiles.count), rawOptionsPointer))
}

// MapAddRasterSourceURL adds a raster source with a TileJSON URL.
func MapAddRasterSourceURL(m *Map, sourceID string, url string, options *StyleTileSourceOptions) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	urlView := newStringView(url)
	defer urlView.free()
	rawOptions, rawOptionsPointer := styleTileSourceOptionsToCPointer(options)
	defer rawOptions.free()
	return Status(C.mln_map_add_raster_source_url((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), urlView.raw(), rawOptionsPointer))
}

// MapAddRasterSourceTiles adds a raster source with inline tile URLs.
func MapAddRasterSourceTiles(m *Map, sourceID string, tiles []string, options *StyleTileSourceOptions) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	rawTiles := newStringViewArray(tiles)
	defer rawTiles.free()
	rawOptions, rawOptionsPointer := styleTileSourceOptionsToCPointer(options)
	defer rawOptions.free()
	return Status(C.mln_map_add_raster_source_tiles((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), rawTiles.data, C.size_t(rawTiles.count), rawOptionsPointer))
}

// MapAddRasterDEMSourceURL adds a raster DEM source with a TileJSON URL.
func MapAddRasterDEMSourceURL(m *Map, sourceID string, url string, options *StyleTileSourceOptions) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	urlView := newStringView(url)
	defer urlView.free()
	rawOptions, rawOptionsPointer := styleTileSourceOptionsToCPointer(options)
	defer rawOptions.free()
	return Status(C.mln_map_add_raster_dem_source_url((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), urlView.raw(), rawOptionsPointer))
}

// MapAddRasterDEMSourceTiles adds a raster DEM source with inline tile URLs.
func MapAddRasterDEMSourceTiles(m *Map, sourceID string, tiles []string, options *StyleTileSourceOptions) Status {
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	rawTiles := newStringViewArray(tiles)
	defer rawTiles.free()
	rawOptions, rawOptionsPointer := styleTileSourceOptionsToCPointer(options)
	defer rawOptions.free()
	return Status(C.mln_map_add_raster_dem_source_tiles((*C.mln_map)(unsafe.Pointer(m)), sourceView.raw(), rawTiles.data, C.size_t(rawTiles.count), rawOptionsPointer))
}

// MapAddHillshadeLayer adds a hillshade layer for a raster DEM source.
func MapAddHillshadeLayer(m *Map, layerID string, sourceID string, beforeLayerID string) Status {
	layerView := newStringView(layerID)
	defer layerView.free()
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	beforeView := newStringView(beforeLayerID)
	defer beforeView.free()
	return Status(C.mln_map_add_hillshade_layer((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), sourceView.raw(), beforeView.raw()))
}

// MapAddColorReliefLayer adds a color-relief layer for a raster DEM source.
func MapAddColorReliefLayer(m *Map, layerID string, sourceID string, beforeLayerID string) Status {
	layerView := newStringView(layerID)
	defer layerView.free()
	sourceView := newStringView(sourceID)
	defer sourceView.free()
	beforeView := newStringView(beforeLayerID)
	defer beforeView.free()
	return Status(C.mln_map_add_color_relief_layer((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), sourceView.raw(), beforeView.raw()))
}

// MapAddLocationIndicatorLayer adds a source-free location indicator layer.
func MapAddLocationIndicatorLayer(m *Map, layerID string, beforeLayerID string) Status {
	layerView := newStringView(layerID)
	defer layerView.free()
	beforeView := newStringView(beforeLayerID)
	defer beforeView.free()
	return Status(C.mln_map_add_location_indicator_layer((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), beforeView.raw()))
}

// MapSetLocationIndicatorLocation sets a location indicator layer location.
func MapSetLocationIndicatorLocation(m *Map, layerID string, coordinate LatLng, altitude float64) Status {
	layerView := newStringView(layerID)
	defer layerView.free()
	return Status(C.mln_map_set_location_indicator_location((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), latLngToC(coordinate), C.double(altitude)))
}

// MapSetLocationIndicatorBearing sets a location indicator layer bearing.
func MapSetLocationIndicatorBearing(m *Map, layerID string, bearing float64) Status {
	layerView := newStringView(layerID)
	defer layerView.free()
	return Status(C.mln_map_set_location_indicator_bearing((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), C.double(bearing)))
}

// MapSetLocationIndicatorAccuracyRadius sets a location indicator layer accuracy radius.
func MapSetLocationIndicatorAccuracyRadius(m *Map, layerID string, radius float64) Status {
	layerView := newStringView(layerID)
	defer layerView.free()
	return Status(C.mln_map_set_location_indicator_accuracy_radius((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), C.double(radius)))
}

// MapSetLocationIndicatorImageName sets one location indicator image-name property.
func MapSetLocationIndicatorImageName(m *Map, layerID string, imageKind uint32, imageID string) Status {
	layerView := newStringView(layerID)
	defer layerView.free()
	imageView := newStringView(imageID)
	defer imageView.free()
	return Status(C.mln_map_set_location_indicator_image_name((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), C.uint32_t(imageKind), imageView.raw()))
}

// MapAddStyleSourceJSON adds one style source from a style-spec JSON object.
func MapAddStyleSourceJSON(m *Map, sourceID string, sourceJSON any) (Status, error) {
	view := newStringView(sourceID)
	defer view.free()
	materializer := newJSONMaterializer()
	defer materializer.free()
	rawJSON, err := materializer.value(sourceJSON)
	if err != nil {
		return StatusInvalidArgument, err
	}
	return Status(C.mln_map_add_style_source_json((*C.mln_map)(unsafe.Pointer(m)), view.raw(), &rawJSON)), nil
}

// MapRemoveStyleSource removes one style source by ID.
func MapRemoveStyleSource(m *Map, sourceID string, outRemoved *bool) Status {
	view := newStringView(sourceID)
	defer view.free()
	var rawRemoved C.bool
	status := Status(C.mln_map_remove_style_source((*C.mln_map)(unsafe.Pointer(m)), view.raw(), &rawRemoved))
	if status == StatusOK {
		*outRemoved = bool(rawRemoved)
	}
	return status
}

// MapStyleSourceExists reports whether one style source ID exists.
func MapStyleSourceExists(m *Map, sourceID string, outExists *bool) Status {
	view := newStringView(sourceID)
	defer view.free()
	var rawExists C.bool
	status := Status(C.mln_map_style_source_exists((*C.mln_map)(unsafe.Pointer(m)), view.raw(), &rawExists))
	if status == StatusOK {
		*outExists = bool(rawExists)
	}
	return status
}

// MapGetStyleSourceType gets one style source type.
func MapGetStyleSourceType(m *Map, sourceID string, outSourceType *uint32, outFound *bool) Status {
	view := newStringView(sourceID)
	defer view.free()
	var rawType C.uint32_t
	var rawFound C.bool
	status := Status(C.mln_map_get_style_source_type((*C.mln_map)(unsafe.Pointer(m)), view.raw(), &rawType, &rawFound))
	if status == StatusOK {
		*outSourceType = uint32(rawType)
		*outFound = bool(rawFound)
	}
	return status
}

// MapGetStyleSourceInfo copies fixed metadata for one style source.
func MapGetStyleSourceInfo(m *Map, sourceID string, outInfo *StyleSourceInfo, outFound *bool) Status {
	view := newStringView(sourceID)
	defer view.free()
	rawInfo := C.mln_style_source_info{size: C.uint32_t(unsafe.Sizeof(C.mln_style_source_info{}))}
	var rawFound C.bool
	status := Status(C.mln_map_get_style_source_info((*C.mln_map)(unsafe.Pointer(m)), view.raw(), &rawInfo, &rawFound))
	if status == StatusOK {
		*outInfo = styleSourceInfoFromC(rawInfo)
		*outFound = bool(rawFound)
	}
	return status
}

// MapCopyStyleSourceAttribution copies one style source attribution.
func MapCopyStyleSourceAttribution(m *Map, sourceID string, capacity int, outAttribution *string, outFound *bool) Status {
	view := newStringView(sourceID)
	defer view.free()
	var buffer unsafe.Pointer
	if capacity > 0 {
		buffer = C.malloc(C.size_t(capacity))
		defer C.free(buffer)
	}
	var rawSize C.size_t
	var rawFound C.bool
	status := Status(C.mln_map_copy_style_source_attribution((*C.mln_map)(unsafe.Pointer(m)), view.raw(), (*C.char)(buffer), C.size_t(capacity), &rawSize, &rawFound))
	if status == StatusOK {
		*outFound = bool(rawFound)
		if rawSize == 0 {
			*outAttribution = ""
		} else {
			*outAttribution = C.GoStringN((*C.char)(buffer), C.int(rawSize))
		}
	}
	return status
}

// MapListStyleSourceIDs copies style source IDs.
func MapListStyleSourceIDs(m *Map, outIDs *[]string) Status {
	var rawList *C.mln_style_id_list
	status := Status(C.mln_map_list_style_source_ids((*C.mln_map)(unsafe.Pointer(m)), &rawList))
	if status != StatusOK {
		return status
	}
	return copyStyleIDList(rawList, outIDs)
}

// MapAddStyleLayerJSON adds one style layer from a style-spec JSON object.
func MapAddStyleLayerJSON(m *Map, layerJSON any, beforeLayerID string) (Status, error) {
	beforeView := newStringView(beforeLayerID)
	defer beforeView.free()
	materializer := newJSONMaterializer()
	defer materializer.free()
	rawJSON, err := materializer.value(layerJSON)
	if err != nil {
		return StatusInvalidArgument, err
	}
	return Status(C.mln_map_add_style_layer_json((*C.mln_map)(unsafe.Pointer(m)), &rawJSON, beforeView.raw())), nil
}

// MapRemoveStyleLayer removes one style layer by ID.
func MapRemoveStyleLayer(m *Map, layerID string, outRemoved *bool) Status {
	view := newStringView(layerID)
	defer view.free()
	var rawRemoved C.bool
	status := Status(C.mln_map_remove_style_layer((*C.mln_map)(unsafe.Pointer(m)), view.raw(), &rawRemoved))
	if status == StatusOK {
		*outRemoved = bool(rawRemoved)
	}
	return status
}

// MapStyleLayerExists reports whether one style layer ID exists.
func MapStyleLayerExists(m *Map, layerID string, outExists *bool) Status {
	view := newStringView(layerID)
	defer view.free()
	var rawExists C.bool
	status := Status(C.mln_map_style_layer_exists((*C.mln_map)(unsafe.Pointer(m)), view.raw(), &rawExists))
	if status == StatusOK {
		*outExists = bool(rawExists)
	}
	return status
}

// MapGetStyleLayerType gets one style layer type string.
func MapGetStyleLayerType(m *Map, layerID string, outLayerType *string, outFound *bool) Status {
	view := newStringView(layerID)
	defer view.free()
	var rawLayerType C.mln_string_view
	var rawFound C.bool
	status := Status(C.mln_map_get_style_layer_type((*C.mln_map)(unsafe.Pointer(m)), view.raw(), &rawLayerType, &rawFound))
	if status == StatusOK {
		*outLayerType = stringViewFromC(rawLayerType)
		*outFound = bool(rawFound)
	}
	return status
}

// MapListStyleLayerIDs copies style layer IDs.
func MapListStyleLayerIDs(m *Map, outIDs *[]string) Status {
	var rawList *C.mln_style_id_list
	status := Status(C.mln_map_list_style_layer_ids((*C.mln_map)(unsafe.Pointer(m)), &rawList))
	if status != StatusOK {
		return status
	}
	return copyStyleIDList(rawList, outIDs)
}

// MapMoveStyleLayer moves one style layer before another layer or to the top.
func MapMoveStyleLayer(m *Map, layerID string, beforeLayerID string) Status {
	layerView := newStringView(layerID)
	defer layerView.free()
	beforeView := newStringView(beforeLayerID)
	defer beforeView.free()
	return Status(C.mln_map_move_style_layer((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), beforeView.raw()))
}

// MapGetStyleLayerJSON copies one style layer as a style-spec JSON object.
func MapGetStyleLayerJSON(m *Map, layerID string, outValue *any, outFound *bool) Status {
	layerView := newStringView(layerID)
	defer layerView.free()
	var rawSnapshot *C.mln_json_snapshot
	var rawFound C.bool
	status := Status(C.mln_map_get_style_layer_json((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), &rawSnapshot, &rawFound))
	if status != StatusOK {
		return status
	}
	*outFound = bool(rawFound)
	if !bool(rawFound) {
		*outValue = nil
		return StatusOK
	}
	return jsonSnapshotToValue(rawSnapshot, outValue)
}

// MapSetStyleLightJSON sets the style light from a style-spec JSON object.
func MapSetStyleLightJSON(m *Map, lightJSON any) (Status, error) {
	materializer := newJSONMaterializer()
	defer materializer.free()
	rawJSON, err := materializer.value(lightJSON)
	if err != nil {
		return StatusInvalidArgument, err
	}
	return Status(C.mln_map_set_style_light_json((*C.mln_map)(unsafe.Pointer(m)), &rawJSON)), nil
}

// MapSetStyleLightProperty sets one style light property.
func MapSetStyleLightProperty(m *Map, propertyName string, value any) (Status, error) {
	propertyView := newStringView(propertyName)
	defer propertyView.free()
	materializer := newJSONMaterializer()
	defer materializer.free()
	rawValue, err := materializer.value(value)
	if err != nil {
		return StatusInvalidArgument, err
	}
	return Status(C.mln_map_set_style_light_property((*C.mln_map)(unsafe.Pointer(m)), propertyView.raw(), &rawValue)), nil
}

// MapGetStyleLightProperty copies one style light property.
func MapGetStyleLightProperty(m *Map, propertyName string, outValue *any) Status {
	propertyView := newStringView(propertyName)
	defer propertyView.free()
	var rawSnapshot *C.mln_json_snapshot
	status := Status(C.mln_map_get_style_light_property((*C.mln_map)(unsafe.Pointer(m)), propertyView.raw(), &rawSnapshot))
	if status != StatusOK {
		return status
	}
	return jsonSnapshotToValue(rawSnapshot, outValue)
}

// MapSetLayerProperty sets one layer property.
func MapSetLayerProperty(m *Map, layerID string, propertyName string, value any) (Status, error) {
	layerView := newStringView(layerID)
	defer layerView.free()
	propertyView := newStringView(propertyName)
	defer propertyView.free()
	materializer := newJSONMaterializer()
	defer materializer.free()
	rawValue, err := materializer.value(value)
	if err != nil {
		return StatusInvalidArgument, err
	}
	return Status(C.mln_map_set_layer_property((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), propertyView.raw(), &rawValue)), nil
}

// MapGetLayerProperty copies one layer property.
func MapGetLayerProperty(m *Map, layerID string, propertyName string, outValue *any) Status {
	layerView := newStringView(layerID)
	defer layerView.free()
	propertyView := newStringView(propertyName)
	defer propertyView.free()
	var rawSnapshot *C.mln_json_snapshot
	status := Status(C.mln_map_get_layer_property((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), propertyView.raw(), &rawSnapshot))
	if status != StatusOK {
		return status
	}
	return jsonSnapshotToValue(rawSnapshot, outValue)
}

// MapSetLayerFilter sets or clears one layer filter.
func MapSetLayerFilter(m *Map, layerID string, filter any) (Status, error) {
	layerView := newStringView(layerID)
	defer layerView.free()
	if filter == nil {
		return Status(C.mln_map_set_layer_filter((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), nil)), nil
	}
	materializer := newJSONMaterializer()
	defer materializer.free()
	rawFilter, err := materializer.value(filter)
	if err != nil {
		return StatusInvalidArgument, err
	}
	return Status(C.mln_map_set_layer_filter((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), &rawFilter)), nil
}

// MapGetLayerFilter copies one layer filter.
func MapGetLayerFilter(m *Map, layerID string, outValue *any) Status {
	layerView := newStringView(layerID)
	defer layerView.free()
	var rawSnapshot *C.mln_json_snapshot
	status := Status(C.mln_map_get_layer_filter((*C.mln_map)(unsafe.Pointer(m)), layerView.raw(), &rawSnapshot))
	if status != StatusOK {
		return status
	}
	return jsonSnapshotToValue(rawSnapshot, outValue)
}

// MapSetDebugOptions sets the map debug overlay mask.
func MapSetDebugOptions(m *Map, options uint32) Status {
	return Status(C.mln_map_set_debug_options((*C.mln_map)(unsafe.Pointer(m)), C.uint32_t(options)))
}

// MapGetDebugOptions gets the map debug overlay mask.
func MapGetDebugOptions(m *Map, out *uint32) Status {
	var raw C.uint32_t
	status := Status(C.mln_map_get_debug_options((*C.mln_map)(unsafe.Pointer(m)), &raw))
	if status == StatusOK {
		*out = uint32(raw)
	}
	return status
}

// MapSetRenderingStatsViewEnabled enables or disables the rendering stats overlay.
func MapSetRenderingStatsViewEnabled(m *Map, enabled bool) Status {
	return Status(C.mln_map_set_rendering_stats_view_enabled((*C.mln_map)(unsafe.Pointer(m)), C.bool(enabled)))
}

// MapGetRenderingStatsViewEnabled gets whether the rendering stats overlay is enabled.
func MapGetRenderingStatsViewEnabled(m *Map, out *bool) Status {
	var raw C.bool
	status := Status(C.mln_map_get_rendering_stats_view_enabled((*C.mln_map)(unsafe.Pointer(m)), &raw))
	if status == StatusOK {
		*out = bool(raw)
	}
	return status
}

// MapIsFullyLoaded gets whether native considers the map fully loaded.
func MapIsFullyLoaded(m *Map, out *bool) Status {
	var raw C.bool
	status := Status(C.mln_map_is_fully_loaded((*C.mln_map)(unsafe.Pointer(m)), &raw))
	if status == StatusOK {
		*out = bool(raw)
	}
	return status
}

// MapDumpDebugLogs dumps native debug logs.
func MapDumpDebugLogs(m *Map) Status {
	return Status(C.mln_map_dump_debug_logs((*C.mln_map)(unsafe.Pointer(m))))
}

// MapGetViewportOptions copies live viewport controls.
func MapGetViewportOptions(m *Map, out *ViewportOptions) Status {
	raw := C.mln_map_viewport_options{size: C.uint32_t(unsafe.Sizeof(C.mln_map_viewport_options{}))}
	status := Status(C.mln_map_get_viewport_options((*C.mln_map)(unsafe.Pointer(m)), &raw))
	if status == StatusOK {
		*out = viewportOptionsFromC(raw)
	}
	return status
}

// MapSetViewportOptions applies selected viewport controls.
func MapSetViewportOptions(m *Map, options ViewportOptions) Status {
	raw := C.mln_map_viewport_options_default()
	raw.fields = C.uint32_t(options.Fields)
	raw.north_orientation = C.uint32_t(options.NorthOrientation)
	raw.constrain_mode = C.uint32_t(options.ConstrainMode)
	raw.viewport_mode = C.uint32_t(options.ViewportMode)
	raw.frustum_offset = edgeInsetsToC(options.FrustumOffset)
	return Status(C.mln_map_set_viewport_options((*C.mln_map)(unsafe.Pointer(m)), &raw))
}

// MapGetTileOptions copies tile prefetch and LOD controls.
func MapGetTileOptions(m *Map, out *TileOptions) Status {
	raw := C.mln_map_tile_options{size: C.uint32_t(unsafe.Sizeof(C.mln_map_tile_options{}))}
	status := Status(C.mln_map_get_tile_options((*C.mln_map)(unsafe.Pointer(m)), &raw))
	if status == StatusOK {
		*out = tileOptionsFromC(raw)
	}
	return status
}

// MapSetTileOptions applies selected tile prefetch and LOD controls.
func MapSetTileOptions(m *Map, options TileOptions) Status {
	raw := C.mln_map_tile_options_default()
	raw.fields = C.uint32_t(options.Fields)
	raw.prefetch_zoom_delta = C.uint32_t(options.PrefetchZoomDelta)
	raw.lod_min_radius = C.double(options.LODMinRadius)
	raw.lod_scale = C.double(options.LODScale)
	raw.lod_pitch_threshold = C.double(options.LODPitchThreshold)
	raw.lod_zoom_shift = C.double(options.LODZoomShift)
	raw.lod_mode = C.uint32_t(options.LODMode)
	return Status(C.mln_map_set_tile_options((*C.mln_map)(unsafe.Pointer(m)), &raw))
}

// MapGetProjectionMode copies axonometric rendering options.
func MapGetProjectionMode(m *Map, out *ProjectionModeOptions) Status {
	raw := C.mln_projection_mode{size: C.uint32_t(unsafe.Sizeof(C.mln_projection_mode{}))}
	status := Status(C.mln_map_get_projection_mode((*C.mln_map)(unsafe.Pointer(m)), &raw))
	if status == StatusOK {
		*out = projectionModeOptionsFromC(raw)
	}
	return status
}

// MapSetProjectionMode applies axonometric rendering options.
func MapSetProjectionMode(m *Map, options ProjectionModeOptions) Status {
	raw := C.mln_projection_mode_default()
	raw.fields = C.uint32_t(options.Fields)
	raw.axonometric = C.bool(options.Axonometric)
	raw.x_skew = C.double(options.XSkew)
	raw.y_skew = C.double(options.YSkew)
	return Status(C.mln_map_set_projection_mode((*C.mln_map)(unsafe.Pointer(m)), &raw))
}

// MapGetCamera copies the current camera snapshot.
func MapGetCamera(m *Map, out *CameraOptions) Status {
	raw := C.mln_camera_options{size: C.uint32_t(unsafe.Sizeof(C.mln_camera_options{}))}
	status := Status(C.mln_map_get_camera((*C.mln_map)(unsafe.Pointer(m)), &raw))
	if status == StatusOK {
		*out = cameraOptionsFromC(raw)
	}
	return status
}

// MapJumpTo applies a camera jump command.
func MapJumpTo(m *Map, camera CameraOptions) Status {
	raw := cameraOptionsToC(camera)
	return Status(C.mln_map_jump_to((*C.mln_map)(unsafe.Pointer(m)), &raw))
}

// MapEaseTo applies a camera ease transition command.
func MapEaseTo(m *Map, camera CameraOptions, animation *AnimationOptions) Status {
	rawCamera := cameraOptionsToC(camera)
	rawAnimation, rawAnimationPointer := animationOptionsToCPointer(animation)
	_ = rawAnimation
	return Status(C.mln_map_ease_to((*C.mln_map)(unsafe.Pointer(m)), &rawCamera, rawAnimationPointer))
}

// MapFlyTo applies a camera fly transition command.
func MapFlyTo(m *Map, camera CameraOptions, animation *AnimationOptions) Status {
	rawCamera := cameraOptionsToC(camera)
	rawAnimation, rawAnimationPointer := animationOptionsToCPointer(animation)
	_ = rawAnimation
	return Status(C.mln_map_fly_to((*C.mln_map)(unsafe.Pointer(m)), &rawCamera, rawAnimationPointer))
}

// MapMoveBy applies a screen-space pan command.
func MapMoveBy(m *Map, delta ScreenPoint) Status {
	return Status(C.mln_map_move_by((*C.mln_map)(unsafe.Pointer(m)), C.double(delta.X), C.double(delta.Y)))
}

// MapMoveByAnimated applies an animated screen-space pan command.
func MapMoveByAnimated(m *Map, delta ScreenPoint, animation *AnimationOptions) Status {
	rawAnimation, rawAnimationPointer := animationOptionsToCPointer(animation)
	_ = rawAnimation
	return Status(C.mln_map_move_by_animated((*C.mln_map)(unsafe.Pointer(m)), C.double(delta.X), C.double(delta.Y), rawAnimationPointer))
}

// MapScaleBy applies a screen-space zoom command.
func MapScaleBy(m *Map, scale float64, anchor *ScreenPoint) Status {
	var rawAnchor C.mln_screen_point
	var rawAnchorPointer *C.mln_screen_point
	if anchor != nil {
		rawAnchor = screenPointToC(*anchor)
		rawAnchorPointer = &rawAnchor
	}
	return Status(C.mln_map_scale_by((*C.mln_map)(unsafe.Pointer(m)), C.double(scale), rawAnchorPointer))
}

// MapScaleByAnimated applies an animated screen-space zoom command.
func MapScaleByAnimated(m *Map, scale float64, anchor *ScreenPoint, animation *AnimationOptions) Status {
	var rawAnchor C.mln_screen_point
	var rawAnchorPointer *C.mln_screen_point
	if anchor != nil {
		rawAnchor = screenPointToC(*anchor)
		rawAnchorPointer = &rawAnchor
	}
	rawAnimation, rawAnimationPointer := animationOptionsToCPointer(animation)
	_ = rawAnimation
	return Status(C.mln_map_scale_by_animated((*C.mln_map)(unsafe.Pointer(m)), C.double(scale), rawAnchorPointer, rawAnimationPointer))
}

// MapRotateBy applies a screen-space rotate command.
func MapRotateBy(m *Map, first ScreenPoint, second ScreenPoint) Status {
	return Status(C.mln_map_rotate_by((*C.mln_map)(unsafe.Pointer(m)), screenPointToC(first), screenPointToC(second)))
}

// MapRotateByAnimated applies an animated screen-space rotate command.
func MapRotateByAnimated(m *Map, first ScreenPoint, second ScreenPoint, animation *AnimationOptions) Status {
	rawAnimation, rawAnimationPointer := animationOptionsToCPointer(animation)
	_ = rawAnimation
	return Status(C.mln_map_rotate_by_animated((*C.mln_map)(unsafe.Pointer(m)), screenPointToC(first), screenPointToC(second), rawAnimationPointer))
}

// MapPitchBy applies a pitch delta command.
func MapPitchBy(m *Map, pitch float64) Status {
	return Status(C.mln_map_pitch_by((*C.mln_map)(unsafe.Pointer(m)), C.double(pitch)))
}

// MapPitchByAnimated applies an animated pitch delta command.
func MapPitchByAnimated(m *Map, pitch float64, animation *AnimationOptions) Status {
	rawAnimation, rawAnimationPointer := animationOptionsToCPointer(animation)
	_ = rawAnimation
	return Status(C.mln_map_pitch_by_animated((*C.mln_map)(unsafe.Pointer(m)), C.double(pitch), rawAnimationPointer))
}

// MapCancelTransitions cancels active camera transitions.
func MapCancelTransitions(m *Map) Status {
	return Status(C.mln_map_cancel_transitions((*C.mln_map)(unsafe.Pointer(m))))
}

// MapCameraForLatLngBounds computes a camera that fits geographic bounds.
func MapCameraForLatLngBounds(m *Map, bounds LatLngBounds, fitOptions *CameraFitOptions, out *CameraOptions) Status {
	rawFitOptions, rawFitOptionsPointer := cameraFitOptionsToCPointer(fitOptions)
	_ = rawFitOptions
	rawCamera := C.mln_camera_options{size: C.uint32_t(unsafe.Sizeof(C.mln_camera_options{}))}
	status := Status(C.mln_map_camera_for_lat_lng_bounds((*C.mln_map)(unsafe.Pointer(m)), latLngBoundsToC(bounds), rawFitOptionsPointer, &rawCamera))
	if status == StatusOK {
		*out = cameraOptionsFromC(rawCamera)
	}
	return status
}

// MapCameraForLatLngs computes a camera that fits geographic coordinates.
func MapCameraForLatLngs(m *Map, coordinates []LatLng, fitOptions *CameraFitOptions, out *CameraOptions) Status {
	if len(coordinates) == 0 {
		return Status(C.mln_map_camera_for_lat_lngs((*C.mln_map)(unsafe.Pointer(m)), nil, 0, nil, nil))
	}
	rawCoordinates := make([]C.mln_lat_lng, len(coordinates))
	for i, coordinate := range coordinates {
		rawCoordinates[i] = latLngToC(coordinate)
	}
	rawFitOptions, rawFitOptionsPointer := cameraFitOptionsToCPointer(fitOptions)
	_ = rawFitOptions
	rawCamera := C.mln_camera_options{size: C.uint32_t(unsafe.Sizeof(C.mln_camera_options{}))}
	status := Status(C.mln_map_camera_for_lat_lngs((*C.mln_map)(unsafe.Pointer(m)), &rawCoordinates[0], C.size_t(len(rawCoordinates)), rawFitOptionsPointer, &rawCamera))
	if status == StatusOK {
		*out = cameraOptionsFromC(rawCamera)
	}
	return status
}

// MapLatLngBoundsForCamera computes wrapped geographic bounds for a camera.
func MapLatLngBoundsForCamera(m *Map, camera CameraOptions, out *LatLngBounds) Status {
	rawCamera := cameraOptionsToC(camera)
	var rawBounds C.mln_lat_lng_bounds
	status := Status(C.mln_map_lat_lng_bounds_for_camera((*C.mln_map)(unsafe.Pointer(m)), &rawCamera, &rawBounds))
	if status == StatusOK {
		*out = latLngBoundsFromC(rawBounds)
	}
	return status
}

// MapLatLngBoundsForCameraUnwrapped computes unwrapped geographic bounds for a camera.
func MapLatLngBoundsForCameraUnwrapped(m *Map, camera CameraOptions, out *LatLngBounds) Status {
	rawCamera := cameraOptionsToC(camera)
	var rawBounds C.mln_lat_lng_bounds
	status := Status(C.mln_map_lat_lng_bounds_for_camera_unwrapped((*C.mln_map)(unsafe.Pointer(m)), &rawCamera, &rawBounds))
	if status == StatusOK {
		*out = latLngBoundsFromC(rawBounds)
	}
	return status
}

// MapGetBounds copies map camera constraint options.
func MapGetBounds(m *Map, out *BoundOptions) Status {
	raw := C.mln_bound_options{size: C.uint32_t(unsafe.Sizeof(C.mln_bound_options{}))}
	status := Status(C.mln_map_get_bounds((*C.mln_map)(unsafe.Pointer(m)), &raw))
	if status == StatusOK {
		*out = boundOptionsFromC(raw)
	}
	return status
}

// MapSetBounds applies selected map camera constraint options.
func MapSetBounds(m *Map, options BoundOptions) Status {
	raw := boundOptionsToC(options)
	return Status(C.mln_map_set_bounds((*C.mln_map)(unsafe.Pointer(m)), &raw))
}

// MapGetFreeCameraOptions copies the current free camera options.
func MapGetFreeCameraOptions(m *Map, out *FreeCameraOptions) Status {
	raw := C.mln_free_camera_options{size: C.uint32_t(unsafe.Sizeof(C.mln_free_camera_options{}))}
	status := Status(C.mln_map_get_free_camera_options((*C.mln_map)(unsafe.Pointer(m)), &raw))
	if status == StatusOK {
		*out = freeCameraOptionsFromC(raw)
	}
	return status
}

// MapSetFreeCameraOptions applies selected free camera options.
func MapSetFreeCameraOptions(m *Map, options FreeCameraOptions) Status {
	raw := freeCameraOptionsToC(options)
	return Status(C.mln_map_set_free_camera_options((*C.mln_map)(unsafe.Pointer(m)), &raw))
}

// MapProjectionCreate creates a standalone projection helper from a map.
func MapProjectionCreate(m *Map, out **Projection) Status {
	var raw *C.mln_map_projection
	status := Status(C.mln_map_projection_create(
		(*C.mln_map)(unsafe.Pointer(m)),
		&raw,
	))
	if status == StatusOK {
		*out = (*Projection)(unsafe.Pointer(raw))
	}
	return status
}

// MapProjectionDestroy destroys a projection helper.
func MapProjectionDestroy(projection *Projection) Status {
	return Status(C.mln_map_projection_destroy((*C.mln_map_projection)(unsafe.Pointer(projection))))
}

// MapProjectionGetCamera copies the projection helper camera snapshot.
func MapProjectionGetCamera(projection *Projection, out *CameraOptions) Status {
	raw := C.mln_camera_options{size: C.uint32_t(unsafe.Sizeof(C.mln_camera_options{}))}
	status := Status(C.mln_map_projection_get_camera((*C.mln_map_projection)(unsafe.Pointer(projection)), &raw))
	if status == StatusOK {
		*out = cameraOptionsFromC(raw)
	}
	return status
}

// MapProjectionSetCamera applies selected camera fields to the projection helper.
func MapProjectionSetCamera(projection *Projection, camera CameraOptions) Status {
	raw := cameraOptionsToC(camera)
	return Status(C.mln_map_projection_set_camera((*C.mln_map_projection)(unsafe.Pointer(projection)), &raw))
}

// MapProjectionSetVisibleCoordinates updates the helper camera to fit coordinates.
func MapProjectionSetVisibleCoordinates(projection *Projection, coordinates []LatLng, padding EdgeInsets) Status {
	if len(coordinates) == 0 {
		return Status(C.mln_map_projection_set_visible_coordinates((*C.mln_map_projection)(unsafe.Pointer(projection)), nil, 0, edgeInsetsToC(padding)))
	}
	rawCoordinates := make([]C.mln_lat_lng, len(coordinates))
	for i, coordinate := range coordinates {
		rawCoordinates[i] = latLngToC(coordinate)
	}
	return Status(C.mln_map_projection_set_visible_coordinates(
		(*C.mln_map_projection)(unsafe.Pointer(projection)),
		&rawCoordinates[0],
		C.size_t(len(rawCoordinates)),
		edgeInsetsToC(padding),
	))
}

// MapProjectionPixelForLatLng converts a coordinate to a screen point.
func MapProjectionPixelForLatLng(projection *Projection, coordinate LatLng, out *ScreenPoint) Status {
	var rawPoint C.mln_screen_point
	status := Status(C.mln_map_projection_pixel_for_lat_lng(
		(*C.mln_map_projection)(unsafe.Pointer(projection)),
		latLngToC(coordinate),
		&rawPoint,
	))
	if status == StatusOK {
		*out = screenPointFromC(rawPoint)
	}
	return status
}

// MapProjectionLatLngForPixel converts a screen point to a coordinate.
func MapProjectionLatLngForPixel(projection *Projection, point ScreenPoint, out *LatLng) Status {
	var rawCoordinate C.mln_lat_lng
	status := Status(C.mln_map_projection_lat_lng_for_pixel(
		(*C.mln_map_projection)(unsafe.Pointer(projection)),
		screenPointToC(point),
		&rawCoordinate,
	))
	if status == StatusOK {
		*out = latLngFromC(rawCoordinate)
	}
	return status
}

// ProjectedMetersForLatLng converts a coordinate to projected meters.
func ProjectedMetersForLatLng(coordinate LatLng, out *ProjectedMeters) Status {
	var rawMeters C.mln_projected_meters
	status := Status(C.mln_projected_meters_for_lat_lng(latLngToC(coordinate), &rawMeters))
	if status == StatusOK {
		*out = projectedMetersFromC(rawMeters)
	}
	return status
}

// LatLngForProjectedMeters converts projected meters to a coordinate.
func LatLngForProjectedMeters(meters ProjectedMeters, out *LatLng) Status {
	var rawCoordinate C.mln_lat_lng
	status := Status(C.mln_lat_lng_for_projected_meters(projectedMetersToC(meters), &rawCoordinate))
	if status == StatusOK {
		*out = latLngFromC(rawCoordinate)
	}
	return status
}

type stringView struct {
	data unsafe.Pointer
	size int
}

type stringViewArray struct {
	views []stringView
	data  *C.mln_string_view
	count int
}

type rawStyleTileSourceOptions struct {
	value       C.mln_style_tile_source_options
	attribution stringView
}

func newStringView(value string) stringView {
	if len(value) == 0 {
		return stringView{}
	}
	return stringView{data: C.CBytes([]byte(value)), size: len(value)}
}

func (view stringView) raw() C.mln_string_view {
	return C.mln_string_view{data: (*C.char)(view.data), size: C.size_t(view.size)}
}

func (view stringView) free() {
	if view.data != nil {
		C.free(view.data)
	}
}

func newStringViewArray(values []string) stringViewArray {
	if len(values) == 0 {
		return stringViewArray{}
	}
	raw := (*C.mln_string_view)(C.malloc(C.size_t(len(values)) * C.size_t(unsafe.Sizeof(C.mln_string_view{}))))
	views := make([]stringView, len(values))
	for i, value := range values {
		views[i] = newStringView(value)
		*(*C.mln_string_view)(unsafe.Add(unsafe.Pointer(raw), uintptr(i)*unsafe.Sizeof(C.mln_string_view{}))) = views[i].raw()
	}
	return stringViewArray{views: views, data: raw, count: len(values)}
}

func (array stringViewArray) free() {
	for _, view := range array.views {
		view.free()
	}
	if array.data != nil {
		C.free(unsafe.Pointer(array.data))
	}
}

func stringViewFromC(view C.mln_string_view) string {
	if view.data == nil || view.size == 0 {
		return ""
	}
	return C.GoStringN(view.data, C.int(view.size))
}

func copyStyleIDList(rawList *C.mln_style_id_list, outIDs *[]string) Status {
	defer C.mln_style_id_list_destroy(rawList)
	var rawCount C.size_t
	status := Status(C.mln_style_id_list_count(rawList, &rawCount))
	if status != StatusOK {
		return status
	}
	ids := make([]string, int(rawCount))
	for i := range ids {
		var rawID C.mln_string_view
		status = Status(C.mln_style_id_list_get(rawList, C.size_t(i), &rawID))
		if status != StatusOK {
			return status
		}
		ids[i] = stringViewFromC(rawID)
	}
	*outIDs = ids
	return StatusOK
}

func styleSourceInfoFromC(info C.mln_style_source_info) StyleSourceInfo {
	return StyleSourceInfo{
		Type:            uint32(info._type),
		IDSize:          uint64(info.id_size),
		IsVolatile:      bool(info.is_volatile),
		HasAttribution:  bool(info.has_attribution),
		AttributionSize: uint64(info.attribution_size),
	}
}

func styleTileSourceOptionsToCPointer(options *StyleTileSourceOptions) (rawStyleTileSourceOptions, *C.mln_style_tile_source_options) {
	if options == nil {
		return rawStyleTileSourceOptions{}, nil
	}
	raw := rawStyleTileSourceOptions{value: C.mln_style_tile_source_options_default()}
	raw.value.fields = C.uint32_t(options.Fields)
	raw.value.min_zoom = C.double(options.MinZoom)
	raw.value.max_zoom = C.double(options.MaxZoom)
	if options.Fields&StyleTileSourceOptionAttribution != 0 {
		raw.attribution = newStringView(options.Attribution)
		raw.value.attribution = raw.attribution.raw()
	}
	raw.value.scheme = C.uint32_t(options.Scheme)
	raw.value.bounds = latLngBoundsToC(options.Bounds)
	raw.value.tile_size = C.uint32_t(options.TileSize)
	raw.value.vector_encoding = C.uint32_t(options.VectorEncoding)
	raw.value.raster_encoding = C.uint32_t(options.RasterEncoding)
	return raw, &raw.value
}

func (raw rawStyleTileSourceOptions) free() {
	raw.attribution.free()
}

type jsonMaterializer struct {
	allocations []unsafe.Pointer
}

func newJSONMaterializer() *jsonMaterializer {
	return &jsonMaterializer{}
}

func (materializer *jsonMaterializer) free() {
	for i := len(materializer.allocations) - 1; i >= 0; i-- {
		C.free(materializer.allocations[i])
	}
}

func (materializer *jsonMaterializer) alloc(size C.size_t) unsafe.Pointer {
	ptr := C.malloc(size)
	materializer.allocations = append(materializer.allocations, ptr)
	return ptr
}

func (materializer *jsonMaterializer) bytes(value string) C.mln_string_view {
	if len(value) == 0 {
		return C.mln_string_view{}
	}
	ptr := C.CBytes([]byte(value))
	materializer.allocations = append(materializer.allocations, ptr)
	return C.mln_string_view{data: (*C.char)(ptr), size: C.size_t(len(value))}
}

func (materializer *jsonMaterializer) value(value any) (C.mln_json_value, error) {
	switch typed := value.(type) {
	case nil:
		return C.mln_go_json_null(), nil
	case bool:
		return C.mln_go_json_bool(C.bool(typed)), nil
	case string:
		return C.mln_go_json_string(materializer.bytes(typed)), nil
	case int:
		return C.mln_go_json_int(C.int64_t(typed)), nil
	case int8:
		return C.mln_go_json_int(C.int64_t(typed)), nil
	case int16:
		return C.mln_go_json_int(C.int64_t(typed)), nil
	case int32:
		return C.mln_go_json_int(C.int64_t(typed)), nil
	case int64:
		return C.mln_go_json_int(C.int64_t(typed)), nil
	case uint:
		return C.mln_go_json_uint(C.uint64_t(typed)), nil
	case uint8:
		return C.mln_go_json_uint(C.uint64_t(typed)), nil
	case uint16:
		return C.mln_go_json_uint(C.uint64_t(typed)), nil
	case uint32:
		return C.mln_go_json_uint(C.uint64_t(typed)), nil
	case uint64:
		return C.mln_go_json_uint(C.uint64_t(typed)), nil
	case float32:
		return materializer.float(float64(typed))
	case float64:
		return materializer.float(typed)
	case []any:
		return materializer.array(typed)
	case []string:
		values := make([]any, len(typed))
		for i, item := range typed {
			values[i] = item
		}
		return materializer.array(values)
	case map[string]any:
		return materializer.object(typed)
	default:
		return C.mln_json_value{}, fmt.Errorf("unsupported JSON value type %T", value)
	}
}

func (materializer *jsonMaterializer) float(value float64) (C.mln_json_value, error) {
	if math.IsNaN(value) || math.IsInf(value, 0) {
		return C.mln_json_value{}, fmt.Errorf("JSON double value must be finite")
	}
	return C.mln_go_json_double(C.double(value)), nil
}

func (materializer *jsonMaterializer) array(values []any) (C.mln_json_value, error) {
	if len(values) == 0 {
		return C.mln_go_json_array(nil, 0), nil
	}
	rawValues := (*C.mln_json_value)(materializer.alloc(C.size_t(len(values)) * C.size_t(unsafe.Sizeof(C.mln_json_value{}))))
	for i, item := range values {
		rawValue, err := materializer.value(item)
		if err != nil {
			return C.mln_json_value{}, err
		}
		*(*C.mln_json_value)(unsafe.Add(unsafe.Pointer(rawValues), uintptr(i)*unsafe.Sizeof(C.mln_json_value{}))) = rawValue
	}
	return C.mln_go_json_array(rawValues, C.size_t(len(values))), nil
}

func (materializer *jsonMaterializer) object(members map[string]any) (C.mln_json_value, error) {
	rawMembers, count, err := materializer.members(members)
	if err != nil {
		return C.mln_json_value{}, err
	}
	return C.mln_go_json_object(rawMembers, count), nil
}

func (materializer *jsonMaterializer) members(members map[string]any) (*C.mln_json_member, C.size_t, error) {
	if len(members) == 0 {
		return nil, 0, nil
	}
	rawMembers := (*C.mln_json_member)(materializer.alloc(C.size_t(len(members)) * C.size_t(unsafe.Sizeof(C.mln_json_member{}))))
	i := 0
	for key, item := range members {
		rawValue, err := materializer.value(item)
		if err != nil {
			return nil, 0, err
		}
		valuePtr := (*C.mln_json_value)(materializer.alloc(C.size_t(unsafe.Sizeof(C.mln_json_value{}))))
		*valuePtr = rawValue
		*(*C.mln_json_member)(unsafe.Add(unsafe.Pointer(rawMembers), uintptr(i)*unsafe.Sizeof(C.mln_json_member{}))) = C.mln_go_json_member(materializer.bytes(key), valuePtr)
		i++
	}
	return rawMembers, C.size_t(len(members)), nil
}

type geoJSONMaterializer struct {
	json *jsonMaterializer
}

func newGeoJSONMaterializer() *geoJSONMaterializer {
	return &geoJSONMaterializer{json: newJSONMaterializer()}
}

func (materializer *geoJSONMaterializer) free() {
	materializer.json.free()
}

func (materializer *geoJSONMaterializer) alloc(size C.size_t) unsafe.Pointer {
	return materializer.json.alloc(size)
}

func (materializer *geoJSONMaterializer) geoJSON(data GeoJSON) (C.mln_geojson, error) {
	switch data.Type {
	case GeoJSONTypeGeometry:
		geometry, err := materializer.geometryPtr(data.Geometry)
		if err != nil {
			return C.mln_geojson{}, err
		}
		return C.mln_go_geojson_geometry(geometry), nil
	case GeoJSONTypeFeature:
		feature, err := materializer.featurePtr(data.Feature)
		if err != nil {
			return C.mln_geojson{}, err
		}
		return C.mln_go_geojson_feature(feature), nil
	case GeoJSONTypeFeatureCollection:
		features, err := materializer.features(data.Features)
		if err != nil {
			return C.mln_geojson{}, err
		}
		return C.mln_go_geojson_feature_collection(features, C.size_t(len(data.Features))), nil
	default:
		return C.mln_geojson{}, fmt.Errorf("unsupported GeoJSON type %d", data.Type)
	}
}

func (materializer *geoJSONMaterializer) geometryPtr(geometry Geometry) (*C.mln_geometry, error) {
	raw, err := materializer.geometry(geometry)
	if err != nil {
		return nil, err
	}
	ptr := (*C.mln_geometry)(materializer.alloc(C.size_t(unsafe.Sizeof(C.mln_geometry{}))))
	*ptr = raw
	return ptr, nil
}

func (materializer *geoJSONMaterializer) geometry(geometry Geometry) (C.mln_geometry, error) {
	switch geometry.Type {
	case GeometryTypeEmpty:
		return C.mln_go_geometry_empty(), nil
	case GeometryTypePoint:
		return C.mln_go_geometry_point(latLngToC(geometry.Point)), nil
	case GeometryTypeLineString:
		return C.mln_go_geometry_line_string(materializer.coordinateSpan(geometry.Points)), nil
	case GeometryTypePolygon:
		rings := materializer.coordinateSpans(geometry.Lines)
		return C.mln_go_geometry_polygon(rings, C.size_t(len(geometry.Lines))), nil
	case GeometryTypeMultiPoint:
		return C.mln_go_geometry_multi_point(materializer.coordinateSpan(geometry.Points)), nil
	case GeometryTypeMultiLineString:
		lines := materializer.coordinateSpans(geometry.Lines)
		return C.mln_go_geometry_multi_line_string(lines, C.size_t(len(geometry.Lines))), nil
	case GeometryTypeMultiPolygon:
		polygons := materializer.polygonGeometries(geometry.Polygons)
		return C.mln_go_geometry_multi_polygon(polygons, C.size_t(len(geometry.Polygons))), nil
	case GeometryTypeGeometryCollection:
		geometries, err := materializer.geometries(geometry.Geometries)
		if err != nil {
			return C.mln_geometry{}, err
		}
		return C.mln_go_geometry_collection(geometries, C.size_t(len(geometry.Geometries))), nil
	default:
		return C.mln_geometry{}, fmt.Errorf("unsupported geometry type %d", geometry.Type)
	}
}

func (materializer *geoJSONMaterializer) coordinateSpan(points []LatLng) C.mln_coordinate_span {
	if len(points) == 0 {
		return C.mln_go_coordinate_span(nil, 0)
	}
	rawPoints := (*C.mln_lat_lng)(materializer.alloc(C.size_t(len(points)) * C.size_t(unsafe.Sizeof(C.mln_lat_lng{}))))
	for i, point := range points {
		*(*C.mln_lat_lng)(unsafe.Add(unsafe.Pointer(rawPoints), uintptr(i)*unsafe.Sizeof(C.mln_lat_lng{}))) = latLngToC(point)
	}
	return C.mln_go_coordinate_span(rawPoints, C.size_t(len(points)))
}

func (materializer *geoJSONMaterializer) coordinateSpans(lines [][]LatLng) *C.mln_coordinate_span {
	if len(lines) == 0 {
		return nil
	}
	rawLines := (*C.mln_coordinate_span)(materializer.alloc(C.size_t(len(lines)) * C.size_t(unsafe.Sizeof(C.mln_coordinate_span{}))))
	for i, line := range lines {
		*(*C.mln_coordinate_span)(unsafe.Add(unsafe.Pointer(rawLines), uintptr(i)*unsafe.Sizeof(C.mln_coordinate_span{}))) = materializer.coordinateSpan(line)
	}
	return rawLines
}

func (materializer *geoJSONMaterializer) polygonGeometries(polygons [][][]LatLng) *C.mln_polygon_geometry {
	if len(polygons) == 0 {
		return nil
	}
	rawPolygons := (*C.mln_polygon_geometry)(materializer.alloc(C.size_t(len(polygons)) * C.size_t(unsafe.Sizeof(C.mln_polygon_geometry{}))))
	for i, polygon := range polygons {
		rings := materializer.coordinateSpans(polygon)
		*(*C.mln_polygon_geometry)(unsafe.Add(unsafe.Pointer(rawPolygons), uintptr(i)*unsafe.Sizeof(C.mln_polygon_geometry{}))) = C.mln_polygon_geometry{
			rings:      rings,
			ring_count: C.size_t(len(polygon)),
		}
	}
	return rawPolygons
}

func (materializer *geoJSONMaterializer) geometries(geometries []Geometry) (*C.mln_geometry, error) {
	if len(geometries) == 0 {
		return nil, nil
	}
	rawGeometries := (*C.mln_geometry)(materializer.alloc(C.size_t(len(geometries)) * C.size_t(unsafe.Sizeof(C.mln_geometry{}))))
	for i, geometry := range geometries {
		rawGeometry, err := materializer.geometry(geometry)
		if err != nil {
			return nil, err
		}
		*(*C.mln_geometry)(unsafe.Add(unsafe.Pointer(rawGeometries), uintptr(i)*unsafe.Sizeof(C.mln_geometry{}))) = rawGeometry
	}
	return rawGeometries, nil
}

func (materializer *geoJSONMaterializer) featurePtr(feature Feature) (*C.mln_feature, error) {
	raw, err := materializer.feature(feature)
	if err != nil {
		return nil, err
	}
	ptr := (*C.mln_feature)(materializer.alloc(C.size_t(unsafe.Sizeof(C.mln_feature{}))))
	*ptr = raw
	return ptr, nil
}

func (materializer *geoJSONMaterializer) feature(feature Feature) (C.mln_feature, error) {
	geometry, err := materializer.geometryPtr(feature.Geometry)
	if err != nil {
		return C.mln_feature{}, err
	}
	properties, propertyCount, err := materializer.json.members(feature.Properties)
	if err != nil {
		return C.mln_feature{}, err
	}
	switch id := feature.Identifier.(type) {
	case nil:
		return C.mln_go_feature_null(geometry, properties, propertyCount), nil
	case uint:
		return C.mln_go_feature_uint(geometry, properties, propertyCount, C.uint64_t(id)), nil
	case uint8:
		return C.mln_go_feature_uint(geometry, properties, propertyCount, C.uint64_t(id)), nil
	case uint16:
		return C.mln_go_feature_uint(geometry, properties, propertyCount, C.uint64_t(id)), nil
	case uint32:
		return C.mln_go_feature_uint(geometry, properties, propertyCount, C.uint64_t(id)), nil
	case uint64:
		return C.mln_go_feature_uint(geometry, properties, propertyCount, C.uint64_t(id)), nil
	case int:
		return C.mln_go_feature_int(geometry, properties, propertyCount, C.int64_t(id)), nil
	case int8:
		return C.mln_go_feature_int(geometry, properties, propertyCount, C.int64_t(id)), nil
	case int16:
		return C.mln_go_feature_int(geometry, properties, propertyCount, C.int64_t(id)), nil
	case int32:
		return C.mln_go_feature_int(geometry, properties, propertyCount, C.int64_t(id)), nil
	case int64:
		return C.mln_go_feature_int(geometry, properties, propertyCount, C.int64_t(id)), nil
	case float32:
		value := float64(id)
		if math.IsNaN(value) || math.IsInf(value, 0) {
			return C.mln_feature{}, fmt.Errorf("feature identifier float must be finite")
		}
		return C.mln_go_feature_double(geometry, properties, propertyCount, C.double(value)), nil
	case float64:
		if math.IsNaN(id) || math.IsInf(id, 0) {
			return C.mln_feature{}, fmt.Errorf("feature identifier float must be finite")
		}
		return C.mln_go_feature_double(geometry, properties, propertyCount, C.double(id)), nil
	case string:
		return C.mln_go_feature_string(geometry, properties, propertyCount, materializer.json.bytes(id)), nil
	default:
		return C.mln_feature{}, fmt.Errorf("unsupported feature identifier type %T", feature.Identifier)
	}
}

func (materializer *geoJSONMaterializer) features(features []Feature) (*C.mln_feature, error) {
	if len(features) == 0 {
		return nil, nil
	}
	rawFeatures := (*C.mln_feature)(materializer.alloc(C.size_t(len(features)) * C.size_t(unsafe.Sizeof(C.mln_feature{}))))
	for i, feature := range features {
		rawFeature, err := materializer.feature(feature)
		if err != nil {
			return nil, err
		}
		*(*C.mln_feature)(unsafe.Add(unsafe.Pointer(rawFeatures), uintptr(i)*unsafe.Sizeof(C.mln_feature{}))) = rawFeature
	}
	return rawFeatures, nil
}

func jsonSnapshotToValue(snapshot *C.mln_json_snapshot, outValue *any) Status {
	defer C.mln_json_snapshot_destroy(snapshot)
	var rawValue *C.mln_json_value
	status := Status(C.mln_json_snapshot_get(snapshot, (**C.mln_json_value)(unsafe.Pointer(&rawValue))))
	if status != StatusOK {
		return status
	}
	value, err := jsonValueFromC(rawValue)
	if err != nil {
		return StatusNativeError
	}
	*outValue = value
	return StatusOK
}

func jsonValueFromC(value *C.mln_json_value) (any, error) {
	if value == nil {
		return nil, nil
	}
	switch uint32(C.mln_go_json_type(value)) {
	case JSONValueTypeNull:
		return nil, nil
	case JSONValueTypeBool:
		return bool(C.mln_go_json_bool_value(value)), nil
	case JSONValueTypeUint:
		return uint64(C.mln_go_json_uint_value(value)), nil
	case JSONValueTypeInt:
		return int64(C.mln_go_json_int_value(value)), nil
	case JSONValueTypeDouble:
		return float64(C.mln_go_json_double_value(value)), nil
	case JSONValueTypeString:
		return stringViewFromC(C.mln_go_json_string_value(value)), nil
	case JSONValueTypeArray:
		count := int(C.mln_go_json_array_count(value))
		items := make([]any, count)
		for i := range items {
			item, err := jsonValueFromC((*C.mln_json_value)(unsafe.Pointer(C.mln_go_json_array_get(value, C.size_t(i)))))
			if err != nil {
				return nil, err
			}
			items[i] = item
		}
		return items, nil
	case JSONValueTypeObject:
		count := int(C.mln_go_json_object_count(value))
		members := make(map[string]any, count)
		for i := 0; i < count; i++ {
			item, err := jsonValueFromC((*C.mln_json_value)(unsafe.Pointer(C.mln_go_json_object_value(value, C.size_t(i)))))
			if err != nil {
				return nil, err
			}
			members[stringViewFromC(C.mln_go_json_object_key(value, C.size_t(i)))] = item
		}
		return members, nil
	default:
		return nil, fmt.Errorf("unknown JSON value type %d", uint32(C.mln_go_json_type(value)))
	}
}

func latLngToC(coordinate LatLng) C.mln_lat_lng {
	return C.mln_lat_lng{latitude: C.double(coordinate.Latitude), longitude: C.double(coordinate.Longitude)}
}

func canonicalTileIDToC(tileID CanonicalTileID) C.mln_canonical_tile_id {
	return C.mln_canonical_tile_id{z: C.uint32_t(tileID.Z), x: C.uint32_t(tileID.X), y: C.uint32_t(tileID.Y)}
}

func premultipliedRGBA8ImageToC(image PremultipliedRGBA8Image) (C.mln_premultiplied_rgba8_image, unsafe.Pointer) {
	raw := C.mln_premultiplied_rgba8_image_default()
	raw.width = C.uint32_t(image.Width)
	raw.height = C.uint32_t(image.Height)
	raw.stride = C.uint32_t(image.Stride)
	var allocation unsafe.Pointer
	if len(image.Pixels) > 0 {
		allocation = C.CBytes(image.Pixels)
		raw.pixels = (*C.uint8_t)(allocation)
	}
	raw.byte_length = C.size_t(len(image.Pixels))
	return raw, allocation
}

func styleImageOptionsToC(options StyleImageOptions) C.mln_style_image_options {
	raw := C.mln_style_image_options_default()
	raw.fields = C.uint32_t(options.Fields)
	raw.pixel_ratio = C.float(options.PixelRatio)
	raw.sdf = C.bool(options.SDF)
	return raw
}

func styleImageInfoFromC(info C.mln_style_image_info) StyleImageInfo {
	return StyleImageInfo{
		Width:      uint32(info.width),
		Height:     uint32(info.height),
		Stride:     uint32(info.stride),
		ByteLength: uint64(info.byte_length),
		PixelRatio: float32(info.pixel_ratio),
		SDF:        bool(info.sdf),
	}
}

func latLngFromC(coordinate C.mln_lat_lng) LatLng {
	return LatLng{Latitude: float64(coordinate.latitude), Longitude: float64(coordinate.longitude)}
}

func latLngBoundsToC(bounds LatLngBounds) C.mln_lat_lng_bounds {
	return C.mln_lat_lng_bounds{
		southwest: latLngToC(bounds.Southwest),
		northeast: latLngToC(bounds.Northeast),
	}
}

func latLngSliceToCPointer(coordinates []LatLng) (*C.mln_lat_lng, C.size_t) {
	if len(coordinates) == 0 {
		return nil, 0
	}
	rawCoordinates := (*C.mln_lat_lng)(C.malloc(C.size_t(len(coordinates)) * C.size_t(unsafe.Sizeof(C.mln_lat_lng{}))))
	for i, coordinate := range coordinates {
		*(*C.mln_lat_lng)(unsafe.Add(unsafe.Pointer(rawCoordinates), uintptr(i)*unsafe.Sizeof(C.mln_lat_lng{}))) = latLngToC(coordinate)
	}
	return rawCoordinates, C.size_t(len(coordinates))
}

func latLngSliceFromC(coordinates *C.mln_lat_lng, count int) []LatLng {
	if coordinates == nil || count == 0 {
		return nil
	}
	out := make([]LatLng, count)
	for i := range out {
		out[i] = latLngFromC(*(*C.mln_lat_lng)(unsafe.Add(unsafe.Pointer(coordinates), uintptr(i)*unsafe.Sizeof(C.mln_lat_lng{}))))
	}
	return out
}

func latLngBoundsFromC(bounds C.mln_lat_lng_bounds) LatLngBounds {
	return LatLngBounds{Southwest: latLngFromC(bounds.southwest), Northeast: latLngFromC(bounds.northeast)}
}

func screenPointToC(point ScreenPoint) C.mln_screen_point {
	return C.mln_screen_point{x: C.double(point.X), y: C.double(point.Y)}
}

func edgeInsetsToC(insets EdgeInsets) C.mln_edge_insets {
	return C.mln_edge_insets{
		top:    C.double(insets.Top),
		left:   C.double(insets.Left),
		bottom: C.double(insets.Bottom),
		right:  C.double(insets.Right),
	}
}

func edgeInsetsFromC(insets C.mln_edge_insets) EdgeInsets {
	return EdgeInsets{Top: float64(insets.top), Left: float64(insets.left), Bottom: float64(insets.bottom), Right: float64(insets.right)}
}

func renderTargetExtentToC(extent RenderTargetExtent) C.mln_render_target_extent {
	raw := C.mln_render_target_extent{}
	raw.size = C.uint32_t(unsafe.Sizeof(C.mln_render_target_extent{}))
	raw.width = C.uint32_t(extent.Width)
	raw.height = C.uint32_t(extent.Height)
	raw.scale_factor = C.double(extent.ScaleFactor)
	return raw
}

func metalContextDescriptorToC(context MetalContextDescriptor) C.mln_metal_context_descriptor {
	raw := C.mln_metal_context_descriptor{}
	raw.size = C.uint32_t(unsafe.Sizeof(C.mln_metal_context_descriptor{}))
	raw.device = C.mln_go_ptr(C.uintptr_t(context.Device))
	return raw
}

func vulkanContextDescriptorToC(context VulkanContextDescriptor) C.mln_vulkan_context_descriptor {
	raw := C.mln_vulkan_context_descriptor{}
	raw.size = C.uint32_t(unsafe.Sizeof(C.mln_vulkan_context_descriptor{}))
	raw.instance = C.mln_go_ptr(C.uintptr_t(context.Instance))
	raw.physical_device = C.mln_go_ptr(C.uintptr_t(context.PhysicalDevice))
	raw.device = C.mln_go_ptr(C.uintptr_t(context.Device))
	raw.graphics_queue = C.mln_go_ptr(C.uintptr_t(context.GraphicsQueue))
	raw.graphics_queue_family_index = C.uint32_t(context.GraphicsQueueFamilyIndex)
	return raw
}

func metalSurfaceDescriptorToC(descriptor MetalSurfaceDescriptor) C.mln_metal_surface_descriptor {
	raw := C.mln_metal_surface_descriptor_default()
	raw.extent = renderTargetExtentToC(descriptor.Extent)
	raw.context = metalContextDescriptorToC(descriptor.Context)
	raw.layer = C.mln_go_ptr(C.uintptr_t(descriptor.Layer))
	return raw
}

func vulkanSurfaceDescriptorToC(descriptor VulkanSurfaceDescriptor) C.mln_vulkan_surface_descriptor {
	raw := C.mln_vulkan_surface_descriptor_default()
	raw.extent = renderTargetExtentToC(descriptor.Extent)
	raw.context = vulkanContextDescriptorToC(descriptor.Context)
	raw.surface = C.mln_go_ptr(C.uintptr_t(descriptor.Surface))
	return raw
}

func metalOwnedTextureDescriptorToC(descriptor MetalOwnedTextureDescriptor) C.mln_metal_owned_texture_descriptor {
	raw := C.mln_metal_owned_texture_descriptor_default()
	raw.extent = renderTargetExtentToC(descriptor.Extent)
	raw.context = metalContextDescriptorToC(descriptor.Context)
	return raw
}

func metalBorrowedTextureDescriptorToC(descriptor MetalBorrowedTextureDescriptor) C.mln_metal_borrowed_texture_descriptor {
	raw := C.mln_metal_borrowed_texture_descriptor_default()
	raw.extent = renderTargetExtentToC(descriptor.Extent)
	raw.texture = C.mln_go_ptr(C.uintptr_t(descriptor.Texture))
	return raw
}

func vulkanOwnedTextureDescriptorToC(descriptor VulkanOwnedTextureDescriptor) C.mln_vulkan_owned_texture_descriptor {
	raw := C.mln_vulkan_owned_texture_descriptor_default()
	raw.extent = renderTargetExtentToC(descriptor.Extent)
	raw.context = vulkanContextDescriptorToC(descriptor.Context)
	return raw
}

func vulkanBorrowedTextureDescriptorToC(descriptor VulkanBorrowedTextureDescriptor) C.mln_vulkan_borrowed_texture_descriptor {
	raw := C.mln_vulkan_borrowed_texture_descriptor_default()
	raw.extent = renderTargetExtentToC(descriptor.Extent)
	raw.context = vulkanContextDescriptorToC(descriptor.Context)
	raw.image = C.mln_go_ptr(C.uintptr_t(descriptor.Image))
	raw.image_view = C.mln_go_ptr(C.uintptr_t(descriptor.ImageView))
	raw.format = C.uint32_t(descriptor.Format)
	raw.initial_layout = C.uint32_t(descriptor.InitialLayout)
	raw.final_layout = C.uint32_t(descriptor.FinalLayout)
	return raw
}

func textureImageInfoFromC(info C.mln_texture_image_info) TextureImageInfo {
	return TextureImageInfo{
		Width:      uint32(info.width),
		Height:     uint32(info.height),
		Stride:     uint32(info.stride),
		ByteLength: uint64(info.byte_length),
	}
}

func metalOwnedTextureFrameFromC(frame C.mln_metal_owned_texture_frame) MetalOwnedTextureFrame {
	return MetalOwnedTextureFrame{
		Generation:  uint64(frame.generation),
		Width:       uint32(frame.width),
		Height:      uint32(frame.height),
		ScaleFactor: float64(frame.scale_factor),
		FrameID:     uint64(frame.frame_id),
		Texture:     uintptr(frame.texture),
		Device:      uintptr(frame.device),
		PixelFormat: uint64(frame.pixel_format),
	}
}

func metalOwnedTextureFrameToC(frame MetalOwnedTextureFrame) C.mln_metal_owned_texture_frame {
	raw := C.mln_metal_owned_texture_frame{}
	raw.size = C.uint32_t(unsafe.Sizeof(C.mln_metal_owned_texture_frame{}))
	raw.generation = C.uint64_t(frame.Generation)
	raw.width = C.uint32_t(frame.Width)
	raw.height = C.uint32_t(frame.Height)
	raw.scale_factor = C.double(frame.ScaleFactor)
	raw.frame_id = C.uint64_t(frame.FrameID)
	raw.texture = C.mln_go_ptr(C.uintptr_t(frame.Texture))
	raw.device = C.mln_go_ptr(C.uintptr_t(frame.Device))
	raw.pixel_format = C.uint64_t(frame.PixelFormat)
	return raw
}

func vulkanOwnedTextureFrameFromC(frame C.mln_vulkan_owned_texture_frame) VulkanOwnedTextureFrame {
	return VulkanOwnedTextureFrame{
		Generation:  uint64(frame.generation),
		Width:       uint32(frame.width),
		Height:      uint32(frame.height),
		ScaleFactor: float64(frame.scale_factor),
		FrameID:     uint64(frame.frame_id),
		Image:       uintptr(frame.image),
		ImageView:   uintptr(frame.image_view),
		Device:      uintptr(frame.device),
		Format:      uint32(frame.format),
		Layout:      uint32(frame.layout),
	}
}

func vulkanOwnedTextureFrameToC(frame VulkanOwnedTextureFrame) C.mln_vulkan_owned_texture_frame {
	raw := C.mln_vulkan_owned_texture_frame{}
	raw.size = C.uint32_t(unsafe.Sizeof(C.mln_vulkan_owned_texture_frame{}))
	raw.generation = C.uint64_t(frame.Generation)
	raw.width = C.uint32_t(frame.Width)
	raw.height = C.uint32_t(frame.Height)
	raw.scale_factor = C.double(frame.ScaleFactor)
	raw.frame_id = C.uint64_t(frame.FrameID)
	raw.image = C.mln_go_ptr(C.uintptr_t(frame.Image))
	raw.image_view = C.mln_go_ptr(C.uintptr_t(frame.ImageView))
	raw.device = C.mln_go_ptr(C.uintptr_t(frame.Device))
	raw.format = C.uint32_t(frame.Format)
	raw.layout = C.uint32_t(frame.Layout)
	return raw
}

func viewportOptionsFromC(options C.mln_map_viewport_options) ViewportOptions {
	return ViewportOptions{
		Fields:           uint32(options.fields),
		NorthOrientation: uint32(options.north_orientation),
		ConstrainMode:    uint32(options.constrain_mode),
		ViewportMode:     uint32(options.viewport_mode),
		FrustumOffset:    edgeInsetsFromC(options.frustum_offset),
	}
}

func tileOptionsFromC(options C.mln_map_tile_options) TileOptions {
	return TileOptions{
		Fields:            uint32(options.fields),
		PrefetchZoomDelta: uint32(options.prefetch_zoom_delta),
		LODMinRadius:      float64(options.lod_min_radius),
		LODScale:          float64(options.lod_scale),
		LODPitchThreshold: float64(options.lod_pitch_threshold),
		LODZoomShift:      float64(options.lod_zoom_shift),
		LODMode:           uint32(options.lod_mode),
	}
}

func projectionModeOptionsFromC(options C.mln_projection_mode) ProjectionModeOptions {
	return ProjectionModeOptions{
		Fields:      uint32(options.fields),
		Axonometric: bool(options.axonometric),
		XSkew:       float64(options.x_skew),
		YSkew:       float64(options.y_skew),
	}
}

func cameraOptionsToC(options CameraOptions) C.mln_camera_options {
	raw := C.mln_camera_options_default()
	raw.fields = C.uint32_t(options.Fields)
	raw.latitude = C.double(options.Center.Latitude)
	raw.longitude = C.double(options.Center.Longitude)
	raw.center_altitude = C.double(options.CenterAltitude)
	raw.padding = edgeInsetsToC(options.Padding)
	raw.anchor = screenPointToC(options.Anchor)
	raw.zoom = C.double(options.Zoom)
	raw.bearing = C.double(options.Bearing)
	raw.pitch = C.double(options.Pitch)
	raw.roll = C.double(options.Roll)
	raw.field_of_view = C.double(options.FieldOfView)
	return raw
}

func cameraOptionsFromC(options C.mln_camera_options) CameraOptions {
	return CameraOptions{
		Fields:         uint32(options.fields),
		Center:         LatLng{Latitude: float64(options.latitude), Longitude: float64(options.longitude)},
		CenterAltitude: float64(options.center_altitude),
		Padding:        edgeInsetsFromC(options.padding),
		Anchor:         screenPointFromC(options.anchor),
		Zoom:           float64(options.zoom),
		Bearing:        float64(options.bearing),
		Pitch:          float64(options.pitch),
		Roll:           float64(options.roll),
		FieldOfView:    float64(options.field_of_view),
	}
}

func animationOptionsToC(options AnimationOptions) C.mln_animation_options {
	raw := C.mln_animation_options_default()
	raw.fields = C.uint32_t(options.Fields)
	raw.duration_ms = C.double(options.DurationMS)
	raw.velocity = C.double(options.Velocity)
	raw.min_zoom = C.double(options.MinZoom)
	raw.easing = C.mln_unit_bezier{
		x1: C.double(options.Easing.X1),
		y1: C.double(options.Easing.Y1),
		x2: C.double(options.Easing.X2),
		y2: C.double(options.Easing.Y2),
	}
	return raw
}

func animationOptionsToCPointer(options *AnimationOptions) (C.mln_animation_options, *C.mln_animation_options) {
	if options == nil {
		return C.mln_animation_options{}, nil
	}
	raw := animationOptionsToC(*options)
	return raw, &raw
}

func cameraFitOptionsToC(options CameraFitOptions) C.mln_camera_fit_options {
	raw := C.mln_camera_fit_options_default()
	raw.fields = C.uint32_t(options.Fields)
	raw.padding = edgeInsetsToC(options.Padding)
	raw.bearing = C.double(options.Bearing)
	raw.pitch = C.double(options.Pitch)
	return raw
}

func cameraFitOptionsToCPointer(options *CameraFitOptions) (C.mln_camera_fit_options, *C.mln_camera_fit_options) {
	if options == nil {
		return C.mln_camera_fit_options{}, nil
	}
	raw := cameraFitOptionsToC(*options)
	return raw, &raw
}

func boundOptionsToC(options BoundOptions) C.mln_bound_options {
	raw := C.mln_bound_options_default()
	raw.fields = C.uint32_t(options.Fields)
	raw.bounds = latLngBoundsToC(options.Bounds)
	raw.min_zoom = C.double(options.MinZoom)
	raw.max_zoom = C.double(options.MaxZoom)
	raw.min_pitch = C.double(options.MinPitch)
	raw.max_pitch = C.double(options.MaxPitch)
	return raw
}

func boundOptionsFromC(options C.mln_bound_options) BoundOptions {
	return BoundOptions{
		Fields:   uint32(options.fields),
		Bounds:   latLngBoundsFromC(options.bounds),
		MinZoom:  float64(options.min_zoom),
		MaxZoom:  float64(options.max_zoom),
		MinPitch: float64(options.min_pitch),
		MaxPitch: float64(options.max_pitch),
	}
}

func freeCameraOptionsToC(options FreeCameraOptions) C.mln_free_camera_options {
	raw := C.mln_free_camera_options_default()
	raw.fields = C.uint32_t(options.Fields)
	raw.position = vec3ToC(options.Position)
	raw.orientation = quaternionToC(options.Orientation)
	return raw
}

func freeCameraOptionsFromC(options C.mln_free_camera_options) FreeCameraOptions {
	return FreeCameraOptions{
		Fields:      uint32(options.fields),
		Position:    vec3FromC(options.position),
		Orientation: quaternionFromC(options.orientation),
	}
}

func vec3ToC(vector Vec3) C.mln_vec3 {
	return C.mln_vec3{x: C.double(vector.X), y: C.double(vector.Y), z: C.double(vector.Z)}
}

func vec3FromC(vector C.mln_vec3) Vec3 {
	return Vec3{X: float64(vector.x), Y: float64(vector.y), Z: float64(vector.z)}
}

func quaternionToC(quaternion Quaternion) C.mln_quaternion {
	return C.mln_quaternion{x: C.double(quaternion.X), y: C.double(quaternion.Y), z: C.double(quaternion.Z), w: C.double(quaternion.W)}
}

func quaternionFromC(quaternion C.mln_quaternion) Quaternion {
	return Quaternion{X: float64(quaternion.x), Y: float64(quaternion.y), Z: float64(quaternion.z), W: float64(quaternion.w)}
}

func screenPointFromC(point C.mln_screen_point) ScreenPoint {
	return ScreenPoint{X: float64(point.x), Y: float64(point.y)}
}

func projectedMetersToC(meters ProjectedMeters) C.mln_projected_meters {
	return C.mln_projected_meters{northing: C.double(meters.Northing), easting: C.double(meters.Easting)}
}

func projectedMetersFromC(meters C.mln_projected_meters) ProjectedMeters {
	return ProjectedMeters{Northing: float64(meters.northing), Easting: float64(meters.easting)}
}
