using System.Diagnostics;
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>
/// App shell. GLFW, the graphics context, and the render session stay on the main thread, which
/// sleeps until input arrives or a native wake posts an empty event. Input becomes map commands, a
/// map update becomes a frame demand, and the wakes have the thread drain events and frame results.
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
        var events = new GlfwWake(graphics.Window.Glfw);
        using var state = MapState.Create(
            graphics.ReadViewport(),
            events.Wake,
            smoke ? SmokeStyle : null
        );
        using var input = new InputController(graphics.Window, state);

        var viewport = graphics.ReadViewport();
        // The session detaches before the map and runtime are released.
        var target = Attach(graphics, state.Map, mode);
        // A session fixes its scale factor at attachment, so a scale change reattaches.
        var attachedScale = viewport.ScaleFactor;
        try
        {
            InputController.PrintControls();
            var smokeDeadline =
                Stopwatch.GetTimestamp() + (long)(Stopwatch.Frequency * SmokeDeadline.TotalSeconds);
            // Wakes may have arrived while the attachment waited, so the loop handles pending work
            // before its first wait.
            while (!graphics.ShouldClose)
            {
                var current = graphics.ReadViewport();
                if (current != viewport)
                {
                    viewport = current;
                    if (!viewport.IsEmpty && viewport.ScaleFactor == attachedScale)
                    {
                        target.Resize(viewport);
                    }
                    else if (!viewport.IsEmpty)
                    {
                        target.Dispose();
                        target = Attach(graphics, state.Map, mode);
                        attachedScale = viewport.ScaleFactor;
                    }
                }
                if (events.Consume() && state.DrainRenderUpdates() && graphics.CanRenderFrame)
                {
                    target.RequestFrame();
                }
                if (target.HandleWakes() && smoke)
                {
                    Console.WriteLine("smoke: rendered a frame");
                    return true;
                }

                if (smoke && Stopwatch.GetTimestamp() >= smokeDeadline)
                {
                    Console.Error.WriteLine($"smoke: no frame rendered within {SmokeDeadline}");
                    return false;
                }
                long? wakeAt = smoke
                    ? Math.Min(smokeDeadline, target.RetryAt ?? long.MaxValue)
                    : target.RetryAt;
                if (wakeAt is { } due)
                {
                    var remaining = Math.Max(due - Stopwatch.GetTimestamp(), 0);
                    graphics.Window.WaitEventsTimeout((double)remaining / Stopwatch.Frequency);
                }
                else
                {
                    graphics.Window.WaitEvents();
                }
            }
            return true;
        }
        finally
        {
            target.Dispose();
        }
    }

    /// <summary>Attaches a session, logs its mode and driver, and demands its first frame.</summary>
    private static RenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        RenderTargetMode mode
    )
    {
        var target = RenderTarget.Attach(graphics, map, mode);
        Console.WriteLine($"render target: {mode.CliName}");
        Console.WriteLine($"render target status: {mode.Status}");
        Console.WriteLine($"render driver: {target.Driver.Label}");
        target.RequestFrame();
        return target;
    }
}
