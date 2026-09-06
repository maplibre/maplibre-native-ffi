using System.Reflection;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Render;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class RenderSessionTests
{
    [Fact]
    public void PhaseThreeEnumsPreserveFrozenValues()
    {
        Assert.Equal(5u, (uint)RenderResult.DeadlineMissed);
        Assert.Equal(2u, (uint)RenderDriverKind.CallerGraphicsThread);
        Assert.Equal(5u, (uint)RenderSessionState.TargetLost);
        Assert.Equal(1u << 2, (uint)RenderSessionCapabilities.ConsumerSync);
        Assert.Equal(1u, (uint)WebGLContextKind.TransferredCanvas);
        Assert.Equal(-9, (int)global::Maplibre.NativeFfi.Error.MaplibreStatus.NotReady);
    }

    [Fact]
    public void AttachOptionsUseNativeDefaultsAndLeaveSourcesInherited()
    {
        var options = new RenderSessionAttachOptions
        {
            Driver = RenderDriverKind.CallerGraphicsThread,
            RequestedTextureRingDepth = 3,
        };
        var native = new mln_render_session_attach_options
        {
            size = (uint)sizeof(mln_render_session_attach_options),
            driver = (uint)options.Driver,
            requested_texture_ring_depth = options.RequestedTextureRingDepth,
        };

        Assert.Equal((uint)RenderDriverKind.CallerGraphicsThread, native.driver);
        Assert.Equal(3u, native.requested_texture_ring_depth);
    }

    [Fact]
    public void WebGLDescriptorsRepresentExistingAndTransferredContexts()
    {
        var existing = RenderStructs.ToNative(
            new WebGLContextDescriptor
            {
                Kind = WebGLContextKind.Existing,
                Context = 27,
                Ownership = OpenGLContextOwnership.Shared,
            }
        );
        var transferred = RenderStructs.ToNative(
            new WebGLContextDescriptor
            {
                Kind = WebGLContextKind.TransferredCanvas,
                CanvasSelector = "#map",
                Ownership = OpenGLContextOwnership.Dedicated,
            }
        );

        Assert.Equal(
            mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_WEBGL,
            existing.platform
        );
        Assert.Equal(27, existing.data.webgl.context);
        Assert.Equal((uint)WebGLContextKind.TransferredCanvas, transferred.data.webgl.kind);
    }

    [Fact]
    public void VulkanDescriptorsCarryHandleBitsWithoutPointerConversion()
    {
        var surface = RenderStructs.ToNative(
            new VulkanSurfaceDescriptor
            {
                Extent = new RenderTargetExtent(640, 480, 1),
                Surface = new VulkanHandle(111),
                Context = new VulkanContextDescriptor
                {
                    Instance = NativePointer.FromBorrowedAddress(222),
                    PhysicalDevice = NativePointer.FromBorrowedAddress(333),
                    Device = NativePointer.FromBorrowedAddress(444),
                    Queue = NativePointer.FromBorrowedAddress(555),
                    GraphicsQueueFamilyIndex = 7,
                },
            }
        );

        Assert.Equal(111ul, surface.surface);
        Assert.Equal(222, (nint)surface.context.instance);
        Assert.Equal(7u, surface.context.graphics_queue_family_index);

        var borrowed = RenderStructs.ToNative(
            new VulkanBorrowedTextureDescriptor
            {
                Extent = new RenderTargetExtent(256, 128, 1),
                PhysicalWidth = 65,
                PhysicalHeight = 33,
                Image = new VulkanHandle(40),
                ImageView = new VulkanHandle(45),
                Format = 50,
                InitialLayout = 55,
                FinalLayout = 60,
            }
        );

        Assert.Equal(40ul, borrowed.image);
        Assert.Equal(45ul, borrowed.image_view);
        Assert.Equal(50u, borrowed.format);
        Assert.Equal(55u, borrowed.initial_layout);
        Assert.Equal(60u, borrowed.final_layout);
    }

    [Fact]
    public void RawSurfaceAttachReturnsSessionAndAcceptsCompletion()
    {
        var method = typeof(NativeMethods).GetMethod(
            "mln_opengl_surface_attach",
            BindingFlags.Public | BindingFlags.Static
        );

        Assert.NotNull(method);
        var parameters = method!.GetParameters();
        Assert.Equal(5, parameters.Length);
        Assert.Equal(typeof(MlnRenderSession*), parameters[3].ParameterType);
        Assert.Equal(typeof(mln_completion*), parameters[4].ParameterType);
    }

    [Fact]
    public void PublicSessionSurfaceUsesTasksAndExplicitDriverService()
    {
        var type = typeof(RenderSessionHandle);

        Assert.Equal(
            typeof(Task),
            type.GetProperty(nameof(RenderSessionHandle.Attachment))!.PropertyType
        );
        Assert.Equal(
            typeof(Task),
            type.GetMethod(nameof(RenderSessionHandle.DetachAsync))!.ReturnType
        );
        Assert.Equal(
            typeof(Task),
            type.GetMethod(nameof(RenderSessionHandle.BarrierAsync))!.ReturnType
        );
        Assert.Equal(
            typeof(int),
            type.GetMethod(nameof(RenderSessionHandle.ServiceDriverWork))!.ReturnType
        );
        Assert.Equal(
            typeof(RenderFrameBatch),
            type.GetMethod(nameof(RenderSessionHandle.DrainFrameResults))!.ReturnType
        );
        Assert.Null(type.GetMethod("RenderUpdate"));
        Assert.Null(type.GetMethod("Resize"));
        Assert.Null(type.GetMethod("ReadPremultipliedRgba8"));
    }

    [Fact]
    public void AcquiredFrameAccessIsScopedToItsLease()
    {
        var scope = new FrameScope();
        var frame = new VulkanOwnedTextureFrame(
            scope,
            1,
            2,
            3,
            4,
            5,
            new VulkanHandle(6),
            new VulkanHandle(7),
            NativePointer.FromBorrowedAddress(8),
            9,
            10
        );

        scope.EnsureActive();
        Assert.Equal(6ul, frame.Image.Bits);
        Assert.Equal(7ul, frame.ImageView.Bits);

        scope.Dispose();
        Assert.Throws<ObjectDisposedException>(scope.EnsureActive);
        Assert.Throws<ObjectDisposedException>(() => frame.ImageView);
    }

    [Fact]
    public void PhaseThreeNativeStructsUseExpectedHandleSizedLayouts()
    {
        Assert.Equal(32, sizeof(mln_frame_demand));
        Assert.Equal(48, sizeof(mln_render_frame_result));
        Assert.Equal(24, sizeof(mln_gpu_sync));
        Assert.Equal(8, Marshal.SizeOf<MlnAcquiredFrame>());
        Assert.Equal(8, Marshal.SizeOf<MlnRenderFrameBatch>());
    }
}
