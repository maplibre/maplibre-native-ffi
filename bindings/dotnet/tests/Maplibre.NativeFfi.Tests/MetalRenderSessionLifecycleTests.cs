using System.Diagnostics;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed partial class MetalRenderSessionLifecycleTests
{
    [LibraryImport(
        "/System/Library/Frameworks/Metal.framework/Metal",
        EntryPoint = "MTLCreateSystemDefaultDevice"
    )]
    private static partial nint MetalCreateSystemDefaultDevice();

    [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_release")]
    private static partial void ObjectiveCRelease(nint value);

    [BindingSpecTest("BND-162", "BND-193")]
    [Fact]
    public async Task CallerDriverAttachmentAndDetachRequireExplicitService()
    {
        Assert.SkipUnless(
            OperatingSystem.IsMacOS()
                && Maplibre.SupportedRenderBackends().HasFlag(RenderBackend.Metal),
            "The selected native preset does not provide Metal."
        );

        var device = MetalCreateSystemDefaultDevice();
        if (device == 0)
        {
            Assert.Fail("Metal is compiled in but reported no system default device.");
        }
        try
        {
            using var runtime = RuntimeHandle.Create(new RuntimeOptions());
            using var map = await MapHandle.CreateAsync(
                runtime,
                new MapOptions
                {
                    Width = 32,
                    Height = 16,
                    ScaleFactor = 1.0,
                }
            );
            using var session = RenderSessionHandle.AttachMetalOwnedTexture(
                map,
                new MetalOwnedTextureDescriptor
                {
                    Extent = new RenderTargetExtent(32, 16, 1.0),
                    Context = new MetalContextDescriptor
                    {
                        Device = NativePointer.FromBorrowedAddress(device),
                    },
                },
                new RenderSessionAttachOptions { Driver = RenderDriverKind.CallerGraphicsThread }
            );

            Assert.False(session.Attachment.IsCompleted);
            ServiceUntilCompleted(session, session.Attachment);
            Assert.Equal(RenderSessionState.Attached, session.GetSnapshot().State);
            Assert.Equal(RenderDriverKind.CallerGraphicsThread, session.GetCapabilities().Driver);

            var detach = session.DetachAsync(TestContext.Current.CancellationToken);
            Assert.False(detach.IsCompleted);
            ServiceUntilCompleted(session, detach);
            Assert.Equal(RenderSessionState.Detached, session.GetSnapshot().State);
        }
        finally
        {
            ObjectiveCRelease(device);
        }
    }

    private static void ServiceUntilCompleted(RenderSessionHandle session, Task operation)
    {
        var cancellationToken = TestContext.Current.CancellationToken;
        var deadline = Stopwatch.StartNew();
        while (!operation.IsCompleted)
        {
            if (deadline.Elapsed > TimeSpan.FromSeconds(30))
            {
                throw new TimeoutException(
                    "The caller-driven session never completed its operation."
                );
            }
            cancellationToken.ThrowIfCancellationRequested();
            session.ServiceDriverWork(0);
            Thread.Yield();
        }

        operation.GetAwaiter().GetResult();
    }
}
