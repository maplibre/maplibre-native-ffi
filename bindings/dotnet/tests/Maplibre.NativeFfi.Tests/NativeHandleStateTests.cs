using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031, xUnit1051

public sealed unsafe class NativeHandleStateTests
{
    private static readonly Lock Gate = new();
    private static mln_status destroyStatus;
    private static int destroyCount;

    [BindingSpecTest("BND-040")]
    [Fact]
    public void CloseIsIdempotentAfterSuccess()
    {
        using var _ = Gate.EnterScope();
        destroyStatus = mln_status.MLN_STATUS_OK;
        destroyCount = 0;
        var state = new NativeHandleState<MlnRuntime>(
            SyntheticHandles.Runtime(1234),
            Destroy,
            "RuntimeHandle"
        );

        state.Close();
        state.Close();

        Assert.True(state.IsClosed);
        Assert.Equal(1, destroyCount);
    }

    [BindingSpecTest("BND-041")]
    [Fact]
    public void FailedCloseKeepsHandleLiveForRetry()
    {
        using var _ = Gate.EnterScope();
        destroyStatus = mln_status.MLN_STATUS_INVALID_STATE;
        destroyCount = 0;
        var state = new NativeHandleState<MlnRuntime>(
            SyntheticHandles.Runtime(1234),
            Destroy,
            "RuntimeHandle"
        );

        var error = Assert.Throws<InvalidStateException>(state.Close);

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.False(state.IsClosed);
        Assert.Equal(1, destroyCount);

        destroyStatus = mln_status.MLN_STATUS_OK;
        state.Close();

        Assert.True(state.IsClosed);
        Assert.Equal(2, destroyCount);
    }

    [BindingSpecTest("BND-046")]
    [Fact]
    public void PointerFailsWhileCloseIsInProgress()
    {
        using var destroy = new BlockingDestroy();
        var state = new NativeHandleState<MlnRuntime>(
            SyntheticHandles.Runtime(1234),
            destroy.Destroy,
            "RuntimeHandle"
        );

        var close = Task.Run(state.Close);
        destroy.WaitUntilStarted();

        var error = Assert.Throws<InvalidStateException>(() => _ = state.Handle);

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Contains("closing", error.Message, StringComparison.OrdinalIgnoreCase);

        destroy.Allow();
        close.GetAwaiter().GetResult();

        Assert.True(state.IsClosed);
        Assert.Equal(1, destroy.Count);
    }

    [BindingSpecTest("BND-046")]
    [Fact]
    public void ConcurrentCloseWaitsForInProgressReleaseWithoutDestroyingTwice()
    {
        using var destroy = new BlockingDestroy();
        var state = new NativeHandleState<MlnRuntime>(
            SyntheticHandles.Runtime(1234),
            destroy.Destroy,
            "RuntimeHandle"
        );

        var firstClose = Task.Run(state.Close);
        destroy.WaitUntilStarted();

        var secondClose = Task.Run(state.Close);
        Assert.False(secondClose.Wait(TimeSpan.FromMilliseconds(50)));
        Assert.Equal(1, destroy.Count);

        destroy.Allow();
        firstClose.GetAwaiter().GetResult();
        secondClose.GetAwaiter().GetResult();

        Assert.True(state.IsClosed);
        Assert.Equal(1, destroy.Count);
    }

    [BindingSpecTest("BND-048")]
    [Fact]
    public void TryCloseSuppressesFailureWithoutClosingHandle()
    {
        using var _ = Gate.EnterScope();
        destroyStatus = mln_status.MLN_STATUS_INVALID_STATE;
        destroyCount = 0;
        var reports = new List<NativeLeakReport>();
        using var capture = NativeLeakReporter.CaptureForTest(reports.Add);
        var state = new NativeHandleState<MlnRuntime>(
            SyntheticHandles.Runtime(1234),
            Destroy,
            "RuntimeHandle"
        );

        Assert.False(state.TryClose());

        Assert.False(state.IsClosed);
        Assert.Equal(1, destroyCount);
        var report = Assert.Single(reports);
        Assert.Equal(NativeLeakReportKind.DisposeFailed, report.Kind);
        Assert.Equal("RuntimeHandle", report.TypeName);
        Assert.Equal(SyntheticHandles.Runtime(1234).Value, report.Handle);
        Assert.Equal(mln_status.MLN_STATUS_INVALID_STATE, report.Status);

        destroyStatus = mln_status.MLN_STATUS_OK;
        state.Close();
    }

    [BindingSpecTest("BND-044")]
    [Fact]
    public void FinalizerReportsLeakedLiveHandleWithoutDestroyingIt()
    {
        using var _ = Gate.EnterScope();
        destroyStatus = mln_status.MLN_STATUS_OK;
        destroyCount = 0;
        var reports = new List<NativeLeakReport>();
        using var capture = NativeLeakReporter.CaptureForTest(reports.Add);

        CreateLeakedState();
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();

        var report = Assert.Single(reports);
        Assert.Equal(NativeLeakReportKind.LeakedHandle, report.Kind);
        Assert.Equal("RuntimeHandle", report.TypeName);
        Assert.Equal(SyntheticHandles.Runtime(5678).Value, report.Handle);
        Assert.Null(report.Status);
        Assert.Equal(0, destroyCount);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void CreateLeakedState()
    {
        _ = new NativeHandleState<MlnRuntime>(
            SyntheticHandles.Runtime(5678),
            Destroy,
            "RuntimeHandle"
        );
    }

    [BindingSpecTest("BND-197")]
    [Fact]
    public void AUseStartingAfterCloseBeginsIsRefused()
    {
        using var _ = Gate.EnterScope();
        destroyStatus = mln_status.MLN_STATUS_OK;
        destroyCount = 0;
        var state = new NativeHandleState<MlnRuntime>(
            SyntheticHandles.Runtime(1234),
            Destroy,
            "RuntimeHandle"
        );

        state.Close();

        Assert.Throws<InvalidStateException>(() => state.Handle);
    }

    /// <summary>A destroy that blocks until the test releases it, so a close stays in progress.</summary>
    private sealed class BlockingDestroy : IDisposable
    {
        private readonly ManualResetEventSlim started = new(false);
        private readonly ManualResetEventSlim allowed = new(false);
        private int count;

        internal int Count => Volatile.Read(ref count);

        internal mln_status Destroy(MlnRuntime handle)
        {
            Assert.False(handle.IsNull);
            Interlocked.Increment(ref count);
            started.Set();
            Assert.True(allowed.Wait(TimeSpan.FromSeconds(5)));
            return mln_status.MLN_STATUS_OK;
        }

        internal void WaitUntilStarted() => Assert.True(started.Wait(TimeSpan.FromSeconds(5)));

        internal void Allow() => allowed.Set();

        public void Dispose()
        {
            started.Dispose();
            allowed.Dispose();
        }
    }

    private static mln_status Destroy(MlnRuntime handle)
    {
        Assert.False(handle.IsNull);
        destroyCount++;
        return destroyStatus;
    }
}

#pragma warning restore xUnit1031, xUnit1051
