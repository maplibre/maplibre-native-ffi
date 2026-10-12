// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Payload for <c>MLN_RUNTIME_EVENT_MAP_TILE_ACTION</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_tile_action</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Operation">
/// One of <c>mln_tile_operation</c>.
/// </param>
public readonly partial record struct RuntimeEventTileAction(
    TileOperation Operation,
    TileId TileId
);
