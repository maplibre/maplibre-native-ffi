using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal readonly record struct Viewport(
    uint LogicalWidth,
    uint LogicalHeight,
    uint PhysicalWidth,
    uint PhysicalHeight,
    double ScaleFactor,
    bool IsEmpty
)
{
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

        var physicalWidthValue = CheckedDimension(physicalWidth);
        var physicalHeightValue = CheckedDimension(physicalHeight);
        var isEmpty =
            logicalWidth <= 0 || logicalHeight <= 0 || physicalWidth <= 0 || physicalHeight <= 0;
        return new Viewport(
            LogicalDimension(physicalWidthValue, scale),
            LogicalDimension(physicalHeightValue, scale),
            physicalWidthValue,
            physicalHeightValue,
            scale,
            isEmpty
        );
    }

    public void Log(string label)
    {
        Console.WriteLine(
            $"{label}: logical={LogicalWidth}x{LogicalHeight} physical={PhysicalWidth}x{PhysicalHeight} scale={ScaleFactor:0.###}{(IsEmpty ? " empty=true" : "")}"
        );
    }

    private static uint CheckedDimension(int value)
    {
        return value <= 0 ? 0 : checked((uint)value);
    }

    private static uint LogicalDimension(uint physicalValue, double scale)
    {
        if (physicalValue == 0)
        {
            return 0;
        }

        return Math.Max(1, checked((uint)Math.Ceiling(physicalValue / scale)));
    }
}
