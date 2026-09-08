using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Log;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class LoggingTests
{
    // MapLibre logs a style load that names an unservable scheme, which drives the installed
    // registration the way a host's own workload would.
    [BindingSpecTest("BND-120")]
    [Fact]
    public void AnInstalledCallbackReceivesRecordsUntilReplacedAndThenCleared()
    {
        var first = new List<LogRecord>();
        var second = new List<LogRecord>();
        try
        {
            Maplibre.SetLogCallback(record =>
            {
                lock (first)
                    first.Add(record);
                return true;
            });
            DriveAFailedStyleLoad("first-logged-scheme");
            Assert.Contains(Copy(first), record => Names(record, "first-logged-scheme"));

            Maplibre.SetLogCallback(record =>
            {
                lock (second)
                    second.Add(record);
                return false;
            });
            DriveAFailedStyleLoad("second-logged-scheme");
            Assert.Contains(Copy(second), record => Names(record, "second-logged-scheme"));
            Assert.DoesNotContain(Copy(first), record => Names(record, "second-logged-scheme"));

            Maplibre.ClearLogCallback();
            DriveAFailedStyleLoad("third-logged-scheme");
            Assert.DoesNotContain(Copy(second), record => Names(record, "third-logged-scheme"));
        }
        finally
        {
            Maplibre.ClearLogCallback();
        }
    }

    private static bool Names(LogRecord record, string scheme) =>
        record.Message.Contains(scheme, StringComparison.Ordinal);

    private static LogRecord[] Copy(List<LogRecord> records)
    {
        lock (records)
            return [.. records];
    }

    // Loads a style whose scheme no provider serves, and returns once the map reports the
    // failure, so every log record the load produced has been dispatched.
    private static void DriveAFailedStyleLoad(string scheme)
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 64, Height = 64 });

        _ = map.SetStyleUrlAsync($"{scheme}://style.json");
        RuntimeEventTestHelpers.WaitForMapEvent(runtime, map, RuntimeEventType.MapLoadingFailed);
    }

    [BindingSpecTest("BND-020")]
    [Fact]
    public void InvalidAsyncSeverityMaskMapsNativeStatus()
    {
        var error = Assert.Throws<InvalidArgumentException>(() =>
            Maplibre.SetAsyncLogSeverities((LogSeverityMask)(1u << 31))
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Equal((int)MaplibreStatus.InvalidArgument, error.RawStatus);
        Assert.Contains("severity", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }

    [BindingSpecTest("BND-120", "BND-121")]
    [Fact]
    public void LogCallbackInstallReplaceClearAndHostFailureUseDocumentedBehavior()
    {
        var records = new List<LogRecord>();
        LogCallback first = record =>
        {
            records.Add(record);
            return true;
        };
        LogCallback second = record =>
        {
            records.Add(record with { Message = "replacement:" + record.Message });
            return false;
        };

        try
        {
            Maplibre.SetLogCallback(first);
            Assert.Equal(
                1u,
                LogCallbackState.EmitForTest(
                    first,
                    (uint)LogSeverity.Warning,
                    (uint)LogEvent.Render,
                    42,
                    "first"
                )
            );

            Maplibre.SetLogCallback(second);
            Assert.Equal(
                0u,
                LogCallbackState.EmitForTest(
                    second,
                    (uint)LogSeverity.Error,
                    (uint)LogEvent.Style,
                    7,
                    "second"
                )
            );

            Maplibre.SetLogCallback(_ => throw new InvalidOperationException("boom"));
            Assert.Equal(
                0u,
                LogCallbackState.EmitForTest(
                    _ => throw new InvalidOperationException("boom"),
                    (uint)LogSeverity.Info,
                    (uint)LogEvent.General,
                    0,
                    "third"
                )
            );

            Maplibre.ClearLogCallback();
        }
        finally
        {
            Maplibre.ClearLogCallback();
        }

        Assert.Collection(
            records,
            first =>
            {
                Assert.Equal(LogSeverity.Warning, first.Severity);
                Assert.Equal((uint)LogSeverity.Warning, first.RawSeverity);
                Assert.Equal(LogEvent.Render, first.Event);
                Assert.Equal((uint)LogEvent.Render, first.RawEvent);
                Assert.Equal(42, first.Code);
                Assert.Equal("first", first.Message);
            },
            second =>
            {
                Assert.Equal(LogSeverity.Error, second.Severity);
                Assert.Equal(LogEvent.Style, second.Event);
                Assert.Equal(7, second.Code);
                Assert.Equal("replacement:second", second.Message);
            }
        );
    }

    [BindingSpecTest("BND-062")]
    [Fact]
    public void UnknownLogEnumValuesPreserveRawValues()
    {
        LogRecord? copiedRecord = null;

        try
        {
            Maplibre.SetLogCallback(record =>
            {
                copiedRecord = record;
                return true;
            });

            Assert.Equal(
                1u,
                LogCallbackState.EmitForTest(
                    record =>
                    {
                        copiedRecord = record;
                        return true;
                    },
                    999,
                    998,
                    0,
                    "unknown"
                )
            );
        }
        finally
        {
            Maplibre.ClearLogCallback();
        }

        Assert.NotNull(copiedRecord);
        Assert.Equal((LogSeverity)999, copiedRecord.Severity);
        Assert.Equal(999u, copiedRecord.RawSeverity);
        Assert.Equal((LogEvent)998, copiedRecord.Event);
        Assert.Equal(998u, copiedRecord.RawEvent);
    }
}
