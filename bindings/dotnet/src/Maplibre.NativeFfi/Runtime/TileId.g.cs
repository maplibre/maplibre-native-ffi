// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Overscaled tile identity reported in tile observer events.
/// </summary>
/// <remarks>
/// See <c>mln_tile_id</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct TileId(
    uint OverscaledZ,
    int Wrap,
    uint CanonicalZ,
    uint CanonicalX,
    uint CanonicalY
);
