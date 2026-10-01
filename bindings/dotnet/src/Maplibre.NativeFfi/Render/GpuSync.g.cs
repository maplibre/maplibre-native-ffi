// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

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
