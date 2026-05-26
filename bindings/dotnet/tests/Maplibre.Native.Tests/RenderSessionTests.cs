using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Query;
using Maplibre.Native.Render;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class RenderSessionTests
{
    [Fact]
    public void SurfaceDescriptorsMaterializeOpaquePointersAndExtent()
    {
        var metal = RenderStructs.ToNative(new MetalSurfaceDescriptor
        {
            Extent = new RenderTargetExtent(320, 240, 2),
            Layer = new NativePointer(123),
            Context = new MetalContextDescriptor { Device = new NativePointer(456) },
        });
        Assert.Equal(320u, metal.extent.width);
        Assert.Equal(240u, metal.extent.height);
        Assert.Equal(2, metal.extent.scale_factor);
        Assert.Equal(123, (nint)metal.layer);
        Assert.Equal(456, (nint)metal.context.device);

        var vulkan = RenderStructs.ToNative(new VulkanSurfaceDescriptor
        {
            Extent = new RenderTargetExtent(640, 480, 1),
            Surface = new NativePointer(111),
            Context = new VulkanContextDescriptor
            {
                Instance = new NativePointer(222),
                PhysicalDevice = new NativePointer(333),
                Device = new NativePointer(444),
                Queue = new NativePointer(555),
                GraphicsQueueFamilyIndex = 7,
            },
        });
        Assert.Equal(640u, vulkan.extent.width);
        Assert.Equal(480u, vulkan.extent.height);
        Assert.Equal(111, (nint)vulkan.surface);
        Assert.Equal(222, (nint)vulkan.context.instance);
        Assert.Equal(333, (nint)vulkan.context.physical_device);
        Assert.Equal(444, (nint)vulkan.context.device);
        Assert.Equal(555, (nint)vulkan.context.graphics_queue);
        Assert.Equal(7u, vulkan.context.graphics_queue_family_index);
    }

    [Fact]
    public void TextureDescriptorsMaterializeOpaquePointersAndExtent()
    {
        var metalOwned = RenderStructs.ToNative(new MetalOwnedTextureDescriptor
        {
            Extent = new RenderTargetExtent(128, 64, 2),
            Context = new MetalContextDescriptor { Device = new NativePointer(10) },
        });
        Assert.Equal(128u, metalOwned.extent.width);
        Assert.Equal(64u, metalOwned.extent.height);
        Assert.Equal(10, (nint)metalOwned.context.device);

        var metalBorrowed = RenderStructs.ToNative(new MetalBorrowedTextureDescriptor
        {
            Extent = new RenderTargetExtent(128, 64, 2),
            Texture = new NativePointer(20),
        });
        Assert.Equal(20, (nint)metalBorrowed.texture);

        var vulkanOwned = RenderStructs.ToNative(new VulkanOwnedTextureDescriptor
        {
            Extent = new RenderTargetExtent(256, 128, 1),
            Context = new VulkanContextDescriptor { Device = new NativePointer(30) },
        });
        Assert.Equal(256u, vulkanOwned.extent.width);
        Assert.Equal(30, (nint)vulkanOwned.context.device);

        var vulkanBorrowed = RenderStructs.ToNative(new VulkanBorrowedTextureDescriptor
        {
            Extent = new RenderTargetExtent(256, 128, 1),
            Image = new NativePointer(40),
            ImageView = new NativePointer(45),
            Format = 50,
            InitialLayout = 55,
            FinalLayout = 60,
        });
        Assert.Equal(40, (nint)vulkanBorrowed.image);
        Assert.Equal(45, (nint)vulkanBorrowed.image_view);
        Assert.Equal(50u, vulkanBorrowed.format);
        Assert.Equal(55u, vulkanBorrowed.initial_layout);
        Assert.Equal(60u, vulkanBorrowed.final_layout);
    }

    [Fact]
    public void RenderDescriptorsPreserveNativeDefaultsWhenExtentOmitted()
    {
        var metal = RenderStructs.ToNative(new MetalSurfaceDescriptor { Layer = new NativePointer(1) });
        Assert.Equal(256u, metal.extent.width);
        Assert.Equal(256u, metal.extent.height);
        Assert.Equal(1, metal.extent.scale_factor);

        var vulkanBorrowed = RenderStructs.ToNative(new VulkanBorrowedTextureDescriptor
        {
            Image = new NativePointer(2),
            ImageView = new NativePointer(3),
        });
        Assert.Equal(256u, vulkanBorrowed.extent.width);
        Assert.Equal(256u, vulkanBorrowed.extent.height);
        Assert.Equal(1, vulkanBorrowed.extent.scale_factor);
        Assert.Equal(5u, vulkanBorrowed.final_layout);
    }

    [Fact]
    public void TextureImageInfoCopiesNativeFields()
    {
        var info = RenderStructs.FromNative(new mln_texture_image_info
        {
            width = 1,
            height = 2,
            stride = 4,
            byte_length = 8,
        });

        Assert.Equal(new TextureImageInfo(1, 2, 4, 8), info);
    }

    [Fact]
    public void NativeBufferRejectsUseAfterDispose()
    {
        using var buffer = new NativeBuffer(4);
        Assert.NotEqual(0, buffer.Pointer.Address);
        Assert.Equal(4, buffer.Span.Length);

        buffer.Dispose();

        Assert.Throws<ObjectDisposedException>(() => buffer.Pointer);
        Assert.Throws<ObjectDisposedException>(() =>
        {
            var span = buffer.Span;
            _ = span.Length;
        });
    }

    [Fact]
    public void TextureFramePropertiesRejectUseAfterScopeClose()
    {
        var metalScope = new FrameScope(nameof(MetalOwnedTextureFrame));
        var metal = new MetalOwnedTextureFrame(metalScope, 1, 2, 3, 4, 5, new NativePointer(6), new NativePointer(7), 8);
        Assert.Equal(6, metal.Texture.Address);
        metalScope.Dispose();
        Assert.Throws<ObjectDisposedException>(() => metal.Texture);

        var vulkanScope = new FrameScope(nameof(VulkanOwnedTextureFrame));
        var vulkan = new VulkanOwnedTextureFrame(vulkanScope, 1, 2, 3, 4, 5, new NativePointer(6), new NativePointer(7), new NativePointer(8), 9, 10);
        Assert.Equal(7, vulkan.ImageView.Address);
        vulkanScope.Dispose();
        Assert.Throws<ObjectDisposedException>(() => vulkan.ImageView);
    }

    [Fact]
    public void FeatureStateSelectorMaterializesOptionalFields()
    {
        using var selector = NativeFeatureStateSelector.From(new FeatureStateSelector
        {
            SourceId = "source",
            SourceLayerId = "layer",
            FeatureId = "feature",
            StateKey = "hover",
        });

        var value = selector.Value;
        Assert.Equal((uint)(
            mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID |
            mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID |
            mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_STATE_KEY), value.fields);
        Assert.Equal("source", RuntimeStructs.CopyUtf8(value.source_id.data, value.source_id.size));
        Assert.Equal("layer", RuntimeStructs.CopyUtf8(value.source_layer_id.data, value.source_layer_id.size));
        Assert.Equal("feature", RuntimeStructs.CopyUtf8(value.feature_id.data, value.feature_id.size));
        Assert.Equal("hover", RuntimeStructs.CopyUtf8(value.state_key.data, value.state_key.size));
    }
}
