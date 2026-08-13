package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"

type nativeProjection uint64

// MapProjectionHandle owns an any-thread standalone projection snapshot.
type MapProjectionHandle struct {
	state *handle.State[nativeProjection]
}

// NewProjection creates a standalone projection helper from this map's ordered
// transform state. Later map changes do not update the helper.
func (m *MapHandle) NewProjection() (*MapProjectionHandle, error) {
	ptr, release, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer m.state.KeepAlive()
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_projection_create_start(C.mln_map(ptr), out))
	})
	if err != nil {
		return nil, err
	}
	defer C.mln_operation_release(operation)
	var raw C.mln_map_projection
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_create_take_result(operation, &raw))
	}); err != nil {
		return nil, err
	}
	state, err := handle.New(nativeProjection(raw), "MapProjectionHandle")
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	return &MapProjectionHandle{state: state}, nil
}

func (projection *MapProjectionHandle) ptr() (nativeProjection, func(), error) {
	if projection == nil || projection.state == nil {
		return 0, nil, newBindingError(ErrInvalidArgument, "MapProjectionHandle is nil")
	}
	borrow, live := projection.state.Borrow()
	if !live {
		return 0, nil, newBindingError(ErrInvalidArgument, "MapProjectionHandle is closed")
	}
	return borrow.Handle(), borrow.Release, nil
}

// Close waits for this projection helper's native close operation.
func (projection *MapProjectionHandle) Close() error {
	if projection == nil || projection.state == nil {
		return newBindingError(ErrInvalidArgument, "MapProjectionHandle is nil")
	}
	var closeErr error
	_, err := projection.state.CloseChecked(func(native nativeProjection) int32 {
		var operation C.mln_operation
		if err := checkNative(func() int32 {
			return int32(C.mln_map_projection_close_start(C.mln_map_projection(native), &operation))
		}); err != nil {
			closeErr = err
			return statusFromError(err)
		}
		defer C.mln_operation_release(operation)
		if err := waitNativeOperation(operation); err != nil {
			closeErr = err
			return statusFromError(err)
		}
		return int32(C.MLN_STATUS_OK)
	})
	if err != nil {
		return newBindingError(ErrInvalidState, err.Error())
	}
	return closeErr
}

// Camera returns an ordered camera snapshot.
func (projection *MapProjectionHandle) Camera() (CameraOptions, error) {
	ptr, release, err := projection.ptr()
	if err != nil {
		return CameraOptions{}, err
	}
	defer release()
	defer projection.state.KeepAlive()
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_projection_get_camera_start(C.mln_map_projection(ptr), out))
	})
	if err != nil {
		return CameraOptions{}, err
	}
	defer C.mln_operation_release(operation)
	camera := C.mln_camera_options_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_get_camera_take_result(operation, &camera))
	}); err != nil {
		return CameraOptions{}, err
	}
	return goCameraOptions(camera), nil
}

// SetCamera submits selected camera fields and returns the command ID.
func (projection *MapProjectionHandle) SetCamera(camera CameraOptions) (uint64, error) {
	ptr, release, err := projection.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer projection.state.KeepAlive()
	raw := cCameraOptions(camera)
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_set_camera(C.mln_map_projection(ptr), &raw, &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// SetVisibleCoordinates submits a fitted-camera command and returns its ID.
func (projection *MapProjectionHandle) SetVisibleCoordinates(coordinates []LatLng, padding EdgeInsets) (uint64, error) {
	ptr, release, err := projection.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer projection.state.KeepAlive()
	raw := cLatLngSlice(coordinates)
	var data *C.mln_lat_lng
	if len(raw) > 0 {
		data = &raw[0]
	}
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_set_visible_coordinates(C.mln_map_projection(ptr), data, C.size_t(len(raw)), cEdgeInsets(padding), &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// SetVisibleGeometry submits a fitted-camera command and returns its ID.
func (projection *MapProjectionHandle) SetVisibleGeometry(geometry []byte, padding EdgeInsets) (uint64, error) {
	ptr, release, err := projection.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer projection.state.KeepAlive()
	raw := newCBufferView(geometry)
	defer raw.free()
	var commandID C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_set_visible_geometry(C.mln_map_projection(ptr), raw.raw(), cEdgeInsets(padding), &commandID))
	}); err != nil {
		return 0, err
	}
	return uint64(commandID), nil
}

// PixelForLatLng performs an ordered coordinate conversion.
func (projection *MapProjectionHandle) PixelForLatLng(coordinate LatLng) (ScreenPoint, error) {
	ptr, release, err := projection.ptr()
	if err != nil {
		return ScreenPoint{}, err
	}
	defer release()
	defer projection.state.KeepAlive()
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_projection_pixel_for_lat_lng_start(C.mln_map_projection(ptr), cLatLng(coordinate), out))
	})
	if err != nil {
		return ScreenPoint{}, err
	}
	defer C.mln_operation_release(operation)
	var point C.mln_screen_point
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_pixel_for_lat_lng_take_result(operation, &point))
	}); err != nil {
		return ScreenPoint{}, err
	}
	return goScreenPoint(point), nil
}

// LatLngForPixel performs an ordered coordinate conversion.
func (projection *MapProjectionHandle) LatLngForPixel(point ScreenPoint) (LatLng, error) {
	ptr, release, err := projection.ptr()
	if err != nil {
		return LatLng{}, err
	}
	defer release()
	defer projection.state.KeepAlive()
	operation, err := waitMapOperation(func(out *C.mln_operation) int32 {
		return int32(C.mln_map_projection_lat_lng_for_pixel_start(C.mln_map_projection(ptr), cScreenPoint(point), out))
	})
	if err != nil {
		return LatLng{}, err
	}
	defer C.mln_operation_release(operation)
	var coordinate C.mln_lat_lng
	if err := checkNative(func() int32 {
		return int32(C.mln_map_projection_lat_lng_for_pixel_take_result(operation, &coordinate))
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
