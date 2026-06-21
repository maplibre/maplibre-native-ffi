using Maplibre.Native.Camera;
using Maplibre.Native.Error;
using Maplibre.Native.Geo;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class MapCameraOptionsTests
{
    private const int CoordinatePrecision = 10;

    private static void AssertClose(LatLng expected, LatLng actual)
    {
        Assert.Equal(expected.Latitude, actual.Latitude, CoordinatePrecision);
        Assert.Equal(expected.Longitude, actual.Longitude, CoordinatePrecision);
    }

    [BindingSpecTest("BND-100")]
    [Fact]
    public void ViewportAndTileOptionsRoundTripThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.SetViewportOptions(
            new ViewportOptions
            {
                NorthOrientation = NorthOrientation.Right,
                ConstrainMode = ConstrainMode.WidthAndHeight,
                ViewportMode = ViewportMode.FlippedY,
                FrustumOffset = new EdgeInsets(1, 2, 3, 4),
            }
        );
        map.SetTileOptions(
            new TileOptions
            {
                PrefetchZoomDelta = 3,
                LodMinimumRadius = 1.5,
                LodScale = 2.5,
                LodPitchThreshold = 45,
                LodZoomShift = 1.25,
                LodMode = TileLodMode.Distance,
            }
        );

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

    [BindingSpecTest("BND-102")]
    [Fact]
    public void CameraFitHelpersCopyDescriptorsThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        var bounds = new LatLngBounds(new LatLng(-10, -20), new LatLng(10, 20));
        var fit = new CameraFitOptions
        {
            Padding = new EdgeInsets(1, 2, 3, 4),
            Bearing = 5,
            Pitch = 10,
        };

        var boundsCamera = map.CameraForLatLngBounds(bounds, fit);
        var coordinatesCamera = map.CameraForLatLngs([bounds.Southwest, bounds.Northeast], fit);
        var geometryCamera = map.CameraForGeometry(
            new Geometry.LineString([bounds.Southwest, bounds.Northeast]),
            fit
        );

        Assert.NotNull(boundsCamera.Center);
        Assert.NotNull(boundsCamera.Zoom);
        Assert.NotNull(coordinatesCamera.Center);
        Assert.NotNull(coordinatesCamera.Zoom);
        Assert.NotNull(geometryCamera.Center);
        Assert.NotNull(geometryCamera.Zoom);
    }

    [BindingSpecTest("BND-102", "BND-103")]
    [Fact]
    public void BoundsAndProjectionOptionsRoundTripThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        var bounds = new LatLngBounds(new LatLng(-10, -20), new LatLng(10, 20));
        map.SetBounds(
            new BoundOptions
            {
                Bounds = bounds,
                MinimumZoom = 1,
                MaximumZoom = 12,
                MinimumPitch = 0,
                MaximumPitch = 60,
            }
        );
        map.SetProjectionMode(
            new ProjectionModeOptions
            {
                Axonometric = true,
                XSkew = 0.1,
                YSkew = 0.2,
            }
        );

        var copiedBounds = map.GetBounds();
        Assert.Equal(bounds, copiedBounds.Bounds);
        Assert.NotNull(copiedBounds.MinimumZoom);
        Assert.Equal(1, copiedBounds.MinimumZoom.Value, 12);
        Assert.NotNull(copiedBounds.MaximumZoom);
        Assert.Equal(12, copiedBounds.MaximumZoom.Value, 12);
        Assert.NotNull(copiedBounds.MinimumPitch);
        Assert.Equal(0, copiedBounds.MinimumPitch.Value, 12);
        Assert.NotNull(copiedBounds.MaximumPitch);
        Assert.Equal(60, copiedBounds.MaximumPitch.Value, 12);

        var projectionMode = map.GetProjectionMode();
        Assert.True(projectionMode.Axonometric);
        Assert.NotNull(projectionMode.XSkew);
        Assert.Equal(0.1, projectionMode.XSkew.Value, 12);
        Assert.NotNull(projectionMode.YSkew);
        Assert.Equal(0.2, projectionMode.YSkew.Value, 12);

        var visibleBounds = map.LatLngBoundsForCamera(
            new CameraOptions { Center = new LatLng(0, 0), Zoom = 1 }
        );
        var unwrappedBounds = map.LatLngBoundsForCameraUnwrapped(
            new CameraOptions { Center = new LatLng(0, 0), Zoom = 1 }
        );
        Assert.True(visibleBounds.Southwest.Latitude <= visibleBounds.Northeast.Latitude);
        Assert.True(unwrappedBounds.Southwest.Latitude <= unwrappedBounds.Northeast.Latitude);
    }

    [BindingSpecTest("BND-104")]
    [Fact]
    public void InvalidMapAndProjectionInputsPropagateNativeDiagnostics()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        var mapError = Assert.Throws<InvalidArgumentException>(() =>
            map.JumpTo(new CameraOptions { Zoom = double.NaN })
        );
        var projectionError = Assert.Throws<InvalidArgumentException>(() =>
            map.SetProjectionMode(new ProjectionModeOptions { XSkew = double.NaN })
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, mapError.Status);
        Assert.NotNull(mapError.RawStatus);
        Assert.NotEmpty(mapError.Diagnostic);
        Assert.Equal(MaplibreStatus.InvalidArgument, projectionError.Status);
        Assert.NotNull(projectionError.RawStatus);
        Assert.NotEmpty(projectionError.Diagnostic);
    }

    [BindingSpecTest("BND-103")]
    [Fact]
    public void CoordinateProjectionRoundTripsThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        var coordinate = new LatLng(12.5, 34.25);

        var point = map.PixelForLatLng(coordinate);
        AssertClose(coordinate, map.LatLngForPixel(point));

        var points = map.PixelsForLatLngs([coordinate, new LatLng(0, 0)]);
        Assert.Equal(2, points.Length);
        var coordinates = map.LatLngsForPixels(points);
        Assert.Equal(2, coordinates.Length);
        AssertClose(coordinate, coordinates[0]);
        AssertClose(new LatLng(0, 0), coordinates[1]);

        Assert.Empty(map.PixelsForLatLngs([]));
        Assert.Empty(map.LatLngsForPixels([]));
    }

    [BindingSpecTest("BND-103")]
    [Fact]
    public void ProjectionSnapshotSupportsCameraAndCoordinateConversions()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        using var projection = map.CreateProjection();
        var coordinate = new LatLng(12.5, 34.25);

        projection.SetCamera(new CameraOptions { Center = new LatLng(0, 0), Zoom = 1 });
        projection.SetVisibleCoordinates(
            [new LatLng(-10, -20), new LatLng(10, 20)],
            new EdgeInsets(1, 2, 3, 4)
        );
        projection.SetVisibleGeometry(
            new Geometry.LineString([new LatLng(-10, -20), new LatLng(10, 20)]),
            new EdgeInsets(1, 2, 3, 4)
        );

        var camera = projection.GetCamera();
        Assert.NotNull(camera.Center);
        var point = projection.PixelForLatLng(coordinate);
        AssertClose(coordinate, projection.LatLngForPixel(point));

        projection.Close();
        Assert.True(projection.IsClosed);
    }

    [BindingSpecTest("BND-043")]
    [Fact]
    public void ProjectionSnapshotRemainsUsableAfterSourceMapCloses()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        using var projection = map.CreateProjection();
        var coordinate = new LatLng(12.5, 34.25);

        map.Close();

        var camera = projection.GetCamera();
        Assert.NotNull(camera);
        var point = projection.PixelForLatLng(coordinate);
        AssertClose(coordinate, projection.LatLngForPixel(point));

        projection.Close();
        runtime.Close();
    }

    [BindingSpecTest("BND-102")]
    [Fact]
    public void FreeCameraOptionsCanBeCopiedThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        var freeCamera = map.GetFreeCameraOptions();
        map.SetFreeCameraOptions(freeCamera);
    }

    [BindingSpecTest("BND-102")]
    [Fact]
    public void CameraTransitionCommandsAcceptOptionalAnimationDescriptors()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        var camera = new CameraOptions { Center = new LatLng(0, 0), Zoom = 1 };
        var animation = new AnimationOptions
        {
            Duration = 0,
            MinimumZoom = 0,
            Easing = new UnitBezier(0, 0, 1, 1),
        };

        map.EaseTo(camera, null);
        map.EaseTo(camera, animation);
        map.FlyTo(camera, null);
        map.FlyTo(camera, animation);
        map.MoveBy(0, 0);
        map.MoveByAnimated(0, 0, null);
        map.MoveByAnimated(0, 0, animation);
        map.ScaleBy(1, null);
        map.ScaleBy(1, new ScreenPoint(256, 256));
        map.ScaleByAnimated(1, null, null);
        map.ScaleByAnimated(1, new ScreenPoint(256, 256), null);
        map.ScaleByAnimated(1, null, animation);
        map.ScaleByAnimated(1, new ScreenPoint(256, 256), animation);
        map.RotateBy(new ScreenPoint(0, 0), new ScreenPoint(1, 1));
        map.RotateByAnimated(new ScreenPoint(0, 0), new ScreenPoint(1, 1), null);
        map.RotateByAnimated(new ScreenPoint(0, 0), new ScreenPoint(1, 1), animation);
        map.PitchBy(0);
        map.PitchByAnimated(0, null);
        map.PitchByAnimated(0, animation);
        map.CancelTransitions();
    }

    [BindingSpecTest("BND-102")]
    [Fact]
    public void JumpToAppliesCameraFieldsThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.JumpTo(
            new CameraOptions
            {
                Center = new LatLng(12.5, 34.25),
                Zoom = 5.5,
                Bearing = 45,
                Pitch = 30,
            }
        );

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
