using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

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
    public async Task ViewportAndTileOptionsRoundTripThroughNativeMap()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

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

        var viewport = await map.GetViewportOptionsAsync(TestContext.Current.CancellationToken);
        Assert.Equal(NorthOrientation.Right, viewport.NorthOrientation);
        Assert.Equal(ConstrainMode.WidthAndHeight, viewport.ConstrainMode);
        Assert.Equal(ViewportMode.FlippedY, viewport.ViewportMode);
        Assert.Equal(new EdgeInsets(1, 2, 3, 4), viewport.FrustumOffset);

        var tile = await map.GetTileOptionsAsync(TestContext.Current.CancellationToken);
        Assert.Equal(3u, tile.PrefetchZoomDelta);
        Assert.Equal(1.5, tile.LodMinimumRadius);
        Assert.Equal(2.5, tile.LodScale);
        Assert.Equal(45, tile.LodPitchThreshold);
        Assert.Equal(1.25, tile.LodZoomShift);
        Assert.Equal(TileLodMode.Distance, tile.LodMode);
    }

    [BindingSpecTest("BND-102")]
    [Fact]
    public async Task CameraFitHelpersCopyDescriptorsThroughNativeMap()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        var bounds = new LatLngBounds(new LatLng(-10, -20), new LatLng(10, 20));
        var fit = new CameraFitOptions
        {
            Padding = new EdgeInsets(1, 2, 3, 4),
            Bearing = 5,
            Pitch = 10,
        };

        var boundsCamera = await map.CameraForLatLngBoundsAsync(
            bounds,
            fit,
            TestContext.Current.CancellationToken
        );
        var coordinatesCamera = await map.CameraForLatLngsAsync(
            [bounds.Southwest, bounds.Northeast],
            fit,
            TestContext.Current.CancellationToken
        );
        var geometryCamera = await map.CameraForGeometryAsync(
            """{"type":"LineString","coordinates":[[-20,-10],[20,10]]}"""u8.ToArray(),
            fit,
            TestContext.Current.CancellationToken
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
    public async Task BoundsAndProjectionOptionsRoundTripThroughNativeMap()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        var bounds = new LatLngBounds(new LatLng(-10, -20), new LatLng(10, 20));
        map.SetBounds(
            new BoundOptions
            {
                Bounds = new BoundsConstraint.Bounded(bounds),
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

        var copiedBounds = await map.GetBoundsAsync(TestContext.Current.CancellationToken);
        Assert.Equal(new BoundsConstraint.Bounded(bounds), copiedBounds.Bounds);
        Assert.NotNull(copiedBounds.MinimumZoom);
        Assert.Equal(1, copiedBounds.MinimumZoom.Value, 12);
        Assert.NotNull(copiedBounds.MaximumZoom);
        Assert.Equal(12, copiedBounds.MaximumZoom.Value, 12);
        Assert.NotNull(copiedBounds.MinimumPitch);
        Assert.Equal(0, copiedBounds.MinimumPitch.Value, 12);
        Assert.NotNull(copiedBounds.MaximumPitch);
        Assert.Equal(60, copiedBounds.MaximumPitch.Value, 12);

        var projectionMode = map.GetSnapshot().ProjectionMode;
        Assert.True(projectionMode.Axonometric);
        Assert.NotNull(projectionMode.XSkew);
        Assert.Equal(0.1, projectionMode.XSkew.Value, 12);
        Assert.NotNull(projectionMode.YSkew);
        Assert.Equal(0.2, projectionMode.YSkew.Value, 12);

        var camera = new CameraOptions { Center = new LatLng(0, 0), Zoom = 1 };
        var visibleBounds = await map.LatLngBoundsForCameraAsync(
            camera,
            TestContext.Current.CancellationToken
        );
        var unwrappedBounds = await map.LatLngBoundsForCameraUnwrappedAsync(
            camera,
            TestContext.Current.CancellationToken
        );
        Assert.True(visibleBounds.Southwest.Latitude <= visibleBounds.Northeast.Latitude);
        Assert.True(unwrappedBounds.Southwest.Latitude <= unwrappedBounds.Northeast.Latitude);
    }

    [BindingSpecTest("BND-102", "BND-104")]
    [Fact]
    public async Task CameraCommandsReturnIdsAndSnapshotsAdvance()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        var before = map.GetCameraSnapshot();

        var commandId = map.UpdateCamera(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Jump,
                Camera = new CameraOptions
                {
                    Center = new LatLng(12.5, 34.25),
                    Zoom = 5.5,
                    Bearing = 45,
                    Pitch = 30,
                },
            }
        );
        var ordered = await map.QueryCameraAsync(TestContext.Current.CancellationToken);

        Assert.NotEqual(0ul, commandId);
        Assert.True(ordered.Generation > before.Generation);
        Assert.Equal(12.5, ordered.Camera.Center!.Value.Latitude, 12);
        Assert.Equal(5.5, ordered.Camera.Zoom!.Value, 12);
        Assert.Equal(ordered, map.GetCameraSnapshot());
    }

    [BindingSpecTest("BND-104")]
    [Fact]
    public void InvalidCameraUpdatePropagatesNativeDiagnostic()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        var error = Assert.Throws<InvalidArgumentException>(() =>
            map.UpdateCamera(
                new CameraUpdate
                {
                    Mode = CameraUpdateMode.Jump,
                    Camera = new CameraOptions { Zoom = double.NaN },
                }
            )
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.NotEmpty(error.Diagnostic);
    }

    [BindingSpecTest("BND-103")]
    [Fact]
    public async Task CoordinateProjectionRoundTripsThroughNativeMap()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        var coordinate = new LatLng(12.5, 34.25);

        var point = await map.PixelForLatLngAsync(
            coordinate,
            TestContext.Current.CancellationToken
        );
        AssertClose(
            coordinate,
            await map.LatLngForPixelAsync(point, TestContext.Current.CancellationToken)
        );
        var points = await map.PixelsForLatLngsAsync(
            [coordinate, new LatLng(0, 0)],
            TestContext.Current.CancellationToken
        );
        var coordinates = await map.LatLngsForPixelsAsync(
            points,
            TestContext.Current.CancellationToken
        );
        AssertClose(coordinate, coordinates[0]);
        AssertClose(new LatLng(0, 0), coordinates[1]);
    }

    [BindingSpecTest("BND-103")]
    [Fact]
    public async Task ProjectionOrderedQueriesCompleteAutonomously()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        using var projection = await map.CreateProjectionAsync(
            TestContext.Current.CancellationToken
        );
        var coordinate = new LatLng(12.5, 34.25);

        var commandId = projection.SetCamera(
            new CameraOptions { Center = new LatLng(0, 0), Zoom = 1 }
        );
        var camera = await projection.GetCameraAsync(TestContext.Current.CancellationToken);
        var point = await projection.PixelForLatLngAsync(
            coordinate,
            TestContext.Current.CancellationToken
        );
        var roundTrip = await projection.LatLngForPixelAsync(
            point,
            TestContext.Current.CancellationToken
        );

        Assert.NotEqual(0ul, commandId);
        Assert.NotNull(camera.Center);
        AssertClose(coordinate, roundTrip);
    }
}
