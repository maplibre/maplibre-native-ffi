using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal readonly record struct Viewport(
    uint LogicalWidth,
    uint LogicalHeight,
    uint PhysicalWidth,
    uint PhysicalHeight,
    double ScaleFactor
)
{
    public bool IsEmpty =>
        LogicalWidth == 0 || LogicalHeight == 0 || PhysicalWidth == 0 || PhysicalHeight == 0;

    public RenderTargetExtent RenderTargetExtent => new(LogicalWidth, LogicalHeight, ScaleFactor);

    public void Log(string label)
    {
        Console.WriteLine(
            $"{label}: logical={LogicalWidth}x{LogicalHeight} physical={PhysicalWidth}x{PhysicalHeight} scale={ScaleFactor:0.###}"
        );
    }
}
