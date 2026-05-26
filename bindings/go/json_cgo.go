package maplibre

/*
#include <stdlib.h>

#include "internal/cgo_json_shim.h"
*/
import "C"

import (
	"fmt"
	"math"
	"unsafe"
)

type cJSONMaterializer struct {
	allocations []unsafe.Pointer
}

func newCJSONMaterializer() *cJSONMaterializer {
	return &cJSONMaterializer{}
}

func (materializer *cJSONMaterializer) free() {
	for i := len(materializer.allocations) - 1; i >= 0; i-- {
		C.free(materializer.allocations[i])
	}
}

func (materializer *cJSONMaterializer) alloc(size C.size_t) unsafe.Pointer {
	ptr := C.malloc(size)
	materializer.allocations = append(materializer.allocations, ptr)
	return ptr
}

func (materializer *cJSONMaterializer) stringView(value string) C.mln_string_view {
	if len(value) == 0 {
		return C.mln_string_view{}
	}
	ptr := C.CBytes([]byte(value))
	materializer.allocations = append(materializer.allocations, ptr)
	return C.mln_string_view{data: (*C.char)(ptr), size: C.size_t(len(value))}
}

func (materializer *cJSONMaterializer) value(value any) (C.mln_json_value, error) {
	switch typed := value.(type) {
	case nil:
		return C.mln_go_json_null(), nil
	case bool:
		return C.mln_go_json_bool(C.bool(typed)), nil
	case string:
		return C.mln_go_json_string(materializer.stringView(typed)), nil
	case int:
		return C.mln_go_json_int(C.int64_t(typed)), nil
	case int8:
		return C.mln_go_json_int(C.int64_t(typed)), nil
	case int16:
		return C.mln_go_json_int(C.int64_t(typed)), nil
	case int32:
		return C.mln_go_json_int(C.int64_t(typed)), nil
	case int64:
		return C.mln_go_json_int(C.int64_t(typed)), nil
	case uint:
		return C.mln_go_json_uint(C.uint64_t(typed)), nil
	case uint8:
		return C.mln_go_json_uint(C.uint64_t(typed)), nil
	case uint16:
		return C.mln_go_json_uint(C.uint64_t(typed)), nil
	case uint32:
		return C.mln_go_json_uint(C.uint64_t(typed)), nil
	case uint64:
		return C.mln_go_json_uint(C.uint64_t(typed)), nil
	case float32:
		return materializer.float(float64(typed))
	case float64:
		return materializer.float(typed)
	case []any:
		return materializer.array(typed)
	case []string:
		values := make([]any, len(typed))
		for i, item := range typed {
			values[i] = item
		}
		return materializer.array(values)
	case map[string]any:
		return materializer.object(typed)
	default:
		return C.mln_json_value{}, fmt.Errorf("unsupported JSON value type %T", value)
	}
}

func (materializer *cJSONMaterializer) float(value float64) (C.mln_json_value, error) {
	if math.IsNaN(value) || math.IsInf(value, 0) {
		return C.mln_json_value{}, fmt.Errorf("JSON double value must be finite")
	}
	return C.mln_go_json_double(C.double(value)), nil
}

func (materializer *cJSONMaterializer) array(values []any) (C.mln_json_value, error) {
	if len(values) == 0 {
		return C.mln_go_json_array(nil, 0), nil
	}
	rawValues := (*C.mln_json_value)(materializer.alloc(C.size_t(len(values)) * C.size_t(unsafe.Sizeof(C.mln_json_value{}))))
	for i, item := range values {
		rawValue, err := materializer.value(item)
		if err != nil {
			return C.mln_json_value{}, err
		}
		*(*C.mln_json_value)(unsafe.Add(unsafe.Pointer(rawValues), uintptr(i)*unsafe.Sizeof(C.mln_json_value{}))) = rawValue
	}
	return C.mln_go_json_array(rawValues, C.size_t(len(values))), nil
}

func (materializer *cJSONMaterializer) object(members map[string]any) (C.mln_json_value, error) {
	rawMembers, count, err := materializer.members(members)
	if err != nil {
		return C.mln_json_value{}, err
	}
	return C.mln_go_json_object(rawMembers, count), nil
}

func (materializer *cJSONMaterializer) members(members map[string]any) (*C.mln_json_member, C.size_t, error) {
	if len(members) == 0 {
		return nil, 0, nil
	}
	rawMembers := (*C.mln_json_member)(materializer.alloc(C.size_t(len(members)) * C.size_t(unsafe.Sizeof(C.mln_json_member{}))))
	i := 0
	for key, item := range members {
		rawValue, err := materializer.value(item)
		if err != nil {
			return nil, 0, err
		}
		valuePtr := (*C.mln_json_value)(materializer.alloc(C.size_t(unsafe.Sizeof(C.mln_json_value{}))))
		*valuePtr = rawValue
		*(*C.mln_json_member)(unsafe.Add(unsafe.Pointer(rawMembers), uintptr(i)*unsafe.Sizeof(C.mln_json_member{}))) = C.mln_go_json_member(materializer.stringView(key), valuePtr)
		i++
	}
	return rawMembers, C.size_t(len(members)), nil
}

type cGeoJSONMaterializer struct {
	json     *cJSONMaterializer
	geometry *cGeometryMaterializer
}

func newCGeoJSONMaterializer() *cGeoJSONMaterializer {
	return &cGeoJSONMaterializer{json: newCJSONMaterializer(), geometry: newCGeometryMaterializer()}
}

func (materializer *cGeoJSONMaterializer) free() {
	materializer.geometry.free()
	materializer.json.free()
}

func (materializer *cGeoJSONMaterializer) alloc(size C.size_t) unsafe.Pointer {
	return materializer.json.alloc(size)
}

func (materializer *cGeoJSONMaterializer) geoJSON(data GeoJSON) (C.mln_geojson, error) {
	switch data.Type {
	case GeoJSONTypeGeometry:
		geometry, err := materializer.geometry.geometryPtr(data.Geometry)
		if err != nil {
			return C.mln_geojson{}, err
		}
		return C.mln_go_geojson_geometry(geometry), nil
	case GeoJSONTypeFeature:
		feature, err := materializer.featurePtr(data.Feature)
		if err != nil {
			return C.mln_geojson{}, err
		}
		return C.mln_go_geojson_feature(feature), nil
	case GeoJSONTypeFeatureCollection:
		features, err := materializer.features(data.Features)
		if err != nil {
			return C.mln_geojson{}, err
		}
		return C.mln_go_geojson_feature_collection(features, C.size_t(len(data.Features))), nil
	default:
		return C.mln_geojson{}, fmt.Errorf("unsupported GeoJSON type %d", data.Type)
	}
}

func (materializer *cGeoJSONMaterializer) featurePtr(feature Feature) (*C.mln_feature, error) {
	raw, err := materializer.feature(feature)
	if err != nil {
		return nil, err
	}
	ptr := (*C.mln_feature)(materializer.alloc(C.size_t(unsafe.Sizeof(C.mln_feature{}))))
	*ptr = raw
	return ptr, nil
}

func (materializer *cGeoJSONMaterializer) feature(feature Feature) (C.mln_feature, error) {
	geometry, err := materializer.geometry.geometryPtr(feature.Geometry)
	if err != nil {
		return C.mln_feature{}, err
	}
	properties, propertyCount, err := materializer.json.members(feature.Properties)
	if err != nil {
		return C.mln_feature{}, err
	}
	switch id := feature.Identifier.(type) {
	case nil:
		return C.mln_go_feature_null(geometry, properties, propertyCount), nil
	case uint:
		return C.mln_go_feature_uint(geometry, properties, propertyCount, C.uint64_t(id)), nil
	case uint8:
		return C.mln_go_feature_uint(geometry, properties, propertyCount, C.uint64_t(id)), nil
	case uint16:
		return C.mln_go_feature_uint(geometry, properties, propertyCount, C.uint64_t(id)), nil
	case uint32:
		return C.mln_go_feature_uint(geometry, properties, propertyCount, C.uint64_t(id)), nil
	case uint64:
		return C.mln_go_feature_uint(geometry, properties, propertyCount, C.uint64_t(id)), nil
	case int:
		return C.mln_go_feature_int(geometry, properties, propertyCount, C.int64_t(id)), nil
	case int8:
		return C.mln_go_feature_int(geometry, properties, propertyCount, C.int64_t(id)), nil
	case int16:
		return C.mln_go_feature_int(geometry, properties, propertyCount, C.int64_t(id)), nil
	case int32:
		return C.mln_go_feature_int(geometry, properties, propertyCount, C.int64_t(id)), nil
	case int64:
		return C.mln_go_feature_int(geometry, properties, propertyCount, C.int64_t(id)), nil
	case float32:
		value := float64(id)
		if math.IsNaN(value) || math.IsInf(value, 0) {
			return C.mln_feature{}, fmt.Errorf("feature identifier float must be finite")
		}
		return C.mln_go_feature_double(geometry, properties, propertyCount, C.double(value)), nil
	case float64:
		if math.IsNaN(id) || math.IsInf(id, 0) {
			return C.mln_feature{}, fmt.Errorf("feature identifier float must be finite")
		}
		return C.mln_go_feature_double(geometry, properties, propertyCount, C.double(id)), nil
	case string:
		return C.mln_go_feature_string(geometry, properties, propertyCount, materializer.json.stringView(id)), nil
	default:
		return C.mln_feature{}, fmt.Errorf("unsupported feature identifier type %T", feature.Identifier)
	}
}

func (materializer *cGeoJSONMaterializer) features(features []Feature) (*C.mln_feature, error) {
	if len(features) == 0 {
		return nil, nil
	}
	rawFeatures := (*C.mln_feature)(materializer.alloc(C.size_t(len(features)) * C.size_t(unsafe.Sizeof(C.mln_feature{}))))
	for i, feature := range features {
		rawFeature, err := materializer.feature(feature)
		if err != nil {
			return nil, err
		}
		*(*C.mln_feature)(unsafe.Add(unsafe.Pointer(rawFeatures), uintptr(i)*unsafe.Sizeof(C.mln_feature{}))) = rawFeature
	}
	return rawFeatures, nil
}

func cJSONSnapshotValue(snapshot *C.mln_json_snapshot) (any, error) {
	defer C.mln_json_snapshot_destroy(snapshot)
	var rawValue *C.mln_json_value
	if err := checkNative(func() int32 {
		return int32(C.mln_json_snapshot_get(snapshot, (**C.mln_json_value)(unsafe.Pointer(&rawValue))))
	}); err != nil {
		return nil, err
	}
	value, err := cJSONValue(rawValue)
	if err != nil {
		return nil, newBindingError(ErrNative, err.Error())
	}
	return value, nil
}

func cJSONValue(value *C.mln_json_value) (any, error) {
	if value == nil {
		return nil, nil
	}
	switch uint32(C.mln_go_json_type(value)) {
	case uint32(C.MLN_JSON_VALUE_TYPE_NULL):
		return nil, nil
	case uint32(C.MLN_JSON_VALUE_TYPE_BOOL):
		return bool(C.mln_go_json_bool_value(value)), nil
	case uint32(C.MLN_JSON_VALUE_TYPE_UINT):
		return uint64(C.mln_go_json_uint_value(value)), nil
	case uint32(C.MLN_JSON_VALUE_TYPE_INT):
		return int64(C.mln_go_json_int_value(value)), nil
	case uint32(C.MLN_JSON_VALUE_TYPE_DOUBLE):
		return float64(C.mln_go_json_double_value(value)), nil
	case uint32(C.MLN_JSON_VALUE_TYPE_STRING):
		return goStringView(C.mln_go_json_string_value(value)), nil
	case uint32(C.MLN_JSON_VALUE_TYPE_ARRAY):
		count := int(C.mln_go_json_array_count(value))
		items := make([]any, count)
		for i := range items {
			item, err := cJSONValue((*C.mln_json_value)(unsafe.Pointer(C.mln_go_json_array_get(value, C.size_t(i)))))
			if err != nil {
				return nil, err
			}
			items[i] = item
		}
		return items, nil
	case uint32(C.MLN_JSON_VALUE_TYPE_OBJECT):
		count := int(C.mln_go_json_object_count(value))
		members := make(map[string]any, count)
		for i := 0; i < count; i++ {
			item, err := cJSONValue((*C.mln_json_value)(unsafe.Pointer(C.mln_go_json_object_value(value, C.size_t(i)))))
			if err != nil {
				return nil, err
			}
			members[goStringView(C.mln_go_json_object_key(value, C.size_t(i)))] = item
		}
		return members, nil
	default:
		return nil, fmt.Errorf("unknown JSON value type %d", uint32(C.mln_go_json_type(value)))
	}
}
