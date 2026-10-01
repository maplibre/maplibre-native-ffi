// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

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
            using var call = Enter(null, "mln_map_options_default");
            return CopyMapOptions(NativeMethods.mln_map_options_default());
        }
    }
}
