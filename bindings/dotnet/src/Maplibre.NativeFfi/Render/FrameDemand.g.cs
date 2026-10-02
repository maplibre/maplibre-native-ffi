// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
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
            using var call = NativeCall.Enter(null, "mln_frame_demand_default");
            return GeneratedValues.CopyFrameDemand(NativeMethods.mln_frame_demand_default());
        }
    }
}
