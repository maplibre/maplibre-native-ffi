// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Global style transition options.
/// </summary>
/// <remarks>
/// See <c>mln_style_transition_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleTransitionOptions
{
    /// <summary>
    /// Transition duration in milliseconds. Must be finite, non-negative, and
    /// short enough to represent as a duration.
    /// </summary>
    public double? DurationMs { get; init; }

    /// <summary>
    /// Transition delay in milliseconds. Must be finite, non-negative, and
    /// short enough to represent as a duration.
    /// </summary>
    public double? DelayMs { get; init; }

    /// <summary>
    /// Whether symbol placement changes cross-fade.
    /// </summary>
    public bool? EnablePlacementTransitions { get; init; }
}
