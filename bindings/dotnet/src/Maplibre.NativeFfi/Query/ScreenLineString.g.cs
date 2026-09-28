// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Query;

public readonly record struct ScreenLineString
{
    public ScreenLineString(ScreenPoint[] Points)
        : this(Points, false) { }

    internal ScreenLineString(ScreenPoint[] Points, bool adopt)
    {
        this.storagePoints = adopt ? Points : Points?.ToArray() ?? [];
    }

    private readonly ScreenPoint[]? storagePoints;
    public ScreenPoint[] Points
    {
        get => storagePoints?.ToArray() ?? [];
        init => storagePoints = value?.ToArray() ?? [];
    }
    internal ScreenPoint[] PointsStorage
    {
        get => storagePoints ?? [];
        init => storagePoints = value;
    }

    public bool Equals(ScreenLineString other) =>
        global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            PointsStorage,
            other.PointsStorage
        );

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(PointsStorage));
        return hash.ToHashCode();
    }
}
