package maplibre

/*
#include <stdlib.h>

#include "maplibre_native_c.h"
*/
import "C"

import (
	"errors"
	"sync"
	"unsafe"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
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
	Width       uint32
	Height      uint32
	ScaleFactor float64
	Mode        MapMode
}

// NewMapOptions returns map creation options for a viewport size and scale.
func NewMapOptions(width, height uint32, scaleFactor float64) MapOptions {
	return MapOptions{Width: width, Height: height, ScaleFactor: scaleFactor, Mode: MapModeContinuous}
}

// MapHandle owns map state for one RuntimeHandle.
type MapHandle struct {
	state         *handle.State[nativeMap]
	runtime       *RuntimeHandle
	nativeAddress uintptr

	customGeometryMu      sync.Mutex
	customGeometrySources map[string]*callback.CustomGeometrySourceState
}

func (m *MapHandle) ptr() (*nativeMap, error) {
	if m == nil || m.state == nil {
		return nil, newBindingError(ErrInvalidArgument, "MapHandle is nil")
	}
	ptr, live := m.state.Ptr()
	if !live {
		return nil, newBindingError(ErrInvalidArgument, "MapHandle is closed")
	}
	return ptr, nil
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

// RequestRepaint requests a repaint for a continuous map.
func (m *MapHandle) RequestRepaint() error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() int32 { return int32(C.mln_map_request_repaint((*C.mln_map)(unsafe.Pointer(ptr)))) })
}

// RequestStillImage requests one still image for a static or tile map.
func (m *MapHandle) RequestStillImage() error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() int32 { return int32(C.mln_map_request_still_image((*C.mln_map)(unsafe.Pointer(ptr)))) })
}

// SetStyleURL loads a style URL through MapLibre Native style APIs.
func (m *MapHandle) SetStyleURL(url string) error {
	if err := validateCStringArgument("style URL", url); err != nil {
		return err
	}
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	cURL := C.CString(url)
	defer C.free(unsafe.Pointer(cURL))
	return checkNative(func() int32 { return int32(C.mln_map_set_style_url((*C.mln_map)(unsafe.Pointer(ptr)), cURL)) })
}

// SetStyleJSON loads inline style JSON through MapLibre Native style APIs.
func (m *MapHandle) SetStyleJSON(json string) error {
	if err := validateCStringArgument("style JSON", json); err != nil {
		return err
	}
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	cJSON := C.CString(json)
	defer C.free(unsafe.Pointer(cJSON))
	if err := checkNative(func() int32 { return int32(C.mln_map_set_style_json((*C.mln_map)(unsafe.Pointer(ptr)), cJSON)) }); err != nil {
		return err
	}
	m.releaseCustomGeometrySources()
	return nil
}

// SetDebugOptions applies MapLibre debug overlay mask bits to a map.
func (m *MapHandle) SetDebugOptions(options MapDebugOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_debug_options((*C.mln_map)(unsafe.Pointer(ptr)), C.uint32_t(options)))
	})
}

// DebugOptions returns the current MapLibre debug overlay mask bits.
func (m *MapHandle) DebugOptions() (MapDebugOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer m.state.KeepAlive()
	var raw C.uint32_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_debug_options((*C.mln_map)(unsafe.Pointer(ptr)), &raw))
	}); err != nil {
		return 0, err
	}
	return MapDebugOptions(raw), nil
}

// SetRenderingStatsViewEnabled enables or disables MapLibre's rendering stats
// overlay view.
func (m *MapHandle) SetRenderingStatsViewEnabled(enabled bool) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_rendering_stats_view_enabled((*C.mln_map)(unsafe.Pointer(ptr)), C.bool(enabled)))
	})
}

// RenderingStatsViewEnabled reports whether MapLibre's rendering stats overlay
// view is enabled.
func (m *MapHandle) RenderingStatsViewEnabled() (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	var enabled C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_rendering_stats_view_enabled((*C.mln_map)(unsafe.Pointer(ptr)), &enabled))
	}); err != nil {
		return false, err
	}
	return bool(enabled), nil
}

// IsFullyLoaded reports whether MapLibre currently considers the map fully
// loaded.
func (m *MapHandle) IsFullyLoaded() (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	var loaded C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_is_fully_loaded((*C.mln_map)(unsafe.Pointer(ptr)), &loaded))
	}); err != nil {
		return false, err
	}
	return bool(loaded), nil
}

// DumpDebugLogs dumps map debug logs through MapLibre Native logging.
func (m *MapHandle) DumpDebugLogs() error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() int32 { return int32(C.mln_map_dump_debug_logs((*C.mln_map)(unsafe.Pointer(ptr)))) })
}

// Camera returns the current camera snapshot.
func (m *MapHandle) Camera() (CameraOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw C.mln_camera_options = C.mln_camera_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_camera((*C.mln_map)(unsafe.Pointer(ptr)), &raw))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(raw), nil
}

// JumpTo applies a camera jump command.
func (m *MapHandle) JumpTo(camera CameraOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	return checkNative(func() int32 {
		return int32(C.mln_map_jump_to((*C.mln_map)(unsafe.Pointer(ptr)), &rawCamera))
	})
}

// EaseTo applies a camera ease transition command. Passing nil animation uses
// the native default animation options.
func (m *MapHandle) EaseTo(camera CameraOptions, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	rawAnimation, rawAnimationPtr := cAnimationOptionsPointer(animation)
	_ = rawAnimation
	return checkNative(func() int32 {
		return int32(C.mln_map_ease_to((*C.mln_map)(unsafe.Pointer(ptr)), &rawCamera, rawAnimationPtr))
	})
}

// FlyTo applies a camera fly transition command. Passing nil animation uses the
// native default animation options.
func (m *MapHandle) FlyTo(camera CameraOptions, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	rawAnimation, rawAnimationPtr := cAnimationOptionsPointer(animation)
	_ = rawAnimation
	return checkNative(func() int32 {
		return int32(C.mln_map_fly_to((*C.mln_map)(unsafe.Pointer(ptr)), &rawCamera, rawAnimationPtr))
	})
}

// MoveBy applies a screen-space pan command.
func (m *MapHandle) MoveBy(delta ScreenPoint) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_map_move_by((*C.mln_map)(unsafe.Pointer(ptr)), C.double(delta.X), C.double(delta.Y)))
	})
}

// MoveByAnimated applies an animated screen-space pan command. Passing nil
// animation uses the native default animation options.
func (m *MapHandle) MoveByAnimated(delta ScreenPoint, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	rawAnimation, rawAnimationPtr := cAnimationOptionsPointer(animation)
	_ = rawAnimation
	return checkNative(func() int32 {
		return int32(C.mln_map_move_by_animated(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			C.double(delta.X),
			C.double(delta.Y),
			rawAnimationPtr,
		))
	})
}

// ScaleBy applies a screen-space zoom command. Passing nil anchor uses the
// native default zoom anchor.
func (m *MapHandle) ScaleBy(scale float64, anchor *ScreenPoint) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	var rawAnchor C.mln_screen_point
	var rawAnchorPtr *C.mln_screen_point
	if anchor != nil {
		rawAnchor = cScreenPoint(*anchor)
		rawAnchorPtr = &rawAnchor
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_scale_by((*C.mln_map)(unsafe.Pointer(ptr)), C.double(scale), rawAnchorPtr))
	})
}

// ScaleByAnimated applies an animated screen-space zoom command. Passing nil
// anchor or animation uses the native default for that option.
func (m *MapHandle) ScaleByAnimated(scale float64, anchor *ScreenPoint, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	var rawAnchor C.mln_screen_point
	var rawAnchorPtr *C.mln_screen_point
	if anchor != nil {
		rawAnchor = cScreenPoint(*anchor)
		rawAnchorPtr = &rawAnchor
	}
	rawAnimation, rawAnimationPtr := cAnimationOptionsPointer(animation)
	_ = rawAnimation
	return checkNative(func() int32 {
		return int32(C.mln_map_scale_by_animated(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			C.double(scale),
			rawAnchorPtr,
			rawAnimationPtr,
		))
	})
}

// RotateBy applies a screen-space rotate command.
func (m *MapHandle) RotateBy(first ScreenPoint, second ScreenPoint) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_map_rotate_by((*C.mln_map)(unsafe.Pointer(ptr)), cScreenPoint(first), cScreenPoint(second)))
	})
}

// RotateByAnimated applies an animated screen-space rotate command. Passing nil
// animation uses the native default animation options.
func (m *MapHandle) RotateByAnimated(first ScreenPoint, second ScreenPoint, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	rawAnimation, rawAnimationPtr := cAnimationOptionsPointer(animation)
	_ = rawAnimation
	return checkNative(func() int32 {
		return int32(C.mln_map_rotate_by_animated(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			cScreenPoint(first),
			cScreenPoint(second),
			rawAnimationPtr,
		))
	})
}

// PitchBy applies a pitch delta command.
func (m *MapHandle) PitchBy(pitch float64) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_map_pitch_by((*C.mln_map)(unsafe.Pointer(ptr)), C.double(pitch)))
	})
}

// PitchByAnimated applies an animated pitch delta command. Passing nil
// animation uses the native default animation options.
func (m *MapHandle) PitchByAnimated(pitch float64, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	rawAnimation, rawAnimationPtr := cAnimationOptionsPointer(animation)
	_ = rawAnimation
	return checkNative(func() int32 {
		return int32(C.mln_map_pitch_by_animated((*C.mln_map)(unsafe.Pointer(ptr)), C.double(pitch), rawAnimationPtr))
	})
}

// CancelTransitions cancels active camera transitions.
func (m *MapHandle) CancelTransitions() error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() int32 { return int32(C.mln_map_cancel_transitions((*C.mln_map)(unsafe.Pointer(ptr)))) })
}

// CameraForLatLngBounds computes a camera that fits geographic bounds. Passing
// nil fitOptions uses native default fitting options.
func (m *MapHandle) CameraForLatLngBounds(bounds LatLngBounds, fitOptions *CameraFitOptions) (CameraOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw C.mln_camera_options = C.mln_camera_options_default()
	rawFitOptions, rawFitOptionsPtr := cCameraFitOptionsPointer(fitOptions)
	_ = rawFitOptions
	if err := checkNative(func() int32 {
		return int32(C.mln_map_camera_for_lat_lng_bounds(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			cLatLngBounds(bounds),
			rawFitOptionsPtr,
			&raw,
		))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(raw), nil
}

// CameraForLatLngs computes a camera that fits geographic coordinates. Passing
// nil fitOptions uses native default fitting options.
func (m *MapHandle) CameraForLatLngs(coordinates []LatLng, fitOptions *CameraFitOptions) (CameraOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw C.mln_camera_options = C.mln_camera_options_default()
	rawCoordinates := cLatLngSlice(coordinates)
	var rawCoordinatesPtr *C.mln_lat_lng
	if len(rawCoordinates) > 0 {
		rawCoordinatesPtr = &rawCoordinates[0]
	}
	rawFitOptions, rawFitOptionsPtr := cCameraFitOptionsPointer(fitOptions)
	_ = rawFitOptions
	if err := checkNative(func() int32 {
		return int32(C.mln_map_camera_for_lat_lngs(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			rawCoordinatesPtr,
			C.size_t(len(rawCoordinates)),
			rawFitOptionsPtr,
			&raw,
		))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(raw), nil
}

// CameraForGeometry computes a camera that fits a geometry. Passing nil
// fitOptions uses native default fitting options.
func (m *MapHandle) CameraForGeometry(geometry Geometry, fitOptions *CameraFitOptions) (CameraOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw C.mln_camera_options = C.mln_camera_options_default()
	materializer := newCGeometryMaterializer()
	defer materializer.free()
	rawGeometry, materialErr := materializer.geometryPtr(geometry)
	if materialErr != nil {
		return CameraOptions{}, newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	rawFitOptions, rawFitOptionsPtr := cCameraFitOptionsPointer(fitOptions)
	_ = rawFitOptions
	if err := checkNative(func() int32 {
		return int32(C.mln_map_camera_for_geometry(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			rawGeometry,
			rawFitOptionsPtr,
			&raw,
		))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(raw), nil
}

// LatLngBoundsForCamera computes wrapped geographic bounds for a camera in the
// current viewport.
func (m *MapHandle) LatLngBoundsForCamera(camera CameraOptions) (LatLngBounds, error) {
	ptr, err := m.ptr()
	if err != nil {
		return LatLngBounds{}, err
	}
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	var raw C.mln_lat_lng_bounds
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lng_bounds_for_camera((*C.mln_map)(unsafe.Pointer(ptr)), &rawCamera, &raw))
	}); err != nil {
		return LatLngBounds{}, err
	}
	return goLatLngBounds(raw), nil
}

// LatLngBoundsForCameraUnwrapped computes unwrapped geographic bounds for a
// camera in the current viewport.
func (m *MapHandle) LatLngBoundsForCameraUnwrapped(camera CameraOptions) (LatLngBounds, error) {
	ptr, err := m.ptr()
	if err != nil {
		return LatLngBounds{}, err
	}
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	var raw C.mln_lat_lng_bounds
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lng_bounds_for_camera_unwrapped((*C.mln_map)(unsafe.Pointer(ptr)), &rawCamera, &raw))
	}); err != nil {
		return LatLngBounds{}, err
	}
	return goLatLngBounds(raw), nil
}

// Bounds returns map camera constraint options.
func (m *MapHandle) Bounds() (BoundOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return BoundOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw C.mln_bound_options = C.mln_bound_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_bounds((*C.mln_map)(unsafe.Pointer(ptr)), &raw))
	}); err != nil {
		return BoundOptions{}, err
	}
	return goBoundOptions(raw), nil
}

// SetBounds applies selected map camera constraint options.
func (m *MapHandle) SetBounds(options BoundOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	rawOptions := cBoundOptions(options)
	return checkNative(func() int32 {
		return int32(C.mln_map_set_bounds((*C.mln_map)(unsafe.Pointer(ptr)), &rawOptions))
	})
}

// FreeCameraOptions returns current free camera position and orientation.
func (m *MapHandle) FreeCameraOptions() (FreeCameraOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return FreeCameraOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw C.mln_free_camera_options = C.mln_free_camera_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_free_camera_options((*C.mln_map)(unsafe.Pointer(ptr)), &raw))
	}); err != nil {
		return FreeCameraOptions{}, err
	}
	return goFreeCameraOptions(raw), nil
}

// SetFreeCameraOptions applies selected free camera position and orientation
// fields.
func (m *MapHandle) SetFreeCameraOptions(options FreeCameraOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	rawOptions := cFreeCameraOptions(options)
	return checkNative(func() int32 {
		return int32(C.mln_map_set_free_camera_options((*C.mln_map)(unsafe.Pointer(ptr)), &rawOptions))
	})
}

// ViewportOptions returns live map viewport and render-transform controls.
func (m *MapHandle) ViewportOptions() (ViewportOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return ViewportOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw C.mln_map_viewport_options = C.mln_map_viewport_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_viewport_options((*C.mln_map)(unsafe.Pointer(ptr)), &raw))
	}); err != nil {
		return ViewportOptions{}, err
	}
	return goViewportOptions(raw), nil
}

// SetViewportOptions applies selected live map viewport and render-transform
// controls.
func (m *MapHandle) SetViewportOptions(options ViewportOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	rawOptions := cViewportOptions(options)
	return checkNative(func() int32 {
		return int32(C.mln_map_set_viewport_options((*C.mln_map)(unsafe.Pointer(ptr)), &rawOptions))
	})
}

// TileOptions returns tile prefetch and LOD tuning controls.
func (m *MapHandle) TileOptions() (TileOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return TileOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw C.mln_map_tile_options = C.mln_map_tile_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_tile_options((*C.mln_map)(unsafe.Pointer(ptr)), &raw))
	}); err != nil {
		return TileOptions{}, err
	}
	return goTileOptions(raw), nil
}

// SetTileOptions applies selected tile prefetch and LOD tuning controls.
func (m *MapHandle) SetTileOptions(options TileOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	rawOptions := cTileOptions(options)
	return checkNative(func() int32 {
		return int32(C.mln_map_set_tile_options((*C.mln_map)(unsafe.Pointer(ptr)), &rawOptions))
	})
}

// ProjectionMode returns current axonometric rendering options.
func (m *MapHandle) ProjectionMode() (ProjectionModeOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return ProjectionModeOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw C.mln_projection_mode = C.mln_projection_mode_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_projection_mode((*C.mln_map)(unsafe.Pointer(ptr)), &raw))
	}); err != nil {
		return ProjectionModeOptions{}, err
	}
	return goProjectionModeOptions(raw), nil
}

// SetProjectionMode applies axonometric rendering option fields.
func (m *MapHandle) SetProjectionMode(options ProjectionModeOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	rawOptions := cProjectionModeOptions(options)
	return checkNative(func() int32 {
		return int32(C.mln_map_set_projection_mode((*C.mln_map)(unsafe.Pointer(ptr)), &rawOptions))
	})
}

// PixelForLatLng converts a geographic coordinate to a logical screen point for
// the current map.
func (m *MapHandle) PixelForLatLng(coordinate LatLng) (ScreenPoint, error) {
	ptr, err := m.ptr()
	if err != nil {
		return ScreenPoint{}, err
	}
	defer m.state.KeepAlive()
	var raw C.mln_screen_point
	if err := checkNative(func() int32 {
		return int32(C.mln_map_pixel_for_lat_lng((*C.mln_map)(unsafe.Pointer(ptr)), cLatLng(coordinate), &raw))
	}); err != nil {
		return ScreenPoint{}, err
	}
	return goScreenPoint(raw), nil
}

// LatLngForPixel converts a logical screen point to a geographic coordinate for
// the current map.
func (m *MapHandle) LatLngForPixel(point ScreenPoint) (LatLng, error) {
	ptr, err := m.ptr()
	if err != nil {
		return LatLng{}, err
	}
	defer m.state.KeepAlive()
	var raw C.mln_lat_lng
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lng_for_pixel((*C.mln_map)(unsafe.Pointer(ptr)), cScreenPoint(point), &raw))
	}); err != nil {
		return LatLng{}, err
	}
	return goLatLng(raw), nil
}

// PixelsForLatLngs converts geographic coordinates to logical screen points for
// the current map.
func (m *MapHandle) PixelsForLatLngs(coordinates []LatLng) ([]ScreenPoint, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	rawCoordinates := cLatLngSlice(coordinates)
	rawPoints := make([]C.mln_screen_point, len(coordinates))
	var rawCoordinatesPtr *C.mln_lat_lng
	var rawPointsPtr *C.mln_screen_point
	if len(coordinates) > 0 {
		rawCoordinatesPtr = &rawCoordinates[0]
		rawPointsPtr = &rawPoints[0]
	}
	if err := checkNative(func() int32 {
		return int32(C.mln_map_pixels_for_lat_lngs(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			rawCoordinatesPtr,
			C.size_t(len(coordinates)),
			rawPointsPtr,
		))
	}); err != nil {
		return nil, err
	}
	return goScreenPointSlice(rawPoints), nil
}

// LatLngsForPixels converts logical screen points to geographic coordinates for
// the current map.
func (m *MapHandle) LatLngsForPixels(points []ScreenPoint) ([]LatLng, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	rawPoints := cScreenPointSlice(points)
	rawCoordinates := make([]C.mln_lat_lng, len(points))
	var rawPointsPtr *C.mln_screen_point
	var rawCoordinatesPtr *C.mln_lat_lng
	if len(points) > 0 {
		rawPointsPtr = &rawPoints[0]
		rawCoordinatesPtr = &rawCoordinates[0]
	}
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lngs_for_pixels(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			rawPointsPtr,
			C.size_t(len(points)),
			rawCoordinatesPtr,
		))
	}); err != nil {
		return nil, err
	}
	return goLatLngSlice(rawCoordinates), nil
}

func (m *MapHandle) releaseCustomGeometrySource(sourceID string) {
	m.customGeometryMu.Lock()
	state := m.customGeometrySources[sourceID]
	delete(m.customGeometrySources, sourceID)
	m.customGeometryMu.Unlock()
	state.Release()
}

func (m *MapHandle) releaseDetachedCustomGeometrySources() {
	ptr, err := m.ptr()
	if err != nil {
		return
	}
	defer m.state.KeepAlive()

	m.customGeometryMu.Lock()
	sources := make(map[string]*callback.CustomGeometrySourceState, len(m.customGeometrySources))
	for sourceID, state := range m.customGeometrySources {
		sources[sourceID] = state
	}
	m.customGeometryMu.Unlock()

	for sourceID, state := range sources {
		sourceView := newCStringView(sourceID)
		var sourceType C.uint32_t
		var found C.bool
		err := checkNative(func() int32 {
			return int32(C.mln_map_get_style_source_type((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), &sourceType, &found))
		})
		sourceView.free()
		if err != nil {
			continue
		}
		if bool(found) && StyleSourceType(sourceType) == StyleSourceTypeCustomVector {
			continue
		}
		m.customGeometryMu.Lock()
		if m.customGeometrySources[sourceID] == state {
			delete(m.customGeometrySources, sourceID)
			m.customGeometryMu.Unlock()
			state.Release()
			continue
		}
		m.customGeometryMu.Unlock()
	}
}

func (m *MapHandle) customGeometrySourceCountForTesting() int {
	m.customGeometryMu.Lock()
	defer m.customGeometryMu.Unlock()
	return len(m.customGeometrySources)
}

func (m *MapHandle) releaseCustomGeometrySources() {
	m.customGeometryMu.Lock()
	states := m.customGeometrySources
	m.customGeometrySources = nil
	m.customGeometryMu.Unlock()
	for _, state := range states {
		state.Release()
	}
}

// Close destroys this map. A successful close makes later calls no-ops. A
// failed close leaves the native handle live so callers can retry on the owner
// thread.
func (m *MapHandle) Close() error {
	if m == nil || m.state == nil {
		return newBindingError(ErrInvalidArgument, "MapHandle is nil")
	}
	defer func() {
		if m.runtime != nil && m.runtime.state != nil {
			m.runtime.state.KeepAlive()
		}
	}()
	if err := checkNative(func() int32 {
		return m.state.Close(func(ptr *nativeMap) int32 {
			return int32(C.mln_map_destroy((*C.mln_map)(unsafe.Pointer(ptr))))
		})
	}); err != nil {
		return err
	}
	if m.runtime != nil {
		m.runtime.unregisterMap(m)
	}
	m.releaseCustomGeometrySources()
	return nil
}
