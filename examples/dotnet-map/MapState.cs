using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>Autonomous runtime and any-thread map state.</summary>
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

    public static MapState Create(Viewport viewport)
    {
        var runtime = RuntimeHandle.Create(new RuntimeOptions { CachePath = ":memory:" });
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
        _ = Map.UpdateCamera(new CameraUpdate());
    }

    public void SetGestureInProgress(bool inProgress)
    {
        _ = Map.UpdateCamera(
            new CameraUpdate { GesturePhase = inProgress ? GesturePhase.Begin : GesturePhase.End }
        );
    }

    public void MoveBy(double deltaX, double deltaY, AnimationOptions? animation = null)
    {
        _ = Map.ApplyCameraDelta(
            new CameraDelta
            {
                Offset = new ScreenPoint(deltaX, deltaY),
                Animation = animation ?? new AnimationOptions(),
            }
        );
    }

    public void ScaleBy(double scale, ScreenPoint? anchor, AnimationOptions? animation = null)
    {
        _ = Map.ApplyCameraDelta(
            new CameraDelta
            {
                Kind = CameraDeltaKind.Scale,
                Amount = scale,
                Anchor = anchor,
                Animation = animation ?? new AnimationOptions(),
            }
        );
    }

    public void AdjustBearing(double delta, AnimationOptions? animation = null)
    {
        _ = Map.ApplyCameraDelta(
            new CameraDelta
            {
                Kind = CameraDeltaKind.Bearing,
                Amount = delta,
                Animation = animation ?? new AnimationOptions(),
            }
        );
    }

    public void AdjustPitch(double delta, AnimationOptions? animation = null)
    {
        _ = Map.ApplyCameraDelta(
            new CameraDelta
            {
                Kind = CameraDeltaKind.Pitch,
                Amount = delta,
                Animation = animation ?? new AnimationOptions(),
            }
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
            Map.Close();
        }
        finally
        {
            runtime.Close();
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
