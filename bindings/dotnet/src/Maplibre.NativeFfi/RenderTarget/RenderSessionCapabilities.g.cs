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
/// <param name="Driver">
/// One <c>mln_render_driver_kind</c> value.
/// </param>
/// <param name="TextureRingDepth">
/// Granted owned-texture slot count, or zero for a target without a ring.
/// </param>
/// <param name="Flags">
/// A bitwise OR of <c>mln_render_session_capability_flag</c> values.
/// </param>
public readonly partial record struct RenderSessionCapabilities(
    RenderDriverKind Driver,
    uint TextureRingDepth,
    RenderSessionCapabilityFlag Flags
);
