namespace Maplibre.Native.Map;

/// <summary>Viewport constraint mode.</summary>
public enum ConstrainMode : uint
{
    None = 0,
    HeightOnly = 1,
    WidthAndHeight = 2,
    Screen = 3,
}

/// <summary>North orientation.</summary>
public enum NorthOrientation : uint
{
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3,
}

/// <summary>Viewport orientation mode.</summary>
public enum ViewportMode : uint
{
    Default = 0,
    FlippedY = 1,
}

/// <summary>Tile level-of-detail mode.</summary>
public enum TileLodMode : uint
{
    Default = 0,
    Distance = 1,
}

/// <summary>Map debug flags.</summary>
[Flags]
public enum DebugOptions : uint
{
    None = 0,
    TileBorders = 1u << 1,
    ParseStatus = 1u << 2,
    Timestamps = 1u << 3,
    Collision = 1u << 4,
    Overdraw = 1u << 5,
    StencilClip = 1u << 6,
    DepthBuffer = 1u << 7,
}

/// <summary>Tile operation reported in runtime tile events.</summary>
public enum TileOperation : uint
{
    RequestedFromCache = 0,
    RequestedFromNetwork = 1,
    LoadFromNetwork = 2,
    LoadFromCache = 3,
    StartParse = 4,
    EndParse = 5,
    Error = 6,
    Cancelled = 7,
    Null = 8,
}
