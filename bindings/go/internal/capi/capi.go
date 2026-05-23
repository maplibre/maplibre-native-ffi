// Package capi contains the private cgo boundary for the public MapLibre
// Native C API.
package capi

/*
#cgo CFLAGS: -std=c2x
#include "maplibre_native_c.h"
*/
import "C"
import "unsafe"

// Runtime is an opaque native runtime handle.
type Runtime struct{ _ byte }

// Map is an opaque native map handle.
type Map struct{ _ byte }

// Projection is an opaque native map projection handle.
type Projection struct{ _ byte }

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

// ProjectedMeters is a spherical Mercator coordinate in meters.
type ProjectedMeters struct {
	Northing float64
	Easting  float64
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
	MapModeContinuous uint32 = uint32(C.MLN_MAP_MODE_CONTINUOUS)
	MapModeStatic     uint32 = uint32(C.MLN_MAP_MODE_STATIC)
	MapModeTile       uint32 = uint32(C.MLN_MAP_MODE_TILE)
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
	return RuntimeEvent{
		Type:        uint32(event._type),
		SourceType:  uint32(event.source_type),
		Source:      uintptr(event.source),
		Code:        int32(event.code),
		PayloadType: uint32(event.payload_type),
		PayloadSize: uintptr(event.payload_size),
		Message:     message,
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

// MapDestroy destroys a map handle.
func MapDestroy(m *Map) Status {
	return Status(C.mln_map_destroy((*C.mln_map)(unsafe.Pointer(m))))
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

func screenPointToC(point ScreenPoint) C.mln_screen_point {
	return C.mln_screen_point{x: C.double(point.X), y: C.double(point.Y)}
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
