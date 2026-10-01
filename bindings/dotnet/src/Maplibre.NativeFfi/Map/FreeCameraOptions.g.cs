// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Map;

public sealed record FreeCameraOptions
{
    public Vec3? Position { get; set; }
    public Quaternion? Orientation { get; set; }
    public static FreeCameraOptions Default
    {
        get
        {
            using var call = Enter(null, "mln_free_camera_options_default");
            return CopyFreeCameraOptions(NativeMethods.mln_free_camera_options_default());
        }
    }
}
