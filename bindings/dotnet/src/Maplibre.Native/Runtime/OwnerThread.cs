using System.Collections.Concurrent;
using System.Runtime.ExceptionServices;
using Maplibre.Native.Error;

namespace Maplibre.Native.Runtime;

/// <summary>Dedicated owner-thread executor for thread-affine MapLibre Native handles.</summary>
public sealed class OwnerThread : IDisposable
{
    private readonly object gate = new();
    private readonly BlockingCollection<OwnerThreadWorkItem> queue = [];
    private readonly ManualResetEventSlim started = new();
    private readonly Thread thread;
    private bool closing;
    private bool closed;
    private int ownerManagedThreadId;

    public OwnerThread()
    {
        thread = new Thread(Run) { IsBackground = true, Name = "MapLibre Native owner thread" };
        thread.Start();
        started.Wait();
    }

    /// <summary>Whether the owner thread has stopped accepting and running work.</summary>
    public bool IsClosed
    {
        get
        {
            lock (gate)
            {
                return closed;
            }
        }
    }

    /// <summary>The managed thread ID for the bound owner thread.</summary>
    public int OwnerManagedThreadId => ownerManagedThreadId;

    /// <summary>Runs an operation on the owner thread and returns after it completes.</summary>
    public void Invoke(Action operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        Invoke(() =>
        {
            operation();
            return true;
        });
    }

    /// <summary>Runs an operation on the owner thread and returns its result.</summary>
    public T Invoke<T>(Func<T> operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        lock (gate)
        {
            if (closing || closed)
            {
                ThrowClosed();
            }
        }

        if (Environment.CurrentManagedThreadId == ownerManagedThreadId)
        {
            return operation();
        }

        var item = new OwnerThreadWorkItem<T>(operation);
        lock (gate)
        {
            if (closing || closed)
            {
                ThrowClosed();
            }

            queue.Add(item);
        }

        return item.GetResult();
    }

    /// <summary>Stops accepting new work and waits for queued work to finish.</summary>
    public void Close()
    {
        var shouldCompleteAdding = false;
        lock (gate)
        {
            if (closed)
            {
                return;
            }

            if (!closing)
            {
                closing = true;
                shouldCompleteAdding = true;
            }
        }

        if (shouldCompleteAdding)
        {
            queue.CompleteAdding();
        }

        if (Environment.CurrentManagedThreadId != ownerManagedThreadId)
        {
            thread.Join();
        }
    }

    public void Dispose()
    {
        Close();
        // Owner-thread disposal cannot join the running thread or dispose queue primitives
        // that are still in use by the current work item.
        if (Environment.CurrentManagedThreadId != ownerManagedThreadId)
        {
            started.Dispose();
            queue.Dispose();
        }
    }

    private void Run()
    {
        ownerManagedThreadId = Environment.CurrentManagedThreadId;
        started.Set();
        try
        {
            foreach (var item in queue.GetConsumingEnumerable())
            {
                item.Execute();
            }
        }
        finally
        {
            lock (gate)
            {
                closing = true;
                closed = true;
            }
        }
    }

    private static void ThrowClosed()
    {
        throw new InvalidStateException(
            MaplibreStatus.InvalidState,
            null,
            "OwnerThread is closed.",
            null
        );
    }
}

internal abstract class OwnerThreadWorkItem
{
    internal abstract void Execute();
}

internal sealed class OwnerThreadWorkItem<T> : OwnerThreadWorkItem
{
    private readonly Func<T> operation;
    private readonly ManualResetEventSlim completed = new();
    private ExceptionDispatchInfo? error;
    private T? result;

    internal OwnerThreadWorkItem(Func<T> operation)
    {
        this.operation = operation;
    }

    internal override void Execute()
    {
        try
        {
            result = operation();
        }
        catch (Exception exception)
        {
            error = ExceptionDispatchInfo.Capture(exception);
        }
        finally
        {
            completed.Set();
        }
    }

    internal T GetResult()
    {
        completed.Wait();
        completed.Dispose();
        error?.Throw();
        return result!;
    }
}
