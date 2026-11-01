// Snippets for the docs site's Create a map page. They compile with the
// tests, which never run them.
package docsnippets

import (
	"context"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

// Attaches a render target, as the Rendering guides show.
func attachTarget(m *maplibre.MapHandle) (*maplibre.RenderSessionHandle, error) {
	return nil, nil
}

func createAMap(ctx context.Context) error {
	// #region runtime
	runtime, err := maplibre.NewRuntime(maplibre.RuntimeOptions{})
	if err != nil {
		return err
	}
	// #endregion runtime

	// #region map
	m, created, err := runtime.CreateMap(maplibre.MapOptions{
		InitialExtent: maplibre.LogicalExtent{Width: 800, Height: 600, ScaleFactor: 2},
		MapMode:       maplibre.MapModeContinuous,
	})
	if err != nil {
		return err
	}
	if _, err := created.Await(ctx); err != nil {
		return err
	}
	// #endregion map

	session, err := attachTarget(m)
	if err != nil {
		return err
	}

	// #region close
	if err := session.Dispose(); err != nil {
		return err
	}

	mapClosed, err := m.Close()
	if err != nil {
		return err
	}
	if _, err := mapClosed.Await(ctx); err != nil {
		return err
	}

	runtimeClosed, err := runtime.Close()
	if err != nil {
		return err
	}
	if _, err := runtimeClosed.Await(ctx); err != nil {
		return err
	}
	// #endregion close
	return nil
}

// An example without an output comment compiles but never runs.
func Example_createAMap() {
	_ = createAMap(context.Background())
}
