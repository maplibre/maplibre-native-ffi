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
        var logicalWidthValue = CheckedDimension(logicalWidth);
        var logicalHeightValue = CheckedDimension(logicalHeight);
        var physicalWidthValue = CheckedDimension(physicalWidth);
        var physicalHeightValue = CheckedDimension(physicalHeight);
        var scale = CalculateScaleFactor(
            logicalWidthValue,
            logicalHeightValue,
            physicalWidthValue,
            physicalHeightValue,
            scaleX,
            scaleY
        );
        var isEmpty =
            logicalWidth <= 0 || logicalHeight <= 0 || physicalWidth <= 0 || physicalHeight <= 0;
        return new Viewport(
            logicalWidthValue,
            logicalHeightValue,
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

    private static double CalculateScaleFactor(
        uint logicalWidth,
        uint logicalHeight,
        uint physicalWidth,
        uint physicalHeight,
        float scaleX,
        float scaleY
    )
    {
        if (logicalWidth > 0 && logicalHeight > 0 && physicalWidth > 0 && physicalHeight > 0)
        {
            var lowerBound = Math.Max(
                (physicalWidth - 1.0) / logicalWidth,
                (physicalHeight - 1.0) / logicalHeight
            );
            var upperBound = Math.Min(
                physicalWidth / (double)logicalWidth,
                physicalHeight / (double)logicalHeight
            );
            if (upperBound > lowerBound)
            {
                return upperBound;
            }
        }

        var scale = Math.Max(scaleX, scaleY);
        return double.IsFinite(scale) && scale > 0 ? scale : 1;
    }
}
