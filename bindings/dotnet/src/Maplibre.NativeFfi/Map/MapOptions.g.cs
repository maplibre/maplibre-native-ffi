// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Map;

public readonly partial record struct MapOptions(
    LogicalExtent InitialExtent,
    MapMode MapMode,
    bool FastPforEnabled,
    RuntimeEventMask EventMask
)
{
    public static MapOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_map_options_default");
            return GeneratedValues.CopyMapOptions(NativeMethods.mln_map_options_default());
        }
    }
}
