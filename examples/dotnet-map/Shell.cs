using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal static class Shell
{
    public const int InitialWidth = 960;
    public const int InitialHeight = 640;

    public static void Run(RenderTargetMode mode, RenderBackend backends)
    {
        _ = mode;
        _ = backends;
        throw new NotImplementedException("dotnet-map shell is not implemented yet.");
    }
}
