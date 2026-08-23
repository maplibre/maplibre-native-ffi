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
// command publishes a new generation in its completion, so a snapshot whose
// Generation is at or past that value observes the commit.
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
	state   *handle.State[nativeMap]
	runtime *RuntimeHandle
	// The map's native handle, which also serves as its public identity.
	id MapID
}

// Test seam for synthetic handles. Production close uses closeNativeMap.
var destroyMapHandle func(nativeMap) int32

func (m *MapHandle) ptr() (nativeMap, error) {
	if m == nil || m.state == nil {
		return 0, newBindingError(ErrInvalidArgument, "MapHandle is nil")
	}
	value, live := m.state.Handle()
	if !live {
		return 0, newBindingError(ErrInvalidArgument, "MapHandle is closed")
	}
	return value, nil
}

func startMapCompletion[T any](
	m *MapHandle,
	start func(C.mln_map, *C.mln_completion) int32,
	convert func(*C.mln_completion_result) (T, error),
) (*Future[T], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return start(C.mln_map(ptr), completion)
	}, convert)
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
	_, err := m.ptr()
	if err != nil {
		return 0, err
	}

	return m.id, nil
}

// SetEventMask submits a command that selects map-originated event types and
// .
func (m *MapHandle) SetEventMask(mask RuntimeEventMask) (*Future[CommandCompletion], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_event_mask(raw, C.uint64_t(mask), completion))
	}, completionCommand)
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
func (m *MapHandle) RequestRepaint() (*Future[CommandCompletion], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_request_repaint(raw, completion))
	}, completionCommand)
}

// RequestStillImage starts one noncoalescing still-image operation for a static
// or tile map.
func (m *MapHandle) RequestStillImage() (*Future[struct{}], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_request_still_image(raw, completion))
	}, completionUnit)
}

// SetStyleURL submits a style URL command. Loading
// failures arrive through runtime events.
func (m *MapHandle) SetStyleURL(url string) (*Future[CommandCompletion], error) {
	if err := validateCStringArgument("style URL", url); err != nil {
		return nil, err
	}
	cURL := C.CString(url)
	defer C.free(unsafe.Pointer(cURL))
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_style_url(raw, cURL, completion))
	}, completionCommand)
}

// SetStyleJSON submits an inline style command.
// Loading failures arrive through runtime events.
func (m *MapHandle) SetStyleJSON(json []byte) (*Future[CommandCompletion], error) {
	if bytes.IndexByte(json, 0) >= 0 {
		return nil, newBindingError(ErrInvalidArgument, "style JSON contains a NUL byte")
	}
	jsonView := newCBufferView(json)
	defer jsonView.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_style_json(raw, jsonView.raw(), completion))
	}, completionCommand)
}

// SetFeatureState submits a copied per-feature-state command. The selector
// and state value are copied before the call returns.
func (m *MapHandle) SetFeatureState(selector FeatureStateSelector, state []byte) (*Future[CommandCompletion], error) {
	rawSelector := newCFeatureStateSelector(selector)
	defer rawSelector.free()
	rawState := newCBufferView(state)
	defer rawState.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_feature_state(raw, &rawSelector.raw, rawState.raw(), completion))
	}, completionCommand)
}

// FeatureState starts an ordered read of copied per-feature state from this
// map's store. Missing feature state is reported as an empty JSON object.
func (m *MapHandle) FeatureState(selector FeatureStateSelector) (*Future[[]byte], error) {
	rawSelector := newCFeatureStateSelector(selector)
	defer rawSelector.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_get_feature_state(raw, &rawSelector.raw, completion))
	}, completionBuffer)
}

// RemoveFeatureState submits a per-feature-state removal command. The selector
// is copied before the call returns.
func (m *MapHandle) RemoveFeatureState(selector FeatureStateSelector) (*Future[CommandCompletion], error) {
	rawSelector := newCFeatureStateSelector(selector)
	defer rawSelector.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_remove_feature_state(raw, &rawSelector.raw, completion))
	}, completionCommand)
}

// LoadedStyleJSON returns an ordered copy of the last loaded style document.
func (m *MapHandle) LoadedStyleJSON() (*Future[[]byte], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_loaded_style_json(raw, completion))
	}, completionBuffer)
}

// StyleURL returns an ordered copy of the last requested style URL.
func (m *MapHandle) StyleURL() (*Future[string], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_style_url(raw, completion))
	}, func(result *C.mln_completion_result) (string, error) {
		value, err := completionBuffer(result)
		return string(value), err
	})
}

// SetDebugOptions applies MapLibre debug overlay mask bits to a map. The
// committed mask is visible through Snapshot as MapSnapshot.DebugOptions.
func (m *MapHandle) SetDebugOptions(options MapDebugOptions) (*Future[CommandCompletion], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_debug_options(raw, C.uint32_t(options), completion))
	}, completionCommand)
}

// SetRenderingStatsViewEnabled enables or disables MapLibre's rendering stats
// overlay view. The committed value is visible through Snapshot as
// MapSnapshot.RenderingStatsViewEnabled.
func (m *MapHandle) SetRenderingStatsViewEnabled(enabled bool) (*Future[CommandCompletion], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_rendering_stats_view_enabled(raw, C.bool(enabled), completion))
	}, completionCommand)
}

// Snapshot returns a copy of the latest immutable map state.
func (m *MapHandle) Snapshot() (MapSnapshot, error) {
	ptr, err := m.ptr()
	if err != nil {
		return MapSnapshot{}, err
	}

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

// Resize submits a logical extent update.
func (m *MapHandle) Resize(extent LogicalExtent) (*Future[CommandCompletion], error) {
	raw := C.mln_logical_extent{
		width:        C.uint32_t(extent.Width),
		height:       C.uint32_t(extent.Height),
		scale_factor: C.double(extent.ScaleFactor),
	}
	return startMapCompletion(m, func(handle C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_resize(handle, raw, completion))
	}, completionCommand)
}

// DumpDebugLogs dumps map debug logs through MapLibre Native logging.
func (m *MapHandle) DumpDebugLogs() (*Future[CommandCompletion], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_dump_debug_logs(raw, completion))
	}, completionCommand)
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
	ptr, err := m.ptr()
	if err != nil {
		return CameraSnapshot{}, err
	}

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
func (m *MapHandle) QueryCamera() (*Future[CameraSnapshot], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_camera_query(raw, completion))
	}, func(result *C.mln_completion_result) (CameraSnapshot, error) {
		raw, err := completionValue[C.mln_camera_query_result](result)
		if err != nil {
			return CameraSnapshot{}, err
		}
		return CameraSnapshot{Generation: uint64(raw.generation), Camera: goCameraOptions(raw.camera)}, nil
	})
}

// UpdateCamera submits one atomic camera update.
func (m *MapHandle) UpdateCamera(update CameraUpdate) (*Future[CommandCompletion], error) {
	raw := cCameraUpdate(update)
	return startMapCompletion(m, func(handle C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_update_camera(handle, &raw, completion))
	}, completionCommand)
}

// ApplyCameraDelta submits one relative camera operation.
func (m *MapHandle) ApplyCameraDelta(delta CameraDelta) (*Future[CommandCompletion], error) {
	raw := cCameraDelta(delta)
	return startMapCompletion(m, func(handle C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_apply_camera_delta(handle, &raw, completion))
	}, completionCommand)
}

// JumpTo submits an atomic camera jump.
func (m *MapHandle) JumpTo(camera CameraOptions) (*Future[CommandCompletion], error) {
	return m.UpdateCamera(CameraUpdate{Mode: CameraUpdateModeJump, Camera: camera})
}

// EaseTo submits an atomic eased camera transition.
func (m *MapHandle) EaseTo(camera CameraOptions, animation *AnimationOptions) (*Future[CommandCompletion], error) {
	return m.UpdateCamera(CameraUpdate{Mode: CameraUpdateModeEase, Camera: camera, Animation: animation})
}

// FlyTo submits an atomic flying camera transition.
func (m *MapHandle) FlyTo(camera CameraOptions, animation *AnimationOptions) (*Future[CommandCompletion], error) {
	return m.UpdateCamera(CameraUpdate{Mode: CameraUpdateModeFly, Camera: camera, Animation: animation})
}

// CameraForLatLngBounds computes a camera that fits geographic bounds. Passing
// nil fitOptions uses native default fitting options.
func (m *MapHandle) CameraForLatLngBounds(bounds LatLngBounds, fitOptions *CameraFitOptions) (*Future[CameraOptions], error) {
	rawFitOptions, rawFitOptionsPtr := cCameraFitOptionsPointer(fitOptions)
	_ = rawFitOptions
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_camera_for_lat_lng_bounds(
			raw, cLatLngBounds(bounds), rawFitOptionsPtr, completion,
		))
	}, func(result *C.mln_completion_result) (CameraOptions, error) {
		raw, err := completionValue[C.mln_camera_options](result)
		return goCameraOptions(raw), err
	})
}

// CameraForLatLngs computes a camera that fits geographic coordinates. Passing
// nil fitOptions uses native default fitting options.
func (m *MapHandle) CameraForLatLngs(coordinates []LatLng, fitOptions *CameraFitOptions) (*Future[CameraOptions], error) {
	rawCoordinates := cLatLngSlice(coordinates)
	var rawCoordinatesPtr *C.mln_lat_lng
	if len(rawCoordinates) > 0 {
		rawCoordinatesPtr = &rawCoordinates[0]
	}
	rawFitOptions, rawFitOptionsPtr := cCameraFitOptionsPointer(fitOptions)
	_ = rawFitOptions
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_camera_for_lat_lngs(
			raw, rawCoordinatesPtr, C.size_t(len(rawCoordinates)),
			rawFitOptionsPtr, completion,
		))
	}, func(result *C.mln_completion_result) (CameraOptions, error) {
		value, err := completionValue[C.mln_camera_options](result)
		return goCameraOptions(value), err
	})
}

// CameraForGeometry computes a camera that fits a geometry. Passing nil
// fitOptions uses native default fitting options.
func (m *MapHandle) CameraForGeometry(geometry []byte, fitOptions *CameraFitOptions) (*Future[CameraOptions], error) {
	rawGeometry := newCBufferView(geometry)
	defer rawGeometry.free()
	rawFitOptions, rawFitOptionsPtr := cCameraFitOptionsPointer(fitOptions)
	_ = rawFitOptions
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_camera_for_geometry(
			raw, rawGeometry.raw(), rawFitOptionsPtr, completion,
		))
	}, func(result *C.mln_completion_result) (CameraOptions, error) {
		value, err := completionValue[C.mln_camera_options](result)
		return goCameraOptions(value), err
	})
}

// LatLngBoundsForCamera computes geographic bounds for a camera from two
// viewport corners.
//
// The box is the hull of the top-left and bottom-right screen corners for that
// camera in the current viewport. When bearing and pitch are zero, the box
// equals the visible area. Those corners are the northwest and southeast of
// the viewport. Longitudes stay in -180 to 180.
func (m *MapHandle) LatLngBoundsForCamera(camera CameraOptions) (*Future[LatLngBounds], error) {
	rawCamera := cCameraOptions(camera)
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_lat_lng_bounds_for_camera(raw, &rawCamera, completion))
	}, func(result *C.mln_completion_result) (LatLngBounds, error) {
		value, err := completionValue[C.mln_lat_lng_bounds](result)
		return goLatLngBounds(value), err
	})
}

// LatLngBoundsForCameraUnwrapped computes geographic bounds for a camera from
// the four viewport corners.
//
// The axis-aligned hull of all four screen corners and the center encompasses
// the projected viewport. Longitudes unwrap onto the shortest path through the
// center. A viewport that crosses the antimeridian reports values outside -180
// to 180.
func (m *MapHandle) LatLngBoundsForCameraUnwrapped(camera CameraOptions) (*Future[LatLngBounds], error) {
	rawCamera := cCameraOptions(camera)
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_lat_lng_bounds_for_camera_unwrapped(raw, &rawCamera, completion))
	}, func(result *C.mln_completion_result) (LatLngBounds, error) {
		value, err := completionValue[C.mln_lat_lng_bounds](result)
		return goLatLngBounds(value), err
	})
}

// SetBounds applies selected map camera constraint options. The committed
// constraints are visible through Snapshot as MapSnapshot.Bounds.
func (m *MapHandle) SetBounds(options BoundOptions) (*Future[CommandCompletion], error) {
	rawOptions, err := cBoundOptions(options)
	if err != nil {
		return nil, err
	}
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_bounds(raw, &rawOptions, completion))
	}, completionCommand)
}

// SetFreeCameraOptions applies selected free camera position and orientation
// fields. The committed options are visible through Snapshot as
// MapSnapshot.FreeCamera.
func (m *MapHandle) SetFreeCameraOptions(options FreeCameraOptions) (*Future[CommandCompletion], error) {
	rawOptions := cFreeCameraOptions(options)
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_free_camera_options(raw, &rawOptions, completion))
	}, completionCommand)
}

// SetViewportOptions applies selected live map viewport and render-transform
// controls. The committed options are visible through Snapshot as
// MapSnapshot.Viewport.
func (m *MapHandle) SetViewportOptions(options ViewportOptions) (*Future[CommandCompletion], error) {
	rawOptions := cViewportOptions(options)
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_viewport_options(raw, &rawOptions, completion))
	}, completionCommand)
}

// SetTileOptions applies selected tile prefetch and LOD tuning controls. The
// committed options are visible through Snapshot as MapSnapshot.Tile.
func (m *MapHandle) SetTileOptions(options TileOptions) (*Future[CommandCompletion], error) {
	rawOptions := cTileOptions(options)
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_tile_options(raw, &rawOptions, completion))
	}, completionCommand)
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
func (m *MapHandle) SetProjectionMode(options ProjectionModeOptions) (*Future[CommandCompletion], error) {
	rawOptions := cProjectionModeOptions(options)
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_projection_mode(raw, &rawOptions, completion))
	}, completionCommand)
}

// PixelForLatLng converts a geographic coordinate to a logical screen point for
// the current map.
func (m *MapHandle) PixelForLatLng(coordinate LatLng) (*Future[ScreenPoint], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_pixel_for_lat_lng(raw, cLatLng(coordinate), completion))
	}, func(result *C.mln_completion_result) (ScreenPoint, error) {
		value, err := completionValue[C.mln_screen_point](result)
		return goScreenPoint(value), err
	})
}

// LatLngForPixel converts a logical screen point to a geographic coordinate for
// the current map.
func (m *MapHandle) LatLngForPixel(point ScreenPoint) (*Future[LatLng], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_lat_lng_for_pixel(raw, cScreenPoint(point), completion))
	}, func(result *C.mln_completion_result) (LatLng, error) {
		value, err := completionValue[C.mln_lat_lng](result)
		return goLatLng(value), err
	})
}

// PixelsForLatLngs converts geographic coordinates to logical screen points for
// the current map.
func (m *MapHandle) PixelsForLatLngs(coordinates []LatLng) (*Future[[]ScreenPoint], error) {
	rawCoordinates := cLatLngSlice(coordinates)
	var rawCoordinatesPtr *C.mln_lat_lng
	if len(coordinates) > 0 {
		rawCoordinatesPtr = &rawCoordinates[0]
	}
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_pixels_for_lat_lngs(
			raw, rawCoordinatesPtr, C.size_t(len(coordinates)), completion,
		))
	}, func(result *C.mln_completion_result) ([]ScreenPoint, error) {
		values, err := completionSlice[C.mln_screen_point](result)
		return goScreenPointSlice(values), err
	})
}

// LatLngsForPixels converts logical screen points to geographic coordinates for
// the current map.
func (m *MapHandle) LatLngsForPixels(points []ScreenPoint) (*Future[[]LatLng], error) {
	rawPoints := cScreenPointSlice(points)
	var rawPointsPtr *C.mln_screen_point
	if len(points) > 0 {
		rawPointsPtr = &rawPoints[0]
	}
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_lat_lngs_for_pixels(
			raw, rawPointsPtr, C.size_t(len(points)), completion,
		))
	}, func(result *C.mln_completion_result) ([]LatLng, error) {
		values, err := completionSlice[C.mln_lat_lng](result)
		return goLatLngSlice(values), err
	})
}

// Close releases this map's public native handle. Use CloseAsync when later
// work depends on the end of native teardown.
func (m *MapHandle) Close() error {
	_, err := m.CloseAsync()
	return err
}

// CloseAsync releases this map's public native handle and returns the future
// for native teardown.
func (m *MapHandle) CloseAsync() (*Future[struct{}], error) {
	if m == nil || m.state == nil {
		return nil, newBindingError(ErrInvalidArgument, "MapHandle is nil")
	}
	defer func() {
		if m.runtime != nil && m.runtime.state != nil {
			m.runtime.state.KeepAlive()
		}
	}()
	var closeErr error
	teardown := completedFuture(struct{}{})
	_ = m.state.Close(func(native nativeMap) int32 {
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
		var err error
		teardown, err = startCompletion(func(completion *C.mln_completion) int32 {
			return int32(C.mln_map_release(C.mln_map(native), completion))
		}, completionUnit)
		if err != nil {
			closeErr = err
			return statusFromError(err)
		}
		return int32(C.MLN_STATUS_OK)
	})
	if closeErr != nil {
		return nil, closeErr
	}
	if m.runtime != nil {
		m.runtime.unregisterMap(m)
	}
	return teardown, nil
}

// mapSizeByIDForTest calls the C snapshot accessor with a raw map id, so a test
// can replay a released id. The safe API cannot express a raw id.
func mapSizeByIDForTest(id nativeMap) error {
	raw := C.mln_map_snapshot{size: C.uint32_t(unsafe.Sizeof(C.mln_map_snapshot{}))}
	return checkNative(func() int32 {
		return int32(C.mln_map_snapshot_get(C.mln_map(id), &raw))
	})
}
