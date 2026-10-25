// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Free camera position and orientation in MapLibre Native camera space.
/// </summary>
/// <remarks>
/// See <c>mln_free_camera_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record FreeCameraOptions
{
    public Vec3? Position { get; init; }
    public Quaternion? Orientation { get; init; }
}
