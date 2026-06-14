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

    public static Viewport FromWindowMetrics(
        int logicalWidth,
        int logicalHeight,
        int physicalWidth,
        int physicalHeight,
        float scaleX,
        float scaleY
    )
    {
        var scale = Math.Max(scaleX, scaleY);
        if (!double.IsFinite(scale) || scale <= 0)
        {
            scale = 1;
        }

        return new Viewport(
            CheckedDimension(logicalWidth),
            CheckedDimension(logicalHeight),
            CheckedDimension(physicalWidth),
            CheckedDimension(physicalHeight),
            scale
        );
    }

    public void Log(string label)
    {
        Console.WriteLine(
            $"{label}: logical={LogicalWidth}x{LogicalHeight} physical={PhysicalWidth}x{PhysicalHeight} scale={ScaleFactor:0.###}"
        );
    }

    private static uint CheckedDimension(int value)
    {
        return value <= 0 ? 0 : checked((uint)value);
    }
}
