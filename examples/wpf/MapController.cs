// A continuous map on the MapView's window. The controller attaches a session to
// the first window, and gives the session each later one. Everything else
// matches the Compose example in examples/compose and the browser example in
// examples/browser.
using System.Diagnostics;
using System.Windows.Media;
using System.Windows.Threading;

namespace Maplibre.NativeFfi.Example.Wpf;

internal readonly record struct Status(string Message, bool Error = false);

// Every method runs on the UI thread. The binding calls the wakes and the event
// handler on its own threads, and they post their work here.
internal sealed class MapController(Dispatcher dispatcher) : IAsyncDisposable
{
    private const string StyleUrl = "https://tiles.openfreemap.org/styles/liberty";

    private RuntimeHandle? runtime;
    private RenderSessionHandle? session;
    private bool failed;

    // The view's latest window and its client size in pixels, and the window
    // that the session presents to.
    private nint window;
    private (int Width, int Height)? pixels;
    private nint target;

    // The size that the session last got.
    private LogicalSize? targetSize;

    // Target changes run one at a time, in the order the view reports them.
    private Task targets = Task.CompletedTask;

    // The session's release, once the controller is disposed.
    private Task? released;
    private bool frameRequested;

    // The WPF frame that the session last got a demand in.
    private TimeSpan demandedFrame;

    public MapHandle? Map { get; private set; }

    // The map's scale factor, which it keeps for its life.
    public double Scale { get; private set; } = 1;

    public Status Status { get; private set; } = new("");

    public event Action<Status>? StatusChanged;

    public async void Start(double scale)
    {
        Scale = scale;
        try
        {
            await StartAsync();
        }
        catch (Exception error)
        {
            Fail(error);
        }
    }

    public void WindowCreated(nint window)
    {
        this.window = window;
        Retarget();
    }

    public void WindowResized(int width, int height)
    {
        // A collapsed view has no client area to present to.
        if (width <= 0 || height <= 0)
            return;
        pixels = (width, height);
        if (target != window)
            Retarget();
        // While a target change runs, it applies the new size when it finishes.
        else if (targets.IsCompleted)
            Resize();
    }

    // The view is destroying the window. The session must let go of it first,
    // so this blocks until the release completes. That can't deadlock: the
    // session's worker thread completes the release, and it never waits for the
    // UI thread. The session keeps its tiles for the next window.
    public void WindowDestroying(nint window)
    {
        if (this.window == window)
            this.window = 0;
        if (target != window)
            return;
        target = 0;
        // Once disposed, the controller's release of the session also lets go
        // of the window.
        if (released is not null)
        {
            WaitFailure(released);
            return;
        }
        if (session is null)
            return;
        // A session that is still attaching can't release its target yet. One
        // that failed to attach has no target.
        if (WaitFailure(session.Ready) is not null)
            return;
        // The session failed, and the frame results report it.
        if (WaitFailure(session.ReleaseTargetAsync()) is { } failure)
            Trace.TraceWarning($"Couldn't release the window: {failure}");
    }

    public async ValueTask DisposeAsync()
    {
        var session = this.session;
        var map = Map;
        var runtime = this.runtime;
        this.session = null;
        Map = null;
        this.runtime = null;
        if (frameRequested)
        {
            CompositionTarget.Rendering -= OnRendering;
            frameRequested = false;
        }
        try
        {
            released = session?.ReleaseAsync() ?? Task.CompletedTask;
            await released;
            if (map is not null)
                await map.ReleaseAsync();
            if (runtime is not null)
                await runtime.ReleaseAsync();
        }
        catch (MaplibreException error)
        {
            Trace.TraceWarning($"Couldn't release the map: {error}");
        }
    }

    private async Task StartAsync()
    {
        var runtime = RuntimeHandle.Create();
        this.runtime = runtime;
        var size = LogicalSize() ?? new LogicalSize(1, 1);
        var map = runtime.CreateMap(
            new MapOptions
            {
                InitialExtent = new LogicalExtent(size.Width, size.Height, Scale),
                MapMode = MapMode.Continuous,
                EventMask =
                    RuntimeEventFlag.MapStyleLoaded
                    | RuntimeEventFlag.MapLoadingFailed
                    | RuntimeEventFlag.MapRenderError,
            }
        );
        Map = map;
        runtime.Discarding.SetEventHandler(
            new RuntimeEventHandler(events => dispatcher.InvokeAsync(() => OnEvents(map, events)))
        );
        await map.Ready;
        // The window closed while the map started.
        if (Map != map)
            return;
        map.Discarding.UpdateCamera(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Jump,
                Camera = new CameraOptions
                {
                    Center = new LatLng(48.8584, 2.2945),
                    Zoom = 15.5,
                    Bearing = -30,
                    Pitch = 55,
                },
            }
        );
        map.Discarding.SetStyleUrl(StyleUrl);
        ShowStatus("Loading the style…");
        Retarget();
    }

    private void OnEvents(MapHandle map, RuntimeEvent[] events)
    {
        foreach (var e in events)
        {
            if (e.Source != map.Id)
                continue;
            switch (e.Type)
            {
                case RuntimeEventType.MapStyleLoaded:
                    ShowStatus("");
                    break;
                case RuntimeEventType.MapLoadingFailed:
                case RuntimeEventType.MapRenderError:
                    Fail(new MapException(e.Message));
                    break;
            }
        }
    }

    // Gives the session the view's latest window, attaching the session first
    // if there is none.
    private void Retarget() => targets = RetargetAfter(targets);

    private async Task RetargetAfter(Task previous)
    {
        await previous;
        try
        {
            await ApplyTarget();
            Resize();
        }
        catch (Exception error)
        {
            Fail(error);
        }
    }

    private async Task ApplyTarget()
    {
        // The view can report its window before the map is ready to take a
        // session.
        var map = Map;
        if (map is null)
            return;
        await map.Ready;
        var window = this.window;
        if (window == 0 || LogicalSize() is not { } size || window == target)
            return;
        target = window;
        var pointer = NativePointer.FromBorrowedAddress(window);
        var session = this.session;
        if (session is null)
        {
            session = map.Attach(
                new RenderTarget.Win32Window(new Win32WindowTarget(size, pointer)),
                device: null,
                new RenderSessionAttachOptions
                {
                    Driver = RenderDriverKind.CoreWorker,
                    RedrawWake = new Wake(() => dispatcher.InvokeAsync(RequestFrame)),
                    FrameWake = new Wake(() => dispatcher.InvokeAsync(DrainFrameResults)),
                }
            );
            this.session = session;
            await session.Ready;
        }
        else
        {
            await session.SetTargetAsync(
                new RenderTarget.Win32Window(new Win32WindowTarget(size, pointer))
            );
        }
        targetSize = size;
        // The session may have asked for frames while it attached.
        RequestFrame();
    }

    // Gives the session the view's latest size.
    private void Resize()
    {
        if (session is null || target == 0 || LogicalSize() is not { } size || size == targetSize)
            return;
        targetSize = size;
        try
        {
            session.Discarding.Resize(new LogicalExtent(size.Width, size.Height, Scale));
        }
        catch (MaplibreException error)
        {
            Fail(error);
        }
    }

    // Demands a frame at WPF's next frame, which the desktop compositor paces to
    // the display. WPF renders frames only while something subscribes, so an
    // idle map costs nothing.
    private void RequestFrame()
    {
        if (frameRequested || session is not { Ready.IsCompletedSuccessfully: true })
            return;
        frameRequested = true;
        CompositionTarget.Rendering += OnRendering;
    }

    private void OnRendering(object? sender, EventArgs e)
    {
        // WPF can raise the event more than once in a frame, so a demand waits
        // for a frame that had none.
        var frame = ((RenderingEventArgs)e).RenderingTime;
        if (frame == demandedFrame)
            return;
        demandedFrame = frame;
        CompositionTarget.Rendering -= OnRendering;
        frameRequested = false;
        try
        {
            session?.DemandFrame(new FrameDemand());
        }
        catch (MaplibreException error)
        {
            Fail(error);
        }
    }

    private void DrainFrameResults()
    {
        if (session is null)
            return;
        var results = session.DrainFrameResults();
        if (results.Any(result => result.Disposition == RenderResult.Failed))
            Fail(new MapException($"The map failed to render a frame: {session.GetFailure()}"));
    }

    private LogicalSize? LogicalSize() =>
        pixels is var (width, height)
            ? new LogicalSize(
                (uint)Math.Max(1, Math.Round(width / Scale)),
                (uint)Math.Max(1, Math.Round(height / Scale))
            )
            : null;

    // Once the controller shows an error, later progress doesn't replace it.
    private void ShowStatus(string message)
    {
        if (failed)
            return;
        Status = new Status(message);
        StatusChanged?.Invoke(Status);
    }

    private void Fail(Exception error)
    {
        Trace.TraceError($"The map failed: {error}");
        if (failed)
            return;
        failed = true;
        Status = new Status(error.Message, Error: true);
        StatusChanged?.Invoke(Status);
    }

    // Blocks the calling thread until the task completes, and returns its
    // failure.
    private static Exception? WaitFailure(Task task)
    {
        try
        {
            task.GetAwaiter().GetResult();
            return null;
        }
        catch (Exception error)
        {
            return error;
        }
    }
}

internal sealed class MapException(string message) : Exception(message);
