using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class CompletionTests
{
    [Fact]
    public async Task DiscardedCommandsAreFencedAndSynchronousRejectionThrows()
    {
        await using var fixture = await NativeFixture.CreateAsync();
        fixture.Map.Discarding.SetDebugOptions(MapDebugOption.TileBorders);
        // A later command on the same map completes after the discarded one.
        await fixture.Map.RequestRepaintAsync();
        Assert.Equal(MapDebugOption.TileBorders, fixture.Map.GetSnapshot().DebugOptions);
        Assert.Throws<MaplibreException>(() =>
            fixture.Map.Discarding.Resize(
                new LogicalExtent
                {
                    Width = 0,
                    Height = 8,
                    ScaleFactor = 1,
                }
            )
        );
    }

    [Fact]
    public async Task ACompletionDeliversOnceAndAFailedConversionFaultsItsTask()
    {
        var (converted, unconverted) = DeliverFakeCompletions();

        Assert.Equal(7, await converted);
        var error = await Assert.ThrowsAsync<InvalidOperationException>(() => unconverted);
        Assert.Contains("no value", error.Message, StringComparison.Ordinal);
    }

    // Each fake call accepts the completion it is handed, and the test plays native's part. The first
    // gets a result, then a second result that native never sends, then its release. The second
    // gets a result without the value its converter needs.
    private static unsafe (Task<int> Converted, Task<int> Unconverted) DeliverFakeCompletions()
    {
        mln_completion first;
        NativeCompletion.Start(
            &first,
            static result => NativeCompletion.Value<int>(result),
            CancellationToken.None,
            out var firstPending
        );
        var converted = firstPending.Submitted(mln_status.MLN_STATUS_OK);
        var seven = 7;
        var eight = 8;
        Deliver(first, &seven, 1);
        Deliver(first, &eight, 1);
        first.release_user_data(first.user_data);

        mln_completion second;
        NativeCompletion.Start(
            &second,
            static result => NativeCompletion.Value<int>(result),
            CancellationToken.None,
            out var secondPending
        );
        var unconverted = secondPending.Submitted(mln_status.MLN_STATUS_OK);
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

        Assert.IsType<MaplibreException>(error);
        Assert.False(Gc.IsAlive(captured));
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static unsafe (Exception? Error, WeakReference Captured) SubmitRejected()
    {
        var captured = new object();
        var error = Record.Exception(() =>
        {
            mln_completion completion;
            NativeCompletion.Start(
                &completion,
                _ =>
                {
                    GC.KeepAlive(captured);
                    return 0;
                },
                CancellationToken.None,
                out var pending
            );
            _ = pending.Submitted(mln_status.MLN_STATUS_INVALID_ARGUMENT);
        });
        return (error, new WeakReference(captured));
    }

    [Fact]
    public async Task AFailedCommandArrivesAsItsDisposition()
    {
        await using var fixture = await NativeFixture.WithEmptyStyleAsync();

        var completion = await fixture.Map.RemoveStyleSourceAsync("missing");

        Assert.Equal(CommandDisposition.Failed, completion.Disposition);
        Assert.Equal((int)Status.NotFound, completion.RawStatus);
        Assert.NotEmpty(completion.Diagnostic);
    }

    // A still image on a static map with no session stays pending until the map closes, so every
    // outcome below comes from the wait rather than from native. A caller abandons a wait with
    // WaitAsync, and only a creation takes a token, so the binding can dispose its late map.
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
        cancellation.Cancel();
        var stillImage = fixture.Map.RequestStillImageAsync();

        await Assert.ThrowsAsync<TimeoutException>(() =>
            stillImage.WaitAsync(TimeSpan.Zero, TestWaits.Token)
        );
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() =>
            stillImage.WaitAsync(cancellation.Token)
        );

        // The native request is still outstanding, and closing the map retires it.
        Assert.False(stillImage.IsCompleted);
        await fixture.Map.CloseAsync();

        // Native creates this map although its wait ended before submission.
        var created = fixture.Runtime.CreateMapAsync(NativeFixture.SmallMap, cancellation.Token);
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() => created);
        // Runtime release refuses a runtime that still owns a map, so it succeeds only because
        // the binding disposed the map that arrived after the wait. The creation runs on the
        // worker before a later runtime command completes.
        await fixture.Runtime.ClearResourceTransformAsync();
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

    // The fake call accepts its completion, and the returned delivery plays native's part.
    private static unsafe (Task<Probe> Task, Func<Probe> Deliver) SubmitHeld(
        CancellationToken cancellationToken
    )
    {
        mln_completion completion;
        var probe = new Probe();
        NativeCompletion.Start(&completion, _ => probe, cancellationToken, out var pending);
        var task = pending.Submitted(mln_status.MLN_STATUS_OK);
        var held = completion;
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
