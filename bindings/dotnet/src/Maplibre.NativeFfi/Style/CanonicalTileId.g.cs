// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A tile's zoom level <c>z</c>, column <c>x</c>, and row <c>y</c>, with row 0
/// at the north.
/// </summary>
/// <remarks>
/// See <c>mln_canonical_tile_id</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct CanonicalTileId(uint Z, uint X, uint Y);
