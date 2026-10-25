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
public sealed record MapOptions
{
    /// <summary>
    /// Initial logical extent. Required. Width and height must be nonzero, and
    /// scale_factor must be finite and positive. scale_factor is fixed for the
    /// map's lifetime.
    /// </summary>
    public required LogicalExtent InitialExtent { get; init; }
    public MapMode MapMode { get; init; }

    /// <summary>
    /// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
    /// encodings.
    /// </summary>
    public bool FastPforEnabled { get; init; }

    /// <summary>
    /// Map-originated event types this map queues, as a bitwise OR of
    /// <c>mln_runtime_event_flag</c> values.
    /// </summary>
    public RuntimeEventFlag? EventMask { get; init; }
}
