namespace Maplibre.Native.Map;

/// <summary>Viewport constraint mode.</summary>
public enum ConstrainMode : uint { None = 0, HeightOnly = 1, WidthAndHeight = 2, Screen = 3 }

/// <summary>North orientation.</summary>
public enum NorthOrientation : uint { Up = 0, Right = 1, Down = 2, Left = 3 }

/// <summary>Viewport orientation mode.</summary>
public enum ViewportMode : uint { Default = 0, FlippedY = 1 }

/// <summary>Tile level-of-detail mode.</summary>
public enum TileLodMode : uint { Default = 0, Distance = 1 }

/// <summary>Map debug flags.</summary>
[Flags]
public enum DebugOptions : uint
{
    None = 0,
    TileBorders = 1u << 0,
    ParseStatus = 1u << 1,
    Timestamps = 1u << 2,
    Collision = 1u << 3,
    Overdraw = 1u << 4,
    StencilClip = 1u << 5,
    DepthBuffer = 1u << 6,
}

/// <summary>Tile operation reported in runtime tile events.</summary>
public enum TileOperation : uint
{
    Null = 0,
    RequestedFromCache = 1,
    RequestedFromNetwork = 2,
    LoadFromCache = 3,
    LoadFromNetwork = 4,
    StartParse = 5,
    EndParse = 6,
    Error = 7,
    Cancelled = 8,
}
