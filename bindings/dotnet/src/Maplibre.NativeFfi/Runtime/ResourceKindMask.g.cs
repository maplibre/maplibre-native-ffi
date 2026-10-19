// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// The resource kinds that a route or rule matches, one bit per
/// <c>mln_resource_kind</c> value. Registration requires at least one known
/// bit. <c>MLN_RESOURCE_KIND_MASK_ALL</c> selects every resource kind.
/// </summary>
/// <remarks>
/// See <c>mln_resource_kind_mask</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum ResourceKindMask : uint
{
    Unknown = 1,
    Style = 2,
    Source = 4,
    Tile = 8,
    Glyphs = 16,
    SpriteImage = 32,
    SpriteJson = 64,
    Image = 128,
    All = 255,
}
