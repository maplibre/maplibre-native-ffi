using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>Autonomous runtime and any-thread map state.</summary>
internal sealed class MapState : IDisposable
{
    private const string StyleUrl = "https://tiles.openfreemap.org/styles/bright";
    private const double MinimumPitch = 0.0;
    private const double MaximumPitch = 60.0;

    private readonly RuntimeHandle runtime;
    private bool closed;
    private ulong gestureId;

    private MapState(RuntimeHandle runtime, MapHandle map)
    {
        this.runtime = runtime;
        Map = map;
    }

    public MapHandle Map { get; }

    public static MapState Create(Viewport viewport)
    {
        var runtime = RuntimeHandle
            .CreateAsync(new RuntimeOptions { CachePath = ":memory:" })
            .GetAwaiter()
            .GetResult();
        MapHandle? map = null;
        try
        {
            map = MapHandle
                .CreateAsync(
                    runtime,
                    new MapOptions
                    {
                        Width = viewport.LogicalWidth,
                        Height = viewport.LogicalHeight,
                        ScaleFactor = viewport.ScaleFactor,
                        MapMode = MapMode.Continuous,
                        EventMask =
                            RuntimeEventMask.MapRenderUpdateAvailable
                            | RuntimeEventMask.MapRenderFrameFinished,
                    }
                )
                .GetAwaiter()
                .GetResult();
            map.SetStyleUrl(StyleUrl);
            map.UpdateCamera(
                new CameraUpdate
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
            );
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
        var current = Map.GetCameraSnapshot().Camera;
        Update(current, null);
    }

    public void SetGestureInProgress(bool inProgress)
    {
        var current = Map.GetCameraSnapshot().Camera;
        UpdateGesture(current, inProgress);
    }

    public void MoveBy(double deltaX, double deltaY, AnimationOptions? animation = null)
    {
        var current = Map.GetCameraSnapshot().Camera;
        Update(MovedCamera(current, deltaX, deltaY), animation);
    }

    public void ScaleBy(double scale, ScreenPoint? anchor, AnimationOptions? animation = null)
    {
        var current = Map.GetCameraSnapshot().Camera;
        Update(
            new CameraOptions { Zoom = (current.Zoom ?? 0) + Math.Log2(scale), Anchor = anchor },
            animation
        );
    }

    public void AdjustBearing(double delta, AnimationOptions? animation = null)
    {
        var current = Map.GetCameraSnapshot().Camera;
        Update(new CameraOptions { Bearing = (current.Bearing ?? 0) + delta }, animation);
    }

    public void AdjustPitch(double delta, AnimationOptions? animation = null)
    {
        var current = Map.GetCameraSnapshot().Camera;
        Update(
            new CameraOptions
            {
                Pitch = Math.Clamp((current.Pitch ?? 0) + delta, MinimumPitch, MaximumPitch),
            },
            animation
        );
    }

    public void ResetOrientation(AnimationOptions animation)
    {
        Update(new CameraOptions { Bearing = 0, Pitch = 0 }, animation);
    }

    public bool DrainRenderRequests()
    {
        var requested = false;
        foreach (var runtimeEvent in runtime.DrainEvents().Events)
        {
            if (!ReferenceEquals(runtimeEvent.MapSource, Map))
            {
                continue;
            }
            if (
                runtimeEvent.Type == RuntimeEventType.MapRenderUpdateAvailable
                || runtimeEvent.Type == RuntimeEventType.MapRenderFrameFinished
                    && runtimeEvent.Payload
                        is RuntimeEventPayload.RenderFrame { NeedsRepaint: true }
            )
            {
                requested = true;
            }
        }
        return requested;
    }

    public void Dispose()
    {
        if (closed)
        {
            return;
        }
        closed = true;
        try
        {
            Map.CloseAsync().GetAwaiter().GetResult();
        }
        finally
        {
            runtime.CloseAsync().GetAwaiter().GetResult();
        }
    }

    private void Update(CameraOptions camera, AnimationOptions? animation)
    {
        _ = Map.UpdateCamera(
            new CameraUpdate
            {
                Mode = animation is null ? CameraUpdateMode.Jump : CameraUpdateMode.Ease,
                Camera = camera,
                Animation = animation ?? new AnimationOptions(),
            }
        );
    }

    private void UpdateGesture(CameraOptions current, bool begin)
    {
        if (begin)
        {
            gestureId++;
        }
        _ = Map.UpdateCamera(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Jump,
                Camera = current,
                GesturePhase = begin ? GesturePhase.Begin : GesturePhase.End,
                GestureId = gestureId,
            }
        );
    }

    private static CameraOptions MovedCamera(CameraOptions current, double deltaX, double deltaY)
    {
        var center = current.Center ?? new LatLng(0, 0);
        var zoom = current.Zoom ?? 0;
        var degreesPerPixel = 360.0 / (512.0 * Math.Pow(2, zoom));
        return new CameraOptions
        {
            Center = new LatLng(
                Math.Clamp(center.Latitude + deltaY * degreesPerPixel, -85, 85),
                center.Longitude - deltaX * degreesPerPixel
            ),
        };
    }
}

/// <summary>One-bit signal that a frame is worth drawing.</summary>
internal sealed class RenderRequest
{
    private bool requested = true;

    public void Set()
    {
        requested = true;
    }

    public bool Consume()
    {
        var current = requested;
        requested = false;
        return current;
    }
}
