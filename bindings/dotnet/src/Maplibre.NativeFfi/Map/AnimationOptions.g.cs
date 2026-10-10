// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Optional animation controls for camera transitions.
/// </summary>
/// <remarks>
/// See <c>mln_animation_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record AnimationOptions
{
    /// <summary>
    /// Duration in milliseconds. Must be finite and non-negative. Values that
    /// would overflow MapLibre Native's internal duration are invalid.
    /// </summary>
    public double? DurationMs { get; set; }

    /// <summary>
    /// Average fly velocity in screenfuls per second. Must be positive and
    /// defaults to 1.2 when omitted.
    /// </summary>
    public double? Velocity { get; set; }

    /// <summary>
    /// Peak zoom for flyTo transitions.
    /// </summary>
    public double? MinZoom { get; set; }
    public UnitBezier? Easing { get; set; }

    /// <summary>
    /// Caller-chosen identity of the command that these options animate, which
    /// <c>mln_map_cancel_camera_transition()</c> matches.
    /// </summary>
    public ulong? TransitionId { get; set; }

    /// <summary>
    /// Reports the end of the command's transitions. Disabled by default.
    /// </summary>
    public CameraTransitionHandler EndHandler { get; set; }
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
