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

public sealed record BoundOptions
{
    public LatLngBounds? Bounds { get; set; }
    public double? MinZoom { get; set; }
    public double? MaxZoom { get; set; }
    public double? MinPitch { get; set; }
    public double? MaxPitch { get; set; }
    public bool Unbounded { get; set; }
    public static BoundOptions Default
    {
        get
        {
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_bound_options_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyBoundOptions(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_bound_options_default()
            );
        }
    }
}
