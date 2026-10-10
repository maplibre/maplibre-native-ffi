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
public readonly partial record struct RenderingStats(
    double EncodingTime,
    double RenderingTime,
    long FrameCount,
    long DrawCallCount,
    long TotalDrawCallCount
);
