// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// What an attached render session supports.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_capabilities</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
/// <param name="TextureRingDepth">
/// How many textures the session's texture ring has.
/// </param>
/// <param name="ProducerSyncKind">
/// The kind of <c>GpuSync</c> that
/// <c>AcquiredFrameHandle.WithProducerSync()</c> reports.
/// </param>
/// <param name="ConsumerSyncKinds">
/// <para>The kinds of <c>GpuSync</c> that <c>AcquiredFrameHandle.Release()</c>
/// accepts.</para>
/// <para>They depend on the target and on what the device supports, so read
/// this field instead of assuming a kind.</para>
/// </param>
public readonly partial record struct RenderSessionCapabilities(
    RenderDriverKind Driver,
    uint TextureRingDepth,
    RenderSessionCapabilityFlag Flags,
    GpuSyncKind ProducerSyncKind,
    GpuSyncKindFlag ConsumerSyncKinds
);
