// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public readonly partial record struct GpuSync(GpuSyncKind Kind, ulong Object, ulong Value)
{
    public static GpuSync Default
    {
        get
        {
            using var call = Enter(null, "mln_gpu_sync_default");
            return CopyGpuSync(NativeMethods.mln_gpu_sync_default());
        }
    }
}
