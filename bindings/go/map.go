package maplibre

import (
	"errors"
	"sync"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
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

	customGeometryMu      sync.Mutex
	customGeometrySources map[string]*callback.CustomGeometrySourceState
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
	if err := checkNative(func() capi.Status { return capi.MapSetStyleJSON(ptr, json) }); err != nil {
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

// CameraForLatLngBounds computes a camera that fits geographic bounds. Passing
// nil fitOptions uses native default fitting options.
func (m *MapHandle) CameraForLatLngBounds(bounds LatLngBounds, fitOptions *CameraFitOptions) (CameraOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.CameraOptions
	if err := checkNative(func() capi.Status {
		return capi.MapCameraForLatLngBounds(ptr, bounds.toCAPI(), cameraFitOptionsToCAPI(fitOptions), &raw)
	}); err != nil {
		return CameraOptions{}, err
	}
	return cameraOptionsFromCAPI(raw), nil
}

// CameraForLatLngs computes a camera that fits geographic coordinates. Passing
// nil fitOptions uses native default fitting options.
func (m *MapHandle) CameraForLatLngs(coordinates []LatLng, fitOptions *CameraFitOptions) (CameraOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.CameraOptions
	if err := checkNative(func() capi.Status {
		return capi.MapCameraForLatLngs(ptr, latLngSliceToCAPI(coordinates), cameraFitOptionsToCAPI(fitOptions), &raw)
	}); err != nil {
		return CameraOptions{}, err
	}
	return cameraOptionsFromCAPI(raw), nil
}

// CameraForGeometry computes a camera that fits a geometry. Passing nil
// fitOptions uses native default fitting options.
func (m *MapHandle) CameraForGeometry(geometry Geometry, fitOptions *CameraFitOptions) (CameraOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.CameraOptions
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.MapCameraForGeometry(ptr, geometry.toCAPI(), cameraFitOptionsToCAPI(fitOptions), &raw)
		return status
	})
	if materialErr != nil {
		return CameraOptions{}, newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	if err != nil {
		return CameraOptions{}, err
	}
	return cameraOptionsFromCAPI(raw), nil
}

// LatLngBoundsForCamera computes wrapped geographic bounds for a camera in the
// current viewport.
func (m *MapHandle) LatLngBoundsForCamera(camera CameraOptions) (LatLngBounds, error) {
	ptr, err := m.ptr()
	if err != nil {
		return LatLngBounds{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.LatLngBounds
	if err := checkNative(func() capi.Status { return capi.MapLatLngBoundsForCamera(ptr, camera.toCAPI(), &raw) }); err != nil {
		return LatLngBounds{}, err
	}
	return latLngBoundsFromCAPI(raw), nil
}

// LatLngBoundsForCameraUnwrapped computes unwrapped geographic bounds for a
// camera in the current viewport.
func (m *MapHandle) LatLngBoundsForCameraUnwrapped(camera CameraOptions) (LatLngBounds, error) {
	ptr, err := m.ptr()
	if err != nil {
		return LatLngBounds{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.LatLngBounds
	if err := checkNative(func() capi.Status { return capi.MapLatLngBoundsForCameraUnwrapped(ptr, camera.toCAPI(), &raw) }); err != nil {
		return LatLngBounds{}, err
	}
	return latLngBoundsFromCAPI(raw), nil
}

// Bounds returns map camera constraint options.
func (m *MapHandle) Bounds() (BoundOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return BoundOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.BoundOptions
	if err := checkNative(func() capi.Status { return capi.MapGetBounds(ptr, &raw) }); err != nil {
		return BoundOptions{}, err
	}
	return boundOptionsFromCAPI(raw), nil
}

// SetBounds applies selected map camera constraint options.
func (m *MapHandle) SetBounds(options BoundOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetBounds(ptr, options.toCAPI()) })
}

// FreeCameraOptions returns current free camera position and orientation.
func (m *MapHandle) FreeCameraOptions() (FreeCameraOptions, error) {
	ptr, err := m.ptr()
	if err != nil {
		return FreeCameraOptions{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.FreeCameraOptions
	if err := checkNative(func() capi.Status { return capi.MapGetFreeCameraOptions(ptr, &raw) }); err != nil {
		return FreeCameraOptions{}, err
	}
	return freeCameraOptionsFromCAPI(raw), nil
}

// SetFreeCameraOptions applies selected free camera position and orientation
// fields.
func (m *MapHandle) SetFreeCameraOptions(options FreeCameraOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetFreeCameraOptions(ptr, options.toCAPI()) })
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

// PixelForLatLng converts a geographic coordinate to a logical screen point for
// the current map.
func (m *MapHandle) PixelForLatLng(coordinate LatLng) (ScreenPoint, error) {
	ptr, err := m.ptr()
	if err != nil {
		return ScreenPoint{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.ScreenPoint
	if err := checkNative(func() capi.Status { return capi.MapPixelForLatLng(ptr, coordinate.toCAPI(), &raw) }); err != nil {
		return ScreenPoint{}, err
	}
	return screenPointFromCAPI(raw), nil
}

// LatLngForPixel converts a logical screen point to a geographic coordinate for
// the current map.
func (m *MapHandle) LatLngForPixel(point ScreenPoint) (LatLng, error) {
	ptr, err := m.ptr()
	if err != nil {
		return LatLng{}, err
	}
	defer m.state.KeepAlive()
	var raw capi.LatLng
	if err := checkNative(func() capi.Status { return capi.MapLatLngForPixel(ptr, point.toCAPI(), &raw) }); err != nil {
		return LatLng{}, err
	}
	return latLngFromCAPI(raw), nil
}

// PixelsForLatLngs converts geographic coordinates to logical screen points for
// the current map.
func (m *MapHandle) PixelsForLatLngs(coordinates []LatLng) ([]ScreenPoint, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	var raw []capi.ScreenPoint
	if err := checkNative(func() capi.Status { return capi.MapPixelsForLatLngs(ptr, latLngSliceToCAPI(coordinates), &raw) }); err != nil {
		return nil, err
	}
	return screenPointSliceFromCAPI(raw), nil
}

// LatLngsForPixels converts logical screen points to geographic coordinates for
// the current map.
func (m *MapHandle) LatLngsForPixels(points []ScreenPoint) ([]LatLng, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	var raw []capi.LatLng
	if err := checkNative(func() capi.Status { return capi.MapLatLngsForPixels(ptr, screenPointSliceToCAPI(points), &raw) }); err != nil {
		return nil, err
	}
	return latLngSliceFromCAPI(raw), nil
}

func (m *MapHandle) releaseCustomGeometrySource(sourceID string) {
	m.customGeometryMu.Lock()
	state := m.customGeometrySources[sourceID]
	delete(m.customGeometrySources, sourceID)
	m.customGeometryMu.Unlock()
	state.Release()
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
	if err := checkNative(func() capi.Status {
		return m.state.Close(capi.MapDestroy)
	}); err != nil {
		return err
	}
	m.releaseCustomGeometrySources()
	return nil
}
