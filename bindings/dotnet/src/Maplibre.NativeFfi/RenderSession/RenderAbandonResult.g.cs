// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The outcome of <c>RenderSessionHandle.Abandon()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_render_abandon_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RenderAbandonResult(
    RenderAbandonDisposition Disposition,
    uint QuarantinedResourceCount
);
