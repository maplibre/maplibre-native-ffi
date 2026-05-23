package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"

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

func (options CameraOptions) toCAPI() capi.CameraOptions {
	var raw capi.CameraOptions
	if options.Center != nil {
		raw.Fields |= capi.CameraOptionCenter
		raw.Center = options.Center.toCAPI()
	}
	if options.CenterAltitude != nil {
		raw.Fields |= capi.CameraOptionCenterAltitude
		raw.CenterAltitude = *options.CenterAltitude
	}
	if options.Padding != nil {
		raw.Fields |= capi.CameraOptionPadding
		raw.Padding = options.Padding.toCAPI()
	}
	if options.Anchor != nil {
		raw.Fields |= capi.CameraOptionAnchor
		raw.Anchor = options.Anchor.toCAPI()
	}
	if options.Zoom != nil {
		raw.Fields |= capi.CameraOptionZoom
		raw.Zoom = *options.Zoom
	}
	if options.Bearing != nil {
		raw.Fields |= capi.CameraOptionBearing
		raw.Bearing = *options.Bearing
	}
	if options.Pitch != nil {
		raw.Fields |= capi.CameraOptionPitch
		raw.Pitch = *options.Pitch
	}
	if options.Roll != nil {
		raw.Fields |= capi.CameraOptionRoll
		raw.Roll = *options.Roll
	}
	if options.FieldOfView != nil {
		raw.Fields |= capi.CameraOptionFOV
		raw.FieldOfView = *options.FieldOfView
	}
	return raw
}

func cameraOptionsFromCAPI(raw capi.CameraOptions) CameraOptions {
	var options CameraOptions
	if raw.Fields&capi.CameraOptionCenter != 0 {
		value := latLngFromCAPI(raw.Center)
		options.Center = &value
	}
	if raw.Fields&capi.CameraOptionCenterAltitude != 0 {
		value := raw.CenterAltitude
		options.CenterAltitude = &value
	}
	if raw.Fields&capi.CameraOptionPadding != 0 {
		value := edgeInsetsFromCAPI(raw.Padding)
		options.Padding = &value
	}
	if raw.Fields&capi.CameraOptionAnchor != 0 {
		value := screenPointFromCAPI(raw.Anchor)
		options.Anchor = &value
	}
	if raw.Fields&capi.CameraOptionZoom != 0 {
		value := raw.Zoom
		options.Zoom = &value
	}
	if raw.Fields&capi.CameraOptionBearing != 0 {
		value := raw.Bearing
		options.Bearing = &value
	}
	if raw.Fields&capi.CameraOptionPitch != 0 {
		value := raw.Pitch
		options.Pitch = &value
	}
	if raw.Fields&capi.CameraOptionRoll != 0 {
		value := raw.Roll
		options.Roll = &value
	}
	if raw.Fields&capi.CameraOptionFOV != 0 {
		value := raw.FieldOfView
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

// WithDuration returns a copy that sets the duration in milliseconds.
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

func (options AnimationOptions) toCAPI() capi.AnimationOptions {
	var raw capi.AnimationOptions
	if options.DurationMS != nil {
		raw.Fields |= capi.AnimationOptionDuration
		raw.DurationMS = *options.DurationMS
	}
	if options.Velocity != nil {
		raw.Fields |= capi.AnimationOptionVelocity
		raw.Velocity = *options.Velocity
	}
	if options.MinZoom != nil {
		raw.Fields |= capi.AnimationOptionMinZoom
		raw.MinZoom = *options.MinZoom
	}
	if options.Easing != nil {
		raw.Fields |= capi.AnimationOptionEasing
		raw.Easing = capi.UnitBezier{X1: options.Easing.X1, Y1: options.Easing.Y1, X2: options.Easing.X2, Y2: options.Easing.Y2}
	}
	return raw
}

func animationOptionsToCAPI(options *AnimationOptions) *capi.AnimationOptions {
	if options == nil {
		return nil
	}
	raw := options.toCAPI()
	return &raw
}

// NorthOrientation controls which screen edge points north.
type NorthOrientation uint32

const (
	NorthOrientationUp    NorthOrientation = NorthOrientation(capi.NorthOrientationUp)
	NorthOrientationRight NorthOrientation = NorthOrientation(capi.NorthOrientationRight)
	NorthOrientationDown  NorthOrientation = NorthOrientation(capi.NorthOrientationDown)
	NorthOrientationLeft  NorthOrientation = NorthOrientation(capi.NorthOrientationLeft)
)

// ConstrainMode controls map panning constraints.
type ConstrainMode uint32

const (
	ConstrainModeNone           ConstrainMode = ConstrainMode(capi.ConstrainModeNone)
	ConstrainModeHeightOnly     ConstrainMode = ConstrainMode(capi.ConstrainModeHeightOnly)
	ConstrainModeWidthAndHeight ConstrainMode = ConstrainMode(capi.ConstrainModeWidthAndHeight)
	ConstrainModeScreen         ConstrainMode = ConstrainMode(capi.ConstrainModeScreen)
)

// ViewportMode controls viewport coordinate orientation.
type ViewportMode uint32

const (
	ViewportModeDefault  ViewportMode = ViewportMode(capi.ViewportModeDefault)
	ViewportModeFlippedY ViewportMode = ViewportMode(capi.ViewportModeFlippedY)
)

// TileLODMode selects the native tile LOD algorithm.
type TileLODMode uint32

const (
	TileLODModeDefault  TileLODMode = TileLODMode(capi.TileLODModeDefault)
	TileLODModeDistance TileLODMode = TileLODMode(capi.TileLODModeDistance)
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

func (options ViewportOptions) toCAPI() capi.ViewportOptions {
	var raw capi.ViewportOptions
	if options.NorthOrientation != nil {
		raw.Fields |= capi.ViewportOptionNorthOrientation
		raw.NorthOrientation = uint32(*options.NorthOrientation)
	}
	if options.ConstrainMode != nil {
		raw.Fields |= capi.ViewportOptionConstrainMode
		raw.ConstrainMode = uint32(*options.ConstrainMode)
	}
	if options.ViewportMode != nil {
		raw.Fields |= capi.ViewportOptionViewportMode
		raw.ViewportMode = uint32(*options.ViewportMode)
	}
	if options.FrustumOffset != nil {
		raw.Fields |= capi.ViewportOptionFrustumOffset
		raw.FrustumOffset = options.FrustumOffset.toCAPI()
	}
	return raw
}

func viewportOptionsFromCAPI(raw capi.ViewportOptions) ViewportOptions {
	var options ViewportOptions
	if raw.Fields&capi.ViewportOptionNorthOrientation != 0 {
		value := NorthOrientation(raw.NorthOrientation)
		options.NorthOrientation = &value
	}
	if raw.Fields&capi.ViewportOptionConstrainMode != 0 {
		value := ConstrainMode(raw.ConstrainMode)
		options.ConstrainMode = &value
	}
	if raw.Fields&capi.ViewportOptionViewportMode != 0 {
		value := ViewportMode(raw.ViewportMode)
		options.ViewportMode = &value
	}
	if raw.Fields&capi.ViewportOptionFrustumOffset != 0 {
		value := edgeInsetsFromCAPI(raw.FrustumOffset)
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

func (options TileOptions) toCAPI() capi.TileOptions {
	var raw capi.TileOptions
	if options.PrefetchZoomDelta != nil {
		raw.Fields |= capi.TileOptionPrefetchZoomDelta
		raw.PrefetchZoomDelta = *options.PrefetchZoomDelta
	}
	if options.LODMinRadius != nil {
		raw.Fields |= capi.TileOptionLODMinRadius
		raw.LODMinRadius = *options.LODMinRadius
	}
	if options.LODScale != nil {
		raw.Fields |= capi.TileOptionLODScale
		raw.LODScale = *options.LODScale
	}
	if options.LODPitchThreshold != nil {
		raw.Fields |= capi.TileOptionLODPitchThreshold
		raw.LODPitchThreshold = *options.LODPitchThreshold
	}
	if options.LODZoomShift != nil {
		raw.Fields |= capi.TileOptionLODZoomShift
		raw.LODZoomShift = *options.LODZoomShift
	}
	if options.LODMode != nil {
		raw.Fields |= capi.TileOptionLODMode
		raw.LODMode = uint32(*options.LODMode)
	}
	return raw
}

func tileOptionsFromCAPI(raw capi.TileOptions) TileOptions {
	var options TileOptions
	if raw.Fields&capi.TileOptionPrefetchZoomDelta != 0 {
		value := raw.PrefetchZoomDelta
		options.PrefetchZoomDelta = &value
	}
	if raw.Fields&capi.TileOptionLODMinRadius != 0 {
		value := raw.LODMinRadius
		options.LODMinRadius = &value
	}
	if raw.Fields&capi.TileOptionLODScale != 0 {
		value := raw.LODScale
		options.LODScale = &value
	}
	if raw.Fields&capi.TileOptionLODPitchThreshold != 0 {
		value := raw.LODPitchThreshold
		options.LODPitchThreshold = &value
	}
	if raw.Fields&capi.TileOptionLODZoomShift != 0 {
		value := raw.LODZoomShift
		options.LODZoomShift = &value
	}
	if raw.Fields&capi.TileOptionLODMode != 0 {
		value := TileLODMode(raw.LODMode)
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

func (options ProjectionModeOptions) toCAPI() capi.ProjectionModeOptions {
	var raw capi.ProjectionModeOptions
	if options.Axonometric != nil {
		raw.Fields |= capi.ProjectionModeAxonometric
		raw.Axonometric = *options.Axonometric
	}
	if options.XSkew != nil {
		raw.Fields |= capi.ProjectionModeXSkew
		raw.XSkew = *options.XSkew
	}
	if options.YSkew != nil {
		raw.Fields |= capi.ProjectionModeYSkew
		raw.YSkew = *options.YSkew
	}
	return raw
}

func projectionModeOptionsFromCAPI(raw capi.ProjectionModeOptions) ProjectionModeOptions {
	var options ProjectionModeOptions
	if raw.Fields&capi.ProjectionModeAxonometric != 0 {
		value := raw.Axonometric
		options.Axonometric = &value
	}
	if raw.Fields&capi.ProjectionModeXSkew != 0 {
		value := raw.XSkew
		options.XSkew = &value
	}
	if raw.Fields&capi.ProjectionModeYSkew != 0 {
		value := raw.YSkew
		options.YSkew = &value
	}
	return options
}
