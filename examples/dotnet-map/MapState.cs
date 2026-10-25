namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>The runtime and its map. Commands go straight to the runtime's own thread.</summary>
internal sealed class MapState : IDisposable
{
    private const string StyleUrl = "https://tiles.openfreemap.org/styles/bright";

    private readonly RuntimeHandle runtime;
    private bool closed;

    private MapState(RuntimeHandle runtime, MapHandle map)
    {
        this.runtime = runtime;
        Map = map;
    }

    public MapHandle Map { get; }

    /// <summary>
    /// Creates the map with the example's style, or with <paramref name="styleJson" /> in its
    /// place, which the smoke run passes so that it needs no network.
    /// </summary>
    public static MapState Create(Viewport viewport, string? styleJson = null)
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default with { CachePath = ":memory:" });
        MapHandle? map = null;
        try
        {
            map = runtime.CreateMap(
                MapOptions.Default with
                {
                    InitialExtent = viewport.LogicalExtent,
                    MapMode = MapMode.Continuous,
                    EventMask = 0,
                }
            );
            // A render target attaches only to a ready map.
            map.Ready.GetAwaiter().GetResult();
            var style = styleJson is null
                ? map.SetStyleUrlAsync(StyleUrl)
                : map.SetStyleJsonAsync(styleJson);
            style.ReportFailure("style load");
            map.UpdateCameraAsync(
                    CameraUpdate.Default with
                    {
                        Mode = CameraUpdateMode.Jump,
                        Camera = new CameraOptions
                        {
                            Center = new LatLng(37.7749, -122.4194),
                            Zoom = 13.0,
                            Bearing = 12.0,
                            Pitch = 30.0,
                        },
                    }
                )
                .ReportFailure("initial camera");
            return new MapState(runtime, map);
        }
        catch
        {
            map?.Dispose();
            runtime.Dispose();
            throw;
        }
    }

    public void CancelTransitions()
    {
        Map.CancelTransitionsAsync().ReportFailure("camera transition cancel");
    }

    public void SetGestureInProgress(bool inProgress)
    {
        Map.UpdateCameraAsync(
                CameraUpdate.Default with
                {
                    GesturePhase = inProgress ? GesturePhase.Begin : GesturePhase.End,
                }
            )
            .ReportFailure("gesture update");
    }

    public void MoveBy(double deltaX, double deltaY, AnimationOptions? animation = null)
    {
        ApplyDelta(
            new CameraDelta
            {
                Offset = new ScreenPoint(deltaX, deltaY),
                Animation = animation ?? new AnimationOptions(),
            }
        );
    }

    public void ScaleBy(double scale, ScreenPoint? anchor, AnimationOptions? animation = null)
    {
        ApplyDelta(
            new CameraDelta
            {
                Scale = scale,
                Anchor = anchor,
                Animation = animation ?? new AnimationOptions(),
            }
        );
    }

    public void AdjustOrientation(
        double? bearing = null,
        double? pitch = null,
        AnimationOptions? animation = null
    )
    {
        ApplyDelta(
            new CameraDelta
            {
                Bearing = bearing,
                Pitch = pitch,
                Animation = animation ?? new AnimationOptions(),
            }
        );
    }

    public void ResetOrientation(AnimationOptions animation)
    {
        Update(new CameraOptions { Bearing = 0, Pitch = 0 }, animation);
    }

    public void Dispose()
    {
        if (closed)
        {
            return;
        }
        closed = true;
        // The shell must terminate GLFW on this thread after the runtime stops posting wakes, so
        // shutdown blocks here until both teardowns finish.
        try
        {
            Map.CloseAsync().GetAwaiter().GetResult();
        }
        finally
        {
            runtime.CloseAsync().GetAwaiter().GetResult();
        }
    }

    private void ApplyDelta(CameraDelta delta) =>
        Map.ApplyCameraDeltaAsync(delta).ReportFailure("camera delta");

    private void Update(CameraOptions camera, AnimationOptions? animation)
    {
        Map.UpdateCameraAsync(
                CameraUpdate.Default with
                {
                    Mode = animation is null ? CameraUpdateMode.Jump : CameraUpdateMode.Ease,
                    Camera = camera,
                    Animation = animation ?? new AnimationOptions(),
                }
            )
            .ReportFailure("camera update");
    }
}
