// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Map;

public sealed record CameraFitOptions
{
    public EdgeInsets? Padding { get; set; }
    public double? Bearing { get; set; }
    public double? Pitch { get; set; }
    public static CameraFitOptions Default
    {
        get
        {
            using var call = Enter(null, "mln_camera_fit_options_default");
            return CopyCameraFitOptions(NativeMethods.mln_camera_fit_options_default());
        }
    }
}
