// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Screen-space line string in logical map pixels.
/// </summary>
/// <remarks>
/// See <c>mln_screen_line_string</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct ScreenLineString
{
    public ScreenLineString(ScreenPoint[] Points)
    {
        this.Points = Points;
    }

    public ScreenPoint[] Points
    {
        get => PointsStorage.ToArray();
        init => PointsStorage = ValueArray.Copy(value);
    }
    internal ValueArray<ScreenPoint> PointsStorage { get; init; }
}
