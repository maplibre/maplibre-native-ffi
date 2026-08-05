using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>App shell: toolkit lifetime, the two loops, and shutdown ordering.</summary>
internal static class Shell
{
    public const int InitialWidth = 960;
    public const int InitialHeight = 640;

    // TODO(map-example-spec): Replace the fixed interval with a display-paced host loop. See Frame loop.
    private static readonly TimeSpan RenderLoopInterval = TimeSpan.FromMilliseconds(8);

    /// <summary>
    /// Backstop for the runtime loop's park; the render loop's wake source normally releases it.
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

        // A dedicated thread, because the native owner-thread checks are keyed on the OS thread;
        // thread pools and async continuations do not guarantee that affinity.
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
            // Only here: the render loop has closed its session by the time it returns, and a map
            // with an attached session cannot be destroyed.
            channel.RequestShutdown();
            runtimeThread.Join();
        }

        channel.ThrowIfFailed();
    }

    /// <summary>
    /// Owns the runtime and the map for their whole lifetime. It never touches the render session:
    /// the render loop attaches its own against the map published here.
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
            // Wait even after a failure: the render loop closes its session before signalling
            // shutdown, and the map cannot be destroyed until then.
            channel.WaitForShutdown();
            // Stop handing out the wake source before disposing it, or the channel signals a closed
            // handle and throws over the original failure.
            commands.OnEnqueue = null;
            channel.ClearWake();
            wake?.Dispose();
            state?.Dispose();
        }
    }

    /// <summary>
    /// Owns the window, input decoding, the graphics context, and the render session it attaches.
    /// </summary>
    private static void RenderLoop(
        IGraphicsContext graphics,
        RenderTargetMode mode,
        CommandQueue commands,
        RenderRequest renderRequest,
        MapChannel channel
    )
    {
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
                        // Every mode resizes against the live session; none needs a re-attach.
                        target.Resize(viewport);
                        renderRequest.Set();
                    }
                }

                // Consume before rendering, so a request the runtime loop publishes during the
                // render call is not discarded.
                if (graphics.CanRenderFrame && renderRequest.Consume() && !Render(graphics, target))
                {
                    // Nothing reached the screen; ask again rather than dropping the frame.
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
