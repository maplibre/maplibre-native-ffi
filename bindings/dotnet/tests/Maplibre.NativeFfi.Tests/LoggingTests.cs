using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class LoggingTests
{
    [BindingSpecTest("")]
    [Fact]
    public void InstalledCallbackReceivesRecordsUntilReplacedAndCleared()
    {
        var first = new System.Collections.Concurrent.ConcurrentQueue<string>();
        var second = new System.Collections.Concurrent.ConcurrentQueue<string>();
        try
        {
            Maplibre.LogSetCallback(
                (_, _, _, message) =>
                {
                    first.Enqueue(message);
                    return 1;
                }
            );
            DriveAFailedStyleLoad("first-logged-scheme");
            Assert.Contains(
                first,
                message => message.Contains("first-logged-scheme", StringComparison.Ordinal)
            );
            Maplibre.LogSetCallback(
                (_, _, _, message) =>
                {
                    second.Enqueue(message);
                    return 0;
                }
            );
            DriveAFailedStyleLoad("second-logged-scheme");
            Assert.Contains(
                second,
                message => message.Contains("second-logged-scheme", StringComparison.Ordinal)
            );
            Assert.DoesNotContain(
                first,
                message => message.Contains("second-logged-scheme", StringComparison.Ordinal)
            );
            Maplibre.LogSetCallback(null);
            DriveAFailedStyleLoad("third-logged-scheme");
            Assert.DoesNotContain(
                second,
                message => message.Contains("third-logged-scheme", StringComparison.Ordinal)
            );
        }
        finally
        {
            Maplibre.LogClearCallback();
        }
    }

    private static void DriveAFailedStyleLoad(string scheme)
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        _ = map.SetStyleUrlAsync($"{scheme}://style.json");
        RuntimeEventTestHelpers.WaitForMapEvent(runtime, map, RuntimeEventType.MapLoadingFailed);
    }

    [BindingSpecTest("")]
    [Fact]
    public void InvalidAsyncSeverityMaskMapsNativeStatus()
    {
        var error = Assert.Throws<InvalidArgumentException>(() =>
            Maplibre.LogSetAsyncSeverityMask(
                (global::Maplibre.NativeFfi.Logging.LogSeverityMask)(1u << 31)
            )
        );
        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Equal((int)MaplibreStatus.InvalidArgument, error.RawStatus);
        Assert.Contains("severity", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }

    [BindingSpecTest("", "")]
    [Fact]
    public unsafe void CallbackCopiesUnknownValuesRejectsReentryAndContainsExceptions()
    {
        (uint Severity, uint Event, long Code, string Message)? copied = null;
        var rejectedReentry = false;
        Func<
            global::Maplibre.NativeFfi.Logging.LogSeverity,
            global::Maplibre.NativeFfi.Logging.LogEvent,
            long,
            string,
            uint
        > callback = (severity, @event, code, message) =>
        {
            copied = ((uint)severity, (uint)@event, code, message);
            try
            {
                _ = Maplibre.CVersion();
            }
            catch (InvalidOperationException)
            {
                rejectedReentry = true;
            }
            throw new FormatException("Host callback failed.");
        };
        using var scope = new NativeCallScope();
        var root = scope.Register(callback);
        delegate* unmanaged[Cdecl]<void*, uint, uint, long, sbyte*, uint> invoke =
            &GeneratedValues.InvokeLogCallback;
        Assert.Equal(0u, invoke(root, 999, 998, 42, scope.CString("é")));
        Assert.Equal((999u, 998u, 42L, "é"), copied);
        Assert.True(rejectedReentry);
        _ = Maplibre.CVersion();
    }
}
