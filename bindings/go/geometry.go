package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"

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

// EdgeInsets is a screen-space inset in logical map pixels.
type EdgeInsets struct {
	Top    float64
	Left   float64
	Bottom float64
	Right  float64
}

// ProjectedMeters is a spherical Mercator coordinate in meters.
type ProjectedMeters struct {
	Northing float64
	Easting  float64
}

// LatLngBounds is a geographic bounds rectangle in degrees.
type LatLngBounds struct {
	Southwest LatLng
	Northeast LatLng
}

func (coordinate LatLng) toCAPI() capi.LatLng {
	return capi.LatLng{Latitude: coordinate.Latitude, Longitude: coordinate.Longitude}
}

func latLngFromCAPI(coordinate capi.LatLng) LatLng {
	return LatLng{Latitude: coordinate.Latitude, Longitude: coordinate.Longitude}
}

func (point ScreenPoint) toCAPI() capi.ScreenPoint {
	return capi.ScreenPoint{X: point.X, Y: point.Y}
}

func (insets EdgeInsets) toCAPI() capi.EdgeInsets {
	return capi.EdgeInsets{Top: insets.Top, Left: insets.Left, Bottom: insets.Bottom, Right: insets.Right}
}

func edgeInsetsFromCAPI(insets capi.EdgeInsets) EdgeInsets {
	return EdgeInsets{Top: insets.Top, Left: insets.Left, Bottom: insets.Bottom, Right: insets.Right}
}

func screenPointFromCAPI(point capi.ScreenPoint) ScreenPoint {
	return ScreenPoint{X: point.X, Y: point.Y}
}

func (meters ProjectedMeters) toCAPI() capi.ProjectedMeters {
	return capi.ProjectedMeters{Northing: meters.Northing, Easting: meters.Easting}
}

func projectedMetersFromCAPI(meters capi.ProjectedMeters) ProjectedMeters {
	return ProjectedMeters{Northing: meters.Northing, Easting: meters.Easting}
}

func (bounds LatLngBounds) toCAPI() capi.LatLngBounds {
	return capi.LatLngBounds{Southwest: bounds.Southwest.toCAPI(), Northeast: bounds.Northeast.toCAPI()}
}
