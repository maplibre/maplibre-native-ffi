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
        using var fixture = OwnedTextureFixture.Create();
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 32, Height = 16 },
            }
        );
        using var session = fixture.Attach(map, new RenderTargetExtent(32, 16, 1));
        await session.Completion;
        try
        {
            Assert.Throws<InvalidStateException>(() => session.ProjectionCreate());
            await map.SetStyleJsonAsync("{\"version\":8,\"sources\":{},\"layers\":[]}"u8.ToArray());
            await map.UpdateCameraAsync(
                CameraUpdate.Default with
                {
                    Camera = new CameraOptions { Zoom = 3 },
                }
            );
            session.RequestFrame(new FrameDemand { Token = 1 });
            await session.BarrierAsync();
            Assert.Equal(
                RenderResult.Rendered,
                Assert.Single(session.DrainFrameCopies()).Disposition
            );
            await map.UpdateCameraAsync(
                CameraUpdate.Default with
                {
                    Camera = new CameraOptions { Zoom = 6 },
                }
            );
            using var projection = session.ProjectionCreate();
            Assert.Equal(3, projection.GetCamera().Zoom);
            await session.ResizeAsync(new RenderTargetExtent(16, 16, 1));
            Assert.Throws<InvalidStateException>(() => session.ProjectionCreate());
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
