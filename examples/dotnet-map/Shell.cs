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
        Console.WriteLine($"render target: {mode.CliName}");
        Console.WriteLine($"render target status: {mode.Status}");
        InputController.PrintControls();
    }
}
