using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Map;
using Maplibre.Native.Query;
using Maplibre.Native.Render;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class RenderSessionTests
{
    [Fact]
    public void SurfaceDescriptorsMaterializeOpaquePointersAndExtent()
    {
        var metal = RenderStructs.ToNative(
            new MetalSurfaceDescriptor
            {
                Extent = new RenderTargetExtent(320, 240, 2),
                Layer = new NativePointer(123),
                Context = new MetalContextDescriptor { Device = new NativePointer(456) },
            }
        );
        Assert.Equal(320u, metal.extent.width);
        Assert.Equal(240u, metal.extent.height);
        Assert.Equal(2, metal.extent.scale_factor);
        Assert.Equal(123, (nint)metal.layer);
        Assert.Equal(456, (nint)metal.context.device);

        var vulkan = RenderStructs.ToNative(
            new VulkanSurfaceDescriptor
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
                    GetInstanceProcAddr = new NativePointer(666),
                    GetDeviceProcAddr = new NativePointer(777),
                },
            }
        );
        Assert.Equal(640u, vulkan.extent.width);
        Assert.Equal(480u, vulkan.extent.height);
        Assert.Equal(111, (nint)vulkan.surface);
        Assert.Equal(222, (nint)vulkan.context.instance);
        Assert.Equal(333, (nint)vulkan.context.physical_device);
        Assert.Equal(444, (nint)vulkan.context.device);
        Assert.Equal(555, (nint)vulkan.context.graphics_queue);
        Assert.Equal(7u, vulkan.context.graphics_queue_family_index);
        Assert.Equal(666, (nint)vulkan.context.get_instance_proc_addr);
        Assert.Equal(777, (nint)vulkan.context.get_device_proc_addr);

        var opengl = RenderStructs.ToNative(
            new OpenGLSurfaceDescriptor
            {
                Extent = new RenderTargetExtent(800, 600, 2),
                Surface = new NativePointer(888),
                Context = new WglContextDescriptor
                {
                    DeviceContext = new NativePointer(999),
                    ShareContext = new NativePointer(1000),
                    GetProcAddress = new NativePointer(1001),
                },
            }
        );
        Assert.Equal(800u, opengl.extent.width);
        Assert.Equal(600u, opengl.extent.height);
        Assert.Equal(888, (nint)opengl.surface);
        Assert.Equal(
            mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_WGL,
            opengl.context.platform
        );
        Assert.Equal(999, (nint)opengl.context.data.wgl.device_context);
        Assert.Equal(1000, (nint)opengl.context.data.wgl.share_context);
        Assert.Equal(1001, (nint)opengl.context.data.wgl.get_proc_address);
    }

    [Fact]
    public void TextureDescriptorsMaterializeOpaquePointersAndExtent()
    {
        var metalOwned = RenderStructs.ToNative(
            new MetalOwnedTextureDescriptor
            {
                Extent = new RenderTargetExtent(128, 64, 2),
                Context = new MetalContextDescriptor { Device = new NativePointer(10) },
            }
        );
        Assert.Equal(128u, metalOwned.extent.width);
        Assert.Equal(64u, metalOwned.extent.height);
        Assert.Equal(10, (nint)metalOwned.context.device);

        var metalBorrowed = RenderStructs.ToNative(
            new MetalBorrowedTextureDescriptor
            {
                Extent = new RenderTargetExtent(128, 64, 2),
                Texture = new NativePointer(20),
            }
        );
        Assert.Equal(20, (nint)metalBorrowed.texture);

        var vulkanOwned = RenderStructs.ToNative(
            new VulkanOwnedTextureDescriptor
            {
                Extent = new RenderTargetExtent(256, 128, 1),
                Context = new VulkanContextDescriptor
                {
                    Device = new NativePointer(30),
                    GetInstanceProcAddr = new NativePointer(31),
                    GetDeviceProcAddr = new NativePointer(32),
                },
            }
        );
        Assert.Equal(256u, vulkanOwned.extent.width);
        Assert.Equal(30, (nint)vulkanOwned.context.device);
        Assert.Equal(31, (nint)vulkanOwned.context.get_instance_proc_addr);
        Assert.Equal(32, (nint)vulkanOwned.context.get_device_proc_addr);

        var vulkanBorrowed = RenderStructs.ToNative(
            new VulkanBorrowedTextureDescriptor
            {
                Extent = new RenderTargetExtent(256, 128, 1),
                Image = new NativePointer(40),
                ImageView = new NativePointer(45),
                Format = 50,
                InitialLayout = 55,
                FinalLayout = 60,
            }
        );
        Assert.Equal(40, (nint)vulkanBorrowed.image);
        Assert.Equal(45, (nint)vulkanBorrowed.image_view);
        Assert.Equal(50u, vulkanBorrowed.format);
        Assert.Equal(55u, vulkanBorrowed.initial_layout);
        Assert.Equal(60u, vulkanBorrowed.final_layout);

        var openglOwned = RenderStructs.ToNative(
            new OpenGLOwnedTextureDescriptor
            {
                Extent = new RenderTargetExtent(512, 256, 1),
                Context = new EglContextDescriptor
                {
                    Display = new NativePointer(60),
                    Config = new NativePointer(61),
                    ShareContext = new NativePointer(62),
                    GetProcAddress = new NativePointer(63),
                },
            }
        );
        Assert.Equal(512u, openglOwned.extent.width);
        Assert.Equal(
            mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_EGL,
            openglOwned.context.platform
        );
        Assert.Equal(60, (nint)openglOwned.context.data.egl.display);
        Assert.Equal(61, (nint)openglOwned.context.data.egl.config);
        Assert.Equal(62, (nint)openglOwned.context.data.egl.share_context);
        Assert.Equal(63, (nint)openglOwned.context.data.egl.get_proc_address);

        var openglBorrowed = RenderStructs.ToNative(
            new OpenGLBorrowedTextureDescriptor
            {
                Extent = new RenderTargetExtent(512, 256, 1),
                Context = new WglContextDescriptor
                {
                    DeviceContext = new NativePointer(70),
                    ShareContext = new NativePointer(71),
                },
                Texture = 72,
                Target = 0x0de1,
            }
        );
        Assert.Equal(72u, openglBorrowed.texture);
        Assert.Equal(0x0de1u, openglBorrowed.target);
        Assert.Equal(70, (nint)openglBorrowed.context.data.wgl.device_context);
    }

    [Fact]
    public void RenderDescriptorsPreserveNativeDefaultsWhenExtentOmitted()
    {
        var metal = RenderStructs.ToNative(
            new MetalSurfaceDescriptor { Layer = new NativePointer(1) }
        );
        Assert.Equal(256u, metal.extent.width);
        Assert.Equal(256u, metal.extent.height);
        Assert.Equal(1, metal.extent.scale_factor);

        var vulkanBorrowed = RenderStructs.ToNative(
            new VulkanBorrowedTextureDescriptor
            {
                Image = new NativePointer(2),
                ImageView = new NativePointer(3),
            }
        );
        Assert.Equal(256u, vulkanBorrowed.extent.width);
        Assert.Equal(256u, vulkanBorrowed.extent.height);
        Assert.Equal(1, vulkanBorrowed.extent.scale_factor);
        Assert.Equal(5u, vulkanBorrowed.final_layout);

        var openglOwned = RenderStructs.ToNative(new OpenGLOwnedTextureDescriptor());
        var openglOwnedDefault = NativeMethods.mln_opengl_owned_texture_descriptor_default();
        Assert.Equal(256u, openglOwned.extent.width);
        Assert.Equal(256u, openglOwned.extent.height);
        Assert.Equal(1, openglOwned.extent.scale_factor);
        Assert.Equal(openglOwnedDefault.context.platform, openglOwned.context.platform);
    }

    [Fact]
    public void RenderExtentValidationRejectsInvalidScaleFactor()
    {
        var error = Assert.Throws<InvalidArgumentException>(() =>
            RenderStructs.ToNative(new RenderTargetExtent(1, 1, 0))
        );
        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);

        Assert.Throws<InvalidArgumentException>(() =>
            RenderStructs.ToNative(new RenderTargetExtent(1, 1, double.NaN))
        );
        Assert.Throws<InvalidArgumentException>(() =>
            RenderStructs.ToNative(new RenderTargetExtent(1, 1, double.PositiveInfinity))
        );
    }

    [Fact]
    public void TextureImageInfoCopiesNativeFields()
    {
        var info = RenderStructs.FromNative(
            new mln_texture_image_info
            {
                width = 1,
                height = 2,
                stride = 4,
                byte_length = 8,
            }
        );

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
        var metal = new MetalOwnedTextureFrame(
            metalScope,
            1,
            2,
            3,
            4,
            5,
            new NativePointer(6),
            new NativePointer(7),
            8
        );
        Assert.Equal(6, metal.Texture.Address);
        metalScope.Dispose();
        Assert.Throws<ObjectDisposedException>(() => metal.Texture);

        var vulkanScope = new FrameScope(nameof(VulkanOwnedTextureFrame));
        var vulkan = new VulkanOwnedTextureFrame(
            vulkanScope,
            1,
            2,
            3,
            4,
            5,
            new NativePointer(6),
            new NativePointer(7),
            new NativePointer(8),
            9,
            10
        );
        Assert.Equal(7, vulkan.ImageView.Address);
        vulkanScope.Dispose();
        Assert.Throws<ObjectDisposedException>(() => vulkan.ImageView);

        var openglScope = new FrameScope(nameof(OpenGLOwnedTextureFrame));
        var opengl = new OpenGLOwnedTextureFrame(
            openglScope,
            1,
            2,
            3,
            4,
            5,
            6,
            0x0de1,
            0x8058,
            0x1908,
            0x1401
        );
        Assert.Equal(6u, opengl.Texture);
        openglScope.Dispose();
        Assert.Throws<ObjectDisposedException>(() => opengl.Texture);
    }

    [Fact]
    public void OpenGLAttachMethodsReportUnsupportedWhenBackendUnavailable()
    {
        if ((Maplibre.SupportedRenderBackends() & RenderBackend.OpenGL) != 0)
        {
            Assert.Skip("OpenGL native build exercises positive attach paths");
        }

        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 64, Height = 64 });
        var context = new WglContextDescriptor
        {
            DeviceContext = new NativePointer(1),
            ShareContext = new NativePointer(1),
        };

        Assert.Throws<UnsupportedFeatureException>(() =>
            map.AttachOpenGLOwnedTexture(
                new OpenGLOwnedTextureDescriptor
                {
                    Extent = new RenderTargetExtent(32, 16, 1),
                    Context = context,
                }
            )
        );
        Assert.Throws<UnsupportedFeatureException>(() =>
            map.AttachOpenGLBorrowedTexture(
                new OpenGLBorrowedTextureDescriptor
                {
                    Extent = new RenderTargetExtent(32, 16, 1),
                    Context = context,
                    Texture = 1,
                    Target = 0x0de1,
                }
            )
        );
        Assert.Throws<UnsupportedFeatureException>(() =>
            map.AttachOpenGLSurface(
                new OpenGLSurfaceDescriptor
                {
                    Extent = new RenderTargetExtent(32, 16, 1),
                    Context = context,
                    Surface = new NativePointer(1),
                }
            )
        );
    }

    [Fact]
    public void FeatureStateSelectorMaterializesOptionalFields()
    {
        using var selector = NativeFeatureStateSelector.From(
            new FeatureStateSelector
            {
                SourceId = "source",
                SourceLayerId = "layer",
                FeatureId = "feature",
                StateKey = "hover",
            }
        );

        var value = selector.Value;
        Assert.Equal(
            (uint)(
                mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
                | mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
                | mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
            ),
            value.fields
        );
        Assert.Equal("source", RuntimeStructs.CopyUtf8(value.source_id.data, value.source_id.size));
        Assert.Equal(
            "layer",
            RuntimeStructs.CopyUtf8(value.source_layer_id.data, value.source_layer_id.size)
        );
        Assert.Equal(
            "feature",
            RuntimeStructs.CopyUtf8(value.feature_id.data, value.feature_id.size)
        );
        Assert.Equal("hover", RuntimeStructs.CopyUtf8(value.state_key.data, value.state_key.size));
    }
}
