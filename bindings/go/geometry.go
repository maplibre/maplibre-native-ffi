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

// ProjectedMeters is a spherical Mercator coordinate in meters.
type ProjectedMeters struct {
	Northing float64
	Easting  float64
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

func screenPointFromCAPI(point capi.ScreenPoint) ScreenPoint {
	return ScreenPoint{X: point.X, Y: point.Y}
}

func (meters ProjectedMeters) toCAPI() capi.ProjectedMeters {
	return capi.ProjectedMeters{Northing: meters.Northing, Easting: meters.Easting}
}

func projectedMetersFromCAPI(meters capi.ProjectedMeters) ProjectedMeters {
	return ProjectedMeters{Northing: meters.Northing, Easting: meters.Easting}
}
