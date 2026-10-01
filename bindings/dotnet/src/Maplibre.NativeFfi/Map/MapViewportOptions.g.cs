// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Map;

public sealed record MapViewportOptions
{
    public NorthOrientation? NorthOrientation { get; set; }
    public ConstrainMode? ConstrainMode { get; set; }
    public ViewportMode? ViewportMode { get; set; }
    public EdgeInsets? FrustumOffset { get; set; }
    public static MapViewportOptions Default
    {
        get
        {
            using var call = Enter(null, "mln_map_viewport_options_default");
            return CopyMapViewportOptions(NativeMethods.mln_map_viewport_options_default());
        }
    }
}
