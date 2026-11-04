// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Why a map failed to load a style.
/// </summary>
/// <remarks>
/// See <c>mln_map_load_error</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public enum MapLoadError : uint
{
    Unknown = 0,

    /// <summary>
    /// The style document isn't a valid style.
    /// </summary>
    StyleParse = 1,

    /// <summary>
    /// The style's request failed, such as with a 404 or a missing file.
    /// </summary>
    StyleLoad = 2,
}
