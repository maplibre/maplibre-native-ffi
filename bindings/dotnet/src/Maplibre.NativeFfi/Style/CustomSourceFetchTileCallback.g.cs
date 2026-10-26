// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Requests one tile of a custom geometry or custom vector source. The callback
/// may run on any native worker thread, concurrently with runtime and map
/// calls.
/// </summary>
/// <remarks>
/// See <c>mln_custom_source_fetch_tile_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public delegate void CustomSourceFetchTileCallback(CanonicalTileId tileId);
