// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// A way to share a texture's memory, as a bit in a set.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_export_kind_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum GpuExportKindFlag : uint
{
    IoSurface = 1,
    DmaBuf = 2,
    AndroidHardwareBuffer = 4,
    DxgiSharedHandle = 8,
    OpaqueFd = 16,
}
