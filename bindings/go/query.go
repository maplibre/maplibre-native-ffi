package maplibre

/*
#include "internal/cgo_geometry_shim.h"
#include "internal/cgo_json_shim.h"
*/
import "C"

import "unsafe"

// RenderedQueryGeometryType identifies a rendered feature query geometry shape.
type RenderedQueryGeometryType uint32

const (
	RenderedQueryGeometryTypePoint      RenderedQueryGeometryType = RenderedQueryGeometryType(C.MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT)
	RenderedQueryGeometryTypeBox        RenderedQueryGeometryType = RenderedQueryGeometryType(C.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX)
	RenderedQueryGeometryTypeLineString RenderedQueryGeometryType = RenderedQueryGeometryType(C.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING)
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

// SourceFeatureQueryOptions configures source feature queries.
type SourceFeatureQueryOptions struct {
	SourceLayerIDs []string
	Filter         any
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

// FeatureStateSelector selects feature state by source, feature, and key.
type FeatureStateSelector struct {
	SourceID      string
	SourceLayerID *string
	FeatureID     *string
	StateKey      *string
}

type cFeatureStateSelector struct {
	raw           C.mln_feature_state_selector
	sourceID      cStringView
	sourceLayerID cStringView
	featureID     cStringView
	stateKey      cStringView
}

func newCFeatureStateSelector(selector FeatureStateSelector) cFeatureStateSelector {
	raw := cFeatureStateSelector{
		raw:      C.mln_feature_state_selector{size: C.uint32_t(unsafe.Sizeof(C.mln_feature_state_selector{}))},
		sourceID: newCStringView(selector.SourceID),
	}
	raw.raw.source_id = raw.sourceID.raw()
	if selector.SourceLayerID != nil {
		raw.raw.fields |= C.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
		raw.sourceLayerID = newCStringView(*selector.SourceLayerID)
		raw.raw.source_layer_id = raw.sourceLayerID.raw()
	}
	if selector.FeatureID != nil {
		raw.raw.fields |= C.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
		raw.featureID = newCStringView(*selector.FeatureID)
		raw.raw.feature_id = raw.featureID.raw()
	}
	if selector.StateKey != nil {
		raw.raw.fields |= C.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
		raw.stateKey = newCStringView(*selector.StateKey)
		raw.raw.state_key = raw.stateKey.raw()
	}
	return raw
}

func (selector cFeatureStateSelector) free() {
	selector.sourceID.free()
	selector.sourceLayerID.free()
	selector.featureID.free()
	selector.stateKey.free()
}

// FeatureExtensionResultType identifies a feature extension result shape.
type FeatureExtensionResultType uint32

const (
	FeatureExtensionResultTypeValue             FeatureExtensionResultType = FeatureExtensionResultType(C.MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE)
	FeatureExtensionResultTypeFeatureCollection FeatureExtensionResultType = FeatureExtensionResultType(C.MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION)
)

// FeatureExtensionResult contains one copied feature extension result.
type FeatureExtensionResult struct {
	Type     FeatureExtensionResultType
	Value    any
	Features []Feature
}

type cRenderedQueryGeometry struct {
	raw    C.mln_rendered_query_geometry
	points []C.mln_screen_point
}

func newCRenderedQueryGeometry(geometry RenderedQueryGeometry) cRenderedQueryGeometry {
	switch geometry.Type {
	case RenderedQueryGeometryTypePoint:
		return cRenderedQueryGeometry{raw: C.mln_rendered_query_geometry_point(cScreenPoint(geometry.Point))}
	case RenderedQueryGeometryTypeBox:
		box := C.mln_screen_box{min: cScreenPoint(geometry.Box.Min), max: cScreenPoint(geometry.Box.Max)}
		return cRenderedQueryGeometry{raw: C.mln_rendered_query_geometry_box(box)}
	case RenderedQueryGeometryTypeLineString:
		points := cScreenPointSlice(geometry.Points)
		var pointsPtr *C.mln_screen_point
		if len(points) > 0 {
			pointsPtr = &points[0]
		}
		return cRenderedQueryGeometry{raw: C.mln_rendered_query_geometry_line_string(pointsPtr, C.size_t(len(points))), points: points}
	default:
		return cRenderedQueryGeometry{raw: C.mln_rendered_query_geometry{size: C.uint32_t(unsafe.Sizeof(C.mln_rendered_query_geometry{})), _type: C.uint32_t(geometry.Type)}}
	}
}

type cRenderedFeatureQueryOptions struct {
	raw          C.mln_rendered_feature_query_options
	layerIDs     cStringViewArray
	materializer *cJSONMaterializer
	filter       C.mln_json_value
}

func newCRenderedFeatureQueryOptions(options *RenderedFeatureQueryOptions) (*cRenderedFeatureQueryOptions, error) {
	if options == nil {
		return nil, nil
	}
	raw := &cRenderedFeatureQueryOptions{raw: C.mln_rendered_feature_query_options_default()}
	if options.LayerIDs != nil {
		raw.raw.fields |= C.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
		raw.layerIDs = newCStringViewArray(options.LayerIDs)
		raw.raw.layer_ids = raw.layerIDs.ptr()
		raw.raw.layer_id_count = raw.layerIDs.count()
	}
	if options.Filter != nil {
		raw.materializer = newCJSONMaterializer()
		filter, err := raw.materializer.value(options.Filter)
		if err != nil {
			raw.free()
			return nil, err
		}
		raw.filter = filter
		raw.raw.filter = &raw.filter
	}
	return raw, nil
}

func (options *cRenderedFeatureQueryOptions) ptr() *C.mln_rendered_feature_query_options {
	if options == nil {
		return nil
	}
	return &options.raw
}

func (options *cRenderedFeatureQueryOptions) free() {
	if options == nil {
		return
	}
	options.layerIDs.free()
	if options.materializer != nil {
		options.materializer.free()
	}
}

type cSourceFeatureQueryOptions struct {
	raw          C.mln_source_feature_query_options
	layerIDs     cStringViewArray
	materializer *cJSONMaterializer
	filter       C.mln_json_value
}

func newCSourceFeatureQueryOptions(options *SourceFeatureQueryOptions) (*cSourceFeatureQueryOptions, error) {
	if options == nil {
		return nil, nil
	}
	raw := &cSourceFeatureQueryOptions{raw: C.mln_source_feature_query_options_default()}
	if options.SourceLayerIDs != nil {
		raw.raw.fields |= C.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
		raw.layerIDs = newCStringViewArray(options.SourceLayerIDs)
		raw.raw.source_layer_ids = raw.layerIDs.ptr()
		raw.raw.source_layer_id_count = raw.layerIDs.count()
	}
	if options.Filter != nil {
		raw.materializer = newCJSONMaterializer()
		filter, err := raw.materializer.value(options.Filter)
		if err != nil {
			raw.free()
			return nil, err
		}
		raw.filter = filter
		raw.raw.filter = &raw.filter
	}
	return raw, nil
}

func (options *cSourceFeatureQueryOptions) ptr() *C.mln_source_feature_query_options {
	if options == nil {
		return nil
	}
	return &options.raw
}

func (options *cSourceFeatureQueryOptions) free() {
	if options == nil {
		return
	}
	options.layerIDs.free()
	if options.materializer != nil {
		options.materializer.free()
	}
}

func cFeatureQueryResultFeatures(result *C.mln_feature_query_result) ([]QueriedFeature, error) {
	defer C.mln_feature_query_result_destroy(result)
	var count C.size_t
	if err := checkNative(func() int32 { return int32(C.mln_feature_query_result_count(result, &count)) }); err != nil {
		return nil, err
	}
	features := make([]QueriedFeature, int(count))
	for i := range features {
		raw := C.mln_queried_feature{size: C.uint32_t(unsafe.Sizeof(C.mln_queried_feature{}))}
		if err := checkNative(func() int32 { return int32(C.mln_feature_query_result_get(result, C.size_t(i), &raw)) }); err != nil {
			return nil, err
		}
		feature, err := cQueriedFeature(raw)
		if err != nil {
			return nil, err
		}
		features[i] = feature
	}
	return features, nil
}

func cQueriedFeature(raw C.mln_queried_feature) (QueriedFeature, error) {
	feature, err := cFeature(&raw.feature)
	if err != nil {
		return QueriedFeature{}, err
	}
	out := QueriedFeature{Feature: feature}
	fields := uint32(raw.fields)
	if fields&uint32(C.MLN_QUERIED_FEATURE_SOURCE_ID) != 0 {
		out.HasSourceID = true
		out.SourceID = goStringView(raw.source_id)
	}
	if fields&uint32(C.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID) != 0 {
		out.HasSourceLayerID = true
		out.SourceLayerID = goStringView(raw.source_layer_id)
	}
	if fields&uint32(C.MLN_QUERIED_FEATURE_STATE) != 0 {
		state, err := cJSONValue((*C.mln_json_value)(unsafe.Pointer(raw.state)))
		if err != nil {
			return QueriedFeature{}, newBindingError(ErrNative, err.Error())
		}
		out.HasState = true
		out.State = state
	}
	return out, nil
}

func cFeature(feature *C.mln_feature) (Feature, error) {
	geometry, err := cGeometry((*C.mln_geometry)(unsafe.Pointer(C.mln_go_feature_geometry(feature))))
	if err != nil {
		return Feature{}, err
	}
	out := Feature{Geometry: geometry, Properties: make(map[string]any, int(C.mln_go_feature_property_count(feature)))}
	for i := 0; i < int(C.mln_go_feature_property_count(feature)); i++ {
		value, err := cJSONValue((*C.mln_json_value)(unsafe.Pointer(C.mln_go_feature_property_value(feature, C.size_t(i)))))
		if err != nil {
			return Feature{}, newBindingError(ErrNative, err.Error())
		}
		out.Properties[goStringView(C.mln_go_feature_property_key(feature, C.size_t(i)))] = value
	}
	switch uint32(C.mln_go_feature_identifier_type(feature)) {
	case C.MLN_FEATURE_IDENTIFIER_TYPE_NULL:
		out.Identifier = nil
	case C.MLN_FEATURE_IDENTIFIER_TYPE_UINT:
		out.Identifier = uint64(C.mln_go_feature_identifier_uint(feature))
	case C.MLN_FEATURE_IDENTIFIER_TYPE_INT:
		out.Identifier = int64(C.mln_go_feature_identifier_int(feature))
	case C.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE:
		out.Identifier = float64(C.mln_go_feature_identifier_double(feature))
	case C.MLN_FEATURE_IDENTIFIER_TYPE_STRING:
		out.Identifier = goStringView(C.mln_go_feature_identifier_string(feature))
	default:
		return Feature{}, newBindingError(ErrNative, "unknown feature identifier type")
	}
	return out, nil
}

func cGeometry(geometry *C.mln_geometry) (Geometry, error) {
	if geometry == nil {
		return Geometry{}, nil
	}
	out := Geometry{Type: GeometryType(C.mln_go_geometry_type(geometry))}
	switch out.Type {
	case GeometryTypeEmpty:
	case GeometryTypePoint:
		out.Point = goLatLng(C.mln_go_geometry_point_value(geometry))
	case GeometryTypeLineString:
		out.Points = cCoordinateSpanSlice(C.mln_go_geometry_line_string_value(geometry))
	case GeometryTypePolygon:
		out.Lines = make([][]LatLng, int(C.mln_go_geometry_polygon_ring_count(geometry)))
		for i := range out.Lines {
			out.Lines[i] = cCoordinateSpanSlice(C.mln_go_geometry_polygon_ring_get(geometry, C.size_t(i)))
		}
	case GeometryTypeMultiPoint:
		out.Points = cCoordinateSpanSlice(C.mln_go_geometry_multi_point_value(geometry))
	case GeometryTypeMultiLineString:
		out.Lines = make([][]LatLng, int(C.mln_go_geometry_multi_line_count(geometry)))
		for i := range out.Lines {
			out.Lines[i] = cCoordinateSpanSlice(C.mln_go_geometry_multi_line_get(geometry, C.size_t(i)))
		}
	case GeometryTypeMultiPolygon:
		out.Polygons = make([][][]LatLng, int(C.mln_go_geometry_multi_polygon_count(geometry)))
		for i := range out.Polygons {
			polygon := C.mln_go_geometry_multi_polygon_get(geometry, C.size_t(i))
			out.Polygons[i] = make([][]LatLng, int(C.mln_go_polygon_geometry_ring_count(polygon)))
			for j := range out.Polygons[i] {
				out.Polygons[i][j] = cCoordinateSpanSlice(C.mln_go_polygon_geometry_ring_get(polygon, C.size_t(j)))
			}
		}
	case GeometryTypeGeometryCollection:
		out.Geometries = make([]Geometry, int(C.mln_go_geometry_collection_count(geometry)))
		for i := range out.Geometries {
			child, err := cGeometry((*C.mln_geometry)(unsafe.Pointer(C.mln_go_geometry_collection_get(geometry, C.size_t(i)))))
			if err != nil {
				return Geometry{}, err
			}
			out.Geometries[i] = child
		}
	default:
		return Geometry{}, newBindingError(ErrNative, "unknown geometry type")
	}
	return out, nil
}

func cCoordinateSpanSlice(span C.mln_coordinate_span) []LatLng {
	out := make([]LatLng, int(C.mln_go_coordinate_span_count(span)))
	for i := range out {
		out[i] = goLatLng(C.mln_go_coordinate_span_get(span, C.size_t(i)))
	}
	return out
}

func cFeatureExtensionResult(result *C.mln_feature_extension_result) (FeatureExtensionResult, error) {
	defer C.mln_feature_extension_result_destroy(result)
	raw := C.mln_feature_extension_result_info{size: C.uint32_t(unsafe.Sizeof(C.mln_feature_extension_result_info{}))}
	if err := checkNative(func() int32 { return int32(C.mln_feature_extension_result_get(result, &raw)) }); err != nil {
		return FeatureExtensionResult{}, err
	}
	out := FeatureExtensionResult{Type: FeatureExtensionResultType(C.mln_go_feature_extension_result_type(&raw))}
	switch out.Type {
	case FeatureExtensionResultTypeValue:
		value, err := cJSONValue((*C.mln_json_value)(unsafe.Pointer(C.mln_go_feature_extension_result_value(&raw))))
		if err != nil {
			return FeatureExtensionResult{}, newBindingError(ErrNative, err.Error())
		}
		out.Value = value
	case FeatureExtensionResultTypeFeatureCollection:
		out.Features = make([]Feature, int(C.mln_go_feature_extension_result_feature_count(&raw)))
		for i := range out.Features {
			feature, err := cFeature((*C.mln_feature)(unsafe.Pointer(C.mln_go_feature_extension_result_feature_get(&raw, C.size_t(i)))))
			if err != nil {
				return FeatureExtensionResult{}, err
			}
			out.Features[i] = feature
		}
	default:
		return out, nil
	}
	return out, nil
}

func featureExtensionResultForTest(resultType uint32) FeatureExtensionResult {
	return FeatureExtensionResult{Type: FeatureExtensionResultType(resultType)}
}

// SetFeatureState sets per-feature state on a render source. The state value is
// copied before the call returns.
func (session *RenderSessionHandle) SetFeatureState(selector FeatureStateSelector, state map[string]any) error {
	ptr, err := session.ptr()
	if err != nil {
		return err
	}
	defer session.state.KeepAlive()
	rawSelector := newCFeatureStateSelector(selector)
	defer rawSelector.free()
	materializer := newCJSONMaterializer()
	defer materializer.free()
	rawState, err := materializer.value(state)
	if err != nil {
		return newBindingError(ErrInvalidArgument, err.Error())
	}
	return checkNative(func() int32 {
		return int32(C.mln_render_session_set_feature_state((*C.mln_render_session)(unsafe.Pointer(ptr)), &rawSelector.raw, &rawState))
	})
}

// FeatureState returns copied per-feature state from a render source.
func (session *RenderSessionHandle) FeatureState(selector FeatureStateSelector) (any, error) {
	ptr, err := session.ptr()
	if err != nil {
		return nil, err
	}
	defer session.state.KeepAlive()
	rawSelector := newCFeatureStateSelector(selector)
	defer rawSelector.free()
	var snapshot *C.mln_json_snapshot
	if err := checkNative(func() int32 {
		return int32(C.mln_render_session_get_feature_state((*C.mln_render_session)(unsafe.Pointer(ptr)), &rawSelector.raw, &snapshot))
	}); err != nil {
		return nil, err
	}
	return cJSONSnapshotValue(snapshot)
}

// RemoveFeatureState removes per-feature state from a render source.
func (session *RenderSessionHandle) RemoveFeatureState(selector FeatureStateSelector) error {
	ptr, err := session.ptr()
	if err != nil {
		return err
	}
	defer session.state.KeepAlive()
	rawSelector := newCFeatureStateSelector(selector)
	defer rawSelector.free()
	return checkNative(func() int32 {
		return int32(C.mln_render_session_remove_feature_state((*C.mln_render_session)(unsafe.Pointer(ptr)), &rawSelector.raw))
	})
}

// QueryRenderedFeatures queries rendered features from the latest render session state.
func (session *RenderSessionHandle) QueryRenderedFeatures(geometry RenderedQueryGeometry, options *RenderedFeatureQueryOptions) ([]QueriedFeature, error) {
	ptr, err := session.ptr()
	if err != nil {
		return nil, err
	}
	defer session.state.KeepAlive()
	rawGeometry := newCRenderedQueryGeometry(geometry)
	rawOptions, err := newCRenderedFeatureQueryOptions(options)
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	defer rawOptions.free()
	var result *C.mln_feature_query_result
	if err := checkNative(func() int32 {
		return int32(C.mln_render_session_query_rendered_features((*C.mln_render_session)(unsafe.Pointer(ptr)), &rawGeometry.raw, rawOptions.ptr(), &result))
	}); err != nil {
		return nil, err
	}
	return cFeatureQueryResultFeatures(result)
}

// QuerySourceFeatures queries source features from the latest render session state.
func (session *RenderSessionHandle) QuerySourceFeatures(sourceID string, options *SourceFeatureQueryOptions) ([]QueriedFeature, error) {
	ptr, err := session.ptr()
	if err != nil {
		return nil, err
	}
	defer session.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawOptions, err := newCSourceFeatureQueryOptions(options)
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	defer rawOptions.free()
	var result *C.mln_feature_query_result
	if err := checkNative(func() int32 {
		return int32(C.mln_render_session_query_source_features((*C.mln_render_session)(unsafe.Pointer(ptr)), sourceView.raw(), rawOptions.ptr(), &result))
	}); err != nil {
		return nil, err
	}
	return cFeatureQueryResultFeatures(result)
}

// QueryFeatureExtensions queries a feature extension from the latest render session state.
func (session *RenderSessionHandle) QueryFeatureExtensions(sourceID string, feature Feature, extension string, extensionField string, arguments any) (FeatureExtensionResult, error) {
	ptr, err := session.ptr()
	if err != nil {
		return FeatureExtensionResult{}, err
	}
	defer session.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	extensionView := newCStringView(extension)
	defer extensionView.free()
	extensionFieldView := newCStringView(extensionField)
	defer extensionFieldView.free()
	geojsonMaterializer := newCGeoJSONMaterializer()
	defer geojsonMaterializer.free()
	rawFeature, err := geojsonMaterializer.featurePtr(feature)
	if err != nil {
		return FeatureExtensionResult{}, newBindingError(ErrInvalidArgument, err.Error())
	}
	jsonMaterializer := newCJSONMaterializer()
	defer jsonMaterializer.free()
	var rawArguments *C.mln_json_value
	if arguments != nil {
		value, err := jsonMaterializer.value(arguments)
		if err != nil {
			return FeatureExtensionResult{}, newBindingError(ErrInvalidArgument, err.Error())
		}
		rawArguments = &value
	}
	var result *C.mln_feature_extension_result
	if err := checkNative(func() int32 {
		return int32(C.mln_render_session_query_feature_extensions(
			(*C.mln_render_session)(unsafe.Pointer(ptr)),
			sourceView.raw(),
			rawFeature,
			extensionView.raw(),
			extensionFieldView.raw(),
			rawArguments,
			&result,
		))
	}); err != nil {
		return FeatureExtensionResult{}, err
	}
	return cFeatureExtensionResult(result)
}
