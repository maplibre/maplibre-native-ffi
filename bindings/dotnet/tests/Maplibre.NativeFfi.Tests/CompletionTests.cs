using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class CompletionTests
{
    [Fact]
    public async Task DiscardedCommandsAreFencedAndSynchronousRejectionThrows()
    {
        await using var fixture = await NativeFixture.CreateAsync();
        fixture.Map.Discarding.SetDebugOptions(MapDebugFlag.TileBorders);
        // A later command on the same map completes after the discarded one.
        await fixture.Map.RequestRepaintAsync();
        Assert.Equal(MapDebugFlag.TileBorders, fixture.Map.GetSnapshot().DebugOptions);
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
            status = mln_status.MLN_STATUS_OK,
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
            status = mln_status.MLN_STATUS_OK,
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
        var (state, captured) = Gc.OnThread(SubmitRejected);
        Gc.CollectUntilReclaimed("the rejected completion capture", captured);
        GC.KeepAlive(state);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static unsafe (object State, WeakReference Captured) SubmitRejected()
    {
        var captured = new object();
        object? state = null;
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
                out var pending
            );
            state = GCHandle.FromIntPtr((nint)completion.user_data).Target;
            _ = pending.Submitted(mln_status.MLN_STATUS_INVALID_ARGUMENT);
        });
        Assert.IsType<MaplibreException>(error);
        return (state!, new WeakReference(captured));
    }

    [Fact]
    public void AReleasedCompletionDropsItsCaptureWhileStateRemainsAlive()
    {
        var (state, captured) = Gc.OnThread(CompleteAndReleaseCapturingState);
        Gc.CollectUntilReclaimed("the released completion capture", captured);
        GC.KeepAlive(state);
    }

    private static unsafe (object State, WeakReference Captured) CompleteAndReleaseCapturingState()
    {
        var captured = new object();
        mln_completion completion;
        NativeCompletion.Start(
            &completion,
            _ =>
            {
                GC.KeepAlive(captured);
                return 7;
            },
            out var pending
        );
        var state = GCHandle.FromIntPtr((nint)completion.user_data).Target!;
        var task = pending.Submitted(mln_status.MLN_STATUS_OK);
        var result = new mln_completion_result { status = mln_status.MLN_STATUS_OK };
        completion.callback(completion.user_data, &result);
        completion.release_user_data(completion.user_data);
        Assert.True(task.IsCompletedSuccessfully);
        Assert.Equal(7, task.Result);
        return (state, new WeakReference(captured));
    }

    // Native fails a command it accepted, and the test plays native's part for a cancelled one,
    // which a live map cannot produce on demand.
    [Fact]
    public async Task AFailedOrCancelledCommandFaultsWithItsStatusAndGeneration()
    {
        await using var fixture = await NativeFixture.CreateAsync();
        var committed = await fixture.Map.SetStyleJsonAsync(NativeFixture.EmptyStyle);

        var failed = await Assert.ThrowsAsync<MaplibreException>(() =>
            fixture.Map.RemoveStyleSourceAsync("missing")
        );
        Assert.Equal(Status.NotFound, failed.Status);
        Assert.NotEmpty(failed.Diagnostic);
        Assert.True(failed.Generation > committed.Generation);

        var cancelled = await Assert.ThrowsAsync<MaplibreException>(DeliverCancelledCommand);
        Assert.Equal(Status.Cancelled, cancelled.Status);
        Assert.Equal("abandoned", cancelled.Diagnostic);
        Assert.Equal(9UL, cancelled.Generation);
    }

    private static unsafe Task<CommandCompletion> DeliverCancelledCommand()
    {
        mln_completion completion;
        NativeCompletion.StartCommand(&completion, out var pending);
        var task = pending.Submitted(mln_status.MLN_STATUS_OK);
        fixed (byte* diagnostic = "abandoned"u8)
        {
            var result = new mln_completion_result
            {
                size = (uint)sizeof(mln_completion_result),
                status = mln_status.MLN_STATUS_CANCELLED,
                disposition = mln_command_disposition.MLN_COMMAND_DISPOSITION_CANCELLED,
                generation = 9,
                diagnostic = new mln_buffer_view { data = diagnostic, size = 9 },
            };
            completion.callback(completion.user_data, &result);
        }
        completion.release_user_data(completion.user_data);
        return task;
    }

    // A still image on a static map with no session stays pending until the map closes, so every
    // outcome below comes from the wait rather than from native. A caller abandons a wait with
    // WaitAsync.
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
    }
}
