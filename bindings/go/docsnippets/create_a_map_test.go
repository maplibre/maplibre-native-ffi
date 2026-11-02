// Snippets for the docs site's Create a map page. They compile with the
// tests, which never run them.
package docsnippets

import (
	"context"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

func reportFailure(err error) {}

// Attaches a render target, as the render target guides show.
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
		InitialExtent: maplibre.LogicalExtent{
			Width: 800, Height: 600, ScaleFactor: 2,
		},
		MapMode: maplibre.MapModeContinuous,
	})
	if err != nil {
		return err
	}
	if _, err := created.Await(ctx); err != nil {
		return err
	}
	// #endregion map

	// #region style
	err = m.Discarding().SetStyleUrl(
		"https://demotiles.maplibre.org/style.json",
	)
	if err != nil {
		return err
	}
	// #endregion style

	zoom := 3.0
	update := maplibre.CameraUpdate{
		Mode:   maplibre.CameraUpdateModeJump,
		Camera: maplibre.CameraOptions{Zoom: &zoom},
	}

	// #region wait
	moved, err := m.UpdateCamera(update)
	if err != nil {
		return err
	}
	if _, err := moved.Await(ctx); err != nil {
		return err
	}

	// Use the discarding variant to ignore the result.
	if err := m.Discarding().UpdateCamera(update); err != nil {
		return err
	}
	// #endregion wait

	// #region errors
	jumped, err := m.UpdateCamera(update)
	if err != nil {
		// The call failed right away.
		reportFailure(err)
	} else if _, err := jumped.Await(ctx); err != nil {
		// The result reports a failure.
		reportFailure(err)
	}
	// #endregion errors

	session, err := attachTarget(m)
	if err != nil {
		return err
	}

	// #region close
	if err := session.Dispose(); err != nil {
		return err
	}
	if err := m.Dispose(); err != nil {
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
