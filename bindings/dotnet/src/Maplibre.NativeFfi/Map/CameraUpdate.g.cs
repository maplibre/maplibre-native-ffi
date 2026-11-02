// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A camera change for <c>MapHandle.UpdateCameraAsync()</c>.
/// </summary>
/// <remarks>
/// <para>A jump uses only the end handler of <c>animation</c>, and the handler
/// runs as soon as the camera moves.</para>
/// <para>See <c>mln_camera_update</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct CameraUpdate(
    CameraUpdateMode Mode,
    CameraOptions Camera,
    AnimationOptions Animation,
    GesturePhase GesturePhase
)
{
    public CameraUpdate()
        : this(default, new(), new(), default) { }
}
