// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Backend synchronization copied by frame access and release calls.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_sync</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct GpuSync(GpuSyncKind Kind, ulong Object, ulong Value)
{
    public static GpuSync Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_gpu_sync_default");
            return GeneratedValues.CopyGpuSync(NativeMethods.mln_gpu_sync_default());
        }
    }
}
