namespace Maplibre.Native.Map;

/// <summary>Map rendering mode.</summary>
public enum MapMode : uint
{
    /// <summary>The map continuously renders when invalidated.</summary>
    Continuous = 0,

    /// <summary>The map renders still images on request.</summary>
    Static = 1,

    /// <summary>The map renders tile-sized outputs on request.</summary>
    Tile = 2,
}
