// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How a camera change animates.
/// </summary>
/// <remarks>
/// See <c>mln_animation_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record AnimationOptions
{
    public double? DurationMs { get; init; }

    /// <summary>
    /// The average speed of a fly animation, in screens per second.
    /// <c>duration_ms</c> overrides it.
    /// </summary>
    public double? Velocity { get; init; }

    /// <summary>
    /// The zoom level at the peak of a fly animation.
    /// </summary>
    public double? MinZoom { get; init; }
    public UnitBezier? Easing { get; init; }

    /// <summary>
    /// An ID that <c>MapHandle.CancelCameraTransitionsWithIdAsync()</c> cancels
    /// the change by. Several changes can share one.
    /// </summary>
    public ulong? TransitionId { get; init; }
    public CameraTransitionHandler EndHandler { get; init; }
}
