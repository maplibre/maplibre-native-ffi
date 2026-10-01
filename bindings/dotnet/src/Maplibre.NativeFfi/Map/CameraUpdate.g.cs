// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Map;

public readonly partial record struct CameraUpdate(
    CameraUpdateMode Mode,
    CameraOptions Camera,
    AnimationOptions Animation,
    GesturePhase GesturePhase
)
{
    public static CameraUpdate Default
    {
        get
        {
            using var call = Enter(null, "mln_camera_update_default");
            return CopyCameraUpdate(NativeMethods.mln_camera_update_default());
        }
    }
}
