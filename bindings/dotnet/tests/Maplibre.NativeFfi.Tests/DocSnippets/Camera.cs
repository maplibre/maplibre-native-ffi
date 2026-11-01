// Snippets for the docs site's Camera page. They compile with the tests, which
// never run them.

namespace Maplibre.NativeFfi.Tests.DocSnippets;

internal static class Camera
{
    private static void UseCamera(Maplibre.NativeFfi.Camera camera) { }

    private static void FlyEnded(bool completed) { }

    // #region gesture
    private static void BeginGesture(MapHandle map)
    {
        map.Discarding.CancelTransitions();
        map.Discarding.ApplyCameraDelta(new CameraDelta { GesturePhase = GesturePhase.Begin });
    }

    private static void ApplyDrag(MapHandle map, ScreenPoint movement)
    {
        map.Discarding.ApplyCameraDelta(
            new CameraDelta { Offset = movement, GesturePhase = GesturePhase.Update }
        );
    }

    private static void ApplyPinch(MapHandle map, double scale, ScreenPoint center)
    {
        map.Discarding.ApplyCameraDelta(
            new CameraDelta
            {
                Scale = scale,
                Anchor = center,
                GesturePhase = GesturePhase.Update,
            }
        );
    }

    private static void EndGesture(MapHandle map)
    {
        map.Discarding.ApplyCameraDelta(new CameraDelta { GesturePhase = GesturePhase.End });
    }

    // #endregion gesture

    internal static async Task Run(MapHandle map)
    {
        // #region read
        var snapshot = map.GetSnapshot();
        UseCamera(snapshot.View.Camera);

        var result = await map.GetCameraAsync();
        UseCamera(result.Camera);
        // #endregion read

        // #region jump
        map.Discarding.UpdateCamera(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Jump,
                Camera = new CameraOptions { Center = new LatLng(48.8566, 2.3522), Zoom = 12 },
            }
        );
        // #endregion jump

        // #region animate
        map.Discarding.UpdateCamera(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Fly,
                Camera = new CameraOptions { Center = new LatLng(40.7128, -74.0060), Zoom = 10 },
                Animation = new AnimationOptions
                {
                    DurationMs = 3000,
                    // Runs on a library thread.
                    EndHandler = new CameraTransitionHandler(end =>
                        FlyEnded(end.Outcome == CameraTransitionOutcome.Completed)
                    ),
                },
            }
        );
        // #endregion animate

        // #region fit
        var bounds = new LatLngBounds(new LatLng(45.8, 5.9), new LatLng(47.8, 10.5));
        var fitted = await map.CameraForLatLngBoundsAsync(bounds);

        map.Discarding.UpdateCamera(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Ease,
                Camera = fitted,
                Animation = new AnimationOptions { DurationMs = 500 },
            }
        );
        // #endregion fit

        // #region limit
        map.Discarding.SetBounds(
            new BoundOptions
            {
                Bounds = bounds,
                MinZoom = 5,
                MaxZoom = 16,
            }
        );
        // #endregion limit

        BeginGesture(map);
        ApplyDrag(map, new ScreenPoint(12, -4));
        ApplyPinch(map, 1.1, new ScreenPoint(200, 150));
        EndGesture(map);

        // #region convert
        using (var projection = MapProjectionHandle.Create(map.GetSnapshot().View))
        {
            var coordinate = projection.LatLngForPixel(new ScreenPoint(100, 200));
            var point = projection.PixelForLatLng(coordinate);
        }
        // #endregion convert
    }
}
