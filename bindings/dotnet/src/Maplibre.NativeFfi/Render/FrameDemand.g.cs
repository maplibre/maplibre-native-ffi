// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Render;

public readonly partial record struct FrameDemand(
    FrameDemandFlag Flags,
    ulong Token,
    ulong CoalescingBoundary,
    ulong TimeoutNs
)
{
    public static FrameDemand Default
    {
        get
        {
            using var call = Enter(null, "mln_frame_demand_default");
            return CopyFrameDemand(NativeMethods.mln_frame_demand_default());
        }
    }
}
