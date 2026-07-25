package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

// CameraOptions configures map camera snapshots and commands.
type CameraOptions struct {
	Center         *LatLng
	CenterAltitude *float64
	Padding        *EdgeInsets
	Anchor         *ScreenPoint
	Zoom           *float64
	Bearing        *float64
	Pitch          *float64
	Roll           *float64
	FieldOfView    *float64
}

// WithCenter returns a copy that sets the center coordinate.
func (options CameraOptions) WithCenter(center LatLng) CameraOptions {
	options.Center = new(LatLng)
	*options.Center = center
	return options
}

// WithZoom returns a copy that sets the zoom field.
func (options CameraOptions) WithZoom(zoom float64) CameraOptions {
	options.Zoom = new(float64)
	*options.Zoom = zoom
	return options
}

// WithBearing returns a copy that sets the bearing field.
func (options CameraOptions) WithBearing(bearing float64) CameraOptions {
	options.Bearing = new(float64)
	*options.Bearing = bearing
	return options
}

// WithPitch returns a copy that sets the pitch field.
func (options CameraOptions) WithPitch(pitch float64) CameraOptions {
	options.Pitch = new(float64)
	*options.Pitch = pitch
	return options
}

func cCameraOptions(options CameraOptions) C.mln_camera_options {
	raw := C.mln_camera_options_default()
	if options.Center != nil {
		raw.fields |= C.MLN_CAMERA_OPTION_CENTER
		raw.latitude = C.double(options.Center.Latitude)
		raw.longitude = C.double(options.Center.Longitude)
	}
	if options.CenterAltitude != nil {
		raw.fields |= C.MLN_CAMERA_OPTION_CENTER_ALTITUDE
		raw.center_altitude = C.double(*options.CenterAltitude)
	}
	if options.Padding != nil {
		raw.fields |= C.MLN_CAMERA_OPTION_PADDING
		raw.padding = cEdgeInsets(*options.Padding)
	}
	if options.Anchor != nil {
		raw.fields |= C.MLN_CAMERA_OPTION_ANCHOR
		raw.anchor = cScreenPoint(*options.Anchor)
	}
	if options.Zoom != nil {
		raw.fields |= C.MLN_CAMERA_OPTION_ZOOM
		raw.zoom = C.double(*options.Zoom)
	}
	if options.Bearing != nil {
		raw.fields |= C.MLN_CAMERA_OPTION_BEARING
		raw.bearing = C.double(*options.Bearing)
	}
	if options.Pitch != nil {
		raw.fields |= C.MLN_CAMERA_OPTION_PITCH
		raw.pitch = C.double(*options.Pitch)
	}
	if options.Roll != nil {
		raw.fields |= C.MLN_CAMERA_OPTION_ROLL
		raw.roll = C.double(*options.Roll)
	}
	if options.FieldOfView != nil {
		raw.fields |= C.MLN_CAMERA_OPTION_FOV
		raw.field_of_view = C.double(*options.FieldOfView)
	}
	return raw
}

func goCameraOptions(raw C.mln_camera_options) CameraOptions {
	var options CameraOptions
	if raw.fields&C.MLN_CAMERA_OPTION_CENTER != 0 {
		value := LatLng{Latitude: float64(raw.latitude), Longitude: float64(raw.longitude)}
		options.Center = &value
	}
	if raw.fields&C.MLN_CAMERA_OPTION_CENTER_ALTITUDE != 0 {
		value := float64(raw.center_altitude)
		options.CenterAltitude = &value
	}
	if raw.fields&C.MLN_CAMERA_OPTION_PADDING != 0 {
		value := goEdgeInsets(raw.padding)
		options.Padding = &value
	}
	if raw.fields&C.MLN_CAMERA_OPTION_ANCHOR != 0 {
		value := goScreenPoint(raw.anchor)
		options.Anchor = &value
	}
	if raw.fields&C.MLN_CAMERA_OPTION_ZOOM != 0 {
		value := float64(raw.zoom)
		options.Zoom = &value
	}
	if raw.fields&C.MLN_CAMERA_OPTION_BEARING != 0 {
		value := float64(raw.bearing)
		options.Bearing = &value
	}
	if raw.fields&C.MLN_CAMERA_OPTION_PITCH != 0 {
		value := float64(raw.pitch)
		options.Pitch = &value
	}
	if raw.fields&C.MLN_CAMERA_OPTION_ROLL != 0 {
		value := float64(raw.roll)
		options.Roll = &value
	}
	if raw.fields&C.MLN_CAMERA_OPTION_FOV != 0 {
		value := float64(raw.field_of_view)
		options.FieldOfView = &value
	}
	return options
}

// UnitBezier contains cubic easing curve control points.
type UnitBezier struct {
	X1 float64
	Y1 float64
	X2 float64
	Y2 float64
}

// AnimationOptions configures camera transition animation behavior.
type AnimationOptions struct {
	DurationMS *float64
	Velocity   *float64
	MinZoom    *float64
	Easing     *UnitBezier
}

// WithDurationMS returns a copy that sets the duration in milliseconds.
func (options AnimationOptions) WithDurationMS(durationMS float64) AnimationOptions {
	options.DurationMS = new(float64)
	*options.DurationMS = durationMS
	return options
}

// WithVelocity returns a copy that sets the fly-to velocity in screenfuls per
// second.
func (options AnimationOptions) WithVelocity(velocity float64) AnimationOptions {
	options.Velocity = new(float64)
	*options.Velocity = velocity
	return options
}

// WithMinZoom returns a copy that sets the peak zoom for fly-to transitions.
func (options AnimationOptions) WithMinZoom(minZoom float64) AnimationOptions {
	options.MinZoom = new(float64)
	*options.MinZoom = minZoom
	return options
}

// WithEasing returns a copy that sets the cubic easing curve.
func (options AnimationOptions) WithEasing(easing UnitBezier) AnimationOptions {
	options.Easing = new(UnitBezier)
	*options.Easing = easing
	return options
}

func cAnimationOptions(options AnimationOptions) C.mln_animation_options {
	raw := C.mln_animation_options_default()
	if options.DurationMS != nil {
		raw.fields |= C.MLN_ANIMATION_OPTION_DURATION
		raw.duration_ms = C.double(*options.DurationMS)
	}
	if options.Velocity != nil {
		raw.fields |= C.MLN_ANIMATION_OPTION_VELOCITY
		raw.velocity = C.double(*options.Velocity)
	}
	if options.MinZoom != nil {
		raw.fields |= C.MLN_ANIMATION_OPTION_MIN_ZOOM
		raw.min_zoom = C.double(*options.MinZoom)
	}
	if options.Easing != nil {
		raw.fields |= C.MLN_ANIMATION_OPTION_EASING
		raw.easing = C.mln_unit_bezier{
			x1: C.double(options.Easing.X1),
			y1: C.double(options.Easing.Y1),
			x2: C.double(options.Easing.X2),
			y2: C.double(options.Easing.Y2),
		}
	}
	return raw
}

func cAnimationOptionsPointer(options *AnimationOptions) (C.mln_animation_options, *C.mln_animation_options) {
	if options == nil {
		return C.mln_animation_options{}, nil
	}
	raw := cAnimationOptions(*options)
	return raw, &raw
}

// CameraFitOptions configures camera fitting queries.
type CameraFitOptions struct {
	Padding *EdgeInsets
	Bearing *float64
	Pitch   *float64
}

// WithPadding returns a copy that sets fit padding.
func (options CameraFitOptions) WithPadding(padding EdgeInsets) CameraFitOptions {
	options.Padding = new(EdgeInsets)
	*options.Padding = padding
	return options
}

// WithBearing returns a copy that sets fit bearing.
func (options CameraFitOptions) WithBearing(bearing float64) CameraFitOptions {
	options.Bearing = new(float64)
	*options.Bearing = bearing
	return options
}

// WithPitch returns a copy that sets fit pitch.
func (options CameraFitOptions) WithPitch(pitch float64) CameraFitOptions {
	options.Pitch = new(float64)
	*options.Pitch = pitch
	return options
}

func cCameraFitOptions(options CameraFitOptions) C.mln_camera_fit_options {
	raw := C.mln_camera_fit_options_default()
	if options.Padding != nil {
		raw.fields |= C.MLN_CAMERA_FIT_OPTION_PADDING
		raw.padding = cEdgeInsets(*options.Padding)
	}
	if options.Bearing != nil {
		raw.fields |= C.MLN_CAMERA_FIT_OPTION_BEARING
		raw.bearing = C.double(*options.Bearing)
	}
	if options.Pitch != nil {
		raw.fields |= C.MLN_CAMERA_FIT_OPTION_PITCH
		raw.pitch = C.double(*options.Pitch)
	}
	return raw
}

func cCameraFitOptionsPointer(options *CameraFitOptions) (C.mln_camera_fit_options, *C.mln_camera_fit_options) {
	if options == nil {
		return C.mln_camera_fit_options{}, nil
	}
	raw := cCameraFitOptions(*options)
	return raw, &raw
}

// BoundOptions configures map camera constraints.
type BoundOptions struct {
	Bounds   *LatLngBounds
	MinZoom  *float64
	MaxZoom  *float64
	MinPitch *float64
	MaxPitch *float64
}

// WithBounds returns a copy that sets geographic camera constraints.
func (options BoundOptions) WithBounds(bounds LatLngBounds) BoundOptions {
	options.Bounds = new(LatLngBounds)
	*options.Bounds = bounds
	return options
}

// WithMinZoom returns a copy that sets minimum zoom.
func (options BoundOptions) WithMinZoom(minZoom float64) BoundOptions {
	options.MinZoom = new(float64)
	*options.MinZoom = minZoom
	return options
}

// WithMaxZoom returns a copy that sets maximum zoom.
func (options BoundOptions) WithMaxZoom(maxZoom float64) BoundOptions {
	options.MaxZoom = new(float64)
	*options.MaxZoom = maxZoom
	return options
}

// WithMinPitch returns a copy that sets minimum pitch.
func (options BoundOptions) WithMinPitch(minPitch float64) BoundOptions {
	options.MinPitch = new(float64)
	*options.MinPitch = minPitch
	return options
}

// WithMaxPitch returns a copy that sets maximum pitch.
func (options BoundOptions) WithMaxPitch(maxPitch float64) BoundOptions {
	options.MaxPitch = new(float64)
	*options.MaxPitch = maxPitch
	return options
}

func cBoundOptions(options BoundOptions) C.mln_bound_options {
	raw := C.mln_bound_options_default()
	if options.Bounds != nil {
		raw.fields |= C.MLN_BOUND_OPTION_BOUNDS
		raw.bounds = cLatLngBounds(*options.Bounds)
	}
	if options.MinZoom != nil {
		raw.fields |= C.MLN_BOUND_OPTION_MIN_ZOOM
		raw.min_zoom = C.double(*options.MinZoom)
	}
	if options.MaxZoom != nil {
		raw.fields |= C.MLN_BOUND_OPTION_MAX_ZOOM
		raw.max_zoom = C.double(*options.MaxZoom)
	}
	if options.MinPitch != nil {
		raw.fields |= C.MLN_BOUND_OPTION_MIN_PITCH
		raw.min_pitch = C.double(*options.MinPitch)
	}
	if options.MaxPitch != nil {
		raw.fields |= C.MLN_BOUND_OPTION_MAX_PITCH
		raw.max_pitch = C.double(*options.MaxPitch)
	}
	return raw
}

func goBoundOptions(raw C.mln_bound_options) BoundOptions {
	var options BoundOptions
	if raw.fields&C.MLN_BOUND_OPTION_BOUNDS != 0 {
		value := goLatLngBounds(raw.bounds)
		options.Bounds = &value
	}
	if raw.fields&C.MLN_BOUND_OPTION_MIN_ZOOM != 0 {
		value := float64(raw.min_zoom)
		options.MinZoom = &value
	}
	if raw.fields&C.MLN_BOUND_OPTION_MAX_ZOOM != 0 {
		value := float64(raw.max_zoom)
		options.MaxZoom = &value
	}
	if raw.fields&C.MLN_BOUND_OPTION_MIN_PITCH != 0 {
		value := float64(raw.min_pitch)
		options.MinPitch = &value
	}
	if raw.fields&C.MLN_BOUND_OPTION_MAX_PITCH != 0 {
		value := float64(raw.max_pitch)
		options.MaxPitch = &value
	}
	return options
}

// FreeCameraOptions configures camera position and orientation directly.
type FreeCameraOptions struct {
	Position    *Vec3
	Orientation *Quaternion
}

// WithPosition returns a copy that sets free camera position.
func (options FreeCameraOptions) WithPosition(position Vec3) FreeCameraOptions {
	options.Position = new(Vec3)
	*options.Position = position
	return options
}

// WithOrientation returns a copy that sets free camera orientation.
func (options FreeCameraOptions) WithOrientation(orientation Quaternion) FreeCameraOptions {
	options.Orientation = new(Quaternion)
	*options.Orientation = orientation
	return options
}

func cFreeCameraOptions(options FreeCameraOptions) C.mln_free_camera_options {
	raw := C.mln_free_camera_options_default()
	if options.Position != nil {
		raw.fields |= C.MLN_FREE_CAMERA_OPTION_POSITION
		raw.position = cVec3(*options.Position)
	}
	if options.Orientation != nil {
		raw.fields |= C.MLN_FREE_CAMERA_OPTION_ORIENTATION
		raw.orientation = cQuaternion(*options.Orientation)
	}
	return raw
}

func goFreeCameraOptions(raw C.mln_free_camera_options) FreeCameraOptions {
	var options FreeCameraOptions
	if raw.fields&C.MLN_FREE_CAMERA_OPTION_POSITION != 0 {
		value := goVec3(raw.position)
		options.Position = &value
	}
	if raw.fields&C.MLN_FREE_CAMERA_OPTION_ORIENTATION != 0 {
		value := goQuaternion(raw.orientation)
		options.Orientation = &value
	}
	return options
}

// NorthOrientation controls which screen edge points north.
type NorthOrientation uint32

const (
	NorthOrientationUp    NorthOrientation = NorthOrientation(C.MLN_NORTH_ORIENTATION_UP)
	NorthOrientationRight NorthOrientation = NorthOrientation(C.MLN_NORTH_ORIENTATION_RIGHT)
	NorthOrientationDown  NorthOrientation = NorthOrientation(C.MLN_NORTH_ORIENTATION_DOWN)
	NorthOrientationLeft  NorthOrientation = NorthOrientation(C.MLN_NORTH_ORIENTATION_LEFT)
)

// ConstrainMode controls map panning constraints.
type ConstrainMode uint32

const (
	ConstrainModeNone           ConstrainMode = ConstrainMode(C.MLN_CONSTRAIN_MODE_NONE)
	ConstrainModeHeightOnly     ConstrainMode = ConstrainMode(C.MLN_CONSTRAIN_MODE_HEIGHT_ONLY)
	ConstrainModeWidthAndHeight ConstrainMode = ConstrainMode(C.MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT)
	ConstrainModeScreen         ConstrainMode = ConstrainMode(C.MLN_CONSTRAIN_MODE_SCREEN)
)

// ViewportMode controls viewport coordinate orientation.
type ViewportMode uint32

const (
	ViewportModeDefault  ViewportMode = ViewportMode(C.MLN_VIEWPORT_MODE_DEFAULT)
	ViewportModeFlippedY ViewportMode = ViewportMode(C.MLN_VIEWPORT_MODE_FLIPPED_Y)
)

// TileLODMode selects the native tile LOD algorithm.
type TileLODMode uint32

const (
	TileLODModeDefault  TileLODMode = TileLODMode(C.MLN_TILE_LOD_MODE_DEFAULT)
	TileLODModeDistance TileLODMode = TileLODMode(C.MLN_TILE_LOD_MODE_DISTANCE)
)

// ViewportOptions configures live viewport and render-transform controls.
type ViewportOptions struct {
	NorthOrientation *NorthOrientation
	ConstrainMode    *ConstrainMode
	ViewportMode     *ViewportMode
	FrustumOffset    *EdgeInsets
}

// WithNorthOrientation returns a copy that sets the north orientation field.
func (options ViewportOptions) WithNorthOrientation(value NorthOrientation) ViewportOptions {
	options.NorthOrientation = new(NorthOrientation)
	*options.NorthOrientation = value
	return options
}

// WithConstrainMode returns a copy that sets the constrain mode field.
func (options ViewportOptions) WithConstrainMode(value ConstrainMode) ViewportOptions {
	options.ConstrainMode = new(ConstrainMode)
	*options.ConstrainMode = value
	return options
}

// WithViewportMode returns a copy that sets the viewport mode field.
func (options ViewportOptions) WithViewportMode(value ViewportMode) ViewportOptions {
	options.ViewportMode = new(ViewportMode)
	*options.ViewportMode = value
	return options
}

// WithFrustumOffset returns a copy that sets the frustum offset field.
func (options ViewportOptions) WithFrustumOffset(value EdgeInsets) ViewportOptions {
	options.FrustumOffset = new(EdgeInsets)
	*options.FrustumOffset = value
	return options
}

func cViewportOptions(options ViewportOptions) C.mln_map_viewport_options {
	raw := C.mln_map_viewport_options_default()
	if options.NorthOrientation != nil {
		raw.fields |= C.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
		raw.north_orientation = C.uint32_t(*options.NorthOrientation)
	}
	if options.ConstrainMode != nil {
		raw.fields |= C.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
		raw.constrain_mode = C.uint32_t(*options.ConstrainMode)
	}
	if options.ViewportMode != nil {
		raw.fields |= C.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
		raw.viewport_mode = C.uint32_t(*options.ViewportMode)
	}
	if options.FrustumOffset != nil {
		raw.fields |= C.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET
		raw.frustum_offset = cEdgeInsets(*options.FrustumOffset)
	}
	return raw
}

func goViewportOptions(raw C.mln_map_viewport_options) ViewportOptions {
	var options ViewportOptions
	if raw.fields&C.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION != 0 {
		value := NorthOrientation(raw.north_orientation)
		options.NorthOrientation = &value
	}
	if raw.fields&C.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE != 0 {
		value := ConstrainMode(raw.constrain_mode)
		options.ConstrainMode = &value
	}
	if raw.fields&C.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE != 0 {
		value := ViewportMode(raw.viewport_mode)
		options.ViewportMode = &value
	}
	if raw.fields&C.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET != 0 {
		value := goEdgeInsets(raw.frustum_offset)
		options.FrustumOffset = &value
	}
	return options
}

// TileOptions configures tile prefetch and LOD tuning controls.
type TileOptions struct {
	PrefetchZoomDelta *uint32
	LODMinRadius      *float64
	LODScale          *float64
	LODPitchThreshold *float64
	LODZoomShift      *float64
	LODMode           *TileLODMode
}

// WithPrefetchZoomDelta returns a copy that sets prefetch zoom delta.
func (options TileOptions) WithPrefetchZoomDelta(value uint32) TileOptions {
	options.PrefetchZoomDelta = new(uint32)
	*options.PrefetchZoomDelta = value
	return options
}

// WithLODMode returns a copy that sets tile LOD mode.
func (options TileOptions) WithLODMode(value TileLODMode) TileOptions {
	options.LODMode = new(TileLODMode)
	*options.LODMode = value
	return options
}

func cTileOptions(options TileOptions) C.mln_map_tile_options {
	raw := C.mln_map_tile_options_default()
	if options.PrefetchZoomDelta != nil {
		raw.fields |= C.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
		raw.prefetch_zoom_delta = C.uint32_t(*options.PrefetchZoomDelta)
	}
	if options.LODMinRadius != nil {
		raw.fields |= C.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS
		raw.lod_min_radius = C.double(*options.LODMinRadius)
	}
	if options.LODScale != nil {
		raw.fields |= C.MLN_MAP_TILE_OPTION_LOD_SCALE
		raw.lod_scale = C.double(*options.LODScale)
	}
	if options.LODPitchThreshold != nil {
		raw.fields |= C.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
		raw.lod_pitch_threshold = C.double(*options.LODPitchThreshold)
	}
	if options.LODZoomShift != nil {
		raw.fields |= C.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT
		raw.lod_zoom_shift = C.double(*options.LODZoomShift)
	}
	if options.LODMode != nil {
		raw.fields |= C.MLN_MAP_TILE_OPTION_LOD_MODE
		raw.lod_mode = C.uint32_t(*options.LODMode)
	}
	return raw
}

func goTileOptions(raw C.mln_map_tile_options) TileOptions {
	var options TileOptions
	if raw.fields&C.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA != 0 {
		value := uint32(raw.prefetch_zoom_delta)
		options.PrefetchZoomDelta = &value
	}
	if raw.fields&C.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS != 0 {
		value := float64(raw.lod_min_radius)
		options.LODMinRadius = &value
	}
	if raw.fields&C.MLN_MAP_TILE_OPTION_LOD_SCALE != 0 {
		value := float64(raw.lod_scale)
		options.LODScale = &value
	}
	if raw.fields&C.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD != 0 {
		value := float64(raw.lod_pitch_threshold)
		options.LODPitchThreshold = &value
	}
	if raw.fields&C.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT != 0 {
		value := float64(raw.lod_zoom_shift)
		options.LODZoomShift = &value
	}
	if raw.fields&C.MLN_MAP_TILE_OPTION_LOD_MODE != 0 {
		value := TileLODMode(raw.lod_mode)
		options.LODMode = &value
	}
	return options
}

// ProjectionModeOptions configures axonometric render transform fields.
type ProjectionModeOptions struct {
	Axonometric *bool
	XSkew       *float64
	YSkew       *float64
}

// WithAxonometric returns a copy that sets the axonometric field.
func (options ProjectionModeOptions) WithAxonometric(value bool) ProjectionModeOptions {
	options.Axonometric = new(bool)
	*options.Axonometric = value
	return options
}

// WithSkew returns a copy that sets x and y skew fields.
func (options ProjectionModeOptions) WithSkew(x, y float64) ProjectionModeOptions {
	options.XSkew = new(float64)
	options.YSkew = new(float64)
	*options.XSkew = x
	*options.YSkew = y
	return options
}

func cProjectionModeOptions(options ProjectionModeOptions) C.mln_projection_mode {
	raw := C.mln_projection_mode_default()
	if options.Axonometric != nil {
		raw.fields |= C.MLN_PROJECTION_MODE_AXONOMETRIC
		raw.axonometric = C.bool(*options.Axonometric)
	}
	if options.XSkew != nil {
		raw.fields |= C.MLN_PROJECTION_MODE_X_SKEW
		raw.x_skew = C.double(*options.XSkew)
	}
	if options.YSkew != nil {
		raw.fields |= C.MLN_PROJECTION_MODE_Y_SKEW
		raw.y_skew = C.double(*options.YSkew)
	}
	return raw
}

func goProjectionModeOptions(raw C.mln_projection_mode) ProjectionModeOptions {
	var options ProjectionModeOptions
	if raw.fields&C.MLN_PROJECTION_MODE_AXONOMETRIC != 0 {
		value := bool(raw.axonometric)
		options.Axonometric = &value
	}
	if raw.fields&C.MLN_PROJECTION_MODE_X_SKEW != 0 {
		value := float64(raw.x_skew)
		options.XSkew = &value
	}
	if raw.fields&C.MLN_PROJECTION_MODE_Y_SKEW != 0 {
		value := float64(raw.y_skew)
		options.YSkew = &value
	}
	return options
}
