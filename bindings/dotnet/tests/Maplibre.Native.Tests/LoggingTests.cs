using System.Reflection;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Log;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class LoggingTests
{
    [Fact]
    public void CanInstallAndClearLogCallback()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();

        Maplibre.SetLogCallback(_ => true);
        Maplibre.ClearLogCallback();
    }

    [Fact]
    public void InvalidAsyncSeverityMaskMapsNativeStatus()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();

        var error = Assert.Throws<InvalidArgumentException>(() =>
            Maplibre.SetAsyncLogSeverities((LogSeverityMask)(1u << 31))
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Equal((int)MaplibreStatus.InvalidArgument, error.RawStatus);
        Assert.Contains("severity", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }

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
