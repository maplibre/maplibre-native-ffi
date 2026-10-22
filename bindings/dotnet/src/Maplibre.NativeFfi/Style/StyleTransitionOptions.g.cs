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
    public double? DurationMs { get; set; }

    /// <summary>
    /// Transition delay in milliseconds. Must be finite, non-negative, and
    /// short enough to represent as a duration.
    /// </summary>
    public double? DelayMs { get; set; }

    /// <summary>
    /// Whether symbol placement changes cross-fade.
    /// </summary>
    public bool? EnablePlacementTransitions { get; set; }
    public static StyleTransitionOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyStyleTransitionOptions(
                NativeMethods.mln_style_transition_options_default()
            );
        }
    }
}
