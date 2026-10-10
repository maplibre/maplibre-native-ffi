// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options used when creating a map.
/// </summary>
/// <remarks>
/// See <c>mln_map_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct MapOptions(
    LogicalExtent InitialExtent,
    MapMode MapMode,
    bool FastPforEnabled,
    RuntimeEventMask EventMask
)
{
    public MapOptions()
        : this(new LogicalExtent(), default, default, RuntimeEventMask.All) { }

    public static MapOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_map_options_default");
            return GeneratedValues.CopyMapOptions(NativeMethods.mln_map_options_default());
        }
    }
}
