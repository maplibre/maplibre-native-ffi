using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RenderTests
{
    public static TheoryData<TestGraphicsBackend> Backends => RenderFixture.Backends;

    [Theory]
    [MemberData(nameof(Backends))]
    public void AnOwnedTextureSessionRendersPixelsTheBindingReadsBack(TestGraphicsBackend backend)
    {
        RenderFixture.Run(
            backend,
            fixture =>
            {
                fixture.Await(fixture.Session.Completion, "the attachment");
                fixture.RenderFrame();

                var image = fixture.Await(
                    fixture.Session.TextureReadPremultipliedRgba8Async(TestWaits.Token),
                    "the readback"
                );

                Assert.Equal(RenderFixture.Width, image.Info.Width);
                Assert.Equal(RenderFixture.Height, image.Info.Height);
                var pixels = image.Data;
                for (var y = 0; y < image.Info.Height; y++)
                {
                    for (var x = 0; x < image.Info.Width; x++)
                    {
                        var offset = checked((int)(y * image.Info.Stride + x * 4));
                        Assert.Equal([255, 0, 0, 255], pixels[offset..(offset + 4)]);
                    }
                }
            }
        );
    }

    [Theory]
    [MemberData(nameof(Backends))]
    public void ADrainBeforeAnyDemandReturnsNull(TestGraphicsBackend backend)
    {
        RenderFixture.Run(
            backend,
            fixture =>
            {
                var session = fixture.Session;
                fixture.Await(session.Completion, "the attachment");
                // Native reports both calls as not ready, which reads as null.
                Assert.Null(session.DrainFrameResults());
                Assert.Null(session.AcquireFrame());
            }
        );
    }

    [Theory]
    [MemberData(nameof(Backends))]
    public void ACallerDrivenSessionIsServicedFromAManagedThread(TestGraphicsBackend backend)
    {
        RenderFixture.Run(
            backend,
            fixture =>
            {
                var session = fixture.Session;
                // Only the graphics thread services the session, and it has not yet.
                Assert.False(session.Completion.IsCompleted);
                fixture.Await(session.Completion, "the attachment");
                Assert.Equal(
                    RenderDriverKind.CallerGraphicsThread,
                    session.GetCapabilities().Driver
                );

                Assert.Equal(RenderResult.Rendered, fixture.RenderFrame().Disposition);

                var detach = session.DetachAsync(TestWaits.Token);
                Assert.False(detach.IsCompleted);
                fixture.Await(detach, "the session to detach");
                Assert.Equal(RenderSessionState.Detached, session.GetSnapshot().State);
            }
        );
    }

    [Theory]
    [MemberData(nameof(Backends))]
    public void ABorrowedFrameViewExpiresWithItsScope(TestGraphicsBackend backend)
    {
        RenderFixture.Run(
            backend,
            fixture =>
            {
                var session = fixture.Session;
                fixture.Await(session.Completion, "the attachment");
                var rendered = fixture.RenderFrame();
                using var frame =
                    session.AcquireFrame()
                    ?? throw new InvalidOperationException("No rendered frame is ready");
                Assert.Equal(rendered.FrameGeneration, frame.GetResult().FrameGeneration);

                Func<uint>? escaped = null;
                void Inside(Func<uint> width)
                {
                    Assert.Equal(RenderFixture.Width, width());
                    // The view borrows the frame, so neither the session nor the frame can end
                    // while it is in scope.
                    var busy = Assert.Throws<MaplibreException>(() => session.Abandon());
                    Assert.Equal(MaplibreStatus.Busy, busy.Status);
                    var inUse = Assert.Throws<InvalidStateException>(() =>
                        frame.Release(GpuSync.Default)
                    );
                    Assert.Equal("AcquiredFrameHandle is in use", inUse.Diagnostic);
                    Assert.Null(inUse.RawStatus);
                    escaped = width;
                }
                switch (backend)
                {
                    case TestGraphicsBackend.Metal:
                        frame.WithMetalTexture(view => Inside(() => view.Width));
                        break;
                    case TestGraphicsBackend.Vulkan:
                        frame.WithVulkanTexture(view => Inside(() => view.Width));
                        break;
                    default:
                        frame.WithOpenglTexture(view => Inside(() => view.Width));
                        break;
                }

                Assert.NotNull(escaped);
                Assert.Throws<InvalidOperationException>(() => escaped());
                frame.Release(GpuSync.Default);
                Assert.True(frame.IsClosed);
                Assert.Throws<InvalidStateException>(() => frame.GetResult());
            }
        );
    }

    [Fact]
    public void AFrameViewRejectsOtherThreadsAndExpiresWithItsLease()
    {
        var scope = new NativeViewScope();
        var frame = new VulkanOwnedTextureFrameView(
            new VulkanOwnedTextureFrame(
                1,
                2,
                3,
                4,
                5,
                6,
                7,
                NativePointer.FromBorrowedAddress(8),
                9,
                10
            ),
            scope
        );
        Assert.Equal(6ul, frame.Image);

        Exception? crossThread = null;
        var thread = new Thread(() => crossThread = Record.Exception(() => frame.Image));
        thread.Start();
        thread.Join();
        Assert.IsType<InvalidOperationException>(crossThread);

        scope.Expire();
        Assert.Throws<InvalidOperationException>(() => frame.ImageView);
    }

    [Fact]
    public unsafe void VulkanDescriptorsCarryEvery64BitHandleBit()
    {
        const ulong surface = 0xFEDC_BA98_7654_3211;
        const ulong image = 0x8000_0000_0000_0001;
        const ulong imageView = 0xFFFF_FFFF_FFFF_FFFE;
        var nativeSurface = GeneratedValues.NativeVulkanSurfaceDescriptor(
            new VulkanSurfaceDescriptor
            {
                Extent = new RenderTargetExtent(64, 32, 1),
                Surface = surface,
                Context = new VulkanContextDescriptor
                {
                    Instance = NativePointer.FromBorrowedAddress(
                        unchecked((nint)0xF000_0000_0000_0222)
                    ),
                    GraphicsQueueFamilyIndex = 7,
                },
            }
        );
        var nativeTexture = GeneratedValues.NativeVulkanBorrowedTextureDescriptor(
            new VulkanBorrowedTextureDescriptor
            {
                Extent = new RenderTargetExtent(64, 32, 1),
                PhysicalWidth = 64,
                PhysicalHeight = 32,
                Image = image,
                ImageView = imageView,
            }
        );

        Assert.Equal(surface, nativeSurface.surface);
        Assert.Equal(unchecked((nint)0xF000_0000_0000_0222), (nint)nativeSurface.context.instance);
        Assert.Equal(7u, nativeSurface.context.graphics_queue_family_index);
        Assert.Equal(image, nativeTexture.image);
        Assert.Equal(imageView, nativeTexture.image_view);
    }
}
