// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A range of image pixels, from <c>from</c> to <c>to</c>, that stretches to
/// fit text.
/// </summary>
/// <remarks>
/// See <c>mln_image_stretch</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct ImageStretch(float From, float To);
