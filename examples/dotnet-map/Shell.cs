using Maplibre.NativeFfi.Render;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>App shell: GLFW/render-session affinity and autonomous map execution.</summary>
internal static class Shell
{
    public const int InitialWidth = 960;
    public const int InitialHeight = 640;

    // TODO(map-example-spec): Replace the fixed interval with a display-paced host loop. See Frame loop.
    private static readonly TimeSpan RenderLoopInterval = TimeSpan.FromMilliseconds(8);

    public static void Run(RenderTargetMode mode, RenderBackend backends)
    {
        // GLFW, the graphics context, and the render session remain on the main thread.
        using var graphics = GraphicsContext.Create(
            "dotnet-map",
            InitialWidth,
            InitialHeight,
            backends
        );
        using var state = MapState.Create(graphics.ReadViewport());
        var renderRequest = new RenderRequest();

        RenderLoop(graphics, mode, state, renderRequest);
    }

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
                        target.Resize(viewport);
                        _ = state.Map.ResizeAsync(
                            new global::Maplibre.NativeFfi.Map.LogicalExtent(
                                viewport.LogicalWidth,
                                viewport.LogicalHeight,
                                viewport.ScaleFactor
                            )
                        );
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
