namespace Maplibre.NativeFfi;

/// <summary>
/// Bit pattern of a borrowed Vulkan non-dispatchable handle. It transfers no ownership and grants
/// no memory access. Zero represents <c>VK_NULL_HANDLE</c>.
/// </summary>
public readonly record struct VulkanHandle(ulong Bits)
{
    /// <summary>Null Vulkan non-dispatchable handle.</summary>
    public static VulkanHandle Null => default;

    /// <summary>Returns whether this value represents <c>VK_NULL_HANDLE</c>.</summary>
    public bool IsNull => Bits == 0;
}
