package maplibre

/*
#include <stdlib.h>

#include "maplibre_native_c.h"
*/
import "C"

import (
	"bytes"
	"errors"
	"unsafe"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/memory"
)

// MapMode selects the native map rendering mode.
type MapMode uint32

const (
	MapModeContinuous MapMode = MapMode(C.MLN_MAP_MODE_CONTINUOUS)
	MapModeStatic     MapMode = MapMode(C.MLN_MAP_MODE_STATIC)
	MapModeTile       MapMode = MapMode(C.MLN_MAP_MODE_TILE)
)

// MapDebugOptions is a mask of native map debug overlays.
type MapDebugOptions uint32

const (
	MapDebugTileBorders MapDebugOptions = MapDebugOptions(C.MLN_MAP_DEBUG_TILE_BORDERS)
	MapDebugParseStatus MapDebugOptions = MapDebugOptions(C.MLN_MAP_DEBUG_PARSE_STATUS)
	MapDebugTimestamps  MapDebugOptions = MapDebugOptions(C.MLN_MAP_DEBUG_TIMESTAMPS)
	MapDebugCollision   MapDebugOptions = MapDebugOptions(C.MLN_MAP_DEBUG_COLLISION)
	MapDebugOverdraw    MapDebugOptions = MapDebugOptions(C.MLN_MAP_DEBUG_OVERDRAW)
	MapDebugStencilClip MapDebugOptions = MapDebugOptions(C.MLN_MAP_DEBUG_STENCIL_CLIP)
	MapDebugDepthBuffer MapDebugOptions = MapDebugOptions(C.MLN_MAP_DEBUG_DEPTH_BUFFER)
)

// Has reports whether all requested debug overlay bits are set.
func (options MapDebugOptions) Has(requested MapDebugOptions) bool {
	return options&requested == requested
}

// MapOptions configures map creation.
type MapOptions struct {
	// Width is the initial logical width in UI pixels.
	Width uint32
	// Height is the initial logical height in UI pixels.
	Height uint32
	// ScaleFactor is the initial UI-to-device pixel scale. It selects sprites,
	// glyphs, and raster tiles. Resize may update it with the logical size.
	ScaleFactor float64
	Mode        MapMode
	// FastPFOREnabled decodes MapLibre Tile (MLT) tiles whose integer streams
	// use FastPFOR encodings, fixed for the lifetime of the map. A map created
	// with this false logs a tile parse warning for those tiles.
	FastPFOREnabled bool
	// EventMask selects the map-originated event types this map queues.
	// NewMapOptions sets it to the native default, which selects every type.
	// The mask applies during construction. See MapHandle.SetEventMask.
	EventMask RuntimeEventMask
}

// Equal reports whether two descriptors hold the same field values.
func (options MapOptions) Equal(other MapOptions) bool {
	return options.Width == other.Width &&
		options.Height == other.Height &&
		options.ScaleFactor == other.ScaleFactor &&
		options.Mode == other.Mode &&
		options.FastPFOREnabled == other.FastPFOREnabled &&
		options.EventMask == other.EventMask
}

// NewMapOptions returns map creation options for a viewport size and scale. The
// returned options select every map-originated event type.
func NewMapOptions(width, height uint32, scaleFactor float64) MapOptions {
	return MapOptions{
		Width:       width,
		Height:      height,
		ScaleFactor: scaleFactor,
		Mode:        MapModeContinuous,
		EventMask:   defaultMapEventMask(),
	}
}

// defaultMapEventMask reads the map default's own event mask. The bits are
// retained rather than named, so a newer native library's default keeps
// selecting event types this build does not define. Those reach a host as
// unknown event and payload domains.
func defaultMapEventMask() RuntimeEventMask {
	return RuntimeEventMask(C.mln_map_options_default().event_mask)
}

// LogicalExtent is a map's logical size and device-pixel scale.
type LogicalExtent struct {
	Width       uint32
	Height      uint32
	ScaleFactor float64
}

// MapSnapshot is a copied immutable map state generation. Every committed map
// command publishes a new generation and reports it in its command-finished
// event, so a snapshot whose Generation is at or past a commit's observes that
// commit.
type MapSnapshot struct {
	Generation uint64
	// DebugOptions is the committed debug overlay mask.
	DebugOptions   MapDebugOptions
	Camera         CameraOptions
	LogicalExtent  LogicalExtent
	ProjectionMode ProjectionModeOptions
	Viewport       ViewportOptions
	Tile           TileOptions
	Bounds         BoundOptions
	FreeCamera     FreeCameraOptions
	// FullyLoaded reports whether every requested style and tile resource
	// finished loading.
	FullyLoaded                  bool
	RenderingStatsViewEnabled    bool
	RepaintDemand                bool
	EventMask                    RuntimeEventMask
	LatestRenderUpdateGeneration uint64
}

// CameraSnapshot is a copied camera and the immutable map generation that
// supplied it.
type CameraSnapshot struct {
	Generation uint64
	Camera     CameraOptions
}

// MapHandle owns map state for one RuntimeHandle.
type MapHandle struct {
	state        *handle.State[nativeMap]
	runtime      *RuntimeHandle
	runtimeChild *handle.Child
	// The map's native handle, which also serves as its public identity.
	id MapID
}

// Test seam for synthetic handles. Production close uses closeNativeMap.
var destroyMapHandle func(nativeMap) int32

func (m *MapHandle) ptr() (nativeMap, func(), error) {
	if m == nil || m.state == nil {
		return 0, nil, newBindingError(ErrInvalidArgument, "MapHandle is nil")
	}
	borrow, live := m.state.Borrow()
	if !live {
		return 0, nil, newBindingError(ErrInvalidArgument, "MapHandle is closed")
	}
	return borrow.Handle(), borrow.Release, nil
}

func waitMapOperation(start func(*C.mln_operation) int32) (C.mln_operation, error) {
	var operation C.mln_operation
	if err := checkNative(func() int32 { return start(&operation) }); err != nil {
		return 0, err
	}
	if err := waitNativeOperation(operation); err != nil {
		C.mln_operation_release(operation)
		return 0, err
	}
	return operation, nil
}

func takeMapBuffer(operation C.mln_operation, take func(C.mln_operation, *C.mln_buffer) int32) ([]byte, error) {
	var buffer C.mln_buffer
	if err := checkNative(func() int32 { return take(operation, &buffer) }); err != nil {
		return nil, err
	}
	if buffer == 0 {
		return nil, nil
	}
	defer C.mln_buffer_destroy(buffer)
	var view C.mln_buffer_view
	if err := checkNative(func() int32 { return int32(C.mln_buffer_get(buffer, &view)) }); err != nil {
		return nil, err
	}
	if view.size == 0 {
		return []byte{}, nil
	}
	return append([]byte(nil), unsafe.Slice((*byte)(view.data), int(view.size))...), nil
}

func validateCStringArgument(name string, value string) error {
	if _, err := memory.NewCString(value); err != nil {
		if errors.Is(err, memory.EmbeddedNulError()) {
			return newBindingError(ErrInvalidArgument, name+" contains embedded NUL")
		}
		return err
	}
	return nil
}

// ID returns this map's event source identity, which matches
// RuntimeEventSource.MapID on runtime events this map raises.
func (m *MapHandle) ID() (MapID, error) {
	_, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	return m.id, nil
}

// SetEventMask submits a command that selects map-originated event types and
// returns its command ID.
func (m *MapHandle) SetEventMask(mask RuntimeEventMask) (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_set_event_mask(C.mln_map(ptr), C.uint64_t(mask), &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// EventMask returns the mask copied in the latest immutable map snapshot.
func (m *MapHandle) EventMask() (RuntimeEventMask, error) {
	snapshot, err := m.Snapshot()
	if err != nil {
		return 0, err
	}
	return snapshot.EventMask, nil
}

// RequestRepaint submits a repaint command for a continuous map.
func (m *MapHandle) RequestRepaint() (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_request_repaint(C.mln_map(ptr), &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// RequestStillImage starts one noncoalescing still-image operation for a static
// or tile map.
func (m *MapHandle) RequestStillImage() (*OperationHandle[struct{}], error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer m.state.KeepAlive()
	return startOperation[struct{}](m.runtime, operationStillImage, operationResultNone, func(_ nativeRuntime, out *C.mln_operation) int32 {
		return int32(C.mln_map_request_still_image_start(C.mln_map(ptr), out))
	})
}

// SetStyleURL submits a style URL command and returns its command ID. Loading
// failures arrive through runtime events.
func (m *MapHandle) SetStyleURL(url string) (uint64, error) {
	if err := validateCStringArgument("style URL", url); err != nil {
		return 0, err
	}
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	cURL := C.CString(url)
	defer C.free(unsafe.Pointer(cURL))
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_set_style_url(C.mln_map(ptr), cURL, &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// SetStyleJSON submits an inline style command and returns its command ID.
// Loading failures arrive through runtime events.
func (m *MapHandle) SetStyleJSON(json []byte) (uint64, error) {
	if bytes.IndexByte(json, 0) >= 0 {
		return 0, newBindingError(ErrInvalidArgument, "style JSON contains a NUL byte")
	}
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	jsonView := newCBufferView(json)
	defer jsonView.free()
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_set_style_json(C.mln_map(ptr), jsonView.raw(), &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// LoadedStyleJSON returns an ordered copy of the last loaded style document.
func (m *MapHandle) LoadedStyleJSON() ([]byte, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer m.state.KeepAlive()
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_loaded_style_json_start(C.mln_map(ptr), out))
	})
	if err != nil {
		return nil, err
	}
	defer C.mln_operation_release(operation)
	return takeMapBuffer(operation, func(operation C.mln_operation, out *C.mln_buffer) int32 {
		return int32(C.mln_map_loaded_style_json_take_result(operation, out))
	})
}

// StyleURL returns an ordered copy of the last requested style URL.
func (m *MapHandle) StyleURL() (string, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return "", err
	}
	defer release()
	defer m.state.KeepAlive()
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_style_url_start(C.mln_map(ptr), out))
	})
	if err != nil {
		return "", err
	}
	defer C.mln_operation_release(operation)
	bytes, err := takeMapBuffer(operation, func(operation C.mln_operation, out *C.mln_buffer) int32 {
		return int32(C.mln_map_style_url_take_result(operation, out))
	})
	return string(bytes), err
}

// SetDebugOptions applies MapLibre debug overlay mask bits to a map. The
// committed mask is visible through Snapshot as MapSnapshot.DebugOptions.
func (m *MapHandle) SetDebugOptions(options MapDebugOptions) (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_set_debug_options(C.mln_map(ptr), C.uint32_t(options), &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// SetRenderingStatsViewEnabled enables or disables MapLibre's rendering stats
// overlay view. The committed value is visible through Snapshot as
// MapSnapshot.RenderingStatsViewEnabled.
func (m *MapHandle) SetRenderingStatsViewEnabled(enabled bool) (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_set_rendering_stats_view_enabled(C.mln_map(ptr), C.bool(enabled), &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// Snapshot returns a copy of the latest immutable map state.
func (m *MapHandle) Snapshot() (MapSnapshot, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return MapSnapshot{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	raw := C.mln_map_snapshot{size: C.uint32_t(unsafe.Sizeof(C.mln_map_snapshot{}))}
	if err := checkNative(func() int32 {
		return int32(C.mln_map_snapshot_get(C.mln_map(ptr), &raw))
	}); err != nil {
		return MapSnapshot{}, err
	}
	return MapSnapshot{
		Generation:   uint64(raw.generation),
		DebugOptions: MapDebugOptions(raw.debug_options),
		Camera:       goCameraOptions(raw.camera),
		LogicalExtent: LogicalExtent{
			Width:       uint32(raw.logical_extent.width),
			Height:      uint32(raw.logical_extent.height),
			ScaleFactor: float64(raw.logical_extent.scale_factor),
		},
		ProjectionMode:               goProjectionModeOptions(raw.projection_mode),
		Viewport:                     goViewportOptions(raw.viewport),
		Tile:                         goTileOptions(raw.tile),
		Bounds:                       goBoundOptions(raw.bounds),
		FreeCamera:                   goFreeCameraOptions(raw.free_camera),
		FullyLoaded:                  bool(raw.fully_loaded),
		RenderingStatsViewEnabled:    bool(raw.rendering_stats_view_enabled),
		RepaintDemand:                bool(raw.repaint_demand),
		EventMask:                    RuntimeEventMask(raw.event_mask),
		LatestRenderUpdateGeneration: uint64(raw.latest_render_update_generation),
	}, nil
}

// Size returns the latest logical extent copied from the map snapshot.
func (m *MapHandle) Size() (width uint32, height uint32, scaleFactor float64, err error) {
	snapshot, err := m.Snapshot()
	if err != nil {
		return 0, 0, 0, err
	}
	extent := snapshot.LogicalExtent
	return extent.Width, extent.Height, extent.ScaleFactor, nil
}

// Resize submits a logical extent update and returns its command ID.
func (m *MapHandle) Resize(extent LogicalExtent) (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	raw := C.mln_logical_extent{
		width:        C.uint32_t(extent.Width),
		height:       C.uint32_t(extent.Height),
		scale_factor: C.double(extent.ScaleFactor),
	}
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_resize(C.mln_map(ptr), raw, &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// DumpDebugLogs dumps map debug logs through MapLibre Native logging.
func (m *MapHandle) DumpDebugLogs() (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_dump_debug_logs(C.mln_map(ptr), &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// Camera returns a copy of the latest published camera.
func (m *MapHandle) Camera() (CameraOptions, error) {
	snapshot, err := m.CameraSnapshot()
	if err != nil {
		return CameraOptions{}, err
	}
	return snapshot.Camera, nil
}

// CameraSnapshot returns a copied camera and its map snapshot generation.
func (m *MapHandle) CameraSnapshot() (CameraSnapshot, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return CameraSnapshot{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	raw := C.mln_camera_options_default()
	var generation C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_camera_snapshot_get(C.mln_map(ptr), &raw, &generation))
	}); err != nil {
		return CameraSnapshot{}, err
	}
	return CameraSnapshot{Generation: uint64(generation), Camera: goCameraOptions(raw)}, nil
}

// QueryCamera starts an ordered camera read that observes every previously
// committed command.
func (m *MapHandle) QueryCamera() (*OperationHandle[CameraSnapshot], error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer m.state.KeepAlive()
	return startOperation[CameraSnapshot](m.runtime, operationCameraQuery, operationResultCamera, func(_ nativeRuntime, out *C.mln_operation) int32 {
		return int32(C.mln_map_camera_query_start(C.mln_map(ptr), out))
	})
}

// UpdateCamera submits one atomic camera update and returns its command ID.
func (m *MapHandle) UpdateCamera(update CameraUpdate) (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	raw := cCameraUpdate(update)
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_update_camera(C.mln_map(ptr), &raw, &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// JumpTo submits an atomic camera jump.
func (m *MapHandle) JumpTo(camera CameraOptions) (uint64, error) {
	return m.UpdateCamera(CameraUpdate{Mode: CameraUpdateModeJump, Camera: camera})
}

// EaseTo submits an atomic eased camera transition.
func (m *MapHandle) EaseTo(camera CameraOptions, animation *AnimationOptions) (uint64, error) {
	return m.UpdateCamera(CameraUpdate{Mode: CameraUpdateModeEase, Camera: camera, Animation: animation})
}

// FlyTo submits an atomic flying camera transition.
func (m *MapHandle) FlyTo(camera CameraOptions, animation *AnimationOptions) (uint64, error) {
	return m.UpdateCamera(CameraUpdate{Mode: CameraUpdateModeFly, Camera: camera, Animation: animation})
}

// CameraForLatLngBounds computes a camera that fits geographic bounds. Passing
// nil fitOptions uses native default fitting options.
func (m *MapHandle) CameraForLatLngBounds(bounds LatLngBounds, fitOptions *CameraFitOptions) (CameraOptions, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.mln_camera_options = C.mln_camera_options_default()
	rawFitOptions, rawFitOptionsPtr := cCameraFitOptionsPointer(fitOptions)
	_ = rawFitOptions
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_camera_for_lat_lng_bounds_start(
			C.mln_map(ptr), cLatLngBounds(bounds), rawFitOptionsPtr, out,
		))
	})
	if err != nil {
		return CameraOptions{}, err
	}
	defer C.mln_operation_release(operation)
	if err := checkNative(func() int32 {
		return int32(C.mln_map_camera_for_lat_lng_bounds_take_result(operation, &raw))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(raw), nil
}

// CameraForLatLngs computes a camera that fits geographic coordinates. Passing
// nil fitOptions uses native default fitting options.
func (m *MapHandle) CameraForLatLngs(coordinates []LatLng, fitOptions *CameraFitOptions) (CameraOptions, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.mln_camera_options = C.mln_camera_options_default()
	rawCoordinates := cLatLngSlice(coordinates)
	var rawCoordinatesPtr *C.mln_lat_lng
	if len(rawCoordinates) > 0 {
		rawCoordinatesPtr = &rawCoordinates[0]
	}
	rawFitOptions, rawFitOptionsPtr := cCameraFitOptionsPointer(fitOptions)
	_ = rawFitOptions
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_camera_for_lat_lngs_start(
			C.mln_map(ptr), rawCoordinatesPtr, C.size_t(len(rawCoordinates)),
			rawFitOptionsPtr, out,
		))
	})
	if err != nil {
		return CameraOptions{}, err
	}
	defer C.mln_operation_release(operation)
	if err := checkNative(func() int32 {
		return int32(C.mln_map_camera_for_lat_lngs_take_result(operation, &raw))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(raw), nil
}

// CameraForGeometry computes a camera that fits a geometry. Passing nil
// fitOptions uses native default fitting options.
func (m *MapHandle) CameraForGeometry(geometry []byte, fitOptions *CameraFitOptions) (CameraOptions, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.mln_camera_options = C.mln_camera_options_default()
	rawGeometry := newCBufferView(geometry)
	defer rawGeometry.free()
	rawFitOptions, rawFitOptionsPtr := cCameraFitOptionsPointer(fitOptions)
	_ = rawFitOptions
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_camera_for_geometry_start(
			C.mln_map(ptr), rawGeometry.raw(), rawFitOptionsPtr, out,
		))
	})
	if err != nil {
		return CameraOptions{}, err
	}
	defer C.mln_operation_release(operation)
	if err := checkNative(func() int32 {
		return int32(C.mln_map_camera_for_geometry_take_result(operation, &raw))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(raw), nil
}

// LatLngBoundsForCamera computes wrapped geographic bounds for a camera in the
// current viewport.
func (m *MapHandle) LatLngBoundsForCamera(camera CameraOptions) (LatLngBounds, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return LatLngBounds{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	var raw C.mln_lat_lng_bounds
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_lat_lng_bounds_for_camera_start(C.mln_map(ptr), &rawCamera, out))
	})
	if err != nil {
		return LatLngBounds{}, err
	}
	defer C.mln_operation_release(operation)
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lng_bounds_for_camera_take_result(operation, &raw))
	}); err != nil {
		return LatLngBounds{}, err
	}
	return goLatLngBounds(raw), nil
}

// LatLngBoundsForCameraUnwrapped computes unwrapped geographic bounds for a
// camera in the current viewport.
func (m *MapHandle) LatLngBoundsForCameraUnwrapped(camera CameraOptions) (LatLngBounds, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return LatLngBounds{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	var raw C.mln_lat_lng_bounds
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_lat_lng_bounds_for_camera_unwrapped_start(C.mln_map(ptr), &rawCamera, out))
	})
	if err != nil {
		return LatLngBounds{}, err
	}
	defer C.mln_operation_release(operation)
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lng_bounds_for_camera_unwrapped_take_result(operation, &raw))
	}); err != nil {
		return LatLngBounds{}, err
	}
	return goLatLngBounds(raw), nil
}

// SetBounds applies selected map camera constraint options. The committed
// constraints are visible through Snapshot as MapSnapshot.Bounds.
func (m *MapHandle) SetBounds(options BoundOptions) (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	rawOptions, err := cBoundOptions(options)
	if err != nil {
		return 0, err
	}
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_set_bounds(C.mln_map(ptr), &rawOptions, &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// SetFreeCameraOptions applies selected free camera position and orientation
// fields. The committed options are visible through Snapshot as
// MapSnapshot.FreeCamera.
func (m *MapHandle) SetFreeCameraOptions(options FreeCameraOptions) (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	rawOptions := cFreeCameraOptions(options)
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_set_free_camera_options(C.mln_map(ptr), &rawOptions, &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// SetViewportOptions applies selected live map viewport and render-transform
// controls. The committed options are visible through Snapshot as
// MapSnapshot.Viewport.
func (m *MapHandle) SetViewportOptions(options ViewportOptions) (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	rawOptions := cViewportOptions(options)
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_set_viewport_options(C.mln_map(ptr), &rawOptions, &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// SetTileOptions applies selected tile prefetch and LOD tuning controls. The
// committed options are visible through Snapshot as MapSnapshot.Tile.
func (m *MapHandle) SetTileOptions(options TileOptions) (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	rawOptions := cTileOptions(options)
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_set_tile_options(C.mln_map(ptr), &rawOptions, &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// ProjectionMode returns the latest published axonometric rendering options.
func (m *MapHandle) ProjectionMode() (ProjectionModeOptions, error) {
	snapshot, err := m.Snapshot()
	if err != nil {
		return ProjectionModeOptions{}, err
	}
	return snapshot.ProjectionMode, nil
}

// SetProjectionMode applies axonometric rendering option fields.
func (m *MapHandle) SetProjectionMode(options ProjectionModeOptions) (uint64, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	rawOptions := cProjectionModeOptions(options)
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_set_projection_mode(C.mln_map(ptr), &rawOptions, &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// PixelForLatLng converts a geographic coordinate to a logical screen point for
// the current map.
func (m *MapHandle) PixelForLatLng(coordinate LatLng) (ScreenPoint, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return ScreenPoint{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.mln_screen_point
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_pixel_for_lat_lng_start(C.mln_map(ptr), cLatLng(coordinate), out))
	})
	if err != nil {
		return ScreenPoint{}, err
	}
	defer C.mln_operation_release(operation)
	if err := checkNative(func() int32 {
		return int32(C.mln_map_pixel_for_lat_lng_take_result(operation, &raw))
	}); err != nil {
		return ScreenPoint{}, err
	}
	return goScreenPoint(raw), nil
}

// LatLngForPixel converts a logical screen point to a geographic coordinate for
// the current map.
func (m *MapHandle) LatLngForPixel(point ScreenPoint) (LatLng, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return LatLng{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.mln_lat_lng
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_lat_lng_for_pixel_start(C.mln_map(ptr), cScreenPoint(point), out))
	})
	if err != nil {
		return LatLng{}, err
	}
	defer C.mln_operation_release(operation)
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lng_for_pixel_take_result(operation, &raw))
	}); err != nil {
		return LatLng{}, err
	}
	return goLatLng(raw), nil
}

// PixelsForLatLngs converts geographic coordinates to logical screen points for
// the current map.
func (m *MapHandle) PixelsForLatLngs(coordinates []LatLng) ([]ScreenPoint, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer m.state.KeepAlive()
	rawCoordinates := cLatLngSlice(coordinates)
	rawPoints := make([]C.mln_screen_point, len(coordinates))
	var rawCoordinatesPtr *C.mln_lat_lng
	var rawPointsPtr *C.mln_screen_point
	if len(coordinates) > 0 {
		rawCoordinatesPtr = &rawCoordinates[0]
		rawPointsPtr = &rawPoints[0]
	}
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_pixels_for_lat_lngs_start(
			C.mln_map(ptr), rawCoordinatesPtr, C.size_t(len(coordinates)), out,
		))
	})
	if err != nil {
		return nil, err
	}
	defer C.mln_operation_release(operation)
	var count C.size_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_pixels_for_lat_lngs_take_result(
			operation, rawPointsPtr, C.size_t(len(rawPoints)), &count,
		))
	}); err != nil {
		return nil, err
	}
	rawPoints = rawPoints[:int(count)]
	return goScreenPointSlice(rawPoints), nil
}

// LatLngsForPixels converts logical screen points to geographic coordinates for
// the current map.
func (m *MapHandle) LatLngsForPixels(points []ScreenPoint) ([]LatLng, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer m.state.KeepAlive()
	rawPoints := cScreenPointSlice(points)
	rawCoordinates := make([]C.mln_lat_lng, len(points))
	var rawPointsPtr *C.mln_screen_point
	var rawCoordinatesPtr *C.mln_lat_lng
	if len(points) > 0 {
		rawPointsPtr = &rawPoints[0]
		rawCoordinatesPtr = &rawCoordinates[0]
	}
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_lat_lngs_for_pixels_start(
			C.mln_map(ptr), rawPointsPtr, C.size_t(len(points)), out,
		))
	})
	if err != nil {
		return nil, err
	}
	defer C.mln_operation_release(operation)
	var count C.size_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lngs_for_pixels_take_result(
			operation, rawCoordinatesPtr, C.size_t(len(rawCoordinates)), &count,
		))
	}); err != nil {
		return nil, err
	}
	rawCoordinates = rawCoordinates[:int(count)]
	return goLatLngSlice(rawCoordinates), nil
}

// Close waits for this map's native close operation. It discards this map's
// queued runtime events and releases callback state that the map still holds.
func (m *MapHandle) Close() error {
	if m == nil || m.state == nil {
		return newBindingError(ErrInvalidArgument, "MapHandle is nil")
	}
	defer func() {
		if m.runtime != nil && m.runtime.state != nil {
			m.runtime.state.KeepAlive()
		}
	}()
	var closeErr error
	_, err := m.state.CloseChecked(func(native nativeMap) int32 {
		var operation C.mln_operation
		if destroyMapHandle != nil {
			status := destroyMapHandle(native)
			if status != int32(C.MLN_STATUS_OK) {
				closeErr = &Error{
					kind:       kindForStatus(status),
					rawStatus:  status,
					hasStatus:  true,
					diagnostic: "synthetic map close failure",
				}
			}
			return status
		}
		if err := checkNative(func() int32 {
			return int32(C.mln_map_close_start(C.mln_map(native), &operation))
		}); err != nil {
			closeErr = err
			return statusFromError(err)
		}
		defer C.mln_operation_release(operation)
		if err := waitNativeOperation(operation); err != nil {
			closeErr = err
			return statusFromError(err)
		}
		return int32(C.MLN_STATUS_OK)
	})
	if err != nil {
		if errors.Is(err, handle.ErrLiveChildren) {
			return newBindingError(ErrInvalidState, "MapHandle has live child handles")
		}
		return newBindingError(ErrInvalidState, err.Error())
	}
	if closeErr != nil {
		return closeErr
	}
	if m.runtime != nil {
		m.runtime.unregisterMap(m)
	}
	m.runtimeChild.Release()
	return nil
}

// mapSizeByIDForTest calls the C snapshot accessor with a raw map id, so a test
// can replay a released id. The safe API cannot express a raw id.
func mapSizeByIDForTest(id nativeMap) error {
	raw := C.mln_map_snapshot{size: C.uint32_t(unsafe.Sizeof(C.mln_map_snapshot{}))}
	return checkNative(func() int32 {
		return int32(C.mln_map_snapshot_get(C.mln_map(id), &raw))
	})
}
