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
            Maplibre.SupportedRenderBackends().HasFlag(RenderBackend.Metal),
            "The selected native preset does not provide Metal."
        );

        Assert.True(OperatingSystem.IsMacOS());
        var device = MetalCreateSystemDefaultDevice();
        Assert.NotEqual(0, device);
        try
        {
            var runtime = RuntimeHandle.Create(new RuntimeOptions());
            var map = await MapHandle.CreateAsync(
                runtime,
                new MapOptions
                {
                    Width = 32,
                    Height = 16,
                    ScaleFactor = 1.0,
                },
                TestContext.Current.CancellationToken
            );
            var session = RenderSessionHandle.AttachMetalOwnedTexture(
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
            Assert.Equal(RenderSessionState.Attached, session.Snapshot.State);
            Assert.Equal(RenderDriverKind.CallerGraphicsThread, session.Capabilities.Driver);

            var detach = session.DetachAsync(TestContext.Current.CancellationToken);
            Assert.False(detach.IsCompleted);
            ServiceUntilCompleted(session, detach);
            Assert.Equal(RenderSessionState.Detached, session.Snapshot.State);

            session.Close();
            map.Close();
            await runtime.CloseAsync();
        }
        finally
        {
            ObjectiveCRelease(device);
        }
    }

    private static void ServiceUntilCompleted(RenderSessionHandle session, Task operation)
    {
        var cancellationToken = TestContext.Current.CancellationToken;
        while (!operation.IsCompleted)
        {
            cancellationToken.ThrowIfCancellationRequested();
            session.ServiceDriverWork(0);
            Thread.Yield();
        }

        operation.GetAwaiter().GetResult();
    }
}
