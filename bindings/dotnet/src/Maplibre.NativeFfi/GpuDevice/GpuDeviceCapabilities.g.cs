// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// What a GPU device can do.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_device_capabilities</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.
/// </remarks>
/// <param name="TargetKinds">
/// The kinds of <c>RenderTarget</c> that sessions on the device render to.
/// </param>
/// <param name="ExportKinds">
/// The ways that the device can share the textures of an exported ring. A kind
/// is here only once the device has exported a texture of it, so a driver that
/// offers an extension but can't allocate the memory reports no kind of it.
/// </param>
/// <param name="SyncKinds">
/// The kinds of <c>GpuSync</c> that the device can signal and wait on.
/// </param>
/// <param name="DepthStencilFormats">
/// The formats of the depth textures that a borrowed ring takes.
/// </param>
/// <param name="DepthExportKinds">
/// The kinds of <c>export_kinds</c> whose rings also share each frame's depth
/// texture, with <c>RenderSessionAttachOptions.KeepDepth</c>. An IOSurface or a
/// dmabuf holds no depth format, so only <c>GpuExportKind.OpaqueFd</c> can be
/// here.
/// </param>
public readonly partial record struct GpuDeviceCapabilities(
    RenderBackend Backend,
    RenderTargetKindFlag TargetKinds,
    GpuExportKindFlag ExportKinds,
    GpuSyncKindFlag SyncKinds,
    DepthStencilFormatFlag DepthStencilFormats,
    GpuExportKindFlag DepthExportKinds
);
