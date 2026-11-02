// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A line through points on the screen.
/// </summary>
/// <remarks>
/// See <c>mln_screen_line_string</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct ScreenLineString
{
    public ScreenLineString(ImmutableArray<ScreenPoint> Points)
    {
        this.Points = Points;
    }

    public ImmutableArray<ScreenPoint> Points
    {
        get => PointsStorage.AsImmutable();
        init => PointsStorage = ValueArray.From(value);
    }
    internal ValueArray<ScreenPoint> PointsStorage { get; init; }
}
