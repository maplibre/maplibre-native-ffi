// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How a gesture keeps moving the camera after the user lets go.
/// </summary>
/// <remarks>
/// <para>A part moves only when its velocity is set, and each part slows on its
/// own. Every part defaults to <c>MomentumDecayModel.Friction</c> with a
/// half-life of 140 ms, and these other defaults:</para>
/// <list type="bullet">
/// <item><description>Pan stops at 10 logical pixels per second, needs a
/// release of at least 200, and counts a release faster than 10000 as
/// 10000.</description></item>
/// <item><description>Zoom stops at 0.01 levels per second and moves at most
/// 2.5 levels. Its snap settles on the nearest whole
/// level.</description></item>
/// <item><description>Bearing stops at 1 degree per second and needs a release
/// of at least 60. Its snap settles on north within 7
/// degrees.</description></item>
/// <item><description>Pitch stops at 1 degree per second and needs a release of
/// at least 5. Its snap settles flat within 7 degrees.</description></item>
/// </list>
/// <para>See <c>mln_gesture_momentum</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct GestureMomentum(
    PanMomentum Pan,
    AnchoredMomentum Zoom,
    AnchoredMomentum Bearing,
    AnchoredMomentum Pitch
)
{
    public GestureMomentum()
        : this(new(), new(), new(), new()) { }
}
