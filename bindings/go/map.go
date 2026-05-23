package maplibre

import (
	"errors"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/memory"
)

// MapMode selects the native map rendering mode.
type MapMode uint32

const (
	MapModeContinuous MapMode = MapMode(capi.MapModeContinuous)
	MapModeStatic     MapMode = MapMode(capi.MapModeStatic)
	MapModeTile       MapMode = MapMode(capi.MapModeTile)
)

// MapDebugOptions is a mask of native map debug overlays.
type MapDebugOptions uint32

const (
	MapDebugTileBorders MapDebugOptions = MapDebugOptions(capi.MapDebugTileBorders)
	MapDebugParseStatus MapDebugOptions = MapDebugOptions(capi.MapDebugParseStatus)
	MapDebugTimestamps  MapDebugOptions = MapDebugOptions(capi.MapDebugTimestamps)
	MapDebugCollision   MapDebugOptions = MapDebugOptions(capi.MapDebugCollision)
	MapDebugOverdraw    MapDebugOptions = MapDebugOptions(capi.MapDebugOverdraw)
	MapDebugStencilClip MapDebugOptions = MapDebugOptions(capi.MapDebugStencilClip)
	MapDebugDepthBuffer MapDebugOptions = MapDebugOptions(capi.MapDebugDepthBuffer)
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

func (options MapOptions) toCAPI() capi.MapOptions {
	return capi.MapOptions{
		Width:       options.Width,
		Height:      options.Height,
		ScaleFactor: options.ScaleFactor,
		MapMode:     uint32(options.Mode),
	}
}

// MapHandle owns map state for one RuntimeHandle.
type MapHandle struct {
	state   *handle.State[capi.Map]
	runtime *RuntimeHandle
}

func (m *MapHandle) ptr() (*capi.Map, error) {
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
	return checkNative(func() capi.Status { return capi.MapRequestRepaint(ptr) })
}

// RequestStillImage requests one still image for a static or tile map.
func (m *MapHandle) RequestStillImage() error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapRequestStillImage(ptr) })
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
	return checkNative(func() capi.Status { return capi.MapSetStyleURL(ptr, url) })
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
	return checkNative(func() capi.Status { return capi.MapSetStyleJSON(ptr, json) })
}

// SetDebugOptions applies MapLibre debug overlay mask bits to a map.
func (m *MapHandle) SetDebugOptions(options MapDebugOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetDebugOptions(ptr, uint32(options)) })
}

// DebugOptions returns the current MapLibre debug overlay mask bits.
func (m *MapHandle) DebugOptions() (MapDebugOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return 0, err
	}
	defer m.state.KeepAlive()
	var raw uint32
	if err := checkNative(func() capi.Status { return capi.MapGetDebugOptions(ptr, &raw) }); err != nil {
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
	return checkNative(func() capi.Status { return capi.MapSetRenderingStatsViewEnabled(ptr, enabled) })
}

// RenderingStatsViewEnabled reports whether MapLibre's rendering stats overlay
// view is enabled.
func (m *MapHandle) RenderingStatsViewEnabled() (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	var enabled bool
	if err := checkNative(func() capi.Status { return capi.MapGetRenderingStatsViewEnabled(ptr, &enabled) }); err != nil {
		return false, err
	}
	return enabled, nil
}

// IsFullyLoaded reports whether MapLibre currently considers the map fully
// loaded.
func (m *MapHandle) IsFullyLoaded() (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	var loaded bool
	if err := checkNative(func() capi.Status { return capi.MapIsFullyLoaded(ptr, &loaded) }); err != nil {
		return false, err
	}
	return loaded, nil
}

// DumpDebugLogs dumps map debug logs through MapLibre Native logging.
func (m *MapHandle) DumpDebugLogs() error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapDumpDebugLogs(ptr) })
}

// Camera returns the current camera snapshot.
func (m *MapHandle) Camera() (CameraOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.CameraOptions
	if err := checkNative(func() capi.Status { return capi.MapGetCamera(ptr, &raw) }); err != nil {
		return CameraOptions{}, err
	}
	return cameraOptionsFromCAPI(raw), nil
}

// JumpTo applies a camera jump command.
func (m *MapHandle) JumpTo(camera CameraOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapJumpTo(ptr, camera.toCAPI()) })
}

// EaseTo applies a camera ease transition command. Passing nil animation uses
// the native default animation options.
func (m *MapHandle) EaseTo(camera CameraOptions, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapEaseTo(ptr, camera.toCAPI(), animationOptionsToCAPI(animation)) })
}

// FlyTo applies a camera fly transition command. Passing nil animation uses the
// native default animation options.
func (m *MapHandle) FlyTo(camera CameraOptions, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapFlyTo(ptr, camera.toCAPI(), animationOptionsToCAPI(animation)) })
}

// MoveBy applies a screen-space pan command.
func (m *MapHandle) MoveBy(delta ScreenPoint) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapMoveBy(ptr, delta.toCAPI()) })
}

// MoveByAnimated applies an animated screen-space pan command. Passing nil
// animation uses the native default animation options.
func (m *MapHandle) MoveByAnimated(delta ScreenPoint, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapMoveByAnimated(ptr, delta.toCAPI(), animationOptionsToCAPI(animation))
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
	var rawAnchor *capi.ScreenPoint
	if anchor != nil {
		converted := anchor.toCAPI()
		rawAnchor = &converted
	}
	return checkNative(func() capi.Status { return capi.MapScaleBy(ptr, scale, rawAnchor) })
}

// ScaleByAnimated applies an animated screen-space zoom command. Passing nil
// anchor or animation uses the native default for that option.
func (m *MapHandle) ScaleByAnimated(scale float64, anchor *ScreenPoint, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	var rawAnchor *capi.ScreenPoint
	if anchor != nil {
		converted := anchor.toCAPI()
		rawAnchor = &converted
	}
	return checkNative(func() capi.Status {
		return capi.MapScaleByAnimated(ptr, scale, rawAnchor, animationOptionsToCAPI(animation))
	})
}

// RotateBy applies a screen-space rotate command.
func (m *MapHandle) RotateBy(first ScreenPoint, second ScreenPoint) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapRotateBy(ptr, first.toCAPI(), second.toCAPI()) })
}

// RotateByAnimated applies an animated screen-space rotate command. Passing nil
// animation uses the native default animation options.
func (m *MapHandle) RotateByAnimated(first ScreenPoint, second ScreenPoint, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapRotateByAnimated(ptr, first.toCAPI(), second.toCAPI(), animationOptionsToCAPI(animation))
	})
}

// PitchBy applies a pitch delta command.
func (m *MapHandle) PitchBy(pitch float64) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapPitchBy(ptr, pitch) })
}

// PitchByAnimated applies an animated pitch delta command. Passing nil
// animation uses the native default animation options.
func (m *MapHandle) PitchByAnimated(pitch float64, animation *AnimationOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapPitchByAnimated(ptr, pitch, animationOptionsToCAPI(animation)) })
}

// CancelTransitions cancels active camera transitions.
func (m *MapHandle) CancelTransitions() error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapCancelTransitions(ptr) })
}

// ViewportOptions returns live map viewport and render-transform controls.
func (m *MapHandle) ViewportOptions() (ViewportOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return ViewportOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.ViewportOptions
	if err := checkNative(func() capi.Status { return capi.MapGetViewportOptions(ptr, &raw) }); err != nil {
		return ViewportOptions{}, err
	}
	return viewportOptionsFromCAPI(raw), nil
}

// SetViewportOptions applies selected live map viewport and render-transform
// controls.
func (m *MapHandle) SetViewportOptions(options ViewportOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetViewportOptions(ptr, options.toCAPI()) })
}

// TileOptions returns tile prefetch and LOD tuning controls.
func (m *MapHandle) TileOptions() (TileOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return TileOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.TileOptions
	if err := checkNative(func() capi.Status { return capi.MapGetTileOptions(ptr, &raw) }); err != nil {
		return TileOptions{}, err
	}
	return tileOptionsFromCAPI(raw), nil
}

// SetTileOptions applies selected tile prefetch and LOD tuning controls.
func (m *MapHandle) SetTileOptions(options TileOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetTileOptions(ptr, options.toCAPI()) })
}

// ProjectionMode returns current axonometric rendering options.
func (m *MapHandle) ProjectionMode() (ProjectionModeOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return ProjectionModeOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.ProjectionModeOptions
	if err := checkNative(func() capi.Status { return capi.MapGetProjectionMode(ptr, &raw) }); err != nil {
		return ProjectionModeOptions{}, err
	}
	return projectionModeOptionsFromCAPI(raw), nil
}

// SetProjectionMode applies axonometric rendering option fields.
func (m *MapHandle) SetProjectionMode(options ProjectionModeOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetProjectionMode(ptr, options.toCAPI()) })
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
	return checkNative(func() capi.Status {
		return m.state.Close(capi.MapDestroy)
	})
}
