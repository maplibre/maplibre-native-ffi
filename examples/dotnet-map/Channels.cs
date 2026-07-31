using System.Collections.Concurrent;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.ExceptionServices;
using Maplibre.Native.Camera;
using Maplibre.Native.Geo;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;

namespace Maplibre.Native.Examples.DotnetMap;

/// <summary>A camera change decoded on the render loop and applied on the map's owner thread.</summary>
/// <remarks>
/// Commands carry deltas rather than absolute targets wherever the map's current camera is an
/// input, because reading the camera and writing the new one has to happen together on the thread
/// that owns the map. A null animation means the command applies immediately.
/// </remarks>
internal abstract record CameraCommand;

internal sealed record CancelTransitionsCommand : CameraCommand;

internal sealed record SetGestureInProgressCommand(bool InProgress) : CameraCommand;

internal sealed record MoveByCommand(double DeltaX, double DeltaY, AnimationOptions? Animation)
    : CameraCommand;

internal sealed record ScaleByCommand(
    double Scale,
    ScreenPoint? Anchor,
    AnimationOptions? Animation
) : CameraCommand;

internal sealed record AdjustBearingCommand(double Delta, AnimationOptions? Animation)
    : CameraCommand;

internal sealed record AdjustPitchCommand(double Delta, AnimationOptions? Animation)
    : CameraCommand;

internal sealed record ResetOrientationCommand(AnimationOptions Animation) : CameraCommand;

/// <summary>Camera commands queued by the render loop for the runtime loop to apply.</summary>
internal sealed class CommandQueue
{
    private readonly ConcurrentQueue<CameraCommand> queue = new();

    /// <summary>
    /// Released by <see cref="Push" /> so a queued command reaches the runtime loop without waiting
    /// out its parking bound. The runtime loop parks inside the native pump rather than on a host
    /// event, so the native wake source is what releases it; that also wakes the loop for the
    /// runtime's own work, which a host event cannot see.
    /// </summary>
    public Action? OnEnqueue { get; set; }

    /// <summary>Render loop: queues a command and wakes the runtime loop.</summary>
    public void Push(CameraCommand command)
    {
        queue.Enqueue(command);
        OnEnqueue?.Invoke();
    }

    /// <summary>Runtime loop: takes the next queued command, if there is one.</summary>
    public bool TryDequeue([NotNullWhen(true)] out CameraCommand? command)
    {
        return queue.TryDequeue(out command);
    }
}

/// <summary>One-bit signal that a frame is worth drawing.</summary>
/// <remarks>
/// The render loop consumes before it renders and sets again when nothing was rendered, so a
/// request the runtime loop publishes during a render is not lost.
/// </remarks>
internal sealed class RenderRequest
{
    private int value = 1;

    public void Set()
    {
        Volatile.Write(ref value, 1);
    }

    /// <summary>Reads and clears the request in one step.</summary>
    public bool Consume()
    {
        return Interlocked.Exchange(ref value, 0) == 1;
    }
}

/// <summary>
/// Publishes the map from the runtime loop to the render loop, and carries shutdown and failure
/// the other way.
/// </summary>
/// <remarks>
/// The render loop holds the published handle only to attach its own session, which native serves
/// from any thread; every other map call stays on the runtime loop.
/// </remarks>
internal sealed class MapChannel : IDisposable
{
    private static readonly TimeSpan MapPollInterval = TimeSpan.FromMilliseconds(1);

    private readonly ManualResetEventSlim mapPublished = new();
    private readonly ManualResetEventSlim shutdownRequested = new();
    private MapHandle? map;
    private WakeSource? wake;
    private ExceptionDispatchInfo? failure;

    public bool ShutdownRequested => shutdownRequested.IsSet;

    /// <summary>Runtime loop: announces the map it just created.</summary>
    public void PublishMap(MapHandle value, WakeSource source)
    {
        wake = source;
        map = value;
        mapPublished.Set();
    }

    /// <summary>Render loop: releases the runtime loop's parked pump.</summary>
    public void WakeRuntimeLoop()
    {
        // The runtime loop clears this before disposing, but the two run on different threads, so
        // tolerate losing the race. A missed wake costs the parking bound, nothing more.
        try
        {
            wake?.Signal();
        }
        catch (ObjectDisposedException) { }
    }

    /// <summary>Runtime loop: stops handing out its wake source before disposing it.</summary>
    public void ClearWake()
    {
        wake = null;
    }

    /// <summary>Render loop: blocks until the map to attach against exists.</summary>
    public MapHandle WaitForMap()
    {
        while (!mapPublished.Wait(MapPollInterval))
        {
            ThrowIfFailed();
        }

        return map!;
    }

    /// <summary>
    /// Render loop: asks the runtime loop to stop. Called only after the render session is closed,
    /// because the map cannot be destroyed before then.
    /// </summary>
    public void RequestShutdown()
    {
        shutdownRequested.Set();
        // Release the pump so shutdown is observed now rather than after the parking bound expires.
        WakeRuntimeLoop();
    }

    /// <summary>Runtime loop: blocks until the render loop is done with its session.</summary>
    public void WaitForShutdown()
    {
        shutdownRequested.Wait();
    }

    /// <summary>Records the first failure either loop reported.</summary>
    public void Fail(Exception error)
    {
        Interlocked.CompareExchange(ref failure, ExceptionDispatchInfo.Capture(error), null);
    }

    /// <summary>Rethrows the recorded failure with its original stack trace.</summary>
    public void ThrowIfFailed()
    {
        Volatile.Read(ref failure)?.Throw();
    }

    public void Dispose()
    {
        mapPublished.Dispose();
        shutdownRequested.Dispose();
    }
}
