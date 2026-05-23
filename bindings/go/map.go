package maplibre

import (
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

// MapMode selects the native map rendering mode.
type MapMode uint32

const (
	MapModeContinuous MapMode = MapMode(capi.MapModeContinuous)
	MapModeStatic     MapMode = MapMode(capi.MapModeStatic)
	MapModeTile       MapMode = MapMode(capi.MapModeTile)
)

// MapOptions configures map creation.
type MapOptions struct {
	Width       uint32
	Height      uint32
	ScaleFactor float64
	Mode        MapMode
}

// NewMapOptions returns map creation options for a viewport size and scale.
func NewMapOptions(width, height uint32, scaleFactor float64) MapOptions {
	return MapOptions{Width: width, Height: height, ScaleFactor: scaleFactor, Mode: MapModeContinuous}
}

func (options MapOptions) toCAPI() capi.MapOptions {
	return capi.MapOptions{
		Width:       options.Width,
		Height:      options.Height,
		ScaleFactor: options.ScaleFactor,
		MapMode:     uint32(options.Mode),
	}
}

// MapHandle owns map state for one RuntimeHandle.
type MapHandle struct {
	state   *handle.State[capi.Map]
	runtime *RuntimeHandle
}

func (m *MapHandle) ptr() (*capi.Map, error) {
	if m == nil || m.state == nil {
		return nil, newBindingError(ErrInvalidArgument, "MapHandle is nil")
	}
	ptr, live := m.state.Ptr()
	if !live {
		return nil, newBindingError(ErrInvalidArgument, "MapHandle is closed")
	}
	return ptr, nil
}

// Close destroys this map. A successful close makes later calls no-ops. A
// failed close leaves the native handle live so callers can retry on the owner
// thread.
func (m *MapHandle) Close() error {
	if m == nil || m.state == nil {
		return newBindingError(ErrInvalidArgument, "MapHandle is nil")
	}
	defer func() {
		if m.runtime != nil && m.runtime.state != nil {
			m.runtime.state.KeepAlive()
		}
	}()
	return checkNative(func() capi.Status {
		return m.state.Close(capi.MapDestroy)
	})
}
