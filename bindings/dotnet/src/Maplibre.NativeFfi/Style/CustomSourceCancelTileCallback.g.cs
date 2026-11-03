// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Reports that a custom source no longer needs a tile.
/// </summary>
/// <remarks>
/// <para><c>map</c> and <c>sourceId</c> name the source.</para>
/// <para>See <c>mln_custom_source_cancel_tile_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate void CustomSourceCancelTileCallback(
    MapHandle map,
    string sourceId,
    CanonicalTileId tileId
);
