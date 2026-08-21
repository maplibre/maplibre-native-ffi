package maplibre

/*
#include <stdlib.h>
#include "maplibre_native_c.h"
*/
import "C"

import (
	"bytes"
	"unsafe"
)

// RenderedQueryGeometryType identifies a rendered feature query geometry shape.
type RenderedQueryGeometryType uint32

const (
	RenderedQueryGeometryTypePoint      RenderedQueryGeometryType = RenderedQueryGeometryType(C.MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT)
	RenderedQueryGeometryTypeBox        RenderedQueryGeometryType = RenderedQueryGeometryType(C.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX)
	RenderedQueryGeometryTypeLineString RenderedQueryGeometryType = RenderedQueryGeometryType(C.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING)
)

// ScreenBox is a screen-space query rectangle in logical map pixels. Corners
// may be given in any order and may extend past the viewport; rendered queries
// normalize the corners and clip the box to the viewport.
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
	Filter   []byte
}

// Equal reports whether two descriptors hold the same field values.
func (options RenderedFeatureQueryOptions) Equal(other RenderedFeatureQueryOptions) bool {
	return equalStrings(options.LayerIDs, other.LayerIDs) &&
		equalOptionalBytes(options.Filter, other.Filter)
}

// SourceFeatureQueryOptions configures source feature queries.
type SourceFeatureQueryOptions struct {
	SourceLayerIDs []string
	Filter         []byte
}

// Equal reports whether two descriptors hold the same field values.
func (options SourceFeatureQueryOptions) Equal(other SourceFeatureQueryOptions) bool {
	return equalStrings(options.SourceLayerIDs, other.SourceLayerIDs) &&
		equalOptionalBytes(options.Filter, other.Filter)
}

// QueriedFeature is one copied feature query hit.
//
// Feature is a UTF-8 GeoJSON Feature. SourceID and SourceLayerID are nil when
// absent. State is nil when absent and otherwise a UTF-8 JSON object.
type QueriedFeature struct {
	Feature       []byte
	SourceID      *string
	SourceLayerID *string
	State         []byte
}

// Equal reports whether two copied hits hold the same field values. Absent
// optional fields stay distinct from present empty values.
func (feature QueriedFeature) Equal(other QueriedFeature) bool {
	return bytes.Equal(feature.Feature, other.Feature) &&
		equalPointer(feature.SourceID, other.SourceID) &&
		equalPointer(feature.SourceLayerID, other.SourceLayerID) &&
		equalOptionalBytes(feature.State, other.State)
}

func equalOptionalBytes(left []byte, right []byte) bool {
	return (left == nil) == (right == nil) && bytes.Equal(left, right)
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

type cRenderedQueryGeometry struct {
	raw    *C.mln_rendered_query_geometry
	points unsafe.Pointer
}

func newCRenderedQueryGeometry(geometry RenderedQueryGeometry) cRenderedQueryGeometry {
	raw := (*C.mln_rendered_query_geometry)(C.malloc(C.size_t(unsafe.Sizeof(C.mln_rendered_query_geometry{}))))
	switch geometry.Type {
	case RenderedQueryGeometryTypePoint:
		*raw = C.mln_rendered_query_geometry_point(cScreenPoint(geometry.Point))
		return cRenderedQueryGeometry{raw: raw}
	case RenderedQueryGeometryTypeBox:
		box := C.mln_screen_box{min: cScreenPoint(geometry.Box.Min), max: cScreenPoint(geometry.Box.Max)}
		*raw = C.mln_rendered_query_geometry_box(box)
		return cRenderedQueryGeometry{raw: raw}
	case RenderedQueryGeometryTypeLineString:
		var pointsPtr *C.mln_screen_point
		var points unsafe.Pointer
		if len(geometry.Points) > 0 {
			points = C.malloc(C.size_t(len(geometry.Points)) * C.size_t(unsafe.Sizeof(C.mln_screen_point{})))
			pointsPtr = (*C.mln_screen_point)(points)
			for i, point := range geometry.Points {
				*(*C.mln_screen_point)(unsafe.Add(points, uintptr(i)*unsafe.Sizeof(C.mln_screen_point{}))) = cScreenPoint(point)
			}
		}
		*raw = C.mln_rendered_query_geometry_line_string(pointsPtr, C.size_t(len(geometry.Points)))
		return cRenderedQueryGeometry{raw: raw, points: points}
	default:
		*raw = C.mln_rendered_query_geometry{size: C.uint32_t(unsafe.Sizeof(C.mln_rendered_query_geometry{})), _type: C.uint32_t(geometry.Type)}
		return cRenderedQueryGeometry{raw: raw}
	}
}

func (geometry cRenderedQueryGeometry) ptr() *C.mln_rendered_query_geometry {
	return geometry.raw
}

func (geometry cRenderedQueryGeometry) free() {
	if geometry.points != nil {
		C.free(geometry.points)
	}
	if geometry.raw != nil {
		C.free(unsafe.Pointer(geometry.raw))
	}
}

type cRenderedFeatureQueryOptions struct {
	raw      *C.mln_rendered_feature_query_options
	layerIDs cStringViewArray
	filter   cBufferView
}

func newCRenderedFeatureQueryOptions(options *RenderedFeatureQueryOptions) (*cRenderedFeatureQueryOptions, error) {
	if options == nil {
		return nil, nil
	}
	raw := &cRenderedFeatureQueryOptions{
		raw: (*C.mln_rendered_feature_query_options)(C.malloc(C.size_t(unsafe.Sizeof(C.mln_rendered_feature_query_options{})))),
	}
	*raw.raw = C.mln_rendered_feature_query_options_default()
	if options.LayerIDs != nil {
		raw.raw.fields |= C.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
		raw.layerIDs = newCStringViewArray(options.LayerIDs)
		raw.raw.layer_ids = raw.layerIDs.ptr()
		raw.raw.layer_id_count = raw.layerIDs.count()
	}
	if options.Filter != nil {
		raw.filter = newCBufferView(options.Filter)
		raw.raw.filter = raw.filter.ptr()
	}
	return raw, nil
}

func (options *cRenderedFeatureQueryOptions) ptr() *C.mln_rendered_feature_query_options {
	if options == nil {
		return nil
	}
	return options.raw
}

func (options *cRenderedFeatureQueryOptions) free() {
	if options == nil {
		return
	}
	options.layerIDs.free()
	options.filter.free()
	if options.raw != nil {
		C.free(unsafe.Pointer(options.raw))
	}
}

type cSourceFeatureQueryOptions struct {
	raw      *C.mln_source_feature_query_options
	layerIDs cStringViewArray
	filter   cBufferView
}

func newCSourceFeatureQueryOptions(options *SourceFeatureQueryOptions) (*cSourceFeatureQueryOptions, error) {
	if options == nil {
		return nil, nil
	}
	raw := &cSourceFeatureQueryOptions{
		raw: (*C.mln_source_feature_query_options)(C.malloc(C.size_t(unsafe.Sizeof(C.mln_source_feature_query_options{})))),
	}
	*raw.raw = C.mln_source_feature_query_options_default()
	if options.SourceLayerIDs != nil {
		raw.raw.fields |= C.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
		raw.layerIDs = newCStringViewArray(options.SourceLayerIDs)
		raw.raw.source_layer_ids = raw.layerIDs.ptr()
		raw.raw.source_layer_id_count = raw.layerIDs.count()
	}
	if options.Filter != nil {
		raw.filter = newCBufferView(options.Filter)
		raw.raw.filter = raw.filter.ptr()
	}
	return raw, nil
}

func (options *cSourceFeatureQueryOptions) ptr() *C.mln_source_feature_query_options {
	if options == nil {
		return nil
	}
	return options.raw
}

func (options *cSourceFeatureQueryOptions) free() {
	if options == nil {
		return
	}
	options.layerIDs.free()
	options.filter.free()
	if options.raw != nil {
		C.free(unsafe.Pointer(options.raw))
	}
}

// QueryRenderedFeatures queries rendered features from the latest render session
// state and returns copied hits.
func (session *RenderSessionHandle) QueryRenderedFeatures(geometry RenderedQueryGeometry, options *RenderedFeatureQueryOptions) ([]QueriedFeature, error) {
	ptr, release, err := session.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer session.state.KeepAlive()
	rawGeometry := newCRenderedQueryGeometry(geometry)
	defer rawGeometry.free()
	rawOptions, err := newCRenderedFeatureQueryOptions(options)
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	defer rawOptions.free()
	var result C.mln_queried_feature_list
	if err := session.withNoAcquiredFrame(func() error {
		return checkNative(func() int32 {
			return int32(C.mln_render_session_query_rendered_features(C.mln_render_session(ptr), rawGeometry.ptr(), rawOptions.ptr(), &result))
		})
	}); err != nil {
		return nil, err
	}
	return queriedFeatureList(result)
}

// QuerySourceFeatures queries source features from the latest render session
// state and returns copied hits.
func (session *RenderSessionHandle) QuerySourceFeatures(sourceID string, options *SourceFeatureQueryOptions) ([]QueriedFeature, error) {
	ptr, release, err := session.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer session.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawOptions, err := newCSourceFeatureQueryOptions(options)
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	defer rawOptions.free()
	var result C.mln_queried_feature_list
	if err := session.withNoAcquiredFrame(func() error {
		return checkNative(func() int32 {
			return int32(C.mln_render_session_query_source_features(C.mln_render_session(ptr), sourceView.raw(), rawOptions.ptr(), &result))
		})
	}); err != nil {
		return nil, err
	}
	return queriedFeatureList(result)
}

func queriedFeatureList(list C.mln_queried_feature_list) ([]QueriedFeature, error) {
	defer C.mln_queried_feature_list_destroy(list)
	var count C.size_t
	if err := checkNative(func() int32 { return int32(C.mln_queried_feature_list_count(list, &count)) }); err != nil {
		return nil, err
	}
	features := make([]QueriedFeature, int(count))
	for i := range features {
		hit := C.mln_queried_feature_default()
		if err := checkNative(func() int32 {
			return int32(C.mln_queried_feature_list_get(list, C.size_t(i), &hit))
		}); err != nil {
			return nil, err
		}
		feature, ok := goByteSlice(hit.feature.data, hit.feature.size)
		if !ok {
			return nil, newBindingError(ErrNative, "native queried feature data is invalid")
		}
		item := QueriedFeature{Feature: feature}
		if hit.fields&C.MLN_QUERIED_FEATURE_SOURCE_ID != 0 {
			sourceID := goStringView(hit.source_id)
			item.SourceID = &sourceID
		}
		if hit.fields&C.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID != 0 {
			sourceLayerID := goStringView(hit.source_layer_id)
			item.SourceLayerID = &sourceLayerID
		}
		if hit.fields&C.MLN_QUERIED_FEATURE_STATE != 0 {
			state, ok := goByteSlice(hit.state.data, hit.state.size)
			if !ok {
				return nil, newBindingError(ErrNative, "native queried feature state is invalid")
			}
			item.State = state
		}
		features[i] = item
	}
	return features, nil
}

// QueryFeatureExtensions queries a feature extension from the latest render
// session state.
//
// feature contains one UTF-8 GeoJSON Feature. arguments, when present, contains
// a UTF-8 JSON object. The result contains either JSON or GeoJSON bytes.
func (session *RenderSessionHandle) QueryFeatureExtensions(sourceID string, feature []byte, extension string, extensionField string, arguments []byte) ([]byte, error) {
	ptr, release, err := session.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer session.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	extensionView := newCStringView(extension)
	defer extensionView.free()
	extensionFieldView := newCStringView(extensionField)
	defer extensionFieldView.free()
	featureView := newCBufferView(feature)
	defer featureView.free()
	var argumentsView cBufferView
	var rawArguments *C.mln_buffer_view
	if arguments != nil {
		argumentsView = newCBufferView(arguments)
		defer argumentsView.free()
		rawArguments = argumentsView.ptr()
	}
	var result C.mln_buffer
	if err := session.withNoAcquiredFrame(func() error {
		return checkNative(func() int32 {
			return int32(C.mln_render_session_query_feature_extensions(
				C.mln_render_session(ptr),
				sourceView.raw(),
				featureView.raw(),
				extensionView.raw(),
				extensionFieldView.raw(),
				rawArguments,
				&result,
			))
		})
	}); err != nil {
		return nil, err
	}
	return goOwnedBuffer(result)
}
