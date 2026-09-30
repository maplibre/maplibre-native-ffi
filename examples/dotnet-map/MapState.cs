using Maplibre.NativeFfi.Camera;
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

    /// <summary>
    /// Creates the map with the example's style, or with <paramref name="styleJson" /> in its
    /// place, which the smoke run passes so that it needs no network.
    /// </summary>
    public static MapState Create(Viewport viewport, byte[]? styleJson = null)
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default with { CachePath = ":memory:" });
        MapHandle? map = null;
        try
        {
            map = runtime
                .MapCreateAsync(
                    MapOptions.Default with
                    {
                        InitialExtent = new LogicalExtent(
                            viewport.LogicalWidth,
                            viewport.LogicalHeight,
                            viewport.ScaleFactor
                        ),
                        MapMode = MapMode.Continuous,
                        EventMask = RuntimeEventMask.MapRenderUpdateAvailable,
                    }
                )
                .GetAwaiter()
                .GetResult();
            var style = styleJson is null
                ? map.SetStyleUrlAsync(StyleUrl)
                : map.SetStyleJsonAsync(styleJson);
            style.GetAwaiter().GetResult();
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
                .GetAwaiter()
                .GetResult();
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
        _ = Map.CancelTransitionsAsync();
    }

    public void SetGestureInProgress(bool inProgress)
    {
        _ = Map.UpdateCameraAsync(
            CameraUpdate.Default with
            {
                GesturePhase = inProgress ? GesturePhase.Begin : GesturePhase.End,
            }
        );
    }

    public void MoveBy(double deltaX, double deltaY, AnimationOptions? animation = null)
    {
        _ = Map.ApplyCameraDeltaAsync(
            new CameraDelta
            {
                Offset = new ScreenPoint(deltaX, deltaY),
                Animation = animation ?? new AnimationOptions(),
            }
        );
    }

    public void ScaleBy(double scale, ScreenPoint? anchor, AnimationOptions? animation = null)
    {
        _ = Map.ApplyCameraDeltaAsync(
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
        _ = Map.ApplyCameraDeltaAsync(
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
        _ = Map.ApplyCameraDeltaAsync(
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
        using var batch = runtime.DrainEvents();
        foreach (var runtimeEvent in batch.Get().Events)
        {
            if (runtimeEvent.Type == RuntimeEventType.MapRenderUpdateAvailable)
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
        _ = Map.UpdateCameraAsync(
            CameraUpdate.Default with
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
