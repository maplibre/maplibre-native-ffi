using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Log;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class LoggingTests
{
    [BindingSpecTest("BND-120")]
    [Fact]
    public void CanInstallAndClearLogCallback()
    {
        Maplibre.SetLogCallback(_ => true);
        Maplibre.ClearLogCallback();
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
