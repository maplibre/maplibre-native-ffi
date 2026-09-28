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

public sealed record CameraOptions
{
    public LatLng? Center { get; set; }
    public double? CenterAltitude { get; set; }
    public EdgeInsets? Padding { get; set; }
    public ScreenPoint? Anchor { get; set; }
    public double? Zoom { get; set; }
    public double? Bearing { get; set; }
    public double? Pitch { get; set; }
    public double? Roll { get; set; }
    public double? FieldOfView { get; set; }
    public static CameraOptions Default
    {
        get
        {
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_camera_options_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyCameraOptions(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_camera_options_default()
            );
        }
    }
}
