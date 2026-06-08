package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

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

// GeometryType identifies the shape stored in a Geometry descriptor.
type GeometryType uint32

const (
	GeometryTypeEmpty              GeometryType = GeometryType(C.MLN_GEOMETRY_TYPE_EMPTY)
	GeometryTypePoint              GeometryType = GeometryType(C.MLN_GEOMETRY_TYPE_POINT)
	GeometryTypeLineString         GeometryType = GeometryType(C.MLN_GEOMETRY_TYPE_LINE_STRING)
	GeometryTypePolygon            GeometryType = GeometryType(C.MLN_GEOMETRY_TYPE_POLYGON)
	GeometryTypeMultiPoint         GeometryType = GeometryType(C.MLN_GEOMETRY_TYPE_MULTI_POINT)
	GeometryTypeMultiLineString    GeometryType = GeometryType(C.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING)
	GeometryTypeMultiPolygon       GeometryType = GeometryType(C.MLN_GEOMETRY_TYPE_MULTI_POLYGON)
	GeometryTypeGeometryCollection GeometryType = GeometryType(C.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION)
)

// Geometry is a GeoJSON geometry descriptor.
type Geometry struct {
	Type       GeometryType
	Point      LatLng
	Points     []LatLng
	Lines      [][]LatLng
	Polygons   [][][]LatLng
	Geometries []Geometry
}

// Feature is a GeoJSON feature descriptor.
type Feature struct {
	Geometry Geometry
	// Properties uses Go map semantics: object member order is not preserved, and
	// duplicate keys collapse to one value.
	Properties map[string]any
	Identifier any
}

// GeoJSONType identifies the root GeoJSON descriptor variant.
type GeoJSONType uint32

const (
	GeoJSONTypeGeometry          GeoJSONType = GeoJSONType(C.MLN_GEOJSON_TYPE_GEOMETRY)
	GeoJSONTypeFeature           GeoJSONType = GeoJSONType(C.MLN_GEOJSON_TYPE_FEATURE)
	GeoJSONTypeFeatureCollection GeoJSONType = GeoJSONType(C.MLN_GEOJSON_TYPE_FEATURE_COLLECTION)
)

// GeoJSON is a GeoJSON geometry, feature, or feature collection descriptor.
type GeoJSON struct {
	Type     GeoJSONType
	Geometry Geometry
	Feature  Feature
	Features []Feature
}

// PointGeometry returns a point geometry descriptor.
func PointGeometry(point LatLng) Geometry {
	return Geometry{Type: GeometryTypePoint, Point: point}
}

// LineStringGeometry returns a line string geometry descriptor.
func LineStringGeometry(points []LatLng) Geometry {
	return Geometry{Type: GeometryTypeLineString, Points: points}
}

// PolygonGeometry returns a polygon geometry descriptor.
func PolygonGeometry(rings [][]LatLng) Geometry {
	return Geometry{Type: GeometryTypePolygon, Lines: rings}
}

// GeoJSONFeatureCollection returns a feature collection GeoJSON descriptor.
func GeoJSONFeatureCollection(features []Feature) GeoJSON {
	return GeoJSON{Type: GeoJSONTypeFeatureCollection, Features: features}
}
