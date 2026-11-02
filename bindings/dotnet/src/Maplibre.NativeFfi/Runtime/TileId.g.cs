// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The ID of a tile.
/// </summary>
/// <remarks>
/// <para><c>canonical_x</c> and <c>canonical_y</c> place the tile in the
/// source's grid at zoom <c>canonical_z</c>. <c>overscaled_z</c> is the zoom
/// that the map draws the tile at, and is greater when the source has no tiles
/// that deep. <c>wrap</c> says which copy of the world holds the tile: zero for
/// the main one, negative to the west, and positive to the east.</para>
/// <para>See <c>mln_tile_id</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct TileId(
    uint OverscaledZ,
    int Wrap,
    uint CanonicalZ,
    uint CanonicalX,
    uint CanonicalY
);
