// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Map;

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
            using var call = Enter(null, "mln_animation_options_default");
            return CopyAnimationOptions(NativeMethods.mln_animation_options_default());
        }
    }
}
