using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

/// <summary>App shell: toolkit lifetime, the two loops, and shutdown ordering.</summary>
internal static class Shell
{
    public const int InitialWidth = 960;
    public const int InitialHeight = 640;

    // TODO(map-example-spec): Replace the fixed interval with a display-paced host loop. See Frame loop.
    private static readonly TimeSpan RenderLoopInterval = TimeSpan.FromMilliseconds(8);
    private static readonly TimeSpan RuntimeLoopInterval = TimeSpan.FromMilliseconds(4);

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
        using var commands = new CommandQueue();
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
        try
        {
            state = MapState.Create(initialViewport);
            channel.PublishMap(state.Map);

            while (!channel.ShutdownRequested)
            {
                state.ApplyCommands(commands);
                if (state.Step())
                {
                    renderRequest.Set();
                }

                // RunOnce never blocks waiting for work, so pace the loop instead of spinning on
                // it, waking early whenever the render loop queues a camera command.
                commands.Wait(RuntimeLoopInterval);
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
                        if (target.NeedsReattachOnResize)
                        {
                            // Reattach is local to this thread now: close the session, rebuild the
                            // target, attach again.
                            var previous = target;
                            target = null;
                            previous.Dispose();
                            target = RenderTargetFactory.Attach(graphics, map, mode);
                        }
                        else
                        {
                            target.Resize(viewport);
                        }

                        renderRequest.Set();
                    }
                }

                // Consume before rendering, so a request the runtime loop publishes during the
                // render call is not discarded.
                if (graphics.CanRenderFrame && renderRequest.Consume() && !Render(graphics, target))
                {
                    // The map applies its logical size on the runtime loop's next RunOnce, so no
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
