using System.Collections.Concurrent;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Status;

namespace Maplibre.NativeFfi.Runtime;

internal sealed unsafe class NotificationReceiver : IDisposable
{
    private readonly Dictionary<ulong, TaskCompletionSource> operations = [];
    private readonly ConcurrentQueue<ReadyEndpoint> observedEndpoints = new();
    private readonly ConcurrentDictionary<ulong, ReadyEndpoint> observedOperations = new();
    private readonly object drainGate = new();
    private readonly object operationGate = new();
    private readonly GCHandle callbackRoot;
    private int drainScheduled;
    private int drainRequested;
    private volatile bool closed;

    internal NotificationReceiver()
    {
        MlnNotificationSource source = default;
        NativeStatus.Check(NativeMethods.mln_notification_source_create(&source));
        Source = source;
        callbackRoot = GCHandle.Alloc(this);
        try
        {
            NativeStatus.Check(
                NativeMethods.mln_notification_source_set_callback(
                    source,
                    &OnNotification,
                    (void*)GCHandle.ToIntPtr(callbackRoot)
                )
            );
        }
        catch
        {
            callbackRoot.Free();
            _ = NativeMethods.mln_notification_source_close(source);
            throw;
        }
    }

    internal MlnNotificationSource Source { get; }

    internal Task WaitForOperationAsync(
        MlnOperation operation,
        CancellationToken cancellationToken = default
    )
    {
        ObjectDisposedException.ThrowIf(closed, this);
        if (IsOperationCompleted(operation))
        {
            return Task.CompletedTask;
        }
        var completion = new TaskCompletionSource(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        lock (operationGate)
        {
            ObjectDisposedException.ThrowIf(closed, this);
            if (operations.ContainsKey(operation.Value))
            {
                throw new InvalidOperationException(
                    "The operation already has a completion waiter."
                );
            }
            operations.Add(operation.Value, completion);
        }

        if (IsOperationCompleted(operation))
        {
            lock (operationGate)
            {
                if (
                    operations.Remove(operation.Value, out var registered)
                    && ReferenceEquals(registered, completion)
                )
                {
                    completion.TrySetResult();
                }
            }
        }

        if (completion.Task.IsCompleted)
        {
            return completion.Task;
        }

        var registration = cancellationToken.Register(() =>
            CancelWait(operation.Value, completion, cancellationToken)
        );
        ScheduleDrain();
        return OperationAwaiter.WaitThen(completion.Task, static () => { }, registration.Dispose);
    }

    internal IReadOnlyList<ReadyEndpoint> DrainReadyEndpoints() =>
        DrainNativeReadyEndpoints(returnObserved: true);

    private IReadOnlyList<ReadyEndpoint> DrainNativeReadyEndpoints(bool returnObserved)
    {
        lock (drainGate)
        {
            ObjectDisposedException.ThrowIf(closed, this);
            MlnReadyBatch batch = default;
            NativeStatus.Check(NativeMethods.mln_notification_source_drain_ready(Source, &batch));

            try
            {
                var view = new mln_ready_batch_view { size = (uint)sizeof(mln_ready_batch_view) };
                NativeStatus.Check(NativeMethods.mln_ready_batch_get(batch, &view));
                if (view.endpoint_size < sizeof(mln_ready_endpoint))
                {
                    throw new InvalidOperationException(
                        "The native ready endpoint stride is smaller than the known layout."
                    );
                }
                if (view.endpoint_count != 0 && view.endpoints is null)
                {
                    throw new InvalidOperationException(
                        "The native ready endpoint batch has a null data pointer."
                    );
                }

                var cursor = (byte*)view.endpoints;
                for (nuint index = 0; index < view.endpoint_count; index++)
                {
                    var endpoint = (mln_ready_endpoint*)cursor;
                    var kind = Enum.IsDefined(typeof(NotificationEndpointKind), endpoint->kind)
                        ? (NotificationEndpointKind)endpoint->kind
                        : 0;
                    var observed = new ReadyEndpoint(kind, endpoint->kind, endpoint->id);
                    if (kind == NotificationEndpointKind.Operation)
                    {
                        TaskCompletionSource? completion;
                        lock (operationGate)
                        {
                            if (operations.Remove(endpoint->id, out completion))
                            {
                                observedEndpoints.Enqueue(observed);
                            }
                            else
                            {
                                observedOperations[endpoint->id] = observed;
                            }
                        }
                        completion?.TrySetResult();
                    }
                    else
                    {
                        observedEndpoints.Enqueue(observed);
                    }
                    cursor += view.endpoint_size;
                }

                var endpoints = new List<ReadyEndpoint>();
                if (returnObserved)
                {
                    while (observedEndpoints.TryDequeue(out var observed))
                    {
                        endpoints.Add(observed);
                    }
                    foreach (var operation in observedOperations)
                    {
                        if (observedOperations.TryRemove(operation.Key, out var observed))
                        {
                            endpoints.Add(observed);
                        }
                    }
                }
                return endpoints;
            }
            finally
            {
                NativeMethods.mln_ready_batch_release(batch);
            }
        }
    }

    public void Dispose()
    {
        lock (drainGate)
        {
            if (closed)
            {
                return;
            }
            lock (operationGate)
            {
                if (operations.Count != 0)
                {
                    throw new InvalidOperationException(
                        "The notification source still has operation waiters."
                    );
                }
            }
            NativeStatus.Check(NativeMethods.mln_notification_source_clear_callback(Source));
            NativeStatus.Check(NativeMethods.mln_notification_source_close(Source));
            closed = true;
            callbackRoot.Free();
        }
    }

    private static bool IsOperationCompleted(MlnOperation operation)
    {
        bool completed;
        NativeStatus.Check(NativeMethods.mln_operation_poll(operation, &completed));
        return completed;
    }

    private void CancelWait(
        ulong operation,
        TaskCompletionSource completion,
        CancellationToken cancellationToken
    )
    {
        lock (operationGate)
        {
            if (
                !operations.Remove(operation, out var registered)
                || !ReferenceEquals(registered, completion)
            )
            {
                return;
            }
        }
        completion.TrySetCanceled(cancellationToken);
    }

    private void ScheduleDrain()
    {
        Volatile.Write(ref drainRequested, 1);
        if (Interlocked.Exchange(ref drainScheduled, 1) == 0)
        {
            ThreadPool.UnsafeQueueUserWorkItem(
                static receiver => receiver.DrainScheduled(),
                this,
                false
            );
        }
    }

    private void DrainScheduled()
    {
        try
        {
            while (Interlocked.Exchange(ref drainRequested, 0) != 0)
            {
                _ = DrainNativeReadyEndpoints(returnObserved: false);
            }
        }
        catch (ObjectDisposedException) { }
        finally
        {
            Volatile.Write(ref drainScheduled, 0);
            if (Volatile.Read(ref drainRequested) != 0)
            {
                ScheduleDrain();
            }
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void OnNotification(void* userData)
    {
        var receiver = (NotificationReceiver?)GCHandle.FromIntPtr((nint)userData).Target;

        receiver?.ScheduleDrain();
    }
}
