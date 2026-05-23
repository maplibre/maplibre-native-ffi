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
	GeometryTypeEmpty              GeometryType = GeometryType(capi.GeometryTypeEmpty)
	GeometryTypePoint              GeometryType = GeometryType(capi.GeometryTypePoint)
	GeometryTypeLineString         GeometryType = GeometryType(capi.GeometryTypeLineString)
	GeometryTypePolygon            GeometryType = GeometryType(capi.GeometryTypePolygon)
	GeometryTypeMultiPoint         GeometryType = GeometryType(capi.GeometryTypeMultiPoint)
	GeometryTypeMultiLineString    GeometryType = GeometryType(capi.GeometryTypeMultiLineString)
	GeometryTypeMultiPolygon       GeometryType = GeometryType(capi.GeometryTypeMultiPolygon)
	GeometryTypeGeometryCollection GeometryType = GeometryType(capi.GeometryTypeGeometryCollection)
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
	Geometry   Geometry
	Properties map[string]any
	Identifier any
}

// GeoJSONType identifies the root GeoJSON descriptor variant.
type GeoJSONType uint32

const (
	GeoJSONTypeGeometry          GeoJSONType = GeoJSONType(capi.GeoJSONTypeGeometry)
	GeoJSONTypeFeature           GeoJSONType = GeoJSONType(capi.GeoJSONTypeFeature)
	GeoJSONTypeFeatureCollection GeoJSONType = GeoJSONType(capi.GeoJSONTypeFeatureCollection)
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

func (vector Vec3) toCAPI() capi.Vec3 {
	return capi.Vec3{X: vector.X, Y: vector.Y, Z: vector.Z}
}

func vec3FromCAPI(vector capi.Vec3) Vec3 {
	return Vec3{X: vector.X, Y: vector.Y, Z: vector.Z}
}

func (quaternion Quaternion) toCAPI() capi.Quaternion {
	return capi.Quaternion{X: quaternion.X, Y: quaternion.Y, Z: quaternion.Z, W: quaternion.W}
}

func quaternionFromCAPI(quaternion capi.Quaternion) Quaternion {
	return Quaternion{X: quaternion.X, Y: quaternion.Y, Z: quaternion.Z, W: quaternion.W}
}

func (bounds LatLngBounds) toCAPI() capi.LatLngBounds {
	return capi.LatLngBounds{Southwest: bounds.Southwest.toCAPI(), Northeast: bounds.Northeast.toCAPI()}
}

func latLngBoundsFromCAPI(bounds capi.LatLngBounds) LatLngBounds {
	return LatLngBounds{Southwest: latLngFromCAPI(bounds.Southwest), Northeast: latLngFromCAPI(bounds.Northeast)}
}

func (tileID CanonicalTileID) toCAPI() capi.CanonicalTileID {
	return capi.CanonicalTileID{Z: tileID.Z, X: tileID.X, Y: tileID.Y}
}

func (geometry Geometry) toCAPI() capi.Geometry {
	return capi.Geometry{
		Type:       uint32(geometry.Type),
		Point:      geometry.Point.toCAPI(),
		Points:     latLngSliceToCAPI(geometry.Points),
		Lines:      latLngLinesToCAPI(geometry.Lines),
		Polygons:   latLngPolygonsToCAPI(geometry.Polygons),
		Geometries: geometriesToCAPI(geometry.Geometries),
	}
}

func (feature Feature) toCAPI() capi.Feature {
	return capi.Feature{Geometry: feature.Geometry.toCAPI(), Properties: feature.Properties, Identifier: feature.Identifier}
}

func (geojson GeoJSON) toCAPI() capi.GeoJSON {
	return capi.GeoJSON{
		Type:     uint32(geojson.Type),
		Geometry: geojson.Geometry.toCAPI(),
		Feature:  geojson.Feature.toCAPI(),
		Features: featuresToCAPI(geojson.Features),
	}
}

func latLngSliceToCAPI(points []LatLng) []capi.LatLng {
	out := make([]capi.LatLng, len(points))
	for i, point := range points {
		out[i] = point.toCAPI()
	}
	return out
}

func latLngSliceFromCAPI(points []capi.LatLng) []LatLng {
	out := make([]LatLng, len(points))
	for i, point := range points {
		out[i] = latLngFromCAPI(point)
	}
	return out
}

func latLngLinesToCAPI(lines [][]LatLng) [][]capi.LatLng {
	out := make([][]capi.LatLng, len(lines))
	for i, line := range lines {
		out[i] = latLngSliceToCAPI(line)
	}
	return out
}

func latLngPolygonsToCAPI(polygons [][][]LatLng) [][][]capi.LatLng {
	out := make([][][]capi.LatLng, len(polygons))
	for i, polygon := range polygons {
		out[i] = latLngLinesToCAPI(polygon)
	}
	return out
}

func geometriesToCAPI(geometries []Geometry) []capi.Geometry {
	out := make([]capi.Geometry, len(geometries))
	for i, geometry := range geometries {
		out[i] = geometry.toCAPI()
	}
	return out
}

func featuresToCAPI(features []Feature) []capi.Feature {
	out := make([]capi.Feature, len(features))
	for i, feature := range features {
		out[i] = feature.toCAPI()
	}
	return out
}
