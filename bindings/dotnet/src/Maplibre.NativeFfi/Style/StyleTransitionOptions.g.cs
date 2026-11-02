// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The default
/// [transition](https://maplibre.org/maplibre-style-spec/transition/) for
/// changes to a style's paint properties.
/// </summary>
/// <remarks>
/// See <c>mln_style_transition_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleTransitionOptions
{
    public double? DurationMs { get; init; }
    public double? DelayMs { get; init; }

    /// <summary>
    /// Whether symbols fade in and out as the map places them.
    /// </summary>
    public bool? EnablePlacementTransitions { get; init; }
}
