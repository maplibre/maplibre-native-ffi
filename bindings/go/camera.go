package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"

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
