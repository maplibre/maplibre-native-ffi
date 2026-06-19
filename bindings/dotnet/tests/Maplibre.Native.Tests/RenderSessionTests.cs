using System.Runtime.InteropServices;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Map;
using Maplibre.Native.Query;
using Maplibre.Native.Render;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class RenderSessionTests
{
    [BindingSpecTest("BND-161")]
    [Fact]
    public void SurfaceDescriptorsMaterializeOpaquePointersAndExtent()
    {
        var metal = RenderStructs.ToNative(
            new MetalSurfaceDescriptor
            {
                Extent = new RenderTargetExtent(320, 240, 2),
                Layer = NativePointer.FromBorrowedAddress(123),
                Context = new MetalContextDescriptor
                {
                    Device = NativePointer.FromBorrowedAddress(456),
                },
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
                Surface = NativePointer.FromBorrowedAddress(111),
                Context = new VulkanContextDescriptor
                {
                    Instance = NativePointer.FromBorrowedAddress(222),
                    PhysicalDevice = NativePointer.FromBorrowedAddress(333),
                    Device = NativePointer.FromBorrowedAddress(444),
                    Queue = NativePointer.FromBorrowedAddress(555),
                    GraphicsQueueFamilyIndex = 7,
                    GetInstanceProcAddr = NativePointer.FromBorrowedAddress(666),
                    GetDeviceProcAddr = NativePointer.FromBorrowedAddress(777),
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
                Surface = NativePointer.FromBorrowedAddress(888),
                Context = new WglContextDescriptor
                {
                    DeviceContext = NativePointer.FromBorrowedAddress(999),
                    ShareContext = NativePointer.FromBorrowedAddress(1000),
                    GetProcAddress = NativePointer.FromBorrowedAddress(1001),
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

    [BindingSpecTest("BND-161")]
    [Fact]
    public void TextureDescriptorsMaterializeOpaquePointersAndExtent()
    {
        var metalOwned = RenderStructs.ToNative(
            new MetalOwnedTextureDescriptor
            {
                Extent = new RenderTargetExtent(128, 64, 2),
                Context = new MetalContextDescriptor
                {
                    Device = NativePointer.FromBorrowedAddress(10),
                },
            }
        );
        Assert.Equal(128u, metalOwned.extent.width);
        Assert.Equal(64u, metalOwned.extent.height);
        Assert.Equal(10, (nint)metalOwned.context.device);

        var metalBorrowed = RenderStructs.ToNative(
            new MetalBorrowedTextureDescriptor
            {
                Extent = new RenderTargetExtent(128, 64, 2),
                Texture = NativePointer.FromBorrowedAddress(20),
            }
        );
        Assert.Equal(20, (nint)metalBorrowed.texture);

        var vulkanOwned = RenderStructs.ToNative(
            new VulkanOwnedTextureDescriptor
            {
                Extent = new RenderTargetExtent(256, 128, 1),
                Context = new VulkanContextDescriptor
                {
                    Device = NativePointer.FromBorrowedAddress(30),
                    GetInstanceProcAddr = NativePointer.FromBorrowedAddress(31),
                    GetDeviceProcAddr = NativePointer.FromBorrowedAddress(32),
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
                Image = NativePointer.FromBorrowedAddress(40),
                ImageView = NativePointer.FromBorrowedAddress(45),
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
                    Display = NativePointer.FromBorrowedAddress(60),
                    Config = NativePointer.FromBorrowedAddress(61),
                    ShareContext = NativePointer.FromBorrowedAddress(62),
                    GetProcAddress = NativePointer.FromBorrowedAddress(63),
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
                    DeviceContext = NativePointer.FromBorrowedAddress(70),
                    ShareContext = NativePointer.FromBorrowedAddress(71),
                },
                Texture = 72,
                Target = 0x0de1,
            }
        );
        Assert.Equal(72u, openglBorrowed.texture);
        Assert.Equal(0x0de1u, openglBorrowed.target);
        Assert.Equal(70, (nint)openglBorrowed.context.data.wgl.device_context);
    }

    [BindingSpecTest("BND-060")]
    [Fact]
    public void RenderDescriptorsPreserveNativeDefaultsWhenExtentOmitted()
    {
        var metal = RenderStructs.ToNative(
            new MetalSurfaceDescriptor { Layer = NativePointer.FromBorrowedAddress(1) }
        );
        Assert.Equal(256u, metal.extent.width);
        Assert.Equal(256u, metal.extent.height);
        Assert.Equal(1, metal.extent.scale_factor);

        var vulkanBorrowed = RenderStructs.ToNative(
            new VulkanBorrowedTextureDescriptor
            {
                Image = NativePointer.FromBorrowedAddress(2),
                ImageView = NativePointer.FromBorrowedAddress(3),
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

    [BindingSpecTest("BND-025")]
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

    [BindingSpecTest("BND-166", "BND-167")]
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

    [BindingSpecTest("BND-023")]
    [Fact]
    public void NativeBufferRejectsUseAfterDispose()
    {
        using var buffer = new NativeBuffer(4);
        Assert.Equal(4, buffer.Span.Length);

        buffer.Dispose();

        Assert.Throws<ObjectDisposedException>(() =>
        {
            var span = buffer.Span;
            _ = span.Length;
        });
    }

    [BindingSpecTest("BND-168", "BND-173")]
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
            NativePointer.FromBorrowedAddress(6),
            NativePointer.FromBorrowedAddress(7),
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
            NativePointer.FromBorrowedAddress(6),
            NativePointer.FromBorrowedAddress(7),
            NativePointer.FromBorrowedAddress(8),
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

    [BindingSpecTest("BND-169")]
    [Fact]
    public void FailedTextureFrameReleaseLeavesFrameLiveForRetry()
    {
        var pointer = (mln_metal_owned_texture_frame*)
            NativeMemory.AllocZeroed((nuint)sizeof(mln_metal_owned_texture_frame));
        pointer->size = (uint)sizeof(mln_metal_owned_texture_frame);
        var session = RenderSessionHandle.CreateForTest((mln_render_session*)1234);
        var scope = new FrameScope(nameof(MetalOwnedTextureFrameHandle));
        var releaseCalls = 0;
        var state = new TextureFrameState<mln_metal_owned_texture_frame>(
            session,
            pointer,
            scope,
            (_, _) =>
                releaseCalls++ == 0
                    ? mln_status.MLN_STATUS_INVALID_STATE
                    : mln_status.MLN_STATUS_OK,
            nameof(MetalOwnedTextureFrameHandle)
        );

        try
        {
            var error = Assert.Throws<InvalidStateException>(state.Close);

            Assert.Equal(MaplibreStatus.InvalidState, error.Status);
            Assert.False(state.IsClosed);
            scope.EnsureActive();
            Assert.Equal(1, releaseCalls);

            state.Close();

            Assert.True(state.IsClosed);
            Assert.Throws<ObjectDisposedException>(scope.EnsureActive);
            Assert.Equal(2, releaseCalls);
            session.Close();
        }
        finally
        {
            if (!state.IsClosed)
            {
                scope.Dispose();
                NativeMemory.Free(pointer);
            }
        }
    }

    [BindingSpecTest("BND-170")]
    [Fact]
    public void SessionOperationsForbiddenDuringActiveTextureFrameFailBeforeNativeCall()
    {
        var reports = new List<NativeLeakReport>();
        using var capture = NativeLeakReporter.CaptureForTest(reports.Add);
        var resizeCalls = 0;
        var renderUpdateCalls = 0;
        var textureReadCalls = 0;
        var destroyCalls = 0;
        var metalAcquireCalls = 0;
        var vulkanAcquireCalls = 0;
        var openGLAcquireCalls = 0;
        using var sessionMethods = RenderSessionHandle.UseSessionMethodsForTest(
            (_, _, _, _) =>
            {
                resizeCalls++;
                return mln_status.MLN_STATUS_OK;
            },
            _ =>
            {
                renderUpdateCalls++;
                return mln_status.MLN_STATUS_OK;
            },
            (_, _, _, _) =>
            {
                textureReadCalls++;
                return mln_status.MLN_STATUS_OK;
            },
            _ =>
            {
                destroyCalls++;
                return mln_status.MLN_STATUS_OK;
            }
        );
        using var metalMethods = RenderSessionHandle.UseMetalFrameMethodsForTest(
            (_, _) =>
            {
                metalAcquireCalls++;
                return mln_status.MLN_STATUS_OK;
            },
            (_, _) => mln_status.MLN_STATUS_OK,
            (_, _) => throw new InvalidOperationException("unexpected frame read")
        );
        using var textureFrameMethods = RenderSessionHandle.UseTextureFrameAcquireMethodsForTest(
            (_, _) =>
            {
                vulkanAcquireCalls++;
                return mln_status.MLN_STATUS_OK;
            },
            (_, _) =>
            {
                openGLAcquireCalls++;
                return mln_status.MLN_STATUS_OK;
            }
        );
        var session = RenderSessionHandle.CreateForTest((mln_render_session*)1234);
        var pointer = (mln_metal_owned_texture_frame*)
            NativeMemory.AllocZeroed((nuint)sizeof(mln_metal_owned_texture_frame));
        pointer->size = (uint)sizeof(mln_metal_owned_texture_frame);
        var scope = new FrameScope(nameof(MetalOwnedTextureFrameHandle));
        var state = new TextureFrameState<mln_metal_owned_texture_frame>(
            session,
            pointer,
            scope,
            (_, _) => mln_status.MLN_STATUS_OK,
            nameof(MetalOwnedTextureFrameHandle)
        );

        try
        {
            AssertActiveFrameError(() => session.Resize(64, 64, 1));
            AssertActiveFrameError(session.RenderUpdate);
            AssertActiveFrameError(session.Detach);
            AssertActiveFrameError(() => _ = session.TextureImageInfo());
            using var buffer = new NativeBuffer(4);
            AssertActiveFrameError(() => session.ReadPremultipliedRgba8(buffer));
            AssertActiveFrameError(() => _ = session.AcquireMetalOwnedTextureFrame());
            AssertActiveFrameError(() => _ = session.AcquireVulkanOwnedTextureFrame());
            AssertActiveFrameError(() => _ = session.AcquireOpenGLOwnedTextureFrame());
            AssertActiveFrameError(session.Close);
            session.Dispose();

            Assert.Equal(0, resizeCalls);
            Assert.Equal(0, renderUpdateCalls);
            Assert.Equal(0, textureReadCalls);
            Assert.Equal(0, destroyCalls);
            Assert.Equal(0, metalAcquireCalls);
            Assert.Equal(0, vulkanAcquireCalls);
            Assert.Equal(0, openGLAcquireCalls);
            var report = Assert.Single(reports);
            Assert.Equal(NativeLeakReportKind.DisposeFailed, report.Kind);
            Assert.Equal(nameof(RenderSessionHandle), report.TypeName);
            Assert.Equal((nint)1234, report.Address);
            Assert.Null(report.Status);
            Assert.Contains("texture frame is active", report.Message, StringComparison.Ordinal);
        }
        finally
        {
            if (!state.IsClosed)
            {
                state.Close();
            }
            session.Close();
        }

        static void AssertActiveFrameError(Action action)
        {
            var error = Assert.Throws<InvalidStateException>(action);

            Assert.Equal(MaplibreStatus.InvalidState, error.Status);
            Assert.Null(error.RawStatus);
            Assert.Contains("texture frame is active", error.Diagnostic, StringComparison.Ordinal);
        }
    }

    [BindingSpecTest("BND-162", "BND-171")]
    [Fact]
    public void OpenGLAttachFamiliesReturnPublicSessionHandlesAndKeepBorrowedHandlesOwnedByCaller()
    {
        // Support invariant for BND-162 and BND-171: deterministic attach hooks
        // verify .NET routes each public OpenGL attach path to the matching native family.
        var attachCalls = new List<string>();
        var destroyed = new List<nint>();
        using var methods = RenderSessionHandle.UseOpenGLAttachMethodsForTest(
            (_, _, outSession) =>
            {
                attachCalls.Add("surface");
                *outSession = (mln_render_session*)101;
                return mln_status.MLN_STATUS_OK;
            },
            (_, _, outSession) =>
            {
                attachCalls.Add("owned");
                *outSession = (mln_render_session*)102;
                return mln_status.MLN_STATUS_OK;
            },
            (_, _, outSession) =>
            {
                attachCalls.Add("borrowed");
                *outSession = (mln_render_session*)103;
                return mln_status.MLN_STATUS_OK;
            }
        );
        using var sessionMethods = RenderSessionHandle.UseSessionMethodsForTest(
            (_, _, _, _) => mln_status.MLN_STATUS_OK,
            _ => mln_status.MLN_STATUS_OK,
            (_, _, _, _) => mln_status.MLN_STATUS_OK,
            session =>
            {
                destroyed.Add((nint)session);
                return mln_status.MLN_STATUS_OK;
            }
        );
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 64, Height = 64 });
        var context = new WglContextDescriptor
        {
            DeviceContext = NativePointer.FromBorrowedAddress(1),
            ShareContext = NativePointer.FromBorrowedAddress(2),
        };
        var borrowed = new OpenGLBorrowedTextureDescriptor
        {
            Extent = new RenderTargetExtent(32, 16, 1),
            Context = context,
            Texture = 77,
            Target = 0x0de1,
        };

        using var surface = RenderSessionHandle.AttachOpenGLSurface(
            map,
            new OpenGLSurfaceDescriptor
            {
                Extent = new RenderTargetExtent(32, 16, 1),
                Context = context,
                Surface = NativePointer.FromBorrowedAddress(3),
            }
        );
        using var owned = RenderSessionHandle.AttachOpenGLOwnedTexture(
            map,
            new OpenGLOwnedTextureDescriptor
            {
                Extent = new RenderTargetExtent(32, 16, 1),
                Context = context,
            }
        );
        using var callerOwned = RenderSessionHandle.AttachOpenGLBorrowedTexture(map, borrowed);

        Assert.False(surface.IsClosed);
        Assert.False(owned.IsClosed);
        Assert.False(callerOwned.IsClosed);
        Assert.Equal(["surface", "owned", "borrowed"], attachCalls);

        surface.Close();
        owned.Close();
        callerOwned.Close();

        Assert.Equal([(nint)101, (nint)102, (nint)103], destroyed);
        Assert.Equal(77u, borrowed.Texture);
        Assert.Equal(0x0de1u, borrowed.Target);
        var borrowedContext = Assert.IsType<WglContextDescriptor>(borrowed.Context);
        Assert.Equal(1, borrowedContext.DeviceContext.Address);
        Assert.Equal(2, borrowedContext.ShareContext.Address);
    }

    [BindingSpecTest("BND-163")]
    [Fact]
    public void SecondRenderSessionAttachInvalidStateLeavesFirstSessionOpen()
    {
        // Support invariant for BND-163: the invalid-state branch is binding-visible
        // but depends on native render-session state that is not deterministic here.
        var attachCalls = 0;
        using var methods = RenderSessionHandle.UseOpenGLAttachMethodsForTest(
            (_, _, outSession) =>
            {
                attachCalls++;
                if (attachCalls == 1)
                {
                    *outSession = (mln_render_session*)101;
                    return mln_status.MLN_STATUS_OK;
                }

                *outSession = null;
                return mln_status.MLN_STATUS_INVALID_STATE;
            },
            (_, _, _) => mln_status.MLN_STATUS_UNSUPPORTED,
            (_, _, _) => mln_status.MLN_STATUS_UNSUPPORTED
        );
        using var sessionMethods = RenderSessionHandle.UseSessionMethodsForTest(
            (_, _, _, _) => mln_status.MLN_STATUS_OK,
            _ => mln_status.MLN_STATUS_OK,
            (_, _, _, _) => mln_status.MLN_STATUS_OK,
            _ => mln_status.MLN_STATUS_OK
        );
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 64, Height = 64 });
        var descriptor = new OpenGLSurfaceDescriptor
        {
            Extent = new RenderTargetExtent(32, 16, 1),
            Context = new WglContextDescriptor
            {
                DeviceContext = NativePointer.FromBorrowedAddress(1),
                ShareContext = NativePointer.FromBorrowedAddress(2),
            },
            Surface = NativePointer.FromBorrowedAddress(3),
        };

        using var first = RenderSessionHandle.AttachOpenGLSurface(map, descriptor);
        var error = Assert.Throws<InvalidStateException>(() =>
            RenderSessionHandle.AttachOpenGLSurface(map, descriptor)
        );

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.False(first.IsClosed);
        Assert.Equal(2, attachCalls);
    }

    [BindingSpecTest("BND-164", "BND-165")]
    [Fact]
    public void RenderUpdateInvalidStateDoesNotCloseSessionAndResizePassesExtent()
    {
        // Support invariant for BND-164 and BND-165: resize/update assertions target
        // public wrapper behavior while native invalid-state timing is deterministic.
        uint resizedWidth = 0;
        uint resizedHeight = 0;
        double resizedScale = 0;
        using var methods = RenderSessionHandle.UseSessionMethodsForTest(
            (_, width, height, scale) =>
            {
                resizedWidth = width;
                resizedHeight = height;
                resizedScale = scale;
                return mln_status.MLN_STATUS_OK;
            },
            _ => mln_status.MLN_STATUS_INVALID_STATE,
            (_, _, _, _) => mln_status.MLN_STATUS_OK,
            _ => mln_status.MLN_STATUS_OK
        );
        var session = RenderSessionHandle.CreateForTest((mln_render_session*)1234);

        session.Resize(320, 240, 2);
        var error = Assert.Throws<InvalidStateException>(session.RenderUpdate);

        Assert.Equal(320u, resizedWidth);
        Assert.Equal(240u, resizedHeight);
        Assert.Equal(2, resizedScale);
        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.False(session.IsClosed);
        session.Close();
    }

    [BindingSpecTest("BND-172")]
    [Fact]
    public void MetalFrameConstructionFailureAfterNativeAcquireReleasesFrame()
    {
        // Support invariant for BND-172: this covers binding cleanup after native
        // frame acquisition when copying into the public handle fails.
        var acquireCalls = 0;
        var releaseCalls = 0;
        using var methods = RenderSessionHandle.UseMetalFrameMethodsForTest(
            (_, frame) =>
            {
                acquireCalls++;
                frame->generation = 1;
                return mln_status.MLN_STATUS_OK;
            },
            (_, _) =>
            {
                releaseCalls++;
                return mln_status.MLN_STATUS_OK;
            },
            (_, _) => throw new InvalidOperationException("copy failed")
        );
        var session = RenderSessionHandle.CreateForTest((mln_render_session*)1234);

        var error = Assert.Throws<InvalidOperationException>(() =>
            session.AcquireMetalOwnedTextureFrame()
        );

        Assert.Equal("copy failed", error.Message);
        Assert.Equal(1, acquireCalls);
        Assert.Equal(1, releaseCalls);
        Assert.False(session.IsClosed);
        session.Close();
    }

    [BindingSpecTest("BND-160")]
    [Fact]
    public void OpenGLAttachMethodsReportUnsupportedWhenBackendUnavailable()
    {
        if ((Maplibre.SupportedRenderBackends() & RenderBackend.OpenGL) != 0)
        {
            Assert.Skip("OpenGL native build exercises positive attach paths");
        }

        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 64, Height = 64 });
        var context = new WglContextDescriptor
        {
            DeviceContext = NativePointer.FromBorrowedAddress(1),
            ShareContext = NativePointer.FromBorrowedAddress(1),
        };

        Assert.Throws<UnsupportedFeatureException>(() =>
            RenderSessionHandle.AttachOpenGLOwnedTexture(
                map,
                new OpenGLOwnedTextureDescriptor
                {
                    Extent = new RenderTargetExtent(32, 16, 1),
                    Context = context,
                }
            )
        );
        Assert.Throws<UnsupportedFeatureException>(() =>
            RenderSessionHandle.AttachOpenGLBorrowedTexture(
                map,
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
            RenderSessionHandle.AttachOpenGLSurface(
                map,
                new OpenGLSurfaceDescriptor
                {
                    Extent = new RenderTargetExtent(32, 16, 1),
                    Context = context,
                    Surface = NativePointer.FromBorrowedAddress(1),
                }
            )
        );
    }

    [BindingSpecTest("BND-061")]
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
