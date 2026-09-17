using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RenderedProjectionTests
{
    [Fact]
    public async Task ProjectionCapturesRenderedCameraAndOutlivesSession()
    {
        Assert.SkipUnless(OwnedTextureFixture.IsAvailable, "No offscreen backend is available.");
        using var fixture = OwnedTextureFixture.Create();
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 32, Height = 16 });
        using var session = fixture.Attach(map, new RenderTargetExtent(32, 16, 1));
        await session.Attachment;
        try
        {
            Assert.Throws<InvalidStateException>(() => session.CreateProjection());
            await map.SetStyleJsonAsync("{\"version\":8,\"sources\":{},\"layers\":[]}"u8.ToArray());
            await map.UpdateCameraAsync(
                new CameraUpdate { Camera = new CameraOptions { Zoom = 3 } }
            );
            session.RequestFrame(new FrameDemand { Token = 1 });
            await session.BarrierAsync();
            Assert.Equal(
                RenderResult.Rendered,
                Assert.Single(session.DrainFrameResults()).Disposition
            );
            await map.UpdateCameraAsync(
                new CameraUpdate { Camera = new CameraOptions { Zoom = 6 } }
            );
            using var projection = session.CreateProjection();
            Assert.Equal(3, projection.GetCamera().Zoom);
            await session.ResizeAsync(new RenderTargetExtent(16, 16, 1));
            Assert.Throws<InvalidStateException>(() => session.CreateProjection());
            await session.DetachAsync();
            session.Close();
            await Task.Run(() => Assert.Equal(3, projection.GetCamera().Zoom));
        }
        finally
        {
            if (!session.IsClosed)
                await session.DetachAsync();
        }
    }
}
