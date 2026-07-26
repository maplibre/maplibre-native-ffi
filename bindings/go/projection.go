package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

import (
	"unsafe"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

type nativeProjection struct{}

// MapProjectionHandle owns a standalone projection snapshot.
type MapProjectionHandle struct {
	state *handle.State[nativeProjection]
}

// NewProjection creates a standalone projection helper from this map's current
// transform. Later map changes do not update the helper.
func (m *MapHandle) NewProjection() (*MapProjectionHandle, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer m.state.KeepAlive()

	var projection *nativeProjection
	if err := checkNative(func() int32 {
		var raw *C.mln_map_projection
		status := int32(C.mln_map_projection_create((*C.mln_map)(unsafe.Pointer(ptr)), &raw))
		if status == int32(C.MLN_STATUS_OK) {
			projection = (*nativeProjection)(unsafe.Pointer(raw))
		}
		return status
	}); err != nil {
		return nil, err
	}
	state, err := handle.New(projection, "MapProjectionHandle")
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	return &MapProjectionHandle{state: state}, nil
}

func (projection *MapProjectionHandle) ptr() (*nativeProjection, func(), error) {
	if projection == nil || projection.state == nil {
		return nil, nil, newBindingError(ErrInvalidArgument, "MapProjectionHandle is nil")
	}
	borrow, live := projection.state.Borrow()
	if !live {
		return nil, nil, newBindingError(ErrInvalidArgument, "MapProjectionHandle is closed")
	}
	return borrow.Ptr(), borrow.Release, nil
}

// Close destroys this projection helper. A successful close makes later calls
// no-ops. A failed close leaves the native handle live so callers can retry on
// the owner thread.
func (projection *MapProjectionHandle) Close() error {
	if projection == nil || projection.state == nil {
		return newBindingError(ErrInvalidArgument, "MapProjectionHandle is nil")
	}
	return checkNative(func() int32 {
		return projection.state.Close(func(ptr *nativeProjection) int32 {
			return int32(C.mln_map_projection_destroy((*C.mln_map_projection)(unsafe.Pointer(ptr))))
		})
	})
}

// Camera returns this projection helper's camera snapshot.
func (projection *MapProjectionHandle) Camera() (CameraOptions, error) {
	ptr, release, err := projection.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer release()
	defer projection.state.KeepAlive()

	var camera C.mln_camera_options = C.mln_camera_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_get_camera((*C.mln_map_projection)(unsafe.Pointer(ptr)), &camera))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(camera), nil
}

// SetCamera applies selected camera fields to this projection helper.
func (projection *MapProjectionHandle) SetCamera(camera CameraOptions) error {
	ptr, release, err := projection.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer projection.state.KeepAlive()

	rawCamera := cCameraOptions(camera)
	return checkNative(func() int32 {
		return int32(C.mln_map_projection_set_camera((*C.mln_map_projection)(unsafe.Pointer(ptr)), &rawCamera))
	})
}

// SetVisibleCoordinates updates this projection helper's camera to fit
// coordinates inside padding.
func (projection *MapProjectionHandle) SetVisibleCoordinates(coordinates []LatLng, padding EdgeInsets) error {
	ptr, release, err := projection.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer projection.state.KeepAlive()

	rawCoordinates := cLatLngSlice(coordinates)
	var rawCoordinatesPtr *C.mln_lat_lng
	if len(rawCoordinates) > 0 {
		rawCoordinatesPtr = &rawCoordinates[0]
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_projection_set_visible_coordinates(
			(*C.mln_map_projection)(unsafe.Pointer(ptr)),
			rawCoordinatesPtr,
			C.size_t(len(rawCoordinates)),
			cEdgeInsets(padding),
		))
	})
}

// SetVisibleGeometry updates this projection helper's camera to fit geometry
// inside padding.
func (projection *MapProjectionHandle) SetVisibleGeometry(geometry Geometry, padding EdgeInsets) error {
	ptr, release, err := projection.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer projection.state.KeepAlive()
	materializer := newCGeometryMaterializer()
	defer materializer.free()
	rawGeometry, materialErr := materializer.geometryPtr(geometry)
	if materialErr != nil {
		return newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_projection_set_visible_geometry(
			(*C.mln_map_projection)(unsafe.Pointer(ptr)),
			rawGeometry,
			cEdgeInsets(padding),
		))
	})
}

// PixelForLatLng converts a geographic coordinate to a logical screen point.
func (projection *MapProjectionHandle) PixelForLatLng(coordinate LatLng) (ScreenPoint, error) {
	ptr, release, err := projection.ptr()
	if err != nil {
		return ScreenPoint{}, err
	}
	defer release()
	defer projection.state.KeepAlive()

	var point C.mln_screen_point
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_pixel_for_lat_lng(
			(*C.mln_map_projection)(unsafe.Pointer(ptr)),
			cLatLng(coordinate),
			&point,
		))
	}); err != nil {
		return ScreenPoint{}, err
	}
	return goScreenPoint(point), nil
}

// LatLngForPixel converts a logical screen point to a geographic coordinate.
func (projection *MapProjectionHandle) LatLngForPixel(point ScreenPoint) (LatLng, error) {
	ptr, release, err := projection.ptr()
	if err != nil {
		return LatLng{}, err
	}
	defer release()
	defer projection.state.KeepAlive()

	var coordinate C.mln_lat_lng
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_lat_lng_for_pixel(
			(*C.mln_map_projection)(unsafe.Pointer(ptr)),
			cScreenPoint(point),
			&coordinate,
		))
	}); err != nil {
		return LatLng{}, err
	}
	return goLatLng(coordinate), nil
}

// ProjectedMetersForLatLng converts a geographic coordinate to Spherical
// Mercator projected meters.
func ProjectedMetersForLatLng(coordinate LatLng) (ProjectedMeters, error) {
	var meters C.mln_projected_meters
	if err := checkNative(func() int32 {
		return int32(C.mln_projected_meters_for_lat_lng(cLatLng(coordinate), &meters))
	}); err != nil {
		return ProjectedMeters{}, err
	}
	return goProjectedMeters(meters), nil
}

// LatLngForProjectedMeters converts Spherical Mercator projected meters to a
// geographic coordinate.
func LatLngForProjectedMeters(meters ProjectedMeters) (LatLng, error) {
	var coordinate C.mln_lat_lng
	if err := checkNative(func() int32 {
		return int32(C.mln_lat_lng_for_projected_meters(cProjectedMeters(meters), &coordinate))
	}); err != nil {
		return LatLng{}, err
	}
	return goLatLng(coordinate), nil
}
