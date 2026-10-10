// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Metal attachment options for an owned texture target.
/// </summary>
/// <remarks>
/// See <c>mln_metal_owned_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Extent">
/// Logical texture extent. A scale_factor that differs from the map's is
/// accepted and logged as a warning.
/// </param>
/// <param name="Context">
/// Metal backend context. device is required.
/// </param>
public readonly partial record struct MetalOwnedTextureDescriptor(
    LogicalExtent Extent,
    MetalContextDescriptor Context
)
{
    public MetalOwnedTextureDescriptor()
        : this(new LogicalExtent(), default) { }

    public static MetalOwnedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_metal_owned_texture_descriptor_default");
            return GeneratedValues.CopyMetalOwnedTextureDescriptor(
                NativeMethods.mln_metal_owned_texture_descriptor_default()
            );
        }
    }
}
