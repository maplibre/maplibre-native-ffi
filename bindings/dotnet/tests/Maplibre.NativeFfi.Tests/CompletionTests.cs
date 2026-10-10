using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class CompletionTests
{
    [Fact]
    public async Task ACompletionDeliversOnceAndAFailedConversionFaultsItsTask()
    {
        var (converted, unconverted) = DeliverFakeCompletions();

        Assert.Equal(7, await converted);
        var error = await Assert.ThrowsAsync<InvalidOperationException>(() => unconverted);
        Assert.Contains("no value", error.Message, StringComparison.Ordinal);
    }

    // Each fake submission keeps the completion it is handed and plays native's part. The first
    // gets a result, then a second result that native never sends, then its release. The second
    // gets a result without the value its converter needs.
    private static unsafe (Task<int> Converted, Task<int> Unconverted) DeliverFakeCompletions()
    {
        mln_completion first = default;
        var converted = NativeCompletion.Submit(
            (completion, _) =>
            {
                first = *completion;
                return mln_status.MLN_STATUS_OK;
            },
            result => NativeCompletion.Value<int>(result),
            CancellationToken.None
        );
        var seven = 7;
        var eight = 8;
        Deliver(first, &seven, 1);
        Deliver(first, &eight, 1);
        first.release_user_data(first.user_data);

        mln_completion second = default;
        var unconverted = NativeCompletion.Submit(
            (completion, _) =>
            {
                second = *completion;
                return mln_status.MLN_STATUS_OK;
            },
            result => NativeCompletion.Value<int>(result),
            CancellationToken.None
        );
        Deliver(second, null, 0);
        second.release_user_data(second.user_data);
        return (converted, unconverted);
    }

    private static unsafe void Deliver(mln_completion completion, int* value, nuint count)
    {
        var result = new mln_completion_result
        {
            size = (uint)sizeof(mln_completion_result),
            status = (int)mln_status.MLN_STATUS_OK,
            value = value,
            value_count = count,
        };
        completion.callback(completion.user_data, &result);
    }

    // A native build whose element grew reports a value_size wider than this binding's element,
    // so the decoder steps by it. A stride narrower than the element cannot hold one.
    [Fact]
    public unsafe void AnArrayResultIsReadAtItsValueSize()
    {
        // Two coordinates, each followed by a member this binding does not know.
        double* wide = stackalloc double[] { 1, 2, -1, 3, 4, -1 };
        var result = new mln_completion_result
        {
            size = (uint)sizeof(mln_completion_result),
            status = (int)mln_status.MLN_STATUS_OK,
            value = wide,
            value_size = 3 * sizeof(double),
            value_count = 2,
        };

        var points = NativeCompletion.Values<mln_lat_lng>(&result);

        Assert.Equal(
            new[] { (1.0, 2.0), (3.0, 4.0) },
            points.Select(point => (point.latitude, point.longitude))
        );
        var narrow = result;
        narrow.value_size = (uint)sizeof(mln_lat_lng) - 1;
        Assert.Throws<InvalidOperationException>(() => ReadLatLngs(narrow));
    }

    private static unsafe mln_lat_lng[] ReadLatLngs(mln_completion_result result) =>
        NativeCompletion.Values<mln_lat_lng>(&result);

    [Fact]
    public void ARejectedSubmissionFreesItsCompletionState()
    {
        var (error, captured) = SubmitRejected();

        Assert.IsType<InvalidArgumentException>(error);
        Assert.False(Gc.IsAlive(captured));
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static unsafe (Exception? Error, WeakReference Captured) SubmitRejected()
    {
        var captured = new object();
        var error = Record.Exception(() =>
        {
            _ = NativeCompletion.Submit(
                (_, _) => mln_status.MLN_STATUS_INVALID_ARGUMENT,
                _ =>
                {
                    GC.KeepAlive(captured);
                    return 0;
                },
                CancellationToken.None
            );
        });
        return (error, new WeakReference(captured));
    }

    [Fact]
    public async Task AFailedCommandArrivesAsItsDisposition()
    {
        await using var fixture = await NativeFixture.WithEmptyStyleAsync();

        var completion = await fixture.Map.RemoveStyleSourceAsync("missing", TestWaits.Token);

        Assert.Equal(CommandDisposition.Failed, completion.Disposition);
        Assert.Equal((int)MaplibreStatus.NotFound, completion.RawStatus);
        Assert.Contains("missing", completion.Diagnostic, StringComparison.Ordinal);
    }

    // A still image on a static map with no session stays pending until the map closes, so every
    // outcome below comes from the wait rather than from native.
    [Fact]
    public async Task AWaitCanTimeOutOrBeCancelled()
    {
        await using var fixture = await NativeFixture.CreateAsync(
            NativeFixture.SmallMap with
            {
                MapMode = MapMode.Static,
            }
        );
        using var cancellation = new CancellationTokenSource();
        var stillImage = fixture.Map.RequestStillImageAsync(cancellation.Token);

        await Assert.ThrowsAsync<TimeoutException>(() =>
            stillImage.WaitAsync(TimeSpan.Zero, TestWaits.Token)
        );
        cancellation.Cancel();
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() => stillImage);

        // The native request is still outstanding, and closing the map retires it.
        await fixture.Map.CloseAsync();

        // Native creates this map although its wait ended before submission.
        var created = fixture.Runtime.CreateMapAsync(NativeFixture.SmallMap, cancellation.Token);
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() => created);
        await fixture.Runtime.BarrierAsync(TestWaits.Token);
        // Runtime release refuses a runtime that still owns a map, so it succeeds only because
        // the binding disposed the map that arrived after the wait.
        await fixture.Runtime.CloseAsync().WaitAsync(TestWaits.Deadline, TestWaits.Token);
    }

    [Fact]
    public async Task AValueThatArrivesAfterCancellationIsDisposed()
    {
        using var cancellation = new CancellationTokenSource();
        var (task, deliver) = SubmitHeld(cancellation.Token);

        cancellation.Cancel();
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() => task);
        var late = deliver();

        Assert.True(late.Disposed);
    }

    private sealed class Probe : IDisposable
    {
        internal bool Disposed { get; private set; }

        public void Dispose() => Disposed = true;
    }

    // The fake submission keeps its completion, and the returned delivery plays native's part.
    private static unsafe (Task<Probe> Task, Func<Probe> Deliver) SubmitHeld(
        CancellationToken cancellationToken
    )
    {
        mln_completion held = default;
        var probe = new Probe();
        var task = NativeCompletion.Submit(
            (completion, _) =>
            {
                held = *completion;
                return mln_status.MLN_STATUS_OK;
            },
            _ => probe,
            cancellationToken
        );
        return (
            task,
            () =>
            {
                Deliver(held, null, 0);
                held.release_user_data(held.user_data);
                return probe;
            }
        );
    }
}
