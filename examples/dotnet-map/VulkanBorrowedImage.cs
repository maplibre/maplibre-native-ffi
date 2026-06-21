using Maplibre.Native;
using Silk.NET.Vulkan;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed unsafe class VulkanBorrowedImage : IDisposable
{
    public const Format ImageFormat = Format.R8G8B8A8Unorm;
    public const ImageLayout InitialLayout = ImageLayout.Undefined;
    public const ImageLayout FinalLayout = ImageLayout.ShaderReadOnlyOptimal;

    private readonly VulkanContext context;
    private Image image;
    private DeviceMemory memory;
    private ImageView view;

    public VulkanBorrowedImage(VulkanContext context, Viewport viewport)
    {
        this.context = context;
        try
        {
            Create(viewport);
        }
        catch
        {
            Dispose();
            throw;
        }
    }

    public NativePointer ImagePointer => NativePointer.FromBorrowedAddress((nint)image.Handle);

    public NativePointer ViewPointer => NativePointer.FromBorrowedAddress((nint)view.Handle);

    public ImageView View => view;

    public void Dispose()
    {
        var vk = context.Api;
        var device = context.Device;
        if (view.Handle != 0)
        {
            vk.DestroyImageView(device, view, null);
            view = default;
        }

        if (image.Handle != 0)
        {
            vk.DestroyImage(device, image, null);
            image = default;
        }

        if (memory.Handle != 0)
        {
            vk.FreeMemory(device, memory, null);
            memory = default;
        }
    }

    private void Create(Viewport viewport)
    {
        var vk = context.Api;
        var imageInfo = new ImageCreateInfo
        {
            SType = StructureType.ImageCreateInfo,
            ImageType = ImageType.Type2D,
            Format = ImageFormat,
            Extent = new Extent3D(viewport.PhysicalWidth, viewport.PhysicalHeight, 1),
            MipLevels = 1,
            ArrayLayers = 1,
            Samples = SampleCountFlags.Count1Bit,
            Tiling = ImageTiling.Optimal,
            Usage = ImageUsageFlags.ColorAttachmentBit | ImageUsageFlags.SampledBit,
            SharingMode = SharingMode.Exclusive,
            InitialLayout = InitialLayout,
        };
        VulkanContext.Check(
            vk.CreateImage(context.Device, &imageInfo, null, out image),
            "vkCreateImage"
        );

        var requirements = vk.GetImageMemoryRequirements(context.Device, image);
        var allocateInfo = new MemoryAllocateInfo
        {
            SType = StructureType.MemoryAllocateInfo,
            AllocationSize = requirements.Size,
            MemoryTypeIndex = FindMemoryType(
                requirements.MemoryTypeBits,
                MemoryPropertyFlags.DeviceLocalBit
            ),
        };
        VulkanContext.Check(
            vk.AllocateMemory(context.Device, &allocateInfo, null, out memory),
            "vkAllocateMemory"
        );
        VulkanContext.Check(
            vk.BindImageMemory(context.Device, image, memory, 0),
            "vkBindImageMemory"
        );

        var subresourceRange = new ImageSubresourceRange
        {
            AspectMask = ImageAspectFlags.ColorBit,
            BaseMipLevel = 0,
            LevelCount = 1,
            BaseArrayLayer = 0,
            LayerCount = 1,
        };
        var viewInfo = new ImageViewCreateInfo
        {
            SType = StructureType.ImageViewCreateInfo,
            Image = image,
            ViewType = ImageViewType.Type2D,
            Format = ImageFormat,
            SubresourceRange = subresourceRange,
        };
        VulkanContext.Check(
            vk.CreateImageView(context.Device, &viewInfo, null, out view),
            "vkCreateImageView"
        );
    }

    private uint FindMemoryType(uint typeBits, MemoryPropertyFlags requiredProperties)
    {
        context.Api.GetPhysicalDeviceMemoryProperties(context.PhysicalDevice, out var properties);
        for (uint i = 0; i < properties.MemoryTypeCount; i++)
        {
            var supported = (typeBits & (1u << checked((int)i))) != 0;
            var memoryType = properties.MemoryTypes[checked((int)i)];
            var hasProperties =
                (memoryType.PropertyFlags & requiredProperties) == requiredProperties;
            if (supported && hasProperties)
            {
                return i;
            }
        }

        throw new InvalidOperationException("No compatible Vulkan memory type found.");
    }
}
