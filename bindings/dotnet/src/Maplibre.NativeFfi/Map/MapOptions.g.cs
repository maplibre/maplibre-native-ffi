// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options used when creating a map.
/// </summary>
/// <remarks>
/// See <c>mln_map_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="InitialExtent">
/// Initial logical extent. Width and height must be nonzero, and scale_factor
/// must be finite and positive. scale_factor is fixed for the map's lifetime.
/// </param>
/// <param name="MapMode">
/// Defaults to <c>MLN_MAP_MODE_CONTINUOUS</c>.
/// </param>
/// <param name="FastPforEnabled">
/// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
/// encodings. Defaults to false.
/// </param>
/// <param name="EventMask">
/// Map-originated event types this map queues, as a bitwise OR of
/// <c>mln_runtime_event_flag</c> values.
/// </param>
public readonly partial record struct MapOptions(
    LogicalExtent InitialExtent,
    MapMode MapMode,
    bool FastPforEnabled,
    RuntimeEventFlag EventMask
)
{
    public MapOptions()
        : this(default!, default!, default!, default!)
    {
        this = Default;
    }

    public static MapOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyMapOptions(NativeMethods.mln_map_options_default());
        }
    }
}
