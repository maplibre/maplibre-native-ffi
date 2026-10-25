// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// MapLibre axonometric rendering options used for snapshots and commands.
/// </summary>
/// <remarks>
/// See <c>mln_projection_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record ProjectionMode
{
    /// <summary>
    /// Enables a non-perspective axonometric render transform.
    /// </summary>
    public bool? Axonometric { get; init; }

    /// <summary>
    /// X-skew factor of the axonometric transform.
    /// </summary>
    public double? XSkew { get; init; }

    /// <summary>
    /// Y-skew factor of the axonometric transform.
    /// </summary>
    public double? YSkew { get; init; }
}
