using System.Diagnostics;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class ProcessExitTests
{
    // Maplibre.NativeFfi.ExitProbe builds beside this assembly. It returns from Main with a
    // runtime, a map, a held request, and the log callback all live.
    [Fact]
    public async Task AProcessExitsCleanlyWithLiveHandlesAndCallbacks()
    {
        var probe = Path.Combine(AppContext.BaseDirectory, "Maplibre.NativeFfi.ExitProbe.dll");
        var start = new ProcessStartInfo(DotnetHost())
        {
            RedirectStandardOutput = true,
            RedirectStandardError = true,
        };
        start.ArgumentList.Add("exec");
        start.ArgumentList.Add(probe);
        using var process = Process.Start(start)!;

        var output = process.StandardOutput.ReadToEndAsync(TestWaits.Token);
        var error = process.StandardError.ReadToEndAsync(TestWaits.Token);
        await process.WaitForExitAsync(TestWaits.Token).WaitAsync(TestWaits.Deadline);

        Assert.True(
            process.ExitCode == 0,
            $"The probe exited with {process.ExitCode}: {await error}"
        );
        Assert.Contains("Exiting with live handles", await output, StringComparison.Ordinal);
    }

    // The host running this suite, or the one the SDK names for child processes.
    private static string DotnetHost()
    {
        var current = Environment.ProcessPath;
        if (
            current is not null
            && Path.GetFileNameWithoutExtension(current).Equals("dotnet", StringComparison.Ordinal)
        )
            return current;
        return Environment.GetEnvironmentVariable("DOTNET_HOST_PATH") ?? "dotnet";
    }
}
