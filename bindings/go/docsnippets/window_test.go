// Snippets for the docs site's Render to a window page. They compile with the
// tests, which never run them.
package docsnippets

import (
	"context"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
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
// Set while the window has no surface, and when it closes.
var paused bool

func requestFrame() error {
	return session.RequestFrame(maplibre.FrameDemand{
		Flags: maplibre.FrameDemandFlagPresent | maplibre.FrameDemandFlagWaitForUpdate,
	})
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
			return replaceSession() // Close the session and attach a new one.
		}
	}
	if !paused {
		runAtNextRefresh(requestFrame)
	}
	return nil
}

// #endregion frame-loop

// #region frame-wake
// Runs on a library thread.
func onFrameWake() {
	runOnEventLoop(drainFrameResults)
}

// #endregion frame-wake

// #region resize
func onWindowResized(width, height uint32) error {
	return session.Discarding().Resize(maplibre.LogicalExtent{
		Width:       width,
		Height:      height,
		ScaleFactor: 2, // The scale factor the session has now.
	})
}

// #endregion resize

// #region service
// Your graphics context is current on this thread. Its goroutine holds
// runtime.LockOSThread.
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
	paused = true
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

func surfaceRestored(ctx context.Context, vulkan maplibre.VulkanContextDescriptor, newSurface uint64) error {
	// #region lost-surface
	restored, err := session.SetVulkanSurfaceTarget(maplibre.VulkanSurfaceDescriptor{
		Extent:  maplibre.LogicalExtent{Width: 800, Height: 600, ScaleFactor: 2},
		Context: vulkan,
		Surface: newSurface,
	})
	if err != nil {
		return err
	}
	if _, err := restored.Await(ctx); err != nil {
		return err
	}
	paused = false
	return requestFrame()
	// #endregion lost-surface
}

func renderToWindow(ctx context.Context, m *maplibre.MapHandle, vulkan maplibre.VulkanContextDescriptor, windowSurface uint64) error {
	// #region attach
	surface := maplibre.VulkanSurfaceDescriptor{
		// The window's size in logical pixels, and its scale factor.
		Extent: maplibre.LogicalExtent{Width: 800, Height: 600, ScaleFactor: 2},
		// Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
		Context: vulkan,
		// Your VkSurfaceKHR.
		Surface: windowSurface,
	}
	options := maplibre.RenderSessionAttachOptions{
		Driver:    maplibre.RenderDriverKindCoreWorker,
		FrameWake: maplibre.Wake{Callback: onFrameWake},
	}

	attached, ready, err := m.AttachVulkanSurface(surface, options)
	if err != nil {
		return err
	}
	session = attached
	if _, err := ready.Await(ctx); err != nil {
		return err
	}
	// #endregion attach

	if err := requestFrame(); err != nil {
		return err
	}
	if err := onWindowResized(1024, 768); err != nil {
		return err
	}

	// #region service
	graphicsThreadOptions := maplibre.RenderSessionAttachOptions{
		Driver:         maplibre.RenderDriverKindCallerGraphicsThread,
		FrameWake:      maplibre.Wake{Callback: onFrameWake},
		DriverWorkWake: maplibre.Wake{Callback: onDriverWorkWake},
	}
	// #endregion service
	_ = graphicsThreadOptions

	// #region close
	paused = true
	closed, err := session.Close()
	if err != nil {
		return err
	}
	if _, err := closed.Await(ctx); err != nil {
		return err
	}

	destroySurface() // For example, with vkDestroySurfaceKHR.
	// #endregion close
	return nil
}

// An example without an output comment compiles but never runs.
func Example_window() {
	ctx := context.Background()
	_ = renderToWindow(ctx, nil, maplibre.VulkanContextDescriptor{}, 0)
	_ = surfaceLost(ctx)
	_ = surfaceRestored(ctx, maplibre.VulkanContextDescriptor{}, 0)
}
