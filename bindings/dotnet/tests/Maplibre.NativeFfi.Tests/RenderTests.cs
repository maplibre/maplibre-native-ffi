using Maplibre.NativeFfi.Internal.Pointer;
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
                fixture.Await(fixture.Session.Ready, "the session to be ready");
                fixture.RenderFrame();

                var image = fixture.Await(fixture.Session.ReadTextureAsync(), "the readback");

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
                fixture.Await(session.Ready, "the session to be ready");
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
                Assert.False(session.Ready.IsCompleted);
                fixture.Await(session.Ready, "the session to be ready");
                Assert.Equal(
                    RenderDriverKind.CallerGraphicsThread,
                    session.GetCapabilities().Driver
                );

                Assert.Equal(RenderResult.Rendered, fixture.RenderFrame().Disposition);

                var detach = session.DetachAsync();
                Assert.False(detach.IsCompleted);
                fixture.Await(detach, "the session to detach");
                Assert.Equal(RenderSessionState.Detached, session.GetSnapshot().State);
            }
        );
    }

    [Theory]
    [MemberData(nameof(Backends))]
    public void AScopeDisposesAnAttachedSessionSoItsMapReleasesBeforeTheGraphicsGo(
        TestGraphicsBackend backend
    )
    {
        RenderFixture.Run(
            backend,
            fixture =>
            {
                using (var session = fixture.Session)
                    fixture.Await(session.Ready, "the session to be ready");

                // A disposal reports no retirement, so the map's release waits for it.
                RenderFixture.Complete(fixture.Map.CloseAsync());
                fixture.Graphics.Dispose();
            }
        );
    }

    [Theory]
    [MemberData(nameof(Backends))]
    public void AnAsyncScopeAwaitsTheRetirementOfAnAttachedSession(TestGraphicsBackend backend)
    {
        RenderFixture.Run(
            backend,
            fixture =>
            {
                fixture.Await(fixture.Session.Ready, "the session to be ready");

                RenderFixture.Complete(DisposeInScope(fixture.Session));
                // The session has retired, so its graphics objects may go before its map does.
                fixture.Graphics.Dispose();
            }
        );

        static async Task DisposeInScope(RenderSessionHandle session)
        {
            await using (session) { }
        }
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
                fixture.Await(session.Ready, "the session to be ready");
                var rendered = fixture.RenderFrame();
                var frame =
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
                    Assert.Equal(Status.Busy, busy.Status);
                    var inUse = Assert.Throws<MaplibreException>(() => frame.Release(null));
                    Assert.Equal(Status.Busy, inUse.Status);
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
                Assert.Throws<MaplibreException>(() => escaped());
                Assert.False(frame.IsClosed);
                frame.Release(null);
                Assert.True(frame.IsClosed);
                Assert.Equal(
                    $"AcquiredFrameHandle(id=0x{frame.Id:x}, closed=true)",
                    frame.ToString()
                );
                // A discard after the release does nothing.
                frame.Discard();
                Assert.Throws<MaplibreException>(() => frame.GetResult());
            }
        );
    }

    [Fact]
    public void AFrameViewRejectsOtherThreadsAndExpiresWithItsLease()
    {
        var scope = new NativeViewScope();
        var frame = new VulkanTextureFrameView(
            new VulkanTextureFrame(
                1,
                2,
                3,
                4,
                5,
                6,
                7,
                8,
                NativePointer.FromBorrowedAddress(9),
                10,
                11
            ),
            scope
        );
        Assert.Equal(7ul, frame.Image);

        Exception? crossThread = null;
        var thread = new Thread(() => crossThread = Record.Exception(() => frame.Image));
        thread.Start();
        thread.Join();
        Assert.IsType<MaplibreException>(crossThread);

        scope.Expire();
        Assert.Throws<MaplibreException>(() => frame.ImageView);
    }
}
