// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record StyleTransitionOptions
{
    public double? DurationMs { get; set; }
    public double? DelayMs { get; set; }
    public bool? EnablePlacementTransitions { get; set; }
    public static StyleTransitionOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_style_transition_options_default");
            return GeneratedValues.CopyStyleTransitionOptions(
                NativeMethods.mln_style_transition_options_default()
            );
        }
    }
}
