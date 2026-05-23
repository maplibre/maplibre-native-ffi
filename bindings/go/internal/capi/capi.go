// Package capi contains the private cgo boundary for the public MapLibre
// Native C API.
package capi

/*
#cgo CFLAGS: -std=c2x
#include <stdlib.h>
#include "maplibre_native_c.h"

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
import "unsafe"

// Runtime is an opaque native runtime handle.
type Runtime struct{ _ byte }

// Map is an opaque native map handle.
type Map struct{ _ byte }

// Projection is an opaque native map projection handle.
type Projection struct{ _ byte }

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

func latLngToC(coordinate LatLng) C.mln_lat_lng {
	return C.mln_lat_lng{latitude: C.double(coordinate.Latitude), longitude: C.double(coordinate.Longitude)}
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

func screenPointFromC(point C.mln_screen_point) ScreenPoint {
	return ScreenPoint{X: float64(point.x), Y: float64(point.y)}
}

func projectedMetersToC(meters ProjectedMeters) C.mln_projected_meters {
	return C.mln_projected_meters{northing: C.double(meters.Northing), easting: C.double(meters.Easting)}
}

func projectedMetersFromC(meters C.mln_projected_meters) ProjectedMeters {
	return ProjectedMeters{Northing: float64(meters.northing), Easting: float64(meters.easting)}
}
