// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Terminal disposition of one accepted frame demand.
/// </summary>
/// <remarks>
/// See <c>mln_render_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public enum RenderResult : uint
{
    /// <summary>
    /// A frame was rendered for acquisition, presentation, or ordered readback.
    /// </summary>
    Rendered = 0,

    /// <summary>
    /// No newer map update was available, the map had no complete frame to
    /// draw, or the update extent differed from the target. The map publishes
    /// another update when it has one. A demand with
    /// <c>MLN_FRAME_DEMAND_WAIT_FOR_UPDATE</c> waits for that update instead,
    /// and finishes with this result only when a barrier ends its wait.
    /// </summary>
    NoUpdate = 1,

    /// <summary>
    /// The target could not produce a frame. The attempt consumes nothing, so a
    /// later demand with the same flags renders what this one would have. This
    /// result does not cause a map update, so the host demands again when the
    /// target can be ready, such as after a paced delay.
    /// </summary>
    TargetNotReady = 3,

    /// <summary>
    /// A newer IF_NEEDED demand with the same flags replaced this demand.
    /// </summary>
    Superseded = 4,
}
