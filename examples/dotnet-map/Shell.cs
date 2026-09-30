using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Render;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>App shell: GLFW/render-session affinity and autonomous map execution.</summary>
internal static class Shell
{
    public const int InitialWidth = 960;
    public const int InitialHeight = 640;

    // TODO(map-example-spec): Replace the fixed interval with a display-paced host loop. See Frame loop.
    private static readonly TimeSpan RenderLoopInterval = TimeSpan.FromMilliseconds(8);

    public static void Run(RenderTargetMode mode, RenderBackendFlag backends)
    {
        // GLFW, the graphics context, and the render session remain on the main thread.
        using var graphics = GraphicsContext.Create(
            "dotnet-map",
            InitialWidth,
            InitialHeight,
            backends,
            visible: true
        );
        using var state = MapState.Create(graphics.ReadViewport());
        var renderRequest = new RenderRequest();

        RenderLoop(graphics, mode, state, renderRequest);
    }

    /// <summary>
    /// Renders frames in a hidden window until one reaches it with the map at rest, then tears
    /// down as a closed window would. The style is inline, so the run needs no network.
    /// </summary>
    /// <returns>Whether a frame rendered before the deadline.</returns>
    public static bool RunSmoke(RenderTargetMode mode, RenderBackendFlag backends)
    {
        using var graphics = GraphicsContext.Create(
            "dotnet-map",
            InitialWidth,
            InitialHeight,
            backends,
            visible: false
        );
        using var state = MapState.Create(graphics.ReadViewport(), SmokeStyle);
        var target = RenderTargetFactory.Attach(graphics, state.Map, mode);
        try
        {
            var elapsed = System.Diagnostics.Stopwatch.StartNew();
            while (elapsed.Elapsed < SmokeDeadline)
            {
                graphics.PollEvents();
                if (graphics.CanRenderFrame && Render(graphics, target))
                {
                    Console.WriteLine($"smoke: rendered a frame with {mode.CliName}");
                    return true;
                }

                graphics.Window.WaitEventsTimeout(RenderLoopInterval.TotalSeconds);
            }

            Console.Error.WriteLine($"smoke: no frame rendered within {SmokeDeadline}");
            return false;
        }
        finally
        {
            target.Dispose();
        }
    }

    private static readonly TimeSpan SmokeDeadline = TimeSpan.FromSeconds(60);

    private static byte[] SmokeStyle =>
        """
            {"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#2a6f97"}}]}
            """u8.ToArray();

    private static void RenderLoop(
        IGraphicsContext graphics,
        RenderTargetMode mode,
        MapState state,
        RenderRequest renderRequest
    )
    {
        var viewport = graphics.ReadViewport();
        IRenderTarget? target = null;
        try
        {
            target = RenderTargetFactory.Attach(graphics, state.Map, mode);
            Console.WriteLine($"render target: {mode.CliName}");
            Console.WriteLine($"render target status: {mode.Status}");
            InputController.PrintControls();
            using var input = new InputController(graphics.Window, state, renderRequest);

            while (!graphics.ShouldClose)
            {
                graphics.PollEvents();
                if (state.DrainRenderRequests())
                {
                    renderRequest.Set();
                }

                var currentViewport = graphics.ReadViewport();
                if (currentViewport != viewport)
                {
                    viewport = currentViewport;
                    if (!viewport.IsEmpty)
                    {
                        graphics.Resize(viewport);
                        // The render target owns the map's extent while a session is attached.
                        target.Resize(viewport);
                        renderRequest.Set();
                    }
                }

                if (graphics.CanRenderFrame && renderRequest.Consume() && !Render(graphics, target))
                {
                    renderRequest.Set();
                }

                graphics.Window.WaitEventsTimeout(RenderLoopInterval.TotalSeconds);
            }
        }
        finally
        {
            // The thread-affine session closes before the map and runtime are released.
            target?.Dispose();
        }
    }

    private static bool Render(IGraphicsContext graphics, IRenderTarget target)
    {
        if (graphics is not MetalContext)
        {
            return target.Render();
        }

        using var pool = MacObjectiveC.AutoreleasePool();
        return target.Render();
    }
}
