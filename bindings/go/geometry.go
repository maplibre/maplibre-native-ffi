package maplibre

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

// Vec3 is a three-component vector.
type Vec3 struct {
	X float64
	Y float64
	Z float64
}

// Quaternion stores x, y, z, w components.
type Quaternion struct {
	X float64
	Y float64
	Z float64
	W float64
}

// LatLngBounds is a geographic bounds rectangle in degrees.
type LatLngBounds struct {
	Southwest LatLng
	Northeast LatLng
}

// CanonicalTileID identifies one canonical tile.
type CanonicalTileID struct {
	Z uint32
	X uint32
	Y uint32
}
