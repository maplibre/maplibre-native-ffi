// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Canonical tile identity used by custom geometry and custom MVT vector source
/// callbacks.
/// </summary>
/// <remarks>
/// See <c>mln_canonical_tile_id</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct CanonicalTileId(uint Z, uint X, uint Y);
