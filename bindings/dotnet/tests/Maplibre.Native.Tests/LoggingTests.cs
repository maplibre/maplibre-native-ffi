using System.Reflection;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Log;
using Xunit;

namespace Maplibre.Native.Tests;

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
    public unsafe void LogCallbackInstallReplaceClearAndHostFailureUseDocumentedBehavior()
    {
        using var methods = LogCallbackState.UseCallbackMethodsForTest(
            (_, _) => mln_status.MLN_STATUS_OK,
            () => mln_status.MLN_STATUS_OK
        );
        var records = new List<LogRecord>();

        try
        {
            Maplibre.SetLogCallback(record =>
            {
                records.Add(record);
                return true;
            });
            Assert.Equal(
                1u,
                LogCallbackState.EmitForTest(
                    (uint)LogSeverity.Warning,
                    (uint)LogEvent.Render,
                    42,
                    "first"
                )
            );

            Maplibre.SetLogCallback(record =>
            {
                records.Add(record with { Message = "replacement:" + record.Message });
                return false;
            });
            Assert.Equal(
                0u,
                LogCallbackState.EmitForTest(
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
                    (uint)LogSeverity.Info,
                    (uint)LogEvent.General,
                    0,
                    "third"
                )
            );

            Maplibre.ClearLogCallback();
            Assert.Equal(
                0u,
                LogCallbackState.EmitForTest(
                    (uint)LogSeverity.Info,
                    (uint)LogEvent.General,
                    0,
                    "after"
                )
            );
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

            Assert.Equal(1u, LogCallbackState.EmitForTest(999, 998, 0, "unknown"));
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

    [BindingSpecTest("BND-026", "BND-122")]
    [Fact]
    public unsafe void LogCallbackInstallFailurePreservesPreviousCallbackAndReleasesReplacement()
    {
        var failInstall = false;
        LogCallbackState? failedReplacement = null;
        var diagnostic = "install failed";
        using var methods = LogCallbackState.UseCallbackMethodsForTest(
            (_, userData) =>
            {
                if (!failInstall)
                {
                    return mln_status.MLN_STATUS_OK;
                }

                failedReplacement = LogCallbackState.StateForTokenForTest((nint)userData);
                return mln_status.MLN_STATUS_INVALID_STATE;
            },
            () => mln_status.MLN_STATUS_OK
        );
        using var diagnostics = NativeStatus.UseDiagnosticProviderForTest(() => diagnostic);

        Maplibre.SetLogCallback(_ => true);
        var previous = Assert.IsType<LogCallbackState>(LogCallbackState.CurrentForTest);

        try
        {
            failInstall = true;
            var error = Assert.Throws<InvalidStateException>(() =>
                Maplibre.SetLogCallback(_ => false)
            );

            Assert.Same(previous, LogCallbackState.CurrentForTest);
            Assert.False(previous.IsRetiredForTest);
            Assert.NotNull(failedReplacement);
            Assert.True(failedReplacement.IsRetiredForTest);
            Assert.Equal("install failed", error.Diagnostic);
        }
        finally
        {
            Maplibre.ClearLogCallback();
        }
    }

    [BindingSpecTest("BND-123")]
    [Fact]
    public void LogCallbackStateDisposeIsIdempotent()
    {
        var state = Assert.IsAssignableFrom<IDisposable>(
            Activator.CreateInstance(
                typeof(LogCallbackState),
                BindingFlags.Instance | BindingFlags.NonPublic,
                binder: null,
                args: [new LogCallback(_ => true)],
                culture: null
            )
        );

        state.Dispose();
        state.Dispose();
    }
}
