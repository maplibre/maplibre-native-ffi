// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Driver and target capabilities fixed for one attached render session.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_capabilities</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
/// <param name="TextureRingDepth">
/// Granted texture ring depth: the slot count of a session-owned ring, or the
/// texture count of a borrowed one. Zero for a surface.
/// </param>
public readonly partial record struct RenderSessionCapabilities(
    RenderDriverKind Driver,
    uint TextureRingDepth,
    RenderSessionCapabilityFlag Flags
);
