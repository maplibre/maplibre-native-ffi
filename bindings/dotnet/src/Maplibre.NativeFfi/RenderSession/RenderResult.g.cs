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
    /// No newer map update was available.
    /// </summary>
    NoUpdate = 1,

    /// <summary>
    /// An ordered extent change had not reached the driver.
    /// </summary>
    SizePending = 2,

    /// <summary>
    /// The target could not produce a frame.
    /// </summary>
    TargetNotReady = 3,

    /// <summary>
    /// A newer demand in the same coalescing boundary replaced this demand.
    /// </summary>
    Superseded = 4,

    /// <summary>
    /// The demand's timeout elapsed before driver work began.
    /// </summary>
    DeadlineMissed = 5,
}
