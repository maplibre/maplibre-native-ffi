using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>The process-global log callback.</summary>
[Collection(nameof(GlobalState))]
public sealed class LoggingTests
{
    // Native swaps the observer under the lock its dispatch holds, so a replaced registration is
    // released before the replacing call returns.
    [Fact]
    public void ReplacingTheLogCallbackReleasesThePreviousOne()
    {
        try
        {
            var first = InstallCapturingCallback();
            Assert.True(Gc.IsAlive(first));

            var second = InstallCapturingCallback();
            Assert.False(Gc.IsAlive(first));
            Assert.True(Gc.IsAlive(second));

            Maplibre.LogClearCallback();
            Assert.False(Gc.IsAlive(second));
        }
        finally
        {
            Maplibre.LogClearCallback();
        }
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference InstallCapturingCallback()
    {
        var captured = new object();
        Maplibre.LogSetCallback(
            new LogHandler(
                (_, _, _, _) =>
                {
                    GC.KeepAlive(captured);
                    return false;
                }
            )
        );
        return new WeakReference(captured);
    }

    [Fact]
    public unsafe void TheLogTrampolineCopiesUnknownValuesAndContainsExceptions()
    {
        (uint Severity, uint Event, long Code, string Message)? copied = null;
        LogCallback callback = (severity, @event, code, message) =>
        {
            copied = ((uint)severity, (uint)@event, code, message);
            throw new FormatException("Host callback failed.");
        };
        using var scope = new NativeCallScope();
        var handler = GeneratedValues.NativeLogHandler(new LogHandler(callback), scope);

        Assert.Equal(
            0,
            handler.callback(
                handler.user_data,
                (mln_log_severity)999,
                (mln_log_event)998,
                42,
                scope.CString("é")
            )
        );
        Assert.Equal((999u, 998u, 42L, "é"), copied);
        _ = Maplibre.CVersion();
    }
}
