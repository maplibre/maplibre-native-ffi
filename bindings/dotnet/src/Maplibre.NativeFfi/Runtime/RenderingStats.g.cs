// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Rendering statistics reported in
/// <c>MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME</c>.
/// </summary>
/// <remarks>
/// See <c>mln_rendering_stats</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="EncodingTime">
/// Frame CPU encoding time in seconds.
/// </param>
/// <param name="RenderingTime">
/// Frame CPU rendering time in seconds.
/// </param>
/// <param name="FrameCount">
/// Number of frames rendered by the native renderer.
/// </param>
/// <param name="DrawCallCount">
/// Draw calls executed during the most recent frame.
/// </param>
/// <param name="TotalDrawCallCount">
/// Total draw calls executed by the native renderer.
/// </param>
public readonly partial record struct RenderingStats(
    double EncodingTime,
    double RenderingTime,
    long FrameCount,
    long DrawCallCount,
    long TotalDrawCallCount
);
