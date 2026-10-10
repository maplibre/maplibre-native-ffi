// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

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
