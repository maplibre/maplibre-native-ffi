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
        Assert.True(native.operation_source.IsNull);
        Assert.True(native.frame_source.IsNull);
        Assert.True(native.driver_work_source.IsNull);
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
    public void RawSurfaceAttachReturnsSessionAndOperation()
    {
        var method = typeof(NativeMethods).GetMethod(
            "mln_opengl_surface_attach_start",
            BindingFlags.Public | BindingFlags.Static
        );

        Assert.NotNull(method);
        var parameters = method!.GetParameters();
        Assert.Equal(5, parameters.Length);
        Assert.Equal(typeof(MlnRenderSession*), parameters[3].ParameterType);
        Assert.Equal(typeof(MlnOperation*), parameters[4].ParameterType);
    }

    [Fact]
    public void PublicSessionSurfaceIsOperationBackedAndHasExplicitDriverService()
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
        scope.EnsureActive();
        scope.Dispose();
        Assert.Throws<ObjectDisposedException>(scope.EnsureActive);
    }

    [Fact]
    public void PhaseThreeNativeStructsUseExpectedHandleSizedLayouts()
    {
        Assert.Equal(40, sizeof(mln_frame_demand));
        Assert.Equal(48, sizeof(mln_render_frame_result));
        Assert.Equal(24, sizeof(mln_gpu_sync));
        Assert.Equal(8, Marshal.SizeOf<MlnAcquiredFrame>());
        Assert.Equal(8, Marshal.SizeOf<MlnRenderFrameBatch>());
    }
}
