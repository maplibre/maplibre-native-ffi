// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

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
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_map_viewport_options_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyMapViewportOptions(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_map_viewport_options_default()
            );
        }
    }
}
