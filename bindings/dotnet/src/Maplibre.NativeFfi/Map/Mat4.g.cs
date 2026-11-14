// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A 4×4 matrix, by its columns: it takes a vector to <c>x</c> times the
/// vector's x, plus <c>y</c> times its y, <c>z</c> times its z, and <c>w</c>
/// times its w. In memory it is 16 doubles in column-major order, as OpenGL,
/// Metal, Vulkan, and WebGPU shaders take a matrix.
/// </summary>
/// <remarks>
/// See <c>mln_mat4</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct Mat4(Vec4 X, Vec4 Y, Vec4 Z, Vec4 W);
