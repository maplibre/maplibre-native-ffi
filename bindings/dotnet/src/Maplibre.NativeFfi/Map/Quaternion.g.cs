// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Quaternion stored as x, y, z, w components.
/// </summary>
/// <remarks>
/// See <c>mln_quaternion</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct Quaternion(double X, double Y, double Z, double W);
