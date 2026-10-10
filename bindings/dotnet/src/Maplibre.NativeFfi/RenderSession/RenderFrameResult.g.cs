// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Immutable result record copied into an owned frame-result batch.
/// </summary>
/// <remarks>
/// See <c>mln_render_frame_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RenderFrameResult(
    RenderResult Disposition,
    ulong Token,
    ulong MapUpdateGeneration,
    ulong ExtentGeneration,
    ulong FrameGeneration,
    bool NeedsRepaint
);
