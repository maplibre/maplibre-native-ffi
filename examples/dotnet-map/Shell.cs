using Maplibre.Native.Render;
using Maplibre.Native.Runtime;

namespace Maplibre.Native.Examples.DotnetMap;

/// <summary>App shell: toolkit lifetime, the two loops, and shutdown ordering.</summary>
internal static class Shell
{
    public const int InitialWidth = 960;
    public const int InitialHeight = 640;

    // TODO(map-example-spec): Replace the fixed interval with a display-paced host loop. See Frame loop.
    private static readonly TimeSpan RenderLoopInterval = TimeSpan.FromMilliseconds(8);

    /// <summary>
    /// Backstop for the runtime loop's park. The render loop's wake source is what normally
    /// releases it, so this only bounds a pump that nothing signals.
    /// </summary>
    private static readonly TimeSpan ParkTimeout = TimeSpan.FromMilliseconds(100);

    public static void Run(RenderTargetMode mode, RenderBackend backends)
    {
        // GLFW creates windows and polls events on the main thread only, so the main thread is the
        // render loop and the runtime loop gets a thread of its own.
        using var graphics = GraphicsContext.Create(
            "dotnet-map",
            InitialWidth,
            InitialHeight,
            backends
        );
        var commands = new CommandQueue();
        using var channel = new MapChannel();
        var renderRequest = new RenderRequest();
        var initialViewport = graphics.ReadViewport();

        // A dedicated thread, because the native owner-thread checks are keyed on the OS thread.
        // Thread pools and async continuations do not guarantee that affinity.
        var runtimeThread = new Thread(() =>
            RuntimeLoop(initialViewport, commands, renderRequest, channel)
        )
        {
            IsBackground = true,
            Name = "maplibre-runtime-loop",
        };
        runtimeThread.Start();

        try
        {
            RenderLoop(graphics, mode, commands, renderRequest, channel);
        }
        finally
        {
            // The render loop has closed its session by the time it returns, and a map with an
            // attached session cannot be destroyed, so shutdown is only requested from here.
            channel.RequestShutdown();
            runtimeThread.Join();
        }

        channel.ThrowIfFailed();
    }

    /// <summary>
    /// Owns the runtime and the map for their whole lifetime, on a thread that is not the one
    /// presenting. It never touches the render session: the render loop attaches its own against
    /// the map published here.
    /// </summary>
    private static void RuntimeLoop(
        Viewport initialViewport,
        CommandQueue commands,
        RenderRequest renderRequest,
        MapChannel channel
    )
    {
        MapState? state = null;
        WakeSource? wake = null;
        try
        {
            state = MapState.Create(initialViewport);
            wake = state.AcquireWakeSource();
            channel.PublishMap(state.Map, wake);
            commands.OnEnqueue = channel.WakeRuntimeLoop;

            while (!channel.ShutdownRequested)
            {
                state.ApplyCommands(commands);
                if (state.Step(ParkTimeout))
                {
                    renderRequest.Set();
                }
            }
        }
        catch (Exception error)
        {
            channel.Fail(error);
        }
        finally
        {
            // Wait for the shutdown signal even after a failure: the render loop closes its
            // session before it sends that signal, and the map cannot be destroyed until then.
            channel.WaitForShutdown();
            // Stop handing the render loop this source before disposing it. Disposing first would
            // leave the channel signalling a closed handle, which throws over the original failure.
            commands.OnEnqueue = null;
            channel.ClearWake();
            wake?.Dispose();
            state?.Dispose();
        }
    }

    /// <summary>
    /// The display-paced render loop. Owns the window, input decoding, the graphics context, and
    /// the render session it attaches.
    /// </summary>
    private static void RenderLoop(
        IGraphicsContext graphics,
        RenderTargetMode mode,
        CommandQueue commands,
        RenderRequest renderRequest,
        MapChannel channel
    )
    {
        // The runtime loop creates the map; this loop attaches its own session against it and owns
        // that session for the rest of the run.
        var map = channel.WaitForMap();
        var viewport = graphics.ReadViewport();
        IRenderTarget? target = null;
        try
        {
            target = RenderTargetFactory.Attach(graphics, map, mode);
            Console.WriteLine($"render target: {mode.CliName}");
            Console.WriteLine($"render target status: {mode.Status}");
            InputController.PrintControls();
            using var input = new InputController(graphics.Window, commands, renderRequest);

            while (!graphics.ShouldClose)
            {
                channel.ThrowIfFailed();
                graphics.PollEvents();

                var currentViewport = graphics.ReadViewport();
                if (currentViewport != viewport)
                {
                    viewport = currentViewport;
                    if (!viewport.IsEmpty)
                    {
                        graphics.Resize(viewport);
                        // Every mode follows a resize without losing its session: the ones the
                        // session sizes resize in place, and a caller-owned texture hands the
                        // session a replacement. Closing and attaching again is reserved for a
                        // target the live session cannot take at all.
                        target.Resize(viewport);
                        renderRequest.Set();
                    }
                }

                // Consume before rendering, so a request the runtime loop publishes during the
                // render call is not discarded.
                if (graphics.CanRenderFrame && renderRequest.Consume() && !Render(graphics, target))
                {
                    // The map applies its logical size on the runtime loop's next Pump, so no
                    // update is rendered until then. Ask again rather than dropping the frame.
                    renderRequest.Set();
                }

                graphics.Window.WaitEventsTimeout(RenderLoopInterval.TotalSeconds);
            }
        }
        finally
        {
            // Close the session before the runtime loop destroys the map.
            target?.Dispose();
        }
    }

    private static bool Render(IGraphicsContext graphics, IRenderTarget target)
    {
        if (graphics is not MetalContext)
        {
            return target.Render();
        }

        using var pool = MacObjectiveC.AutoreleasePool();
        return target.Render();
    }
}
