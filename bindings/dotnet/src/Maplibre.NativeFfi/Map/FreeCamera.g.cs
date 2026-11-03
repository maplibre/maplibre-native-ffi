// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A map camera's whole position and orientation in three dimensions.
/// </summary>
/// <remarks>
/// <para>Its members mean the same as those of <c>FreeCameraOptions</c>.</para>
/// <para>See <c>mln_free_camera</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct FreeCamera(Vec3 Position, Quaternion Orientation);
