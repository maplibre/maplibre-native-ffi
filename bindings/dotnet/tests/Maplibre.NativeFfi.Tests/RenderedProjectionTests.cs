using System.Runtime.InteropServices;
using System.Text;
using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RenderedProjectionTests
{
    public static bool SupportsMetal =>
        Maplibre.SupportedRenderBackends().HasFlag(RenderBackend.Metal);

    [DllImport("/System/Library/Frameworks/Metal.framework/Metal")]
    private static extern nint MTLCreateSystemDefaultDevice();

    [DllImport("/usr/lib/libobjc.A.dylib")]
    private static extern void objc_release(nint value);

    [Fact(SkipUnless = nameof(SupportsMetal), Skip = "This test attaches a Metal render target.")]
    public void ProjectionCapturesRenderedCameraAndOutlivesSession()
    {
        var device = MTLCreateSystemDefaultDevice();
        Assert.NotEqual(0, device);
        try
        {
            using var runtime = RuntimeHandle.Create(new RuntimeOptions());
            using var map = MapHandle.Create(runtime, new MapOptions { Width = 128, Height = 64 });
            using var session = RenderSessionHandle.AttachMetalOwnedTexture(
                map,
                new MetalOwnedTextureDescriptor
                {
                    Extent = new RenderTargetExtent(128, 64, 1),
                    Context = new MetalContextDescriptor
                    {
                        Device = NativePointer.FromBorrowedAddress(device),
                    },
                }
            );
            Assert.Throws<InvalidStateException>(() => session.CreateProjection());
            map.SetStyleJson(
                Encoding.UTF8.GetBytes("{\"version\":8,\"sources\":{},\"layers\":[]}")
            );
            runtime.Pump(TimeSpan.Zero);
            var coordinate = new LatLng(37.78, -122.41);
            map.JumpTo(
                new CameraOptions
                {
                    Center = new LatLng(37.7749, -122.4194),
                    Zoom = 12,
                    Bearing = 23,
                    Pitch = 40,
                }
            );
            runtime.Pump(TimeSpan.Zero);
            var expected = map.PixelForLatLng(coordinate);
            Assert.Equal(RenderResult.Rendered, session.RenderUpdate().Result);
            map.JumpTo(new CameraOptions { Center = new LatLng(37.80, -122.45) });
            runtime.Pump(TimeSpan.Zero);
            using var projection = session.CreateProjection();
            AssertPoint(expected, projection.PixelForLatLng(coordinate));
            Assert.True(Math.Abs(map.PixelForLatLng(coordinate).X - expected.X) > 1);
            using (var frame = session.AcquireMetalOwnedTextureFrame())
            using (var captured = session.CreateProjection())
                AssertPoint(expected, captured.PixelForLatLng(coordinate));
            session.Resize(96, 48, 2);
            Assert.Throws<InvalidStateException>(() => session.CreateProjection());
            runtime.Pump(TimeSpan.Zero);
            Assert.Equal(RenderResult.Rendered, session.RenderUpdate().Result);
            using (var resized = session.CreateProjection())
                AssertPoint(map.PixelForLatLng(coordinate), resized.PixelForLatLng(coordinate));
            session.Close();
            map.Close();
            runtime.Close();
            Exception? failure = null;
            var worker = new Thread(() =>
            {
                failure = Record.Exception(() =>
                    AssertPoint(expected, projection.PixelForLatLng(coordinate))
                );
            });
            worker.Start();
            worker.Join();
            Assert.Null(failure);
        }
        finally
        {
            objc_release(device);
        }
    }

    private static void AssertPoint(ScreenPoint expected, ScreenPoint actual)
    {
        Assert.Equal(expected.X, actual.X, 6);
        Assert.Equal(expected.Y, actual.Y, 6);
    }
}
