package maplibre

/*
#include <stdlib.h>

#include "maplibre_native_c.h"
*/
import "C"

import "unsafe"

type cStringView struct {
	data unsafe.Pointer
	size int
}

func newCStringView(value string) cStringView {
	if len(value) == 0 {
		return cStringView{}
	}
	return cStringView{data: C.CBytes([]byte(value)), size: len(value)}
}

func (view cStringView) raw() C.mln_string_view {
	return C.mln_string_view{data: (*C.char)(view.data), size: C.size_t(view.size)}
}

func (view cStringView) free() {
	if view.data != nil {
		C.free(view.data)
	}
}

type cStringViewArray struct {
	views      []cStringView
	raw        unsafe.Pointer
	valueCount int
}

func newCStringViewArray(values []string) cStringViewArray {
	out := cStringViewArray{
		views:      make([]cStringView, len(values)),
		valueCount: len(values),
	}
	if len(values) > 0 {
		out.raw = C.malloc(C.size_t(len(values)) * C.size_t(unsafe.Sizeof(C.mln_string_view{})))
	}
	for i, value := range values {
		view := newCStringView(value)
		out.views[i] = view
		*(*C.mln_string_view)(unsafe.Add(out.raw, uintptr(i)*unsafe.Sizeof(C.mln_string_view{}))) = view.raw()
	}
	return out
}

func (array cStringViewArray) ptr() *C.mln_string_view {
	if array.raw == nil {
		return nil
	}
	return (*C.mln_string_view)(array.raw)
}

func (array cStringViewArray) count() C.size_t {
	return C.size_t(array.valueCount)
}

func (array cStringViewArray) free() {
	for i := range array.views {
		array.views[i].free()
	}
	if array.raw != nil {
		C.free(array.raw)
	}
}

func goStringView(view C.mln_string_view) string {
	return goCharBytes(view.data, view.size)
}

func goCharBytes(data *C.char, size C.size_t) string {
	bytes, ok := goByteSlice(unsafe.Pointer(data), size)
	if !ok {
		return ""
	}
	return string(bytes)
}

func goByteSlice(data unsafe.Pointer, size C.size_t) ([]byte, bool) {
	if size == 0 {
		return nil, true
	}
	if data == nil {
		return nil, false
	}
	n := uintptr(size)
	if n > uintptr(int(^uint(0)>>1)) {
		return nil, false
	}
	return append([]byte(nil), unsafe.Slice((*byte)(data), int(n))...), true
}

func cLatLng(coordinate LatLng) C.mln_lat_lng {
	return C.mln_lat_lng{
		latitude:  C.double(coordinate.Latitude),
		longitude: C.double(coordinate.Longitude),
	}
}

func cCanonicalTileID(tileID CanonicalTileID) C.mln_canonical_tile_id {
	return C.mln_canonical_tile_id{z: C.uint32_t(tileID.Z), x: C.uint32_t(tileID.X), y: C.uint32_t(tileID.Y)}
}

func goLatLng(coordinate C.mln_lat_lng) LatLng {
	return LatLng{
		Latitude:  float64(coordinate.latitude),
		Longitude: float64(coordinate.longitude),
	}
}

func cLatLngSlice(coordinates []LatLng) []C.mln_lat_lng {
	out := make([]C.mln_lat_lng, len(coordinates))
	for i, coordinate := range coordinates {
		out[i] = cLatLng(coordinate)
	}
	return out
}

func goLatLngSlice(coordinates []C.mln_lat_lng) []LatLng {
	out := make([]LatLng, len(coordinates))
	for i, coordinate := range coordinates {
		out[i] = goLatLng(coordinate)
	}
	return out
}

func cScreenPoint(point ScreenPoint) C.mln_screen_point {
	return C.mln_screen_point{
		x: C.double(point.X),
		y: C.double(point.Y),
	}
}

func goScreenPoint(point C.mln_screen_point) ScreenPoint {
	return ScreenPoint{
		X: float64(point.x),
		Y: float64(point.y),
	}
}

func cScreenPointSlice(points []ScreenPoint) []C.mln_screen_point {
	out := make([]C.mln_screen_point, len(points))
	for i, point := range points {
		out[i] = cScreenPoint(point)
	}
	return out
}

func goScreenPointSlice(points []C.mln_screen_point) []ScreenPoint {
	out := make([]ScreenPoint, len(points))
	for i, point := range points {
		out[i] = goScreenPoint(point)
	}
	return out
}

func cEdgeInsets(insets EdgeInsets) C.mln_edge_insets {
	return C.mln_edge_insets{
		top:    C.double(insets.Top),
		left:   C.double(insets.Left),
		bottom: C.double(insets.Bottom),
		right:  C.double(insets.Right),
	}
}

func goEdgeInsets(insets C.mln_edge_insets) EdgeInsets {
	return EdgeInsets{
		Top:    float64(insets.top),
		Left:   float64(insets.left),
		Bottom: float64(insets.bottom),
		Right:  float64(insets.right),
	}
}

func cLatLngBounds(bounds LatLngBounds) C.mln_lat_lng_bounds {
	return C.mln_lat_lng_bounds{
		southwest: cLatLng(bounds.Southwest),
		northeast: cLatLng(bounds.Northeast),
	}
}

func goLatLngBounds(bounds C.mln_lat_lng_bounds) LatLngBounds {
	return LatLngBounds{
		Southwest: goLatLng(bounds.southwest),
		Northeast: goLatLng(bounds.northeast),
	}
}

func cProjectedMeters(meters ProjectedMeters) C.mln_projected_meters {
	return C.mln_projected_meters{
		northing: C.double(meters.Northing),
		easting:  C.double(meters.Easting),
	}
}

func goProjectedMeters(meters C.mln_projected_meters) ProjectedMeters {
	return ProjectedMeters{
		Northing: float64(meters.northing),
		Easting:  float64(meters.easting),
	}
}

func cVec3(vector Vec3) C.mln_vec3 {
	return C.mln_vec3{
		x: C.double(vector.X),
		y: C.double(vector.Y),
		z: C.double(vector.Z),
	}
}

func goVec3(vector C.mln_vec3) Vec3 {
	return Vec3{
		X: float64(vector.x),
		Y: float64(vector.y),
		Z: float64(vector.z),
	}
}

func cQuaternion(quaternion Quaternion) C.mln_quaternion {
	return C.mln_quaternion{
		x: C.double(quaternion.X),
		y: C.double(quaternion.Y),
		z: C.double(quaternion.Z),
		w: C.double(quaternion.W),
	}
}

func goQuaternion(quaternion C.mln_quaternion) Quaternion {
	return Quaternion{
		X: float64(quaternion.x),
		Y: float64(quaternion.y),
		Z: float64(quaternion.z),
		W: float64(quaternion.w),
	}
}
