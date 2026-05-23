package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"

// RenderedQueryGeometryType identifies a rendered feature query geometry shape.
type RenderedQueryGeometryType uint32

const (
	RenderedQueryGeometryTypePoint      RenderedQueryGeometryType = RenderedQueryGeometryType(capi.RenderedQueryGeometryTypePoint)
	RenderedQueryGeometryTypeBox        RenderedQueryGeometryType = RenderedQueryGeometryType(capi.RenderedQueryGeometryTypeBox)
	RenderedQueryGeometryTypeLineString RenderedQueryGeometryType = RenderedQueryGeometryType(capi.RenderedQueryGeometryTypeLineString)
)

// ScreenBox is a screen-space query rectangle.
type ScreenBox struct {
	Min ScreenPoint
	Max ScreenPoint
}

// RenderedQueryGeometry describes a rendered feature query geometry.
type RenderedQueryGeometry struct {
	Type   RenderedQueryGeometryType
	Point  ScreenPoint
	Box    ScreenBox
	Points []ScreenPoint
}

// RenderedQueryPoint returns a point rendered-query geometry.
func RenderedQueryPoint(point ScreenPoint) RenderedQueryGeometry {
	return RenderedQueryGeometry{Type: RenderedQueryGeometryTypePoint, Point: point}
}

// RenderedQueryBox returns a box rendered-query geometry.
func RenderedQueryBox(box ScreenBox) RenderedQueryGeometry {
	return RenderedQueryGeometry{Type: RenderedQueryGeometryTypeBox, Box: box}
}

// RenderedQueryLineString returns a line-string rendered-query geometry.
func RenderedQueryLineString(points []ScreenPoint) RenderedQueryGeometry {
	return RenderedQueryGeometry{Type: RenderedQueryGeometryTypeLineString, Points: points}
}

// RenderedFeatureQueryOptions configures rendered feature queries.
type RenderedFeatureQueryOptions struct {
	LayerIDs []string
	Filter   any
}

func (options RenderedFeatureQueryOptions) toCAPI() capi.RenderedFeatureQueryOptions {
	var raw capi.RenderedFeatureQueryOptions
	if options.LayerIDs != nil {
		raw.Fields |= capi.RenderedFeatureQueryOptionLayerIDs
		raw.LayerIDs = options.LayerIDs
	}
	raw.Filter = options.Filter
	return raw
}

func renderedFeatureQueryOptionsToCAPI(options *RenderedFeatureQueryOptions) *capi.RenderedFeatureQueryOptions {
	if options == nil {
		return nil
	}
	raw := options.toCAPI()
	return &raw
}

// SourceFeatureQueryOptions configures source feature queries.
type SourceFeatureQueryOptions struct {
	SourceLayerIDs []string
	Filter         any
}

func (options SourceFeatureQueryOptions) toCAPI() capi.SourceFeatureQueryOptions {
	var raw capi.SourceFeatureQueryOptions
	if options.SourceLayerIDs != nil {
		raw.Fields |= capi.SourceFeatureQueryOptionLayerIDs
		raw.SourceLayerIDs = options.SourceLayerIDs
	}
	raw.Filter = options.Filter
	return raw
}

func sourceFeatureQueryOptionsToCAPI(options *SourceFeatureQueryOptions) *capi.SourceFeatureQueryOptions {
	if options == nil {
		return nil
	}
	raw := options.toCAPI()
	return &raw
}

// QueriedFeature contains one copied feature query result.
type QueriedFeature struct {
	Feature          Feature
	SourceID         string
	HasSourceID      bool
	SourceLayerID    string
	HasSourceLayerID bool
	State            any
	HasState         bool
}

// FeatureExtensionResultType identifies a feature extension result shape.
type FeatureExtensionResultType uint32

const (
	FeatureExtensionResultTypeValue             FeatureExtensionResultType = FeatureExtensionResultType(capi.FeatureExtensionResultTypeValue)
	FeatureExtensionResultTypeFeatureCollection FeatureExtensionResultType = FeatureExtensionResultType(capi.FeatureExtensionResultTypeFeatureCollection)
)

// FeatureExtensionResult contains one copied feature extension result.
type FeatureExtensionResult struct {
	Type     FeatureExtensionResultType
	Value    any
	Features []Feature
}

func (geometry RenderedQueryGeometry) toCAPI() capi.RenderedQueryGeometry {
	return capi.RenderedQueryGeometry{
		Type:   uint32(geometry.Type),
		Point:  geometry.Point.toCAPI(),
		Box:    capi.ScreenBox{Min: geometry.Box.Min.toCAPI(), Max: geometry.Box.Max.toCAPI()},
		Points: screenPointSliceToCAPI(geometry.Points),
	}
}

func screenPointSliceToCAPI(points []ScreenPoint) []capi.ScreenPoint {
	out := make([]capi.ScreenPoint, len(points))
	for i, point := range points {
		out[i] = point.toCAPI()
	}
	return out
}

func queriedFeaturesFromCAPI(features []capi.QueryFeature) []QueriedFeature {
	out := make([]QueriedFeature, len(features))
	for i, feature := range features {
		out[i] = QueriedFeature{
			Feature:          featureFromCAPI(feature.Feature),
			SourceID:         feature.SourceID,
			HasSourceID:      feature.HasSourceID,
			SourceLayerID:    feature.SourceLayerID,
			HasSourceLayerID: feature.HasSourceLayerID,
			State:            feature.State,
			HasState:         feature.HasState,
		}
	}
	return out
}

func featureFromCAPI(feature capi.Feature) Feature {
	return Feature{Geometry: geometryFromCAPI(feature.Geometry), Properties: feature.Properties, Identifier: feature.Identifier}
}

func geometryFromCAPI(geometry capi.Geometry) Geometry {
	return Geometry{
		Type:       GeometryType(geometry.Type),
		Point:      latLngFromCAPI(geometry.Point),
		Points:     latLngSliceFromCAPI(geometry.Points),
		Lines:      latLngLinesFromCAPI(geometry.Lines),
		Polygons:   latLngPolygonsFromCAPI(geometry.Polygons),
		Geometries: geometriesFromCAPI(geometry.Geometries),
	}
}

func latLngLinesFromCAPI(lines [][]capi.LatLng) [][]LatLng {
	out := make([][]LatLng, len(lines))
	for i, line := range lines {
		out[i] = latLngSliceFromCAPI(line)
	}
	return out
}

func latLngPolygonsFromCAPI(polygons [][][]capi.LatLng) [][][]LatLng {
	out := make([][][]LatLng, len(polygons))
	for i, polygon := range polygons {
		out[i] = latLngLinesFromCAPI(polygon)
	}
	return out
}

func geometriesFromCAPI(geometries []capi.Geometry) []Geometry {
	out := make([]Geometry, len(geometries))
	for i, geometry := range geometries {
		out[i] = geometryFromCAPI(geometry)
	}
	return out
}

func featureExtensionResultFromCAPI(result capi.FeatureExtensionResultInfo) FeatureExtensionResult {
	out := FeatureExtensionResult{Type: FeatureExtensionResultType(result.Type), Value: result.Value}
	out.Features = make([]Feature, len(result.Features))
	for i, feature := range result.Features {
		out.Features[i] = featureFromCAPI(feature)
	}
	return out
}

// QueryRenderedFeatures queries rendered features from the latest render session state.
func (session *RenderSessionHandle) QueryRenderedFeatures(geometry RenderedQueryGeometry, options *RenderedFeatureQueryOptions) ([]QueriedFeature, error) {
	ptr, err := session.ptr()
	if err != nil {
		return nil, err
	}
	defer session.state.KeepAlive()
	var features []capi.QueryFeature
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.RenderSessionQueryRenderedFeatures(ptr, geometry.toCAPI(), renderedFeatureQueryOptionsToCAPI(options), &features)
		return status
	})
	if materialErr != nil {
		return nil, newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return queriedFeaturesFromCAPI(features), err
}

// QuerySourceFeatures queries source features from the latest render session state.
func (session *RenderSessionHandle) QuerySourceFeatures(sourceID string, options *SourceFeatureQueryOptions) ([]QueriedFeature, error) {
	ptr, err := session.ptr()
	if err != nil {
		return nil, err
	}
	defer session.state.KeepAlive()
	var features []capi.QueryFeature
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.RenderSessionQuerySourceFeatures(ptr, sourceID, sourceFeatureQueryOptionsToCAPI(options), &features)
		return status
	})
	if materialErr != nil {
		return nil, newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return queriedFeaturesFromCAPI(features), err
}

// QueryFeatureExtensions queries a feature extension from the latest render session state.
func (session *RenderSessionHandle) QueryFeatureExtensions(sourceID string, feature Feature, extension string, extensionField string, arguments any) (FeatureExtensionResult, error) {
	ptr, err := session.ptr()
	if err != nil {
		return FeatureExtensionResult{}, err
	}
	defer session.state.KeepAlive()
	var result capi.FeatureExtensionResultInfo
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.RenderSessionQueryFeatureExtensions(ptr, sourceID, feature.toCAPI(), extension, extensionField, arguments, &result)
		return status
	})
	if materialErr != nil {
		return FeatureExtensionResult{}, newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return featureExtensionResultFromCAPI(result), err
}
