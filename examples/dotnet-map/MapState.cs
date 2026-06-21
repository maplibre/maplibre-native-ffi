using Maplibre.Native.Camera;
using Maplibre.Native.Error;
using Maplibre.Native.Geo;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class MapState : IDisposable
{
    private const string StyleUrl = "https://tiles.openfreemap.org/styles/bright";

    private readonly RuntimeHandle runtime;
    private readonly IGraphicsContext graphics;
    private readonly RenderTargetMode renderTargetMode;
    private IRenderTarget? renderTarget;
    private bool closed;

    private MapState(
        RuntimeHandle runtime,
        MapHandle map,
        IGraphicsContext graphics,
        RenderTargetMode renderTargetMode,
        IRenderTarget renderTarget
    )
    {
        this.runtime = runtime;
        Map = map;
        this.graphics = graphics;
        this.renderTargetMode = renderTargetMode;
        this.renderTarget = renderTarget;
        RenderPending = true;
    }

    public MapHandle Map { get; }

    public bool RenderPending { get; private set; }

    public static MapState Create(
        IGraphicsContext graphics,
        Viewport viewport,
        RenderTargetMode renderTargetMode
    )
    {
        ArgumentNullException.ThrowIfNull(graphics);
        var runtime = RuntimeHandle.Create(new RuntimeOptions { CachePath = ":memory:" });
        var map = MapHandle.Create(
            runtime,
            new MapOptions
            {
                Width = viewport.LogicalWidth,
                Height = viewport.LogicalHeight,
                ScaleFactor = viewport.ScaleFactor,
                MapMode = MapMode.Continuous,
            }
        );
        IRenderTarget? renderTarget = null;
        try
        {
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
            renderTarget = RenderTargetFactory.Attach(graphics, map, renderTargetMode);
            return new MapState(runtime, map, graphics, renderTargetMode, renderTarget);
        }
        catch
        {
            renderTarget?.Dispose();
            map.Dispose();
            runtime.Dispose();
            throw;
        }
    }

    public void RequestRender()
    {
        RenderPending = true;
    }

    public bool Step()
    {
        return Step(canRender: true);
    }

    public bool Step(bool canRender)
    {
        runtime.RunOnce();
        DrainEvents();
        if (!canRender || !RenderPending)
        {
            return false;
        }

        try
        {
            if (graphics is MetalContext)
            {
                using var pool = MacObjectiveC.AutoreleasePool();
                if (!CurrentRenderTarget().Render())
                {
                    return true;
                }
            }
            else
            {
                if (!CurrentRenderTarget().Render())
                {
                    return true;
                }
            }
            RenderPending = false;
        }
        catch (InvalidStateException)
        {
            RenderPending = true;
        }

        return true;
    }

    public void DrainEvents()
    {
        while (runtime.PollEvent() is { } runtimeEvent)
        {
            if (!ReferenceEquals(runtimeEvent.MapSource, Map))
            {
                continue;
            }

            if (runtimeEvent.Type == RuntimeEventType.MapRenderUpdateAvailable)
            {
                RenderPending = true;
            }
            else if (
                runtimeEvent.Type == RuntimeEventType.MapRenderFrameFinished
                && runtimeEvent.Payload is RuntimeEventPayload.RenderFrame { NeedsRepaint: true }
            )
            {
                RenderPending = true;
            }
        }
    }

    public void Resize(Viewport viewport)
    {
        graphics.Resize(viewport);
        var currentRenderTarget = CurrentRenderTarget();
        if (currentRenderTarget.NeedsReattachOnResize)
        {
            renderTarget = null;
            currentRenderTarget.Dispose();
            renderTarget = RenderTargetFactory.Attach(graphics, Map, renderTargetMode);
        }
        else
        {
            currentRenderTarget.Resize(viewport);
        }

        RenderPending = true;
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
            renderTarget?.Dispose();
        }
        finally
        {
            try
            {
                Map.Dispose();
            }
            finally
            {
                runtime.Dispose();
            }
        }
    }

    private IRenderTarget CurrentRenderTarget()
    {
        return renderTarget
            ?? throw new InvalidOperationException("The render target is not attached.");
    }
}
