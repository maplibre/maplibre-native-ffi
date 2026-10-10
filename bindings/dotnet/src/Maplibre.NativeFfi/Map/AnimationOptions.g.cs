// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record AnimationOptions
{
    public double? DurationMs { get; set; }
    public double? Velocity { get; set; }
    public double? MinZoom { get; set; }
    public UnitBezier? Easing { get; set; }
    public ulong? TransitionId { get; set; }
    public static AnimationOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_animation_options_default");
            return GeneratedValues.CopyAnimationOptions(
                NativeMethods.mln_animation_options_default()
            );
        }
    }
}
