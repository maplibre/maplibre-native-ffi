using System.Diagnostics;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RenderSessionTests
{
    [Fact]
    public unsafe void WebGLDescriptorsRepresentExistingAndTransferredContexts()
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

    [BindingSpecTest("BND-161")]
    [Fact]
    public unsafe void VulkanDescriptorsCarryHandleBitsWithoutPointerConversion()
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

    [BindingSpecTest("BND-173")]
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

    [BindingSpecTest("BND-162", "BND-163", "BND-164", "BND-167", "BND-168", "BND-169", "BND-178")]
    [Fact]
    public async Task OwnedTextureSessionRendersAcquiresAndDetaches()
    {
        Assert.SkipUnless(
            OwnedTextureFixture.IsAvailable,
            "The loaded native library compiles no backend this suite can attach offscreen."
        );

        using var fixture = OwnedTextureFixture.Create();
        var extent = new RenderTargetExtent(32, 16, 1.0);
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions
            {
                Width = 32,
                Height = 16,
                ScaleFactor = 1.0,
            }
        );

        using var session = fixture.Attach(map, extent);
        Assert.Empty(session.DrainFrameResults());
        await session.Attachment.WaitAsync(
            TimeSpan.FromSeconds(30),
            TestContext.Current.CancellationToken
        );
        Assert.Equal(RenderSessionState.Attached, session.GetSnapshot().State);
        Assert.Equal(RenderDriverKind.CoreWorker, session.GetCapabilities().Driver);
        Assert.True(
            session.GetCapabilities().Flags.HasFlag(RenderSessionCapabilities.FrameAcquisition)
        );

        var second = Assert.Throws<InvalidStateException>(() => fixture.Attach(map, extent));
        Assert.Contains("render session", second.Message, StringComparison.OrdinalIgnoreCase);

        session.RequestFrame(new FrameDemand(FrameDemandFlags.None, 7, 0, 0));
        var result = Assert.Single(
            PollFor(() => session.DrainFrameResults(), results => results.Count > 0)
        );
        Assert.Equal(7ul, result.Token);
        Assert.Equal(RenderResult.Rendered, result.Disposition);

        Assert.True(session.TryAcquireFrame(out var frame));
        Assert.Equal(result.FrameGeneration, frame.Result.FrameGeneration);
        frame.Release(null);
        Assert.Throws<ObjectDisposedException>(() => frame.Result);

        var closeWhileAttached = Assert.Throws<InvalidStateException>(session.Close);
        Assert.NotEmpty(closeWhileAttached.Message);

        await session.DetachAsync(TestContext.Current.CancellationToken);
        Assert.Equal(RenderSessionState.Detached, session.GetSnapshot().State);
        session.Close();
        Assert.True(session.IsClosed);
    }

    [BindingSpecTest("BND-165", "BND-176", "BND-183")]
    [Fact]
    public async Task OwnedTextureSessionRejectsRetargetAndScaleFactorChange()
    {
        Assert.SkipUnless(
            OwnedTextureFixture.IsAvailable,
            "The loaded native library compiles no backend this suite can attach offscreen."
        );

        using var fixture = OwnedTextureFixture.Create();
        var extent = new RenderTargetExtent(32, 16, 1.0);
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions
            {
                Width = 32,
                Height = 16,
                ScaleFactor = 1.0,
            }
        );

        using var session = fixture.Attach(map, extent);
        await session.Attachment.WaitAsync(
            TimeSpan.FromSeconds(30),
            TestContext.Current.CancellationToken
        );

        await Assert.ThrowsAsync<UnsupportedFeatureException>(async () =>
            await fixture.SetTargetAsync(session, extent)
        );

        Assert.Throws<InvalidArgumentException>(() =>
        {
            _ = session.ResizeAsync(
                new RenderTargetExtent(64, 32, 2.0),
                TestContext.Current.CancellationToken
            );
        });

        Assert.Throws<InvalidArgumentException>(() =>
            session.RequestFrame(new FrameDemand((FrameDemandFlags)0x8000u, 0, 0, 0))
        );

        await session.ResizeAsync(
            new RenderTargetExtent(64, 32, 1.0),
            TestContext.Current.CancellationToken
        );
        var resized = session.GetSnapshot();
        Assert.Equal(64u, resized.Extent.Width);
        Assert.Equal(32u, resized.Extent.Height);

        await session.DetachAsync(TestContext.Current.CancellationToken);
        session.Close();
    }

    /// <summary>Polls a nonblocking read until it satisfies the predicate or the deadline passes.</summary>
    private static T PollFor<T>(Func<T> read, Func<T, bool> isSatisfied)
    {
        var deadline = Stopwatch.StartNew();
        while (deadline.Elapsed < TimeSpan.FromSeconds(30))
        {
            var value = read();
            if (isSatisfied(value))
            {
                return value;
            }
            Thread.Sleep(1);
        }

        throw new TimeoutException("The render session never produced the expected result.");
    }
}
