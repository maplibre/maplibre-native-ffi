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
    /// Duration in milliseconds. Must be finite, non-negative, and short enough
    /// to represent as a duration.
    /// </summary>
    public double? DurationMs { get; init; }

    /// <summary>
    /// Average fly velocity in screenfuls per second. Must be positive and
    /// finite; defaults to 1.2 when omitted. A flight whose derived duration is
    /// too long to represent fails its completion with
    /// <c>MLN_STATUS_NATIVE_ERROR</c>. An explicit duration overrides velocity.
    /// </summary>
    public double? Velocity { get; init; }

    /// <summary>
    /// Peak zoom for flyTo transitions.
    /// </summary>
    public double? MinZoom { get; init; }
    public UnitBezier? Easing { get; init; }

    /// <summary>
    /// Caller-chosen identity of the command that these options animate, which
    /// <c>mln_map_cancel_camera_transition()</c> matches.
    /// </summary>
    public ulong? TransitionId { get; init; }

    /// <summary>
    /// Reports the end of the command's transitions. Disabled by default.
    /// </summary>
    public CameraTransitionHandler EndHandler { get; init; }
    public static AnimationOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyAnimationOptions(
                NativeMethods.mln_animation_options_default()
            );
        }
    }
}
