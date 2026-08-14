package maplibre

/*
#include <stdlib.h>

#include "maplibre_native_c.h"
*/
import "C"

import (
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
	// Width is the initial logical width in UI pixels, replaced by the extent of
	// the first attached render session.
	Width uint32
	// Height is the initial logical height in UI pixels, replaced by the extent
	// of the first attached render session.
	Height uint32
	// ScaleFactor is the UI-to-device pixel scale, fixed for the lifetime of the
	// map. It selects sprites, glyphs, and raster tiles. A render session whose
	// own scale factor differs logs a warning and renders imagery chosen for
	// this density.
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

// MapHandle owns map state for one RuntimeHandle.
type MapHandle struct {
	state        *handle.State[nativeMap]
	runtime      *RuntimeHandle
	runtimeChild *handle.Child
	// The map's native handle, which also serves as its public identity.
	id MapID
}

var destroyMapHandle = func(native nativeMap) int32 {
	return int32(C.mln_map_destroy(C.mln_map(native)))
}

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

// SetEventMask selects which map-originated event types this map queues. It
// accepts RuntimeEventMaskAll, reads the bits in RuntimeEventMaskAllMapEvents,
// and returns ErrInvalidArgument for a bit outside RuntimeEventMaskAll.
//
// Select every event type the caller reads. Render-update-available is the map's
// only invalidation report, the two still-image types are the only reports that
// a still-image request finished, and loading-failed and render-error carry
// native failure text. Narrowing gates later events and keeps queued ones, so a
// caller drains what it already caused.
func (m *MapHandle) SetEventMask(mask RuntimeEventMask) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()

	return checkNative(func() int32 {
		return int32(C.mln_map_set_event_mask(C.mln_map(ptr), C.uint64_t(mask)))
	})
}

// EventMask reports which map-originated event types this map queues. A map that
// has not been narrowed reports RuntimeEventMaskAll.
func (m *MapHandle) EventMask() (RuntimeEventMask, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()

	var raw C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_event_mask(C.mln_map(ptr), &raw))
	}); err != nil {
		return 0, err
	}
	return RuntimeEventMask(raw), nil
}

// RequestRepaint requests a repaint for a continuous map.
func (m *MapHandle) RequestRepaint() error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	return checkNative(func() int32 { return int32(C.mln_map_request_repaint(C.mln_map(ptr))) })
}

// RequestStillImage requests one still image for a static or tile map.
func (m *MapHandle) RequestStillImage() error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	return checkNative(func() int32 { return int32(C.mln_map_request_still_image(C.mln_map(ptr))) })
}

// SetStyleURL loads a style URL. Loading is asynchronous: a style that is
// missing, unreachable, or malformed still returns success here and reports
// through a map-loading-failed runtime event. A well-formed style that MapLibre
// rejects semantically produces neither an error nor an event.
func (m *MapHandle) SetStyleURL(url string) error {
	if err := validateCStringArgument("style URL", url); err != nil {
		return err
	}
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	cURL := C.CString(url)
	defer C.free(unsafe.Pointer(cURL))
	return checkNative(func() int32 { return int32(C.mln_map_set_style_url(C.mln_map(ptr), cURL)) })
}

// SetStyleJSON loads inline style JSON. Malformed JSON is reported twice: this
// call returns the parse error, and the same message arrives as a
// map-loading-failed runtime event. A well-formed style that MapLibre rejects
// semantically produces neither an error nor an event.
func (m *MapHandle) SetStyleJSON(json []byte) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	jsonView := newCBufferView(json)
	defer jsonView.free()
	return checkNative(func() int32 { return int32(C.mln_map_set_style_json(C.mln_map(ptr), jsonView.raw())) })
}

// LoadedStyleJSON returns the style document this map's style was last parsed
// from, byte for byte, rather than a serialization of the live style. Runtime
// mutations such as adding a layer do not change it, and a failed parse leaves
// the previously parsed document in place. The result is empty only when no
// document has been parsed.
func (m *MapHandle) LoadedStyleJSON() ([]byte, error) {
	return m.copyMapBytes(func(rawMap C.mln_map, data *C.uint8_t, capacity C.size_t, size *C.size_t) int32 {
		return int32(C.mln_map_copy_loaded_style_json(rawMap, data, capacity, size))
	})
}

// StyleURL returns the URL this map's style was last requested from.
// SetStyleURL records the URL when the request is made, before the response
// arrives or the document parses, and SetStyleJSON clears it, so this and
// LoadedStyleJSON can disagree while a load is in flight or after one fails.
//
// The result is empty for a style loaded from inline JSON, a map that has
// loaded no style, and a URL load requested with an empty string alike.
func (m *MapHandle) StyleURL() (string, error) {
	return m.copyMapText(func(rawMap C.mln_map, text *C.char, capacity C.size_t, size *C.size_t) int32 {
		return int32(C.mln_map_copy_style_url(rawMap, text, capacity, size))
	})
}

// SetDebugOptions applies MapLibre debug overlay mask bits to a map.
func (m *MapHandle) SetDebugOptions(options MapDebugOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_debug_options(C.mln_map(ptr), C.uint32_t(options)))
	})
}

// DebugOptions returns the current MapLibre debug overlay mask bits.
func (m *MapHandle) DebugOptions() (MapDebugOptions, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.uint32_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_debug_options(C.mln_map(ptr), &raw))
	}); err != nil {
		return 0, err
	}
	return MapDebugOptions(raw), nil
}

// SetRenderingStatsViewEnabled enables or disables MapLibre's rendering stats
// overlay view.
func (m *MapHandle) SetRenderingStatsViewEnabled(enabled bool) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_rendering_stats_view_enabled(C.mln_map(ptr), C.bool(enabled)))
	})
}

// RenderingStatsViewEnabled reports whether MapLibre's rendering stats overlay
// view is enabled.
func (m *MapHandle) RenderingStatsViewEnabled() (bool, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer release()
	defer m.state.KeepAlive()
	var enabled C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_rendering_stats_view_enabled(C.mln_map(ptr), &enabled))
	}); err != nil {
		return false, err
	}
	return bool(enabled), nil
}

// IsFullyLoaded reports whether MapLibre currently considers the map fully
// loaded.
func (m *MapHandle) IsFullyLoaded() (bool, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer release()
	defer m.state.KeepAlive()
	var loaded C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_is_fully_loaded(C.mln_map(ptr), &loaded))
	}); err != nil {
		return false, err
	}
	return bool(loaded), nil
}

// Size returns the map's logical viewport size in UI pixels and its scale
// factor. The scale factor is independent of any render target's.
func (m *MapHandle) Size() (width uint32, height uint32, scaleFactor float64, err error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return 0, 0, 0, err
	}
	defer release()
	defer m.state.KeepAlive()
	var rawWidth, rawHeight C.uint32_t
	var rawScaleFactor C.double
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_size(C.mln_map(ptr), &rawWidth, &rawHeight, &rawScaleFactor))
	}); err != nil {
		return 0, 0, 0, err
	}
	return uint32(rawWidth), uint32(rawHeight), float64(rawScaleFactor), nil
}

// DumpDebugLogs dumps map debug logs through MapLibre Native logging.
func (m *MapHandle) DumpDebugLogs() error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	return checkNative(func() int32 { return int32(C.mln_map_dump_debug_logs(C.mln_map(ptr))) })
}

// Camera returns the current camera snapshot.
func (m *MapHandle) Camera() (CameraOptions, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.mln_camera_options = C.mln_camera_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_camera(C.mln_map(ptr), &raw))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(raw), nil
}

// JumpTo applies a camera jump command.
func (m *MapHandle) JumpTo(camera CameraOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	return checkNative(func() int32 {
		return int32(C.mln_map_jump_to(C.mln_map(ptr), &rawCamera))
	})
}

// EaseTo applies a camera ease transition command. A nil animation, or one with
// no Duration, uses the native default duration of zero, so the camera reaches
// the target immediately; set Duration explicitly to animate.
func (m *MapHandle) EaseTo(camera CameraOptions, animation *AnimationOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	rawAnimation, rawAnimationPtr := cAnimationOptionsPointer(animation)
	_ = rawAnimation
	return checkNative(func() int32 {
		return int32(C.mln_map_ease_to(C.mln_map(ptr), &rawCamera, rawAnimationPtr))
	})
}

// FlyTo applies a camera fly transition command. A nil animation, or one with
// no Duration, flies at a default velocity of 1.2 ρ-screenfuls per second, so
// the duration scales with the distance travelled.
func (m *MapHandle) FlyTo(camera CameraOptions, animation *AnimationOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	rawAnimation, rawAnimationPtr := cAnimationOptionsPointer(animation)
	_ = rawAnimation
	return checkNative(func() int32 {
		return int32(C.mln_map_fly_to(C.mln_map(ptr), &rawCamera, rawAnimationPtr))
	})
}

// MoveBy applies a screen-space pan command.
func (m *MapHandle) MoveBy(delta ScreenPoint) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_map_move_by(C.mln_map(ptr), C.double(delta.X), C.double(delta.Y)))
	})
}

// MoveByAnimated applies an animated screen-space pan command. A nil
// animation, or one with no Duration, applies the change instantly; see EaseTo.
func (m *MapHandle) MoveByAnimated(delta ScreenPoint, animation *AnimationOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	rawAnimation, rawAnimationPtr := cAnimationOptionsPointer(animation)
	_ = rawAnimation
	return checkNative(func() int32 {
		return int32(C.mln_map_move_by_animated(
			C.mln_map(ptr),
			C.double(delta.X),
			C.double(delta.Y),
			rawAnimationPtr,
		))
	})
}

// ScaleBy applies a screen-space zoom command. Passing nil anchor uses the
// native default zoom anchor.
func (m *MapHandle) ScaleBy(scale float64, anchor *ScreenPoint) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	var rawAnchor C.mln_screen_point
	var rawAnchorPtr *C.mln_screen_point
	if anchor != nil {
		rawAnchor = cScreenPoint(*anchor)
		rawAnchorPtr = &rawAnchor
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_scale_by(C.mln_map(ptr), C.double(scale), rawAnchorPtr))
	})
}

// ScaleByAnimated applies an animated screen-space zoom command. Passing nil
// anchor or animation uses the native default for that option.
func (m *MapHandle) ScaleByAnimated(scale float64, anchor *ScreenPoint, animation *AnimationOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
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
			C.mln_map(ptr),
			C.double(scale),
			rawAnchorPtr,
			rawAnimationPtr,
		))
	})
}

// RotateBy applies a screen-space rotate command.
func (m *MapHandle) RotateBy(first ScreenPoint, second ScreenPoint) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_map_rotate_by(C.mln_map(ptr), cScreenPoint(first), cScreenPoint(second)))
	})
}

// RotateByAnimated applies an animated screen-space rotate command. A nil
// animation, or one with no Duration, applies the change instantly; see EaseTo.
func (m *MapHandle) RotateByAnimated(first ScreenPoint, second ScreenPoint, animation *AnimationOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	rawAnimation, rawAnimationPtr := cAnimationOptionsPointer(animation)
	_ = rawAnimation
	return checkNative(func() int32 {
		return int32(C.mln_map_rotate_by_animated(
			C.mln_map(ptr),
			cScreenPoint(first),
			cScreenPoint(second),
			rawAnimationPtr,
		))
	})
}

// PitchBy applies a pitch delta command.
func (m *MapHandle) PitchBy(pitch float64) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_map_pitch_by(C.mln_map(ptr), C.double(pitch)))
	})
}

// PitchByAnimated applies an animated pitch delta command. A nil animation, or
// one with no Duration, applies the change instantly; see EaseTo.
func (m *MapHandle) PitchByAnimated(pitch float64, animation *AnimationOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	rawAnimation, rawAnimationPtr := cAnimationOptionsPointer(animation)
	_ = rawAnimation
	return checkNative(func() int32 {
		return int32(C.mln_map_pitch_by_animated(C.mln_map(ptr), C.double(pitch), rawAnimationPtr))
	})
}

// CancelTransitions cancels active camera transitions.
func (m *MapHandle) CancelTransitions() error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	return checkNative(func() int32 { return int32(C.mln_map_cancel_transitions(C.mln_map(ptr))) })
}

// SetGestureInProgress marks whether a host-driven gesture is in progress. The
// flag stays set until the host clears it, so pair every true with a false.
func (m *MapHandle) SetGestureInProgress(inProgress bool) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_gesture_in_progress(C.mln_map(ptr), C.bool(inProgress)))
	})
}

// IsGestureInProgress reports whether a host-driven gesture is currently in
// progress.
func (m *MapHandle) IsGestureInProgress() (bool, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer release()
	defer m.state.KeepAlive()
	var inProgress C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_is_gesture_in_progress(C.mln_map(ptr), &inProgress))
	}); err != nil {
		return false, err
	}
	return bool(inProgress), nil
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
	if err := checkNative(func() int32 {
		return int32(C.mln_map_camera_for_lat_lng_bounds(
			C.mln_map(ptr),
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
	if err := checkNative(func() int32 {
		return int32(C.mln_map_camera_for_lat_lngs(
			C.mln_map(ptr),
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
	if err := checkNative(func() int32 {
		return int32(C.mln_map_camera_for_geometry(
			C.mln_map(ptr),
			rawGeometry.raw(),
			rawFitOptionsPtr,
			&raw,
		))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(raw), nil
}

// LatLngBoundsForCamera computes geographic bounds for a camera from two
// viewport corners.
//
// The box is the hull of the top-left and bottom-right screen corners for that
// camera in the current viewport. When bearing and pitch are zero, the box
// equals the visible area. Those corners are the northwest and southeast of
// the viewport. Longitudes stay in -180 to 180.
func (m *MapHandle) LatLngBoundsForCamera(camera CameraOptions) (LatLngBounds, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return LatLngBounds{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	var raw C.mln_lat_lng_bounds
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lng_bounds_for_camera(C.mln_map(ptr), &rawCamera, &raw))
	}); err != nil {
		return LatLngBounds{}, err
	}
	return goLatLngBounds(raw), nil
}

// LatLngBoundsForCameraUnwrapped computes geographic bounds for a camera from
// the four viewport corners.
//
// The axis-aligned hull of all four screen corners and the center encompasses
// the projected viewport. Longitudes unwrap onto the shortest path through the
// center. A viewport that crosses the antimeridian reports values outside -180
// to 180.
func (m *MapHandle) LatLngBoundsForCameraUnwrapped(camera CameraOptions) (LatLngBounds, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return LatLngBounds{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	rawCamera := cCameraOptions(camera)
	var raw C.mln_lat_lng_bounds
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lng_bounds_for_camera_unwrapped(C.mln_map(ptr), &rawCamera, &raw))
	}); err != nil {
		return LatLngBounds{}, err
	}
	return goLatLngBounds(raw), nil
}

// Bounds returns map camera constraint options.
func (m *MapHandle) Bounds() (BoundOptions, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return BoundOptions{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.mln_bound_options = C.mln_bound_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_bounds(C.mln_map(ptr), &raw))
	}); err != nil {
		return BoundOptions{}, err
	}
	return goBoundOptions(raw), nil
}

// SetBounds applies selected map camera constraint options.
func (m *MapHandle) SetBounds(options BoundOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	rawOptions, err := cBoundOptions(options)
	if err != nil {
		return err
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_set_bounds(C.mln_map(ptr), &rawOptions))
	})
}

// FreeCameraOptions returns current free camera position and orientation.
func (m *MapHandle) FreeCameraOptions() (FreeCameraOptions, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return FreeCameraOptions{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.mln_free_camera_options = C.mln_free_camera_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_free_camera_options(C.mln_map(ptr), &raw))
	}); err != nil {
		return FreeCameraOptions{}, err
	}
	return goFreeCameraOptions(raw), nil
}

// SetFreeCameraOptions applies selected free camera position and orientation
// fields.
func (m *MapHandle) SetFreeCameraOptions(options FreeCameraOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	rawOptions := cFreeCameraOptions(options)
	return checkNative(func() int32 {
		return int32(C.mln_map_set_free_camera_options(C.mln_map(ptr), &rawOptions))
	})
}

// ViewportOptions returns live map viewport and render-transform controls.
func (m *MapHandle) ViewportOptions() (ViewportOptions, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return ViewportOptions{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.mln_map_viewport_options = C.mln_map_viewport_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_viewport_options(C.mln_map(ptr), &raw))
	}); err != nil {
		return ViewportOptions{}, err
	}
	return goViewportOptions(raw), nil
}

// SetViewportOptions applies selected live map viewport and render-transform
// controls.
func (m *MapHandle) SetViewportOptions(options ViewportOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	rawOptions := cViewportOptions(options)
	return checkNative(func() int32 {
		return int32(C.mln_map_set_viewport_options(C.mln_map(ptr), &rawOptions))
	})
}

// TileOptions returns tile prefetch and LOD tuning controls.
func (m *MapHandle) TileOptions() (TileOptions, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return TileOptions{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.mln_map_tile_options = C.mln_map_tile_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_tile_options(C.mln_map(ptr), &raw))
	}); err != nil {
		return TileOptions{}, err
	}
	return goTileOptions(raw), nil
}

// SetTileOptions applies selected tile prefetch and LOD tuning controls.
func (m *MapHandle) SetTileOptions(options TileOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	rawOptions := cTileOptions(options)
	return checkNative(func() int32 {
		return int32(C.mln_map_set_tile_options(C.mln_map(ptr), &rawOptions))
	})
}

// ProjectionMode returns current axonometric rendering options.
func (m *MapHandle) ProjectionMode() (ProjectionModeOptions, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return ProjectionModeOptions{}, err
	}
	defer release()
	defer m.state.KeepAlive()
	var raw C.mln_projection_mode = C.mln_projection_mode_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_projection_mode(C.mln_map(ptr), &raw))
	}); err != nil {
		return ProjectionModeOptions{}, err
	}
	return goProjectionModeOptions(raw), nil
}

// SetProjectionMode applies axonometric rendering option fields.
func (m *MapHandle) SetProjectionMode(options ProjectionModeOptions) error {
	ptr, release, err := m.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer m.state.KeepAlive()
	rawOptions := cProjectionModeOptions(options)
	return checkNative(func() int32 {
		return int32(C.mln_map_set_projection_mode(C.mln_map(ptr), &rawOptions))
	})
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
	if err := checkNative(func() int32 {
		return int32(C.mln_map_pixel_for_lat_lng(C.mln_map(ptr), cLatLng(coordinate), &raw))
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
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lng_for_pixel(C.mln_map(ptr), cScreenPoint(point), &raw))
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
	if err := checkNative(func() int32 {
		return int32(C.mln_map_pixels_for_lat_lngs(
			C.mln_map(ptr),
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
	if err := checkNative(func() int32 {
		return int32(C.mln_map_lat_lngs_for_pixels(
			C.mln_map(ptr),
			rawPointsPtr,
			C.size_t(len(points)),
			rawCoordinatesPtr,
		))
	}); err != nil {
		return nil, err
	}
	return goLatLngSlice(rawCoordinates), nil
}

// Close destroys this map. A successful close makes later calls no-ops. A
// failed close leaves the native handle live so callers can retry on the owner
// thread. Close discards this map's queued runtime events and its recorded
// loading failure without a flush and without a terminal event, and releases the
// callback state of every custom geometry source the map still holds.
func (m *MapHandle) Close() error {
	if m == nil || m.state == nil {
		return newBindingError(ErrInvalidArgument, "MapHandle is nil")
	}
	defer func() {
		if m.runtime != nil && m.runtime.state != nil {
			m.runtime.state.KeepAlive()
		}
	}()
	var bindingErr error
	if err := checkNative(func() int32 {
		status, err := m.state.CloseChecked(func(native nativeMap) int32 {
			return destroyMapHandle(native)
		})
		if err != nil {
			if errors.Is(err, handle.ErrLiveChildren) {
				bindingErr = newBindingError(ErrInvalidState, "MapHandle has live child handles")
				return int32(C.MLN_STATUS_OK)
			}
			bindingErr = newBindingError(ErrInvalidState, err.Error())
			return int32(C.MLN_STATUS_OK)
		}
		return status
	}); err != nil {
		return err
	}
	if bindingErr != nil {
		return bindingErr
	}
	if m.runtime != nil {
		m.runtime.unregisterMap(m)
	}
	m.runtimeChild.Release()
	return nil
}

// mapSizeByIDForTest calls the C size accessor with a raw map id, so a test can
// replay a released id or use one from another thread. The safe API expresses
// neither.
func mapSizeByIDForTest(id nativeMap) error {
	var width, height C.uint32_t
	var scale C.double
	return checkNative(func() int32 {
		return int32(C.mln_map_get_size(C.mln_map(id), &width, &height, &scale))
	})
}

// pumpRuntimeWithMapIDForTest passes a map id where a runtime id belongs, which
// the distinct Go types make unexpressible in the safe API.
func pumpRuntimeWithMapIDForTest(id nativeMap) error {
	return checkNative(func() int32 {
		return int32(C.mln_runtime_pump(C.mln_runtime(id), 0))
	})
}
