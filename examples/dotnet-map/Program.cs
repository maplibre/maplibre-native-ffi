using Maplibre.Native;
using Maplibre.Native.Log;
using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

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
            var backends = Maplibre.SupportedRenderBackends();
            Console.WriteLine($"native render backends: {backends}");
            if (!SupportsUsableBackend(backends))
            {
                Console.Error.WriteLine(
                    "The loaded MapLibre native library does not support a backend usable by dotnet-map on this platform."
                );
                return 1;
            }

            Maplibre.SetAsyncLogSeverities(LogSeverityMask.All);
            Maplibre.SetLogCallback(PrintNativeLog);
            try
            {
                Shell.Run(parseResult.Mode.Value, backends);
            }
            finally
            {
                Maplibre.ClearLogCallback();
                Maplibre.RestoreDefaultAsyncLogSeverities();
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

    private static bool SupportsUsableBackend(RenderBackend backends)
    {
        if (OperatingSystem.IsMacOS())
        {
            return backends.HasFlag(RenderBackend.Metal) || backends.HasFlag(RenderBackend.Vulkan);
        }

        return backends.HasFlag(RenderBackend.OpenGL) || backends.HasFlag(RenderBackend.Vulkan);
    }

    private static bool PrintNativeLog(LogRecord record)
    {
        Console.Error.WriteLine(
            $"MapLibre {record.Severity} {record.Event} {record.Code}: {record.Message}"
        );
        return true;
    }

    private sealed record ParseResult(RenderTargetMode? Mode, bool ShowedHelp);
}
