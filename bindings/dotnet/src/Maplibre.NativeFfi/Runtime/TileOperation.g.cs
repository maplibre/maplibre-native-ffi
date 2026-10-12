// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Tile operations reported by tile observer events.
/// </summary>
/// <remarks>
/// See <c>mln_tile_operation</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
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
