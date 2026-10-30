namespace Maplibre.NativeFfi.Tests;

internal static class Program
{
    // Apple installs a UI synchronization context on the entry thread. The
    // headless runner must not post its continuations to that blocked thread.
    private static int Main() =>
        Task.Run(() => MobileTestRunner.Run(Console.WriteLine)).GetAwaiter().GetResult();
}
