// Snippets for the docs site's Render to a window page. They compile with the
// tests, which never run them.

using System.Runtime.Versioning;

namespace Maplibre.NativeFfi.Tests.DocSnippets;

[SupportedOSPlatform("android")]
[SupportedOSPlatform("linux")]
[SupportedOSPlatform("macos")]
[SupportedOSPlatform("windows")]
internal static class Window
{
    // Stand-ins for the host's event loop, display timer, and graphics thread.
    private static void RunOnEventLoop(Action task) { }

    private static void RunAtNextRefresh(Action task) { }

    private static void RunOnGraphicsThread(Action task) { }

    private static void ReplaceSession() { }

    private static void DestroySurface() { }

    // #region attach
    private static RenderSessionHandle session = null!;

    // #endregion attach

    // #region frame-loop
    // Draws the map if it changed since the last frame.
    private static void DemandFrame() =>
        session.DemandFrame(new FrameDemand());

    private static void DrainFrameResults()
    {
        var results = session.DrainFrameResults();
        if (results.Length == 0)
            return; // No results yet.

        if (
            results.Any(result =>
                result.Disposition == RenderResult.Failed
            )
        )
            // Release the session and attach a new one.
            ReplaceSession();
    }

    // #endregion frame-loop

    // #region frame-wake
    // Both run on a library thread.
    private static void OnRedrawWake() =>
        RunAtNextRefresh(DemandFrame);

    private static void OnFrameWake() =>
        RunOnEventLoop(DrainFrameResults);

    // #endregion frame-wake

    // #region resize
    // Runs when the window changes size, or moves
    // to a display with another scale factor.
    private static void OnWindowResized(
        uint width,
        uint height,
        double scaleFactor
    )
    {
        // The window's new size in logical pixels,
        // and its scale factor.
        session.Discarding.Resize(
            new LogicalExtent(width, height, scaleFactor)
        );
    }

    // #endregion resize

    // #region service
    // Your graphics context is current on this thread.
    private static void ServiceSession() =>
        session.ServiceDriverWork();

    // Runs on a library thread.
    private static void OnDriverWorkWake() =>
        RunOnGraphicsThread(ServiceSession);

    // #endregion service

    internal static async Task SurfaceLost()
    {
        // #region lost-surface
        await session.ReleaseTargetAsync();
        // #endregion lost-surface
    }

    internal static async Task SurfaceRestored(
        NativePointer display,
        NativePointer newSurface
    )
    {
        // #region lost-surface
        await session.SetTargetAsync(
            new RenderTarget.WaylandSurface(
                new WaylandSurfaceTarget
                {
                    LogicalSize = new LogicalSize(800, 600),
                    Display = display,
                    Surface = newSurface,
                }
            )
        );
        // #endregion lost-surface
    }

    internal static async Task Run(
        MapHandle map,
        NativePointer display,
        NativePointer windowSurface
    )
    {
        // #region attach
        // With no device, the session creates one
        // on the platform's own backend.
        session = map.Attach(
            new RenderTarget.WaylandSurface(
                new WaylandSurfaceTarget
                {
                    // The window's size in logical
                    // pixels. The map's scale factor
                    // turns it into pixels.
                    LogicalSize = new LogicalSize(800, 600),
                    // Your wl_display and wl_surface.
                    Display = display,
                    Surface = windowSurface,
                }
            ),
            null,
            new RenderSessionAttachOptions
            {
                Driver = RenderDriverKind.CoreWorker,
                RedrawWake = new Wake(OnRedrawWake),
                FrameWake = new Wake(OnFrameWake),
            }
        );
        await session.Ready;
        // #endregion attach

        OnWindowResized(1024, 768, 2);

        // #region service
        var graphicsThreadOptions = new RenderSessionAttachOptions
        {
            Driver = RenderDriverKind.CallerGraphicsThread,
            RedrawWake = new Wake(OnRedrawWake),
            FrameWake = new Wake(OnFrameWake),
            DriverWorkWake = new Wake(OnDriverWorkWake),
        };
        // #endregion service
        _ = graphicsThreadOptions;

        await session.ReleaseAsync();
        DestroySurface(); // For example, with wl_surface_destroy.
    }
}
