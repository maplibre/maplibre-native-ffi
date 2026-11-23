// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How a gesture recognizer moves each part of the camera.
/// </summary>
/// <remarks>
/// <para>Settings whose bits aren't set keep their values. A new recognizer
/// moves every part, and glides each by friction with a half-life of 140 ms, a
/// minimum speed of 0, and no maximum speed or distance, with these
/// exceptions:</para>
/// <list type="bullet">
/// <item><description>Pan stops at 10 logical pixels per second, needs a
/// release of at least 200, and counts a release faster than 10000 as
/// 10000.</description></item>
/// <item><description>Zoom stops at 0.01 levels per second and glides at most
/// 2.5 levels. Its snap, when on, settles on the nearest whole level within
/// half a level.</description></item>
/// <item><description>Bearing stops at 1 degree per second and needs a release
/// of at least 60. It snaps to north within 7 degrees, and has a notch of kind
/// 0 at north.</description></item>
/// <item><description>Pitch stops at 1 degree per second and needs a release of
/// at least 5. Its snap, when on, settles flat within 7
/// degrees.</description></item>
/// </list>
/// <para>Snaps take 300 ms. A double tap zooms in a level and a two-finger tap
/// out.</para>
/// <para>See <c>mln_gesture_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record GestureOptions
{
    /// <summary>
    /// Zoom levels that a double tap zooms in, eased; with
    /// <c>GestureMode.ZoomOut</c> it zooms out instead. 0 leaves double taps to
    /// the host. Finite.
    /// </summary>
    public double? DoubleTapZoom { get; init; }

    /// <summary>
    /// Zoom levels that a two-finger tap zooms, eased; negative zooms out. 0
    /// leaves two-finger taps to the host. Finite.
    /// </summary>
    public double? TwoFingerTapZoom { get; init; }
    public GesturePanOptions Pan { get; init; } = new();
    public GestureAxisOptions Zoom { get; init; } = new();
    public GestureAxisOptions Bearing { get; init; } = new();
    public GestureAxisOptions Pitch { get; init; } = new();
}
