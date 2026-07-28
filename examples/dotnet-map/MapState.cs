using Maplibre.Native.Camera;
using Maplibre.Native.Geo;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;

namespace Maplibre.Native.Examples.DotnetMap;

/// <summary>Runtime and map, owned for their whole lifetime by the runtime loop thread.</summary>
/// <remarks>
/// The render target is not here: it belongs to the render loop thread, which owns the window and
/// the graphics context.
/// </remarks>
internal sealed class MapState : IDisposable
{
    private const string StyleUrl = "https://tiles.openfreemap.org/styles/bright";
    private const double MinimumPitch = 0.0;
    private const double MaximumPitch = 60.0;

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
            map = MapHandle.Create(
                runtime,
                new MapOptions
                {
                    Width = viewport.LogicalWidth,
                    Height = viewport.LogicalHeight,
                    ScaleFactor = viewport.ScaleFactor,
                    MapMode = MapMode.Continuous,
                }
            );
            map.SetStyleUrl(StyleUrl);
            map.JumpTo(
                new CameraOptions
                {
                    Center = new LatLng(37.7749, -122.4194),
                    Zoom = 13.0,
                    Bearing = 12.0,
                    Pitch = 30.0,
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

    /// <summary>Applies every queued camera command on the map's owner thread.</summary>
    public void ApplyCommands(CommandQueue commands)
    {
        ArgumentNullException.ThrowIfNull(commands);
        while (commands.TryDequeue(out var command))
        {
            Apply(command);
        }
    }

    /// <summary>Pumps the runtime once, reporting whether the map wants another frame.</summary>
    public bool Step()
    {
        runtime.RunOnce();
        return DrainEvents();
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
            Map.Dispose();
        }
        finally
        {
            runtime.Dispose();
        }
    }

    private bool DrainEvents()
    {
        var renderUpdateAvailable = false;
        while (runtime.PollEvent() is { } runtimeEvent)
        {
            if (!ReferenceEquals(runtimeEvent.MapSource, Map))
            {
                continue;
            }

            if (
                runtimeEvent.Type == RuntimeEventType.MapRenderUpdateAvailable
                || (
                    runtimeEvent.Type == RuntimeEventType.MapRenderFrameFinished
                    && runtimeEvent.Payload
                        is RuntimeEventPayload.RenderFrame { NeedsRepaint: true }
                )
            )
            {
                renderUpdateAvailable = true;
            }
        }

        return renderUpdateAvailable;
    }

    /// <summary>
    /// Applies one decoded camera command. Runs on the map's owner thread, which is why the
    /// read-modify-write commands read the current camera here rather than on the render loop that
    /// produced them.
    /// </summary>
    private void Apply(CameraCommand command)
    {
        switch (command)
        {
            case CancelTransitionsCommand:
                Map.CancelTransitions();
                break;
            case MoveByCommand { Animation: null } move:
                Map.MoveBy(move.DeltaX, move.DeltaY);
                break;
            case MoveByCommand move:
                Map.MoveByAnimated(move.DeltaX, move.DeltaY, move.Animation);
                break;
            case ScaleByCommand { Animation: null } zoom:
                Map.ScaleBy(zoom.Scale, zoom.Anchor);
                break;
            case ScaleByCommand zoom:
                Map.ScaleByAnimated(zoom.Scale, zoom.Anchor, zoom.Animation);
                break;
            case AdjustBearingCommand bearing:
                ApplyCamera(
                    new CameraOptions { Bearing = CurrentBearing() + bearing.Delta },
                    bearing.Animation
                );
                break;
            case AdjustPitchCommand pitch:
                ApplyCamera(
                    new CameraOptions
                    {
                        Pitch = Math.Clamp(
                            CurrentPitch() + pitch.Delta,
                            MinimumPitch,
                            MaximumPitch
                        ),
                    },
                    pitch.Animation
                );
                break;
            case ResetOrientationCommand reset:
                Map.EaseTo(new CameraOptions { Bearing = 0.0, Pitch = 0.0 }, reset.Animation);
                break;
            default:
                throw new ArgumentOutOfRangeException(nameof(command));
        }
    }

    private void ApplyCamera(CameraOptions camera, AnimationOptions? animation)
    {
        if (animation is null)
        {
            Map.JumpTo(camera);
        }
        else
        {
            Map.EaseTo(camera, animation);
        }
    }

    private double CurrentBearing()
    {
        return Map.GetCamera().Bearing ?? 0.0;
    }

    private double CurrentPitch()
    {
        return Map.GetCamera().Pitch ?? 0.0;
    }
}
