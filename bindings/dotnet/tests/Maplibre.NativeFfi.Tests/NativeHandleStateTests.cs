using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031, xUnit1051

public sealed unsafe class NativeHandleStateTests
{
    [Fact]
    public void ABorrowHoldsOffACloseFromAnotherThreadUntilItEnds()
    {
        var destroyed = 0;
        var state = new NativeHandleState<MlnRuntime>(
            SyntheticHandles.Runtime(1234),
            (_, _) =>
            {
                Interlocked.Increment(ref destroyed);
                return mln_status.MLN_STATUS_OK;
            },
            "RuntimeHandle"
        );
        Assert.Throws<FormatException>(
            (Action)(
                () =>
                {
                    using var read = state.Borrow();
                    Assert.Equal(SyntheticHandles.Runtime(1234).Value, read.Handle.Value);
                    Task.Run(() => Assert.Throws<InvalidStateException>(state.Close))
                        .GetAwaiter()
                        .GetResult();
                    Assert.Equal(0, destroyed);
                    throw new FormatException("Copy failed.");
                }
            )
        );
        state.Close();
        Assert.Equal(1, destroyed);
        Assert.Throws<InvalidStateException>(() =>
        {
            using var read = state.Borrow();
        });
    }

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

    [Fact]
    public void ConcurrentCloseFailsWithoutBlockingOrDestroyingTwice()
    {
        using var destroy = new BlockingDestroy();
        var state = new NativeHandleState<MlnRuntime>(
            SyntheticHandles.Runtime(1234),
            destroy.Destroy,
            "RuntimeHandle"
        );

        var firstClose = Task.Run(state.Close);
        destroy.WaitUntilStarted();

        var secondClose = Task.Run(() => Assert.Throws<InvalidStateException>(state.Close));
        secondClose.GetAwaiter().GetResult();
        Assert.Equal(1, destroy.Count);

        destroy.Allow();
        firstClose.GetAwaiter().GetResult();
        secondClose.GetAwaiter().GetResult();

        Assert.True(state.IsClosed);
        Assert.Equal(1, destroy.Count);
    }

    /// <summary>A destroy that blocks until the test releases it, so a close stays in progress.</summary>
    private sealed class BlockingDestroy : IDisposable
    {
        private readonly ManualResetEventSlim started = new(false);
        private readonly ManualResetEventSlim allowed = new(false);
        private int count;

        internal int Count => Volatile.Read(ref count);

        internal mln_status Destroy(MlnRuntime handle, mln_diagnostic* diagnostic)
        {
            Assert.False(handle.IsNull);
            Interlocked.Increment(ref count);
            started.Set();
            Assert.True(allowed.Wait(TestWaits.Deadline));
            return mln_status.MLN_STATUS_OK;
        }

        internal void WaitUntilStarted() => Assert.True(started.Wait(TestWaits.Deadline));

        internal void Allow() => allowed.Set();

        public void Dispose()
        {
            started.Dispose();
            allowed.Dispose();
        }
    }
}

#pragma warning restore xUnit1031, xUnit1051
