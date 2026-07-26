package maplibre

/*
#include <stdlib.h>

#include "internal/cgo_geometry_shim.h"
*/
import "C"

import (
	"fmt"
	"unsafe"
)

type cGeometryMaterializer struct {
	allocations []unsafe.Pointer
}

func newCGeometryMaterializer() *cGeometryMaterializer {
	return &cGeometryMaterializer{}
}

func (materializer *cGeometryMaterializer) free() {
	for i := len(materializer.allocations) - 1; i >= 0; i-- {
		C.free(materializer.allocations[i])
	}
}

func (materializer *cGeometryMaterializer) alloc(size C.size_t) unsafe.Pointer {
	ptr := C.malloc(size)
	materializer.allocations = append(materializer.allocations, ptr)
	return ptr
}

func (materializer *cGeometryMaterializer) geometryPtr(geometry Geometry) (*C.mln_geometry, error) {
	raw, err := materializer.geometry(geometry)
	if err != nil {
		return nil, err
	}
	ptr := (*C.mln_geometry)(materializer.alloc(C.size_t(unsafe.Sizeof(C.mln_geometry{}))))
	*ptr = raw
	return ptr, nil
}

func (materializer *cGeometryMaterializer) geometry(geometry Geometry) (C.mln_geometry, error) {
	switch geometry.Type {
	case GeometryTypeEmpty:
		return C.mln_go_geometry_empty(), nil
	case GeometryTypePoint:
		return C.mln_go_geometry_point(cLatLng(geometry.Point)), nil
	case GeometryTypeLineString:
		return C.mln_go_geometry_line_string(materializer.coordinateSpan(geometry.Points)), nil
	case GeometryTypePolygon:
		rings := materializer.coordinateSpans(geometry.Lines)
		return C.mln_go_geometry_polygon(rings, C.size_t(len(geometry.Lines))), nil
	case GeometryTypeMultiPoint:
		return C.mln_go_geometry_multi_point(materializer.coordinateSpan(geometry.Points)), nil
	case GeometryTypeMultiLineString:
		lines := materializer.coordinateSpans(geometry.Lines)
		return C.mln_go_geometry_multi_line_string(lines, C.size_t(len(geometry.Lines))), nil
	case GeometryTypeMultiPolygon:
		polygons := materializer.polygonGeometries(geometry.Polygons)
		return C.mln_go_geometry_multi_polygon(polygons, C.size_t(len(geometry.Polygons))), nil
	case GeometryTypeGeometryCollection:
		geometries, err := materializer.geometries(geometry.Geometries)
		if err != nil {
			return C.mln_geometry{}, err
		}
		return C.mln_go_geometry_collection(geometries, C.size_t(len(geometry.Geometries))), nil
	default:
		return C.mln_geometry{}, fmt.Errorf("unsupported geometry type %d", geometry.Type)
	}
}

func (materializer *cGeometryMaterializer) coordinateSpan(points []LatLng) C.mln_coordinate_span {
	if len(points) == 0 {
		return C.mln_go_coordinate_span(nil, 0)
	}
	rawPoints := (*C.mln_lat_lng)(materializer.alloc(C.size_t(len(points)) * C.size_t(unsafe.Sizeof(C.mln_lat_lng{}))))
	for i, point := range points {
		*(*C.mln_lat_lng)(unsafe.Add(unsafe.Pointer(rawPoints), uintptr(i)*unsafe.Sizeof(C.mln_lat_lng{}))) = cLatLng(point)
	}
	return C.mln_go_coordinate_span(rawPoints, C.size_t(len(points)))
}

func (materializer *cGeometryMaterializer) coordinateSpans(lines [][]LatLng) *C.mln_coordinate_span {
	if len(lines) == 0 {
		return nil
	}
	rawLines := (*C.mln_coordinate_span)(materializer.alloc(C.size_t(len(lines)) * C.size_t(unsafe.Sizeof(C.mln_coordinate_span{}))))
	for i, line := range lines {
		*(*C.mln_coordinate_span)(unsafe.Add(unsafe.Pointer(rawLines), uintptr(i)*unsafe.Sizeof(C.mln_coordinate_span{}))) = materializer.coordinateSpan(line)
	}
	return rawLines
}

func (materializer *cGeometryMaterializer) polygonGeometries(polygons [][][]LatLng) *C.mln_polygon_geometry {
	if len(polygons) == 0 {
		return nil
	}
	rawPolygons := (*C.mln_polygon_geometry)(materializer.alloc(C.size_t(len(polygons)) * C.size_t(unsafe.Sizeof(C.mln_polygon_geometry{}))))
	for i, polygon := range polygons {
		rings := materializer.coordinateSpans(polygon)
		*(*C.mln_polygon_geometry)(unsafe.Add(unsafe.Pointer(rawPolygons), uintptr(i)*unsafe.Sizeof(C.mln_polygon_geometry{}))) = C.mln_go_polygon_geometry(rings, C.size_t(len(polygon)))
	}
	return rawPolygons
}

func (materializer *cGeometryMaterializer) geometries(geometries []Geometry) (*C.mln_geometry, error) {
	if len(geometries) == 0 {
		return nil, nil
	}
	rawGeometries := (*C.mln_geometry)(materializer.alloc(C.size_t(len(geometries)) * C.size_t(unsafe.Sizeof(C.mln_geometry{}))))
	for i, geometry := range geometries {
		rawGeometry, err := materializer.geometry(geometry)
		if err != nil {
			return nil, err
		}
		*(*C.mln_geometry)(unsafe.Add(unsafe.Pointer(rawGeometries), uintptr(i)*unsafe.Sizeof(C.mln_geometry{}))) = rawGeometry
	}
	return rawGeometries, nil
}
