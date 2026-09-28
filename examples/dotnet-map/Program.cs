using Maplibre.NativeFfi;
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Render;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

internal static class Program
{
    private static int Main(string[] args)
    {
        var parseResult = ParseArgs(args);
        if (parseResult.ShowedHelp)
        {
            return 0;
        }

        if (parseResult.Mode is null)
        {
            return 1;
        }

        try
        {
            Maplibre.LoadNativeLibrary();
            var backends = Maplibre.SupportedRenderBackendMask();
            Console.WriteLine($"native render backends: {backends}");
            if (!SupportsUsableBackend(backends))
            {
                Console.Error.WriteLine(
                    "The loaded MapLibre native library does not support a backend usable by dotnet-map."
                );
                return 1;
            }

            Maplibre.LogSetAsyncSeverityMask(LogSeverityMask.All);
            Maplibre.LogSetCallback(PrintNativeLog);
            try
            {
                Shell.Run(parseResult.Mode.Value, backends);
            }
            finally
            {
                Maplibre.LogClearCallback();
                Maplibre.LogSetAsyncSeverityMask(LogSeverityMask.Default);
            }

            return 0;
        }
        catch (Exception error)
        {
            Console.Error.WriteLine(error.Message);
            return 1;
        }
    }

    private static ParseResult ParseArgs(string[] args)
    {
        if (args is ["--help"])
        {
            PrintUsage(Console.Out);
            return new ParseResult(null, ShowedHelp: true);
        }

        if (args.Length != 1 || args[0].StartsWith("-", StringComparison.Ordinal))
        {
            PrintUsage(Console.Error);
            return new ParseResult(null, ShowedHelp: false);
        }

        if (RenderTargetMode.TryParse(args[0], out var mode))
        {
            return new ParseResult(mode, ShowedHelp: false);
        }

        Console.Error.WriteLine($"Unknown render target mode: {args[0]}");
        PrintUsage(Console.Error);
        return new ParseResult(null, ShowedHelp: false);
    }

    private static void PrintUsage(TextWriter writer)
    {
        writer.WriteLine("Usage: dotnet-map <mode>");
        writer.WriteLine();
        writer.WriteLine("Modes:");
        writer.WriteLine("  owned-texture     session-owned texture render target");
        writer.WriteLine("  borrowed-texture  caller-owned texture render target");
        writer.WriteLine("  native-surface    native surface render target");
    }

    private static bool SupportsUsableBackend(RenderBackendFlag backends)
    {
        return backends.HasFlag(RenderBackendFlag.Metal)
            || backends.HasFlag(RenderBackendFlag.Opengl)
            || backends.HasFlag(RenderBackendFlag.Vulkan);
    }

    private static uint PrintNativeLog(
        LogSeverity severity,
        LogEvent logEvent,
        long code,
        string message
    )
    {
        Console.Error.WriteLine($"MapLibre {severity} {logEvent} {code}: {message}");
        return 1;
    }

    private sealed record ParseResult(RenderTargetMode? Mode, bool ShowedHelp);
}
