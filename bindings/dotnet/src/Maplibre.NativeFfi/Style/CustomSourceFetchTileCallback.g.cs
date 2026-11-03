// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Asks for the data of a tile in a custom source.
/// </summary>
/// <remarks>
/// <para><c>map</c> and <c>sourceId</c> name the source that asks. Return
/// quickly. Once the data is ready, pass it to
/// <c>MapHandle.SetCustomGeometrySourceTileDataAsync()</c> or
/// <c>MapHandle.SetCustomVectorSourceTileDataAsync()</c>. If it fails to load,
/// report that with <c>MapHandle.SetCustomGeometrySourceTileErrorAsync()</c> or
/// <c>MapHandle.SetCustomVectorSourceTileErrorAsync()</c>.</para>
/// <para>See <c>mln_custom_source_fetch_tile_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate void CustomSourceFetchTileCallback(
    MapHandle map,
    string sourceId,
    CanonicalTileId tileId
);
