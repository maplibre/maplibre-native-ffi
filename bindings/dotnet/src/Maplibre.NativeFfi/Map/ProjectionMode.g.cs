// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Settings that skew the map or draw it without perspective.
/// </summary>
/// <remarks>
/// See <c>mln_projection_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record ProjectionMode
{
    /// <summary>
    /// Whether the map draws without perspective.
    /// </summary>
    public bool? Axonometric { get; init; }
    public double? XSkew { get; init; }
    public double? YSkew { get; init; }
}
