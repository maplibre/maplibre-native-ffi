using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal static class Shell
{
    public const int InitialWidth = 960;
    public const int InitialHeight = 640;

    public static void Run(RenderTargetMode mode, RenderBackend backends)
    {
        using var graphics = GraphicsContext.Create(
            "dotnet-map",
            InitialWidth,
            InitialHeight,
            backends
        );
        var viewport = graphics.ReadViewport();
        using var mapState = MapState.Create(graphics, viewport, mode);
        using var input = new InputController(
            graphics.Window,
            mapState.Map,
            mapState.RequestRender
        );
        Console.WriteLine($"render target: {mode.CliName}");
        Console.WriteLine($"render target status: {mode.Status}");
        InputController.PrintControls();

        // TODO(map-example-spec): Replace poll-and-wait with a display-paced host loop. See Frame loop.
        while (!graphics.ShouldClose)
        {
            graphics.PollEvents();
            var currentViewport = graphics.ReadViewport();
            if (currentViewport != viewport)
            {
                viewport = currentViewport;
                if (!viewport.IsEmpty)
                {
                    mapState.Resize(viewport);
                }
            }

            var madeProgress = mapState.Step(graphics.CanRenderFrame);
            if (!madeProgress)
            {
                graphics.Window.WaitEventsTimeout(0.01);
            }
        }
    }
}
