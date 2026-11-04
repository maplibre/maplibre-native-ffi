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
    private static void OnWindowResized(uint width, uint height)
    {
        // The window's new size in logical pixels.
        session.Discarding.Resize(new LogicalSize(width, height));
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

    internal static async Task SurfaceRestored(ulong newSurface)
    {
        // #region lost-surface
        await session.SetVulkanSurfaceTargetAsync(
            new VulkanSurfaceReplacement
            {
                LogicalSize = new LogicalSize(800, 600),
                Surface = newSurface,
            }
        );
        // #endregion lost-surface
    }

    internal static async Task Run(
        MapHandle map,
        VulkanContextDescriptor vulkan,
        ulong windowSurface
    )
    {
        // #region attach
        session = map.AttachVulkanSurface(
            new VulkanSurfaceDescriptor
            {
                // The window's size in logical
                // pixels. The map's scale factor
                // turns it into pixels.
                LogicalSize = new LogicalSize(800, 600),
                // Your VkInstance, VkPhysicalDevice,
                // VkDevice, and graphics VkQueue.
                Context = vulkan,
                // Your VkSurfaceKHR.
                Surface = windowSurface,
            },
            new RenderSessionAttachOptions
            {
                Driver = RenderDriverKind.CoreWorker,
                RedrawWake = new Wake(OnRedrawWake),
                FrameWake = new Wake(OnFrameWake),
            }
        );
        await session.Ready;
        // #endregion attach

        OnWindowResized(1024, 768);

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
        DestroySurface(); // For example, with vkDestroySurfaceKHR.
    }
}
