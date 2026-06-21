using System.Runtime.CompilerServices;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Handle;
using Xunit;

namespace Maplibre.Native.Tests;

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
        var state = new NativeHandleState<mln_runtime>(
            (mln_runtime*)1234,
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
        var state = new NativeHandleState<mln_runtime>(
            (mln_runtime*)1234,
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
        using var destroyStarted = new ManualResetEventSlim(false);
        using var allowDestroy = new ManualResetEventSlim(false);
        var destroyCount = 0;
        var state = new NativeHandleState<mln_runtime>(
            (mln_runtime*)1234,
            DestroyAfterRelease,
            "RuntimeHandle"
        );

        var close = Task.Run(state.Close);
        Assert.True(destroyStarted.Wait(TimeSpan.FromSeconds(5)));

        var error = Assert.Throws<InvalidStateException>(() => _ = state.Pointer);

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Contains("closing", error.Message, StringComparison.OrdinalIgnoreCase);

        allowDestroy.Set();
        close.GetAwaiter().GetResult();

        Assert.True(state.IsClosed);
        Assert.Equal(1, destroyCount);

        mln_status DestroyAfterRelease(mln_runtime* handle)
        {
            Assert.NotEqual((nint)0, (nint)handle);
            destroyCount++;
            destroyStarted.Set();
            Assert.True(allowDestroy.Wait(TimeSpan.FromSeconds(5)));
            return mln_status.MLN_STATUS_OK;
        }
    }

    [BindingSpecTest("BND-046")]
    [Fact]
    public void ConcurrentCloseWaitsForInProgressReleaseWithoutDestroyingTwice()
    {
        using var destroyStarted = new ManualResetEventSlim(false);
        using var allowDestroy = new ManualResetEventSlim(false);
        var destroyCount = 0;
        var state = new NativeHandleState<mln_runtime>(
            (mln_runtime*)1234,
            DestroyAfterRelease,
            "RuntimeHandle"
        );

        var firstClose = Task.Run(state.Close);
        Assert.True(destroyStarted.Wait(TimeSpan.FromSeconds(5)));

        var secondClose = Task.Run(state.Close);
        Assert.False(secondClose.Wait(TimeSpan.FromMilliseconds(50)));
        Assert.Equal(1, destroyCount);

        allowDestroy.Set();
        firstClose.GetAwaiter().GetResult();
        secondClose.GetAwaiter().GetResult();

        Assert.True(state.IsClosed);
        Assert.Equal(1, destroyCount);

        mln_status DestroyAfterRelease(mln_runtime* handle)
        {
            Assert.NotEqual((nint)0, (nint)handle);
            destroyCount++;
            destroyStarted.Set();
            Assert.True(allowDestroy.Wait(TimeSpan.FromSeconds(5)));
            return mln_status.MLN_STATUS_OK;
        }
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
        var state = new NativeHandleState<mln_runtime>(
            (mln_runtime*)1234,
            Destroy,
            "RuntimeHandle"
        );

        Assert.False(state.TryClose());

        Assert.False(state.IsClosed);
        Assert.Equal(1, destroyCount);
        var report = Assert.Single(reports);
        Assert.Equal(NativeLeakReportKind.DisposeFailed, report.Kind);
        Assert.Equal("RuntimeHandle", report.TypeName);
        Assert.Equal((nint)1234, report.Address);
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
        Assert.Equal((nint)5678, report.Address);
        Assert.Null(report.Status);
        Assert.Equal(0, destroyCount);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void CreateLeakedState()
    {
        _ = new NativeHandleState<mln_runtime>((mln_runtime*)5678, Destroy, "RuntimeHandle");
    }

    private static mln_status Destroy(mln_runtime* handle)
    {
        Assert.NotEqual((nint)0, (nint)handle);
        destroyCount++;
        return destroyStatus;
    }
}

#pragma warning restore xUnit1031, xUnit1051
