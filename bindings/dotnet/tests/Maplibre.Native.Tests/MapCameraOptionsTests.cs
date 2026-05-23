using Maplibre.Native.Camera;
using Maplibre.Native.Geo;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class MapCameraOptionsTests
{
    [Fact]
    public void ViewportAndTileOptionsRoundTripThroughNativeMap()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.SetViewportOptions(new ViewportOptions
        {
            NorthOrientation = NorthOrientation.Right,
            ConstrainMode = ConstrainMode.WidthAndHeight,
            ViewportMode = ViewportMode.FlippedY,
            FrustumOffset = new EdgeInsets(1, 2, 3, 4),
        });
        map.SetTileOptions(new TileOptions
        {
            PrefetchZoomDelta = 3,
            LodMinimumRadius = 1.5,
            LodScale = 2.5,
            LodPitchThreshold = 45,
            LodZoomShift = 1.25,
            LodMode = TileLodMode.Distance,
        });

        var viewport = map.GetViewportOptions();
        Assert.Equal(NorthOrientation.Right, viewport.NorthOrientation);
        Assert.Equal(ConstrainMode.WidthAndHeight, viewport.ConstrainMode);
        Assert.Equal(ViewportMode.FlippedY, viewport.ViewportMode);
        Assert.Equal(new EdgeInsets(1, 2, 3, 4), viewport.FrustumOffset);

        var tile = map.GetTileOptions();
        Assert.Equal(3u, tile.PrefetchZoomDelta);
        Assert.Equal(1.5, tile.LodMinimumRadius);
        Assert.Equal(2.5, tile.LodScale);
        Assert.Equal(45, tile.LodPitchThreshold);
        Assert.Equal(1.25, tile.LodZoomShift);
        Assert.Equal(TileLodMode.Distance, tile.LodMode);
    }

    [Fact]
    public void JumpToAppliesCameraFieldsThroughNativeMap()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.JumpTo(new CameraOptions
        {
            Center = new LatLng(12.5, 34.25),
            Zoom = 5.5,
            Bearing = 45,
            Pitch = 30,
        });

        var camera = map.GetCamera();
        Assert.NotNull(camera.Center);
        Assert.Equal(12.5, camera.Center.Value.Latitude, 12);
        Assert.Equal(34.25, camera.Center.Value.Longitude, 12);
        Assert.NotNull(camera.Zoom);
        Assert.Equal(5.5, camera.Zoom.Value, 12);
        Assert.NotNull(camera.Bearing);
        Assert.Equal(45, camera.Bearing.Value, 12);
        Assert.NotNull(camera.Pitch);
        Assert.Equal(30, camera.Pitch.Value, 12);
    }
}
