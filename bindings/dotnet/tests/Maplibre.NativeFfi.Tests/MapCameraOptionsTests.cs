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
    public void ViewportAndTileOptionsRoundTripThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        map.SetViewportOptionsAsync(
            new ViewportOptions
            {
                NorthOrientation = NorthOrientation.Right,
                ConstrainMode = ConstrainMode.WidthAndHeight,
                ViewportMode = ViewportMode.FlippedY,
                FrustumOffset = new EdgeInsets(1, 2, 3, 4),
            },
            TestContext.Current.CancellationToken
        );
        var completion = map.SetTileOptionsAsync(
            new TileOptions
            {
                PrefetchZoomDelta = 3,
                LodMinimumRadius = 1.5,
                LodScale = 2.5,
                LodPitchThreshold = 45,
                LodZoomShift = 1.25,
                LodMode = TileLodMode.Distance,
            },
            TestContext.Current.CancellationToken
        );
        RuntimeEventTestHelpers.AssertCommitted(completion);

        var snapshot = map.GetSnapshot();
        var viewport = snapshot.Viewport;
        Assert.Equal(NorthOrientation.Right, viewport.NorthOrientation);
        Assert.Equal(ConstrainMode.WidthAndHeight, viewport.ConstrainMode);
        Assert.Equal(ViewportMode.FlippedY, viewport.ViewportMode);
        Assert.Equal(new EdgeInsets(1, 2, 3, 4), viewport.FrustumOffset);

        var tile = snapshot.Tile;
        Assert.Equal(3u, tile.PrefetchZoomDelta);
        Assert.Equal(1.5, tile.LodMinimumRadius);
        Assert.Equal(2.5, tile.LodScale);
        Assert.Equal(45, tile.LodPitchThreshold);
        Assert.Equal(1.25, tile.LodZoomShift);
        Assert.Equal(TileLodMode.Distance, tile.LodMode);

        var freeCameraCompletion = map.SetFreeCameraOptionsAsync(
            new FreeCameraOptions
            {
                Position = new Vec3(0.5, 0.5, 0.125),
                Orientation = new Quaternion(0, 0, 0, 1),
            },
            TestContext.Current.CancellationToken
        );
        RuntimeEventTestHelpers.AssertCommitted(freeCameraCompletion);
        var freeCamera = map.GetSnapshot().FreeCamera;
        Assert.NotNull(freeCamera.Position);
        Assert.Equal(0.5, freeCamera.Position.Value.X, 12);
        Assert.Equal(0.5, freeCamera.Position.Value.Y, 12);
        Assert.Equal(0.125, freeCamera.Position.Value.Z, 12);
        Assert.NotNull(freeCamera.Orientation);
    }

    [BindingSpecTest("BND-102")]
    [Fact]
    public async Task CameraFitHelpersCopyDescriptorsThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
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
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        var bounds = new LatLngBounds(new LatLng(-10, -20), new LatLng(10, 20));
        _ = map.SetBoundsAsync(
            new BoundOptions
            {
                Bounds = new BoundsConstraint.Bounded(bounds),
                MinimumZoom = 1,
                MaximumZoom = 12,
                MinimumPitch = 0,
                MaximumPitch = 60,
            },
            TestContext.Current.CancellationToken
        );
        var completion = map.SetProjectionModeAsync(
            new ProjectionModeOptions
            {
                Axonometric = true,
                XSkew = 0.1,
                YSkew = 0.2,
            },
            TestContext.Current.CancellationToken
        );
        RuntimeEventTestHelpers.AssertCommitted(completion);

        var copiedBounds = map.GetSnapshot().Bounds;
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

    // Unbounded and world bounds are different constraints: world bounds clamp longitude, so a
    // pan past the antimeridian comes back inside, while unbounded keeps going.
    [BindingSpecTest("BND-102")]
    [Fact]
    public async Task UnboundedCentersDifferFromWorldBoundedOnes()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.SetBoundsAsync(
                new BoundOptions { Bounds = BoundsConstraint.Unbounded.Instance },
                TestContext.Current.CancellationToken
            )
        );
        Assert.Equal(BoundsConstraint.Unbounded.Instance, map.GetSnapshot().Bounds.Bounds);

        var world = new LatLngBounds(new LatLng(-90, -180), new LatLng(90, 180));
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetBoundsAsync(
                new BoundOptions { Bounds = new BoundsConstraint.Bounded(world) },
                TestContext.Current.CancellationToken
            )
        );
        Assert.Equal(new BoundsConstraint.Bounded(world), map.GetSnapshot().Bounds.Bounds);

        RuntimeEventTestHelpers.AssertCommitted(
            map.UpdateCameraAsync(
                new CameraUpdate
                {
                    Mode = CameraUpdateMode.Jump,
                    Camera = new CameraOptions { Center = new LatLng(0, 200), Zoom = 1 },
                },
                TestContext.Current.CancellationToken
            )
        );
        var clamped = await map.QueryCameraAsync(TestContext.Current.CancellationToken);
        Assert.InRange(clamped.Camera.Center!.Value.Longitude, -180, 180);
    }

    [BindingSpecTest("BND-102", "BND-104")]
    [Fact]
    public async Task CameraCommandsReturnIdsAndSnapshotsAdvance()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        var before = map.GetCameraSnapshot();

        var command = map.UpdateCameraAsync(
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
            },
            TestContext.Current.CancellationToken
        );
        var ordered = await map.QueryCameraAsync(TestContext.Current.CancellationToken);

        RuntimeEventTestHelpers.AssertCommitted(command);
        Assert.True(ordered.Generation > before.Generation);
        Assert.Equal(12.5, ordered.Camera.Center!.Value.Latitude, 12);
        Assert.Equal(5.5, ordered.Camera.Zoom!.Value, 12);
        Assert.Equal(ordered, map.GetCameraSnapshot());
    }

    [BindingSpecTest("BND-104", "BND-105")]
    [Fact]
    public void AnEasedTransitionReportsItsIdOnceAndACancelEndsTheNextOne()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.UpdateCameraAsync(
                new CameraUpdate
                {
                    Mode = CameraUpdateMode.Ease,
                    Camera = new CameraOptions { Center = new LatLng(10, 20), Zoom = 4 },
                    Animation = new AnimationOptions { Duration = 5000, TransitionId = 11 },
                },
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.UpdateCameraAsync(
                new CameraUpdate
                {
                    Mode = CameraUpdateMode.Ease,
                    Camera = new CameraOptions { Center = new LatLng(-10, -20), Zoom = 6 },
                    Animation = new AnimationOptions { Duration = 5000, TransitionId = 12 },
                },
                TestContext.Current.CancellationToken
            )
        );

        // Starting the second transition ends the first, which reports its own ID.
        var drained = WaitForTransition(runtime, map, 11);
        Assert.Contains(
            drained,
            runtimeEvent => runtimeEvent.Type == RuntimeEventType.MapCameraDidChange
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.UpdateCameraAsync(
                new CameraUpdate
                {
                    Mode = CameraUpdateMode.Jump,
                    Camera = new CameraOptions(),
                    GesturePhase = GesturePhase.Cancel,
                },
                TestContext.Current.CancellationToken
            )
        );
        WaitForTransition(runtime, map, 12);
    }

    [BindingSpecTest("BND-104")]
    [Fact]
    public void CancellingTransitionsEndsAFlyTransitionByItsId()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.UpdateCameraAsync(
                new CameraUpdate
                {
                    Mode = CameraUpdateMode.Fly,
                    Camera = new CameraOptions { Center = new LatLng(45, 90), Zoom = 9 },
                    Animation = new AnimationOptions { Duration = 5000, TransitionId = 21 },
                },
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.CancelTransitionsAsync(TestContext.Current.CancellationToken)
        );

        WaitForTransition(runtime, map, 21);

        // Cancelling with nothing running commits and changes nothing.
        RuntimeEventTestHelpers.AssertCommitted(
            map.CancelTransitionsAsync(TestContext.Current.CancellationToken)
        );
    }

    [BindingSpecTest("BND-104")]
    [Fact]
    public void AGestureMarksTheSnapshotUntilTheGestureEnds()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        Assert.False(map.GetSnapshot().GestureInProgress);

        RuntimeEventTestHelpers.AssertCommitted(
            map.UpdateCameraAsync(
                new CameraUpdate
                {
                    Mode = CameraUpdateMode.Jump,
                    Camera = new CameraOptions { Zoom = 3 },
                    GesturePhase = GesturePhase.Begin,
                },
                TestContext.Current.CancellationToken
            )
        );
        Assert.True(map.GetSnapshot().GestureInProgress);

        RuntimeEventTestHelpers.AssertCommitted(
            map.UpdateCameraAsync(
                new CameraUpdate
                {
                    Mode = CameraUpdateMode.Jump,
                    Camera = new CameraOptions { Zoom = 4 },
                    GesturePhase = GesturePhase.End,
                },
                TestContext.Current.CancellationToken
            )
        );
        Assert.False(map.GetSnapshot().GestureInProgress);
    }

    [BindingSpecTest("BND-104")]
    [Fact]
    public async Task ARelativeCameraDeltaMovesThePublishedCamera()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.UpdateCameraAsync(
                new CameraUpdate
                {
                    Mode = CameraUpdateMode.Jump,
                    Camera = new CameraOptions { Center = new LatLng(0, 0), Zoom = 4 },
                },
                TestContext.Current.CancellationToken
            )
        );
        var before = map.GetCameraSnapshot();

        RuntimeEventTestHelpers.AssertCommitted(
            map.ApplyCameraDeltaAsync(
                new CameraDelta { Kind = CameraDeltaKind.Scale, Amount = 2 },
                TestContext.Current.CancellationToken
            )
        );

        var after = await map.QueryCameraAsync(TestContext.Current.CancellationToken);
        Assert.True(after.Generation > before.Generation);
        Assert.Equal(5, after.Camera.Zoom!.Value, 12);
    }

    // Drains until the one transition-finished event carrying `transitionId` arrives, then
    // reports every event drained on the way.
    private static IReadOnlyList<RuntimeEvent> WaitForTransition(
        RuntimeHandle runtime,
        MapHandle map,
        ulong transitionId
    )
    {
        static bool Finishes(RuntimeEvent runtimeEvent, ulong transitionId) =>
            runtimeEvent.Payload is RuntimeEventPayload.CameraTransitionFinished finished
            && finished.TransitionId == transitionId;

        var drained = RuntimeEventTestHelpers
            .DrainUntil(runtime, batch => batch.Any(one => Finishes(one, transitionId)))
            .All;

        var single = Assert.Single(drained, one => Finishes(one, transitionId));
        Assert.Equal(RuntimeEventType.MapCameraTransitionFinished, single.Type);
        Assert.Same(map, single.MapSource);
        return drained;
    }

    [BindingSpecTest("BND-104")]
    [Fact]
    public async Task InvalidCameraUpdatePropagatesNativeDiagnostic()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        var error = await Assert.ThrowsAsync<InvalidArgumentException>(() =>
            map.UpdateCameraAsync(
                new CameraUpdate
                {
                    Mode = CameraUpdateMode.Jump,
                    Camera = new CameraOptions { Zoom = double.NaN },
                },
                TestContext.Current.CancellationToken
            )
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.NotEmpty(error.Diagnostic);
    }

    [BindingSpecTest("BND-043", "BND-103")]
    [Fact]
    public async Task CoordinateProjectionRoundTripsThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
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
    public async Task UnwrappedCoordinateConversionsPreserveVisibleWorldCopies()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 1024, Height = 512 }
        );
        await map.UpdateCameraAsync(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Jump,
                Camera = new CameraOptions { Center = new LatLng(0, 180), Zoom = 0 },
            },
            TestContext.Current.CancellationToken
        );
        ScreenPoint[] points = [new(0, 256), new(1024, 256)];

        var wrapped = await map.LatLngsForPixelsAsync(
            points,
            TestContext.Current.CancellationToken
        );
        var unwrapped = await map.LatLngsForPixelsUnwrappedAsync(
            points,
            TestContext.Current.CancellationToken
        );
        Assert.All(wrapped, coordinate => Assert.InRange(coordinate.Longitude, -180, 180));
        Assert.True(unwrapped[1].Longitude - unwrapped[0].Longitude > 360);
        var wrappedRight = await map.LatLngForPixelAsync(
            points[1],
            TestContext.Current.CancellationToken
        );
        Assert.InRange(wrappedRight.Longitude, -180, 180);
        var right = await map.LatLngForPixelUnwrappedAsync(
            points[1],
            TestContext.Current.CancellationToken
        );
        Assert.Equal(unwrapped[1].Longitude, right.Longitude, CoordinatePrecision);

        using var projection = await map.CreateProjectionAsync();
        Assert.InRange(projection.LatLngForPixel(points[1]).Longitude, -180, 180);
        var projectedRight = projection.LatLngForPixelUnwrapped(points[1]);
        Assert.Equal(right.Longitude, projectedRight.Longitude, CoordinatePrecision);
    }

    [BindingSpecTest("BND-103")]
    [Fact]
    public async Task ProjectionConvertsSynchronouslyAndObservesItsOwnSetters()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        using var projection = await map.CreateProjectionAsync();
        var coordinate = new LatLng(12.5, 34.25);

        // Synchronous round trip: pixel -> latlng -> pixel.
        var point = projection.PixelForLatLng(coordinate);
        AssertClose(coordinate, projection.LatLngForPixel(point));

        // A setter changes later conversions on the same handle.
        projection.SetCamera(new CameraOptions { Center = new LatLng(40, -75), Zoom = 6 });
        var camera = projection.GetCamera();
        Assert.NotNull(camera.Center);
        AssertClose(new LatLng(40, -75), camera.Center.Value);
        var movedPoint = projection.PixelForLatLng(coordinate);
        Assert.NotEqual(point, movedPoint);
        AssertClose(coordinate, projection.LatLngForPixel(movedPoint));

        projection.SetVisibleCoordinates(
            [new LatLng(-10, -20), new LatLng(10, 20)],
            new EdgeInsets(1, 1, 1, 1)
        );
        Assert.NotNull(projection.GetCamera().Center);

        // A projection is usable from a second thread.
        var threadRoundTrip = await Task.Run(
            () => projection.LatLngForPixel(projection.PixelForLatLng(coordinate)),
            TestContext.Current.CancellationToken
        );
        AssertClose(coordinate, threadRoundTrip);

        map.Close();
        runtime.Close();

        // The projection stays usable after its map and runtime close, including from a
        // thread that first touches it after those closes.
        Exception? failure = null;
        var worker = new Thread(() =>
        {
            failure = Record.Exception(() =>
            {
                var closedCamera = projection.GetCamera();
                Assert.NotNull(closedCamera);
                AssertClose(
                    coordinate,
                    projection.LatLngForPixel(projection.PixelForLatLng(coordinate))
                );
            });
        });
        worker.Start();
        worker.Join();
        Assert.Null(failure);

        // Close is synchronous and idempotent through Dispose.
        projection.Close();
        Assert.True(projection.IsClosed);
    }

    [BindingSpecTest("BND-103")]
    [Fact]
    public async Task ProjectionCreatedAfterCameraCommandObservesThatCommand()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        var center = new LatLng(40, -75);
        _ = map.UpdateCameraAsync(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Jump,
                Camera = new CameraOptions { Center = center, Zoom = 6 },
            },
            TestContext.Current.CancellationToken
        );

        using var projection = await map.CreateProjectionAsync();
        var camera = projection.GetCamera();
        Assert.NotNull(camera.Center);
        AssertClose(center, camera.Center.Value);

        // The projection never observes map changes made after creation.
        _ = map.UpdateCameraAsync(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Jump,
                Camera = new CameraOptions { Center = new LatLng(0, 0), Zoom = 1 },
            },
            TestContext.Current.CancellationToken
        );
        await map.QueryCameraAsync(TestContext.Current.CancellationToken);
        AssertClose(center, projection.GetCamera().Center!.Value);
    }
}
