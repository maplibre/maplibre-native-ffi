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

    /// <summary>Submits a copied any-thread camera command and returns its native command id.</summary>
    public ulong Apply(CameraCommand command)
    {
        var current = Map.GetCameraSnapshot().Camera;
        return command switch
        {
            CancelTransitionsCommand => Submit(current, null),
            SetGestureInProgressCommand gesture => SubmitGesture(current, gesture.InProgress),
            MoveByCommand move => Submit(MovedCamera(current, move), move.Animation),
            ScaleByCommand zoom => Submit(
                new CameraOptions
                {
                    Zoom = (current.Zoom ?? 0) + Math.Log2(zoom.Scale),
                    Anchor = zoom.Anchor,
                },
                zoom.Animation
            ),
            AdjustBearingCommand bearing => Submit(
                new CameraOptions { Bearing = (current.Bearing ?? 0) + bearing.Delta },
                bearing.Animation
            ),
            AdjustPitchCommand pitch => Submit(
                new CameraOptions
                {
                    Pitch = Math.Clamp(
                        (current.Pitch ?? 0) + pitch.Delta,
                        MinimumPitch,
                        MaximumPitch
                    ),
                },
                pitch.Animation
            ),
            ResetOrientationCommand reset => Submit(
                new CameraOptions { Bearing = 0, Pitch = 0 },
                reset.Animation
            ),
            _ => throw new ArgumentOutOfRangeException(nameof(command)),
        };
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

    private ulong Submit(CameraOptions camera, AnimationOptions? animation)
    {
        return Map.UpdateCamera(
            new CameraUpdate
            {
                Mode = animation is null ? CameraUpdateMode.Jump : CameraUpdateMode.Ease,
                Camera = camera,
                Animation = animation ?? new AnimationOptions(),
            }
        );
    }

    private ulong SubmitGesture(CameraOptions current, bool begin)
    {
        if (begin)
        {
            gestureId++;
        }
        return Map.UpdateCamera(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Jump,
                Camera = current,
                GesturePhase = begin ? GesturePhase.Begin : GesturePhase.End,
                GestureId = gestureId,
            }
        );
    }

    private CameraOptions MovedCamera(CameraOptions current, MoveByCommand move)
    {
        var center = current.Center ?? new LatLng(0, 0);
        var zoom = current.Zoom ?? 0;
        var degreesPerPixel = 360.0 / (512.0 * Math.Pow(2, zoom));
        return new CameraOptions
        {
            Center = new LatLng(
                Math.Clamp(center.Latitude + move.DeltaY * degreesPerPixel, -85, 85),
                center.Longitude - move.DeltaX * degreesPerPixel
            ),
        };
    }
}
