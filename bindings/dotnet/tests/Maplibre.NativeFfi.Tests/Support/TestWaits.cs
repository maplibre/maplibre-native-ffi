using System.Diagnostics;
using System.Globalization;
using System.Runtime.ExceptionServices;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>The deadline every wait in the suite shares.</summary>
/// <remarks>
/// A wait blocks on a signal and fails at the deadline; nothing passes or fails by elapsed time.
/// <c>MLN_TEST_TIMEOUT_SCALE</c> stretches the deadline on slow runners, as it does for the C
/// suite.
/// </remarks>
internal static class TestWaits
{
    internal static TimeSpan Deadline { get; } = TimeSpan.FromSeconds(10 * Scale());

    internal static CancellationToken Token => TestContext.Current.CancellationToken;

    /// <summary>Waits for a signal until the shared deadline, failing the test when it passes.</summary>
    internal static void Wait(SemaphoreSlim signal, Stopwatch elapsed, string what)
    {
        var remaining = Deadline - elapsed.Elapsed;
        if (remaining <= TimeSpan.Zero || !signal.Wait(remaining, Token))
            throw new TimeoutException($"Timed out waiting for {what}.");
    }

    /// <summary>Awaits a signal until the shared deadline, failing the test when it passes.</summary>
    internal static async Task WaitAsync(SemaphoreSlim signal, Stopwatch elapsed, string what)
    {
        var remaining = Deadline - elapsed.Elapsed;
        if (remaining <= TimeSpan.Zero || !await signal.WaitAsync(remaining, Token))
            throw new TimeoutException($"Timed out waiting for {what}.");
    }

    private static double Scale() =>
        double.TryParse(
            Environment.GetEnvironmentVariable("MLN_TEST_TIMEOUT_SCALE"),
            NumberStyles.Float,
            CultureInfo.InvariantCulture,
            out var scale
        )
        && scale >= 1
            ? scale
            : 1;
}

/// <summary>Forces the collections that liveness assertions depend on.</summary>
internal static class Gc
{
    internal static void Collect()
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
    }

    // Mono conservatively scans native stacks. Run capture construction and
    // liveness probes on threads that exit, so stale stack words cannot root them.
    internal static T OnThread<T>(Func<T> action)
    {
        T result = default!;
        Exception? failure = null;
        var worker = new Thread(() =>
        {
            try
            {
                result = action();
            }
            catch (Exception error)
            {
                failure = error;
            }
        });
        worker.Start();
        worker.Join();
        if (failure is not null)
            ExceptionDispatchInfo.Capture(failure).Throw();
        return result;
    }

    internal static void OnThread(Action action) =>
        OnThread(() =>
        {
            action();
            return true;
        });

    internal static bool IsAlive(WeakReference value) =>
        OnThread(() =>
        {
            Collect();
            return value.IsAlive;
        });

    /// <summary>
    /// Collects until every value is reclaimed, failing the test at the shared deadline.
    /// </summary>
    /// <remarks>
    /// A completion's task finishes inside its callback, but native releases the completion's GC
    /// handle afterwards on the runtime worker, so an object the completion reaches can stay
    /// rooted for a moment after the awaited call returns. Each pass's collections and finalizer
    /// wait give the worker that moment.
    /// </remarks>
    internal static void CollectUntilReclaimed(string what, params WeakReference[] values)
    {
        var elapsed = Stopwatch.StartNew();
        while (values.Any(IsAlive))
        {
            if (elapsed.Elapsed >= TestWaits.Deadline)
                throw new TimeoutException($"Timed out waiting for {what} to be collected.");
        }
    }
}
