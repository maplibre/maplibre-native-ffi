using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class LoggingTests
{
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
                scope.Utf8("é")
            )
        );
        Assert.Equal((999u, 998u, 42L, "é"), copied);
    }
}
