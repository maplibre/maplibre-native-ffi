// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Statistics about the frames that a map renders.
/// </summary>
/// <remarks>
/// See <c>mln_rendering_stats</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="EncodingTime">
/// The time that the frame took to encode, in seconds.
/// </param>
/// <param name="RenderingTime">
/// The time that the frame took to render, in seconds.
/// </param>
/// <param name="FrameCount">
/// The frames rendered so far.
/// </param>
/// <param name="DrawCallCount">
/// The draw calls in the frame.
/// </param>
/// <param name="TotalDrawCallCount">
/// The draw calls in every frame so far.
/// </param>
public readonly partial record struct RenderingStats(
    double EncodingTime,
    double RenderingTime,
    long FrameCount,
    long DrawCallCount,
    long TotalDrawCallCount
);
