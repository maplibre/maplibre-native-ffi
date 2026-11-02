// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How one frame request finished.
/// </summary>
/// <remarks>
/// See <c>mln_render_frame_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
/// <param name="MapUpdateGeneration">
/// The map update that a rendered frame shows. The frame includes a command's
/// change when this is at least the command's
/// <c>CommandCompletion.RenderUpdateGeneration</c>.
/// </param>
/// <param name="View">
/// The camera and extent that a rendered frame shows.
/// </param>
public readonly partial record struct RenderFrameResult(
    RenderResult Disposition,
    ulong Token,
    ulong MapUpdateGeneration,
    ulong Generation,
    ulong FrameGeneration,
    bool NeedsRepaint,
    MapView View,
    RenderMode Mode
);
