// Snippets for the docs site's Render to a window page. They compile with the
// tests, which never run them.
package docsnippets

import (
	"context"

	"maplibre.org/maplibre-native-ffi/bindings/go/maplibre"
)

// Stand-ins for the host's event loop, display timer, and graphics thread.
// Each reports the task's error.
func runOnEventLoop(task func() error) {}

func runAtNextRefresh(task func() error) {}

func runOnGraphicsThread(task func() error) {}

func replaceSession() error { return nil }

func destroySurface() {}

// #region attach
var session *maplibre.RenderSessionHandle

// #endregion attach

// #region frame-loop
// Draws the map if it changed since the last frame.
func demandFrame() error {
	return session.DemandFrame(maplibre.FrameDemand{})
}

func drainFrameResults() error {
	results, err := session.DrainFrameResults()
	if err != nil {
		return err
	}
	if len(results) == 0 {
		return nil // No results yet.
	}

	for _, result := range results {
		if result.Disposition == maplibre.RenderResultFailed {
			return replaceSession() // Release the session and attach a new one.
		}
	}
	return nil
}

// #endregion frame-loop

// #region frame-wake
// Both run on a library thread.
func onRedrawWake() {
	runAtNextRefresh(demandFrame)
}

func onFrameWake() {
	runOnEventLoop(drainFrameResults)
}

// #endregion frame-wake

// #region resize
// Runs when the window changes size, or moves to a display with
// another scale factor.
func onWindowResized(width, height uint32, scaleFactor float64) error {
	// The window's new size in logical pixels, and its scale factor.
	return session.Discarding().Resize(maplibre.LogicalExtent{
		Width: width, Height: height, ScaleFactor: scaleFactor,
	})
}

// #endregion resize

// #region service
// Your graphics context is current on this thread.
// Its goroutine holds runtime.LockOSThread.
func serviceSession() error {
	return session.ServiceDriverWork()
}

// Runs on a library thread.
func onDriverWorkWake() {
	runOnGraphicsThread(serviceSession)
}

// #endregion service

func surfaceLost(ctx context.Context) error {
	// #region lost-surface
	released, err := session.ReleaseTarget()
	if err != nil {
		return err
	}
	if _, err := released.Await(ctx); err != nil {
		return err
	}
	// #endregion lost-surface
	return nil
}

func surfaceRestored(ctx context.Context, display, newSurface uintptr) error {
	// #region lost-surface
	replacement := maplibre.RenderTargetWaylandSurfaceVariant{
		Value: maplibre.WaylandSurfaceTarget{
			LogicalSize: maplibre.LogicalSize{Width: 800, Height: 600},
			Display:     display,
			Surface:     newSurface,
		},
	}
	restored, err := session.SetTarget(replacement)
	if err != nil {
		return err
	}
	if _, err := restored.Await(ctx); err != nil {
		return err
	}
	// #endregion lost-surface
	return nil
}

func renderToWindow(ctx context.Context, m *maplibre.MapHandle, display, windowSurface uintptr) error {
	// #region attach
	surface := maplibre.RenderTargetWaylandSurfaceVariant{
		Value: maplibre.WaylandSurfaceTarget{
			// The window's size in logical pixels. The map's scale factor
			// turns it into pixels.
			LogicalSize: maplibre.LogicalSize{Width: 800, Height: 600},
			// Your wl_display and wl_surface.
			Display: display,
			Surface: windowSurface,
		},
	}
	options := maplibre.RenderSessionAttachOptions{
		Driver:     maplibre.RenderDriverKindCoreWorker,
		RedrawWake: maplibre.Wake{Callback: onRedrawWake},
		FrameWake:  maplibre.Wake{Callback: onFrameWake},
	}

	// With no device, the session creates one on the platform's own
	// backend.
	attached, ready, err := m.Attach(surface, nil, options)
	if err != nil {
		return err
	}
	session = attached
	if _, err := ready.Await(ctx); err != nil {
		return err
	}
	// #endregion attach

	if err := onWindowResized(1024, 768, 2); err != nil {
		return err
	}

	// #region service
	graphicsThreadOptions := maplibre.RenderSessionAttachOptions{
		Driver:         maplibre.RenderDriverKindCallerGraphicsThread,
		RedrawWake:     maplibre.Wake{Callback: onRedrawWake},
		FrameWake:      maplibre.Wake{Callback: onFrameWake},
		DriverWorkWake: maplibre.Wake{Callback: onDriverWorkWake},
	}
	// #endregion service
	_ = graphicsThreadOptions

	released, err := session.Release()
	if err != nil {
		return err
	}
	if _, err := released.Await(ctx); err != nil {
		return err
	}

	destroySurface() // For example, with wl_surface_destroy.
	return nil
}

// An example without an output comment compiles but never runs.
func Example_window() {
	ctx := context.Background()
	_ = renderToWindow(ctx, nil, 0, 0)
	_ = surfaceLost(ctx)
	_ = surfaceRestored(ctx, 0, 0)
}
