using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Render;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>
/// App shell. GLFW, the graphics context, and the render session stay on the main thread, which
/// sleeps until input arrives or a native wake posts an empty event. Input becomes map commands, a
/// map update becomes a frame demand, and each wake has the thread drain events, service driver
/// work, or drain frame results.
/// </summary>
internal static class Shell
{
    public const int InitialWidth = 960;
    public const int InitialHeight = 640;

    private static readonly TimeSpan SmokeDeadline = TimeSpan.FromSeconds(60);

    private static byte[] SmokeStyle =>
        """
            {"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#2a6f97"}}]}
            """u8.ToArray();

    /// <summary>
    /// Runs the example until its window closes. A smoke run instead renders an inline style in a
    /// hidden window, so it needs no network, and tears down as a closed window would after the
    /// first frame that reaches the window.
    /// </summary>
    /// <returns>False when a smoke run rendered no frame before its deadline.</returns>
    public static bool Run(RenderTargetMode mode, RenderBackendFlag backends, bool smoke)
    {
        using var graphics = GraphicsContext.Create(
            "dotnet-map",
            InitialWidth,
            InitialHeight,
            backends,
            visible: !smoke
        );
        var wakes = new LoopWakes(graphics.Window.Glfw);
        using var state = MapState.Create(
            graphics.ReadViewport(),
            wakes.Events.Wake,
            smoke ? SmokeStyle : null
        );
        // The thread-affine session closes before the map and runtime are released.
        using var target = RenderTarget.Attach(graphics, state.Map, mode, wakes);
        Console.WriteLine($"render target: {mode.CliName}");
        Console.WriteLine($"render target status: {mode.Status}");
        InputController.PrintControls();
        using var input = new InputController(graphics.Window, state);

        var viewport = graphics.ReadViewport();
        var elapsed = System.Diagnostics.Stopwatch.StartNew();
        // The session attached after the map took its style and camera, so it starts with one frame.
        target.RequestFrame();
        while (!graphics.ShouldClose)
        {
            if (smoke)
            {
                var remaining = SmokeDeadline - elapsed.Elapsed;
                if (remaining <= TimeSpan.Zero)
                {
                    Console.Error.WriteLine($"smoke: no frame rendered within {SmokeDeadline}");
                    return false;
                }
                graphics.Window.WaitEventsTimeout(remaining.TotalSeconds);
            }
            else
            {
                graphics.Window.WaitEvents();
            }

            using var pool = graphics is MetalContext ? MacObjectiveC.AutoreleasePool() : null;
            var currentViewport = graphics.ReadViewport();
            if (currentViewport != viewport)
            {
                viewport = currentViewport;
                if (!viewport.IsEmpty)
                {
                    graphics.Resize(viewport);
                    target.Resize(viewport);
                }
            }
            if (wakes.Events.Consume() && state.DrainRenderUpdates() && graphics.CanRenderFrame)
            {
                target.RequestFrame();
            }
            if (wakes.DriverWork.Consume())
            {
                target.ServiceDriverWork();
            }
            if (wakes.Frames.Consume() && target.DrainFrameResults() && smoke)
            {
                Console.WriteLine($"smoke: rendered a frame with {mode.CliName}");
                return true;
            }
        }
        return true;
    }
}
