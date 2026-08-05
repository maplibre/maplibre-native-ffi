using System.Collections.Concurrent;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.ExceptionServices;
using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>A camera change decoded on the render loop and applied on the map's owner thread.</summary>
/// <remarks>
/// Commands carry deltas wherever the current camera is an input, because the read and the write
/// have to happen together on the owner thread. A null animation applies the command immediately.
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
    /// out its parking bound. The runtime loop parks inside the native pump, so only the native
    /// wake source releases it.
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
internal sealed class RenderRequest
{
    private int value = 1;

    public void Set()
    {
        Volatile.Write(ref value, 1);
    }

    public bool Consume()
    {
        return Interlocked.Exchange(ref value, 0) == 1;
    }
}

/// <summary>
/// Publishes the map from the runtime loop to the render loop, and carries shutdown and failure
/// the other way. The render loop uses the published handle only to attach its own session; every
/// other map call stays on the runtime loop.
/// </summary>
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
        // The runtime loop clears this before disposing on another thread; a lost race costs one
        // parking bound.
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
    /// Render loop: asks the runtime loop to stop. Call only after closing the render session,
    /// because the map cannot be destroyed before then.
    /// </summary>
    public void RequestShutdown()
    {
        shutdownRequested.Set();
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
