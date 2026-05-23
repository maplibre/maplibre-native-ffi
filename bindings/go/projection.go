package maplibre

import (
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

// MapProjectionHandle owns a standalone projection snapshot.
type MapProjectionHandle struct {
	state *handle.State[capi.Projection]
}

// NewProjection creates a standalone projection helper from this map's current
// transform. Later map changes do not update the helper.
func (m *MapHandle) NewProjection() (*MapProjectionHandle, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()

	var projection *capi.Projection
	if err := checkNative(func() capi.Status { return capi.MapProjectionCreate(ptr, &projection) }); err != nil {
		return nil, err
	}
	state, err := handle.New(projection, "MapProjectionHandle")
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	return &MapProjectionHandle{state: state}, nil
}

func (projection *MapProjectionHandle) ptr() (*capi.Projection, error) {
	if projection == nil || projection.state == nil {
		return nil, newBindingError(ErrInvalidArgument, "MapProjectionHandle is nil")
	}
	ptr, live := projection.state.Ptr()
	if !live {
		return nil, newBindingError(ErrInvalidArgument, "MapProjectionHandle is closed")
	}
	return ptr, nil
}

// Close destroys this projection helper. A successful close makes later calls
// no-ops. A failed close leaves the native handle live so callers can retry on
// the owner thread.
func (projection *MapProjectionHandle) Close() error {
	if projection == nil || projection.state == nil {
		return newBindingError(ErrInvalidArgument, "MapProjectionHandle is nil")
	}
	return checkNative(func() capi.Status {
		return projection.state.Close(capi.MapProjectionDestroy)
	})
}

// PixelForLatLng converts a geographic coordinate to a logical screen point.
func (projection *MapProjectionHandle) PixelForLatLng(coordinate LatLng) (ScreenPoint, error) {
	ptr, err := projection.ptr()
	if err != nil {
		return ScreenPoint{}, err
	}
	defer projection.state.KeepAlive()

	var point capi.ScreenPoint
	if err := checkNative(func() capi.Status {
		return capi.MapProjectionPixelForLatLng(ptr, coordinate.toCAPI(), &point)
	}); err != nil {
		return ScreenPoint{}, err
	}
	return screenPointFromCAPI(point), nil
}

// LatLngForPixel converts a logical screen point to a geographic coordinate.
func (projection *MapProjectionHandle) LatLngForPixel(point ScreenPoint) (LatLng, error) {
	ptr, err := projection.ptr()
	if err != nil {
		return LatLng{}, err
	}
	defer projection.state.KeepAlive()

	var coordinate capi.LatLng
	if err := checkNative(func() capi.Status {
		return capi.MapProjectionLatLngForPixel(ptr, point.toCAPI(), &coordinate)
	}); err != nil {
		return LatLng{}, err
	}
	return latLngFromCAPI(coordinate), nil
}

// ProjectedMetersForLatLng converts a geographic coordinate to Spherical
// Mercator projected meters.
func ProjectedMetersForLatLng(coordinate LatLng) (ProjectedMeters, error) {
	var meters capi.ProjectedMeters
	if err := checkNative(func() capi.Status {
		return capi.ProjectedMetersForLatLng(coordinate.toCAPI(), &meters)
	}); err != nil {
		return ProjectedMeters{}, err
	}
	return projectedMetersFromCAPI(meters), nil
}

// LatLngForProjectedMeters converts Spherical Mercator projected meters to a
// geographic coordinate.
func LatLngForProjectedMeters(meters ProjectedMeters) (LatLng, error) {
	var coordinate capi.LatLng
	if err := checkNative(func() capi.Status {
		return capi.LatLngForProjectedMeters(meters.toCAPI(), &coordinate)
	}); err != nil {
		return LatLng{}, err
	}
	return latLngFromCAPI(coordinate), nil
}
