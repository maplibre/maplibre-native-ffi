using System.Diagnostics;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
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
        using var scope = new NativeCallScope();
        var existing = GeneratedValues.NativeOpenglContextDescriptor(
            new OpenglContextDescriptor(
                OpenglContextOwnership.Shared,
                new OpenglContextDescriptor.DataValue.Webgl(
                    new WebglContextDescriptor
                    {
                        Kind = WebglContextKind.Existing,
                        Context = 27,
                        CanvasSelector = "",
                    }
                )
            ),
            scope
        );
        var transferred = GeneratedValues.NativeOpenglContextDescriptor(
            new OpenglContextDescriptor(
                OpenglContextOwnership.Dedicated,
                new OpenglContextDescriptor.DataValue.Webgl(
                    new WebglContextDescriptor
                    {
                        Kind = WebglContextKind.TransferredCanvas,
                        CanvasSelector = "#map",
                    }
                )
            ),
            scope
        );

        Assert.Equal(
            mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_WEBGL,
            existing.platform
        );
        Assert.Equal(27, existing.data.webgl.context);
        Assert.Equal((uint)WebglContextKind.TransferredCanvas, transferred.data.webgl.kind);
    }

    [Fact]
    public unsafe void VulkanDescriptorsCarryHandleBitsWithoutPointerConversion()
    {
        var surface = GeneratedValues.NativeVulkanSurfaceDescriptor(
            new VulkanSurfaceDescriptor
            {
                Extent = new RenderTargetExtent(640, 480, 1),
                Surface = 111,
                Context = new VulkanContextDescriptor
                {
                    Instance = NativePointer.FromBorrowedAddress(222),
                    PhysicalDevice = NativePointer.FromBorrowedAddress(333),
                    Device = NativePointer.FromBorrowedAddress(444),
                    GraphicsQueue = NativePointer.FromBorrowedAddress(555),
                    GraphicsQueueFamilyIndex = 7,
                },
            }
        );

        Assert.Equal(111ul, surface.surface);
        Assert.Equal(222, (nint)surface.context.instance);
        Assert.Equal(7u, surface.context.graphics_queue_family_index);

        var borrowed = GeneratedValues.NativeVulkanBorrowedTextureDescriptor(
            new VulkanBorrowedTextureDescriptor
            {
                Extent = new RenderTargetExtent(256, 128, 1),
                PhysicalWidth = 65,
                PhysicalHeight = 33,
                Image = 40,
                ImageView = 45,
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
    public void AcquiredFrameAccessIsScopedToItsLease()
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
        Assert.Equal(7ul, frame.ImageView);
        Exception? crossThread = null;
        var thread = new Thread(() => crossThread = Record.Exception(() => frame.Image));
        thread.Start();
        thread.Join();
        Assert.IsType<InvalidOperationException>(crossThread);
        scope.Expire();
        Assert.Throws<InvalidOperationException>(() => frame.ImageView);
    }

    [Fact]
    public async Task OwnedTextureSessionRendersReadsBackAndDetaches()
    {
        using var fixture = OwnedTextureFixture.Create();
        var extent = new RenderTargetExtent(32, 16, 1.0);
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with
                {
                    Width = 32,
                    Height = 16,
                    ScaleFactor = 1.0,
                },
            }
        );

        await map.SetStyleJsonAsync(
            """
            {"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#ff0000"}}]}
            """u8.ToArray(),
            TestContext.Current.CancellationToken
        );

        using var session = fixture.Attach(map, extent);
        await WithDetachAsync(
            session,
            async () =>
            {
                Assert.Empty(session.DrainFrameCopies());
                await session.Completion.WaitAsync(
                    TimeSpan.FromSeconds(30),
                    TestContext.Current.CancellationToken
                );
                Assert.Equal(RenderSessionState.Attached, session.GetSnapshot().State);
                var capabilities = session.GetCapabilities();
                Assert.Equal(RenderDriverKind.CoreWorker, capabilities.Driver);
                Assert.Equal(
                    fixture.SupportsFrameAcquisition,
                    capabilities.Flags.HasFlag(RenderSessionCapabilityFlag.FrameAcquisition)
                );
                Assert.True(capabilities.Flags.HasFlag(RenderSessionCapabilityFlag.Readback));

                var second = Assert.Throws<InvalidStateException>(() =>
                    fixture.Attach(map, extent)
                );
                Assert.Contains(
                    "render session",
                    second.Message,
                    StringComparison.OrdinalIgnoreCase
                );

                session.RequestFrame(new FrameDemand((FrameDemandFlag)0, 7, 0, 0));
                var result = Assert.Single(
                    PollFor(() => session.DrainFrameCopies(), results => results.Count > 0)
                );
                Assert.Equal(7ul, result.Token);
                Assert.Equal(RenderResult.Rendered, result.Disposition);

                var image = await session.TextureReadPremultipliedRgba8Async(
                    TestContext.Current.CancellationToken
                );
                Assert.Equal(32u, image.Info.Width);
                Assert.Equal(16u, image.Info.Height);
                var pixels = image.Data;
                Assert.Equal((ulong)image.Data.Length, (ulong)pixels.Length);
                Assert.True(image.Info.Stride >= image.Info.Width * 4);
                for (var y = 0; y < image.Info.Height; y++)
                {
                    for (var x = 0; x < image.Info.Width; x++)
                    {
                        var offset = checked((int)(y * image.Info.Stride + x * 4));
                        Assert.Equal(new byte[] { 255, 0, 0, 255 }, pixels[offset..(offset + 4)]);
                    }
                }

                if (fixture.SupportsFrameAcquisition)
                {
                    using var frame = session.AcquireFrame();
                    try
                    {
                        Assert.Equal(result.FrameGeneration, frame.GetResult().FrameGeneration);
                        GpuSyncView? escaped = null;
                        frame.WithProducerSync(sync =>
                        {
                            escaped = sync;
                            _ = sync.Kind;
                            var busy = Assert.Throws<MaplibreException>(() => session.Abandon());
                            Assert.Equal(MaplibreStatus.Busy, busy.Status);
                            Assert.Throws<InvalidStateException>(() =>
                                frame.Release(GpuSync.Default)
                            );
                        });
                        Assert.Throws<InvalidOperationException>(() => escaped!.Kind);
                        Assert.Throws<ApplicationException>(() =>
                            frame.WithProducerSync(_ =>
                                throw new ApplicationException("host failure")
                            )
                        );
                        frame.WithProducerSync(sync => _ = sync.Kind);
                    }
                    finally
                    {
                        frame.Release(GpuSync.Default);
                    }
                    Assert.Throws<InvalidStateException>(() => frame.GetResult());
                }

                var closeWhileAttached = Assert.Throws<InvalidStateException>(session.Close);
                Assert.NotEmpty(closeWhileAttached.Message);
            }
        );
        Assert.Equal(RenderSessionState.Detached, session.GetSnapshot().State);
        session.Close();
        Assert.True(session.IsClosed);
    }

    [Fact]
    public async Task ScopedViewSurvivesSiblingFrameFinalizationAndExpiresAfterReturn()
    {
        Assert.SkipUnless(
            OperatingSystem.IsMacOS()
                && Maplibre
                    .SupportedRenderBackendMask()
                    .HasFlag(global::Maplibre.NativeFfi.Base.RenderBackendFlag.Metal),
            "The selected native preset does not provide Metal frame acquisition."
        );
        using var fixture = OwnedTextureFixture.Create();
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = await runtime.MapCreateAsync(
            MapOptions.Default with
            {
                InitialExtent = new LogicalExtent(16, 16, 1),
            }
        );
        await map.SetStyleJsonAsync(TestStyles.Empty);
        using var session = fixture.Attach(map, new RenderTargetExtent(16, 16, 1));
        await session.Completion;
        session.RequestFrame(new FrameDemand((FrameDemandFlag)0, 1, 0, 0));
        _ = PollFor(() => session.DrainFrameCopies(), batch => batch.Count != 0);
        using var first = session.AcquireFrame();
        session.RequestFrame(new FrameDemand((FrameDemandFlag)0, 2, 0, 0));
        _ = PollFor(() => session.DrainFrameCopies(), batch => batch.Count != 0);
        var dropped = DropFrame(session);
        MetalOwnedTextureFrameView? escaped = null;
        first.WithMetalTexture(view =>
        {
            escaped = view;
            Assert.Equal(16u, view.Width);
            GC.Collect();
            GC.WaitForPendingFinalizers();
            Assert.False(dropped.IsAlive);
            Assert.NotEqual(0, view.Texture.Address);
            Assert.Equal(
                MaplibreStatus.TargetLost,
                Assert.Throws<MaplibreException>(() => first.WithMetalTexture(_ => { })).Status
            );
        });
        Assert.Throws<InvalidOperationException>(() => escaped!.Texture);
        Assert.Equal(
            MaplibreStatus.TargetLost,
            Assert.Throws<MaplibreException>(() => first.WithMetalTexture(_ => { })).Status
        );
        first.Release(GpuSync.Default);
    }

    [System.Runtime.CompilerServices.MethodImpl(
        System.Runtime.CompilerServices.MethodImplOptions.NoInlining
    )]
    private static WeakReference DropFrame(RenderSessionHandle session) =>
        new(session.AcquireFrame());

    [Fact]
    public async Task OwnedTextureSessionRejectsRetargetAndScaleFactorChange()
    {
        using var fixture = OwnedTextureFixture.Create();
        var extent = new RenderTargetExtent(32, 16, 1.0);
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with
                {
                    Width = 32,
                    Height = 16,
                    ScaleFactor = 1.0,
                },
            }
        );

        using var session = fixture.Attach(map, extent);
        await WithDetachAsync(
            session,
            async () =>
            {
                await session.Completion.WaitAsync(
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
                    session.RequestFrame(new FrameDemand((FrameDemandFlag)0x8000u, 0, 0, 0))
                );

                await session.ResizeAsync(
                    new RenderTargetExtent(64, 32, 1.0),
                    TestContext.Current.CancellationToken
                );
                var resized = session.GetSnapshot();
                Assert.Equal(64u, resized.Extent.Width);
                Assert.Equal(32u, resized.Extent.Height);
            }
        );
        session.Close();
    }

    private static async Task WithDetachAsync(RenderSessionHandle session, Func<Task> test)
    {
        Exception? failure = null;
        try
        {
            await test();
        }
        catch (Exception error)
        {
            failure = error;
            throw;
        }
        finally
        {
            try
            {
                // Cleanup remains required after test cancellation.
                await session.DetachAsync().WaitAsync(TimeSpan.FromSeconds(30));
            }
            catch (Exception cleanupError) when (failure is not null)
            {
                failure.Data["DetachFailure"] = cleanupError;
                try
                {
                    session.Abandon();
                }
                catch (Exception abandonError)
                {
                    failure.Data["AbandonFailure"] = abandonError;
                }
            }
        }
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
