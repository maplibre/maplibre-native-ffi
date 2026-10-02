// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Runtime;

public readonly partial record struct RenderingStats(
    double EncodingTime,
    double RenderingTime,
    long FrameCount,
    long DrawCallCount,
    long TotalDrawCallCount
);
