package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"

type nativeProjection uint64

// MapProjectionHandle owns an any-thread standalone projection snapshot. Every
// call after creation is synchronous, runs on the calling goroutine, and is
// internally serialized, so a projection is safe to share across goroutines. A
// projection never observes map changes made after its creation.
type MapProjectionHandle struct {
	state *handle.State[nativeProjection]
}

// NewProjection creates a standalone projection helper that copies this map's
// transform state after every earlier map command. Later map changes do not
// update the helper.
func (m *MapHandle) NewProjection() (*Future[*MapProjectionHandle], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_projection_create(raw, completion))
	}, func(result *C.mln_completion_result) (*MapProjectionHandle, error) {
		raw, err := completionValue[C.mln_map_projection](result)
		if err != nil {
			return nil, err
		}
		state, err := handle.New(nativeProjection(raw), "MapProjectionHandle")
		if err != nil {
			return nil, newBindingError(ErrInvalidArgument, err.Error())
		}
		return &MapProjectionHandle{state: state}, nil
	})
}

func (projection *MapProjectionHandle) ptr() (nativeProjection, error) {
	if projection == nil || projection.state == nil {
		return 0, newBindingError(ErrInvalidArgument, "MapProjectionHandle is nil")
	}
	value, live := projection.state.Handle()
	if !live {
		return 0, newBindingError(ErrInvalidArgument, "MapProjectionHandle is closed")
	}
	return value, nil
}

// Close destroys this projection helper synchronously. A successful close
// makes later calls fail. A failed close leaves the native handle live so
// callers can retry.
func (projection *MapProjectionHandle) Close() error {
	if projection == nil || projection.state == nil {
		return newBindingError(ErrInvalidArgument, "MapProjectionHandle is nil")
	}
	return projection.state.Close(func(native nativeProjection) error {
		return checkNative(func() int32 {
			return int32(C.mln_map_projection_close(C.mln_map_projection(native)))
		})
	})
}

// Camera returns a copy of the projection camera. The result observes every
// earlier projection setter.
func (projection *MapProjectionHandle) Camera() (CameraOptions, error) {
	ptr, err := projection.ptr()
	if err != nil {
		return CameraOptions{}, err
	}

	defer projection.state.KeepAlive()
	camera := C.mln_camera_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_get_camera(C.mln_map_projection(ptr), &camera))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(camera), nil
}

// SetCamera applies selected camera fields synchronously, so a later read or
// conversion observes them. The map's camera is unaffected.
func (projection *MapProjectionHandle) SetCamera(camera CameraOptions) error {
	ptr, err := projection.ptr()
	if err != nil {
		return err
	}

	defer projection.state.KeepAlive()
	raw := cCameraOptions(camera)
	return checkNative(func() int32 {
		return int32(C.mln_map_projection_set_camera(C.mln_map_projection(ptr), &raw))
	})
}

// SetVisibleCoordinates applies a camera fitted to geographic coordinates
// synchronously, so a later read or conversion observes it.
func (projection *MapProjectionHandle) SetVisibleCoordinates(coordinates []LatLng, padding EdgeInsets) error {
	ptr, err := projection.ptr()
	if err != nil {
		return err
	}

	defer projection.state.KeepAlive()
	raw := cLatLngSlice(coordinates)
	var data *C.mln_lat_lng
	if len(raw) > 0 {
		data = &raw[0]
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_projection_set_visible_coordinates(C.mln_map_projection(ptr), data, C.size_t(len(raw)), cEdgeInsets(padding)))
	})
}

// SetVisibleGeometry applies a camera fitted to GeoJSON Geometry bytes
// synchronously, so a later read or conversion observes it.
func (projection *MapProjectionHandle) SetVisibleGeometry(geometry []byte, padding EdgeInsets) error {
	ptr, err := projection.ptr()
	if err != nil {
		return err
	}

	defer projection.state.KeepAlive()
	raw := newCBufferView(geometry)
	defer raw.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_projection_set_visible_geometry(C.mln_map_projection(ptr), raw.raw(), cEdgeInsets(padding)))
	})
}

// PixelForLatLng converts a geographic coordinate to a logical screen point.
// The result observes every earlier projection setter.
func (projection *MapProjectionHandle) PixelForLatLng(coordinate LatLng) (ScreenPoint, error) {
	ptr, err := projection.ptr()
	if err != nil {
		return ScreenPoint{}, err
	}

	defer projection.state.KeepAlive()
	var point C.mln_screen_point
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_pixel_for_lat_lng(C.mln_map_projection(ptr), cLatLng(coordinate), &point))
	}); err != nil {
		return ScreenPoint{}, err
	}
	return goScreenPoint(point), nil
}

// LatLngForPixel converts a logical screen point to a geographic coordinate.
// The result observes every earlier projection setter.
func (projection *MapProjectionHandle) LatLngForPixel(point ScreenPoint) (LatLng, error) {
	ptr, err := projection.ptr()
	if err != nil {
		return LatLng{}, err
	}

	defer projection.state.KeepAlive()
	var coordinate C.mln_lat_lng
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_lat_lng_for_pixel(C.mln_map_projection(ptr), cScreenPoint(point), &coordinate))
	}); err != nil {
		return LatLng{}, err
	}
	return goLatLng(coordinate), nil
}

// LatLngForPixelUnwrapped converts a logical screen point to an unwrapped
// geographic coordinate. The longitude preserves the visible world copy. The
// result observes every earlier projection setter.
func (projection *MapProjectionHandle) LatLngForPixelUnwrapped(point ScreenPoint) (LatLng, error) {
	ptr, err := projection.ptr()
	if err != nil {
		return LatLng{}, err
	}

	defer projection.state.KeepAlive()
	var coordinate C.mln_lat_lng
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_lat_lng_for_pixel_unwrapped(
			C.mln_map_projection(ptr),
			cScreenPoint(point),
			&coordinate,
		))
	}); err != nil {
		return LatLng{}, err
	}
	return goLatLng(coordinate), nil
}

// ProjectedMetersForLatLng converts a geographic coordinate to Spherical Mercator projected meters.
func ProjectedMetersForLatLng(coordinate LatLng) (ProjectedMeters, error) {
	var meters C.mln_projected_meters
	if err := checkNative(func() int32 { return int32(C.mln_projected_meters_for_lat_lng(cLatLng(coordinate), &meters)) }); err != nil {
		return ProjectedMeters{}, err
	}
	return goProjectedMeters(meters), nil
}

// LatLngForProjectedMeters converts Spherical Mercator projected meters to a geographic coordinate.
func LatLngForProjectedMeters(meters ProjectedMeters) (LatLng, error) {
	var coordinate C.mln_lat_lng
	if err := checkNative(func() int32 { return int32(C.mln_lat_lng_for_projected_meters(cProjectedMeters(meters), &coordinate)) }); err != nil {
		return LatLng{}, err
	}
	return goLatLng(coordinate), nil
}
