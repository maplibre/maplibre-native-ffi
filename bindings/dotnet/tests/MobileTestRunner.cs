using Xunit.Runner.Common;
using Xunit.Sdk;
using Xunit.v3;

namespace Maplibre.NativeFfi.Tests;

// Mobile apps bundle their assemblies, so the console runner's DLL-on-disk
// discovery cannot find them. The framework API discovers the loaded assembly.
internal sealed class MobileTestRunner(Action<string> write) : IMessageSink
{
    private readonly object gate = new();
    private readonly Dictionary<string, string> names = new();
    private bool failed;
    private bool finished;

    internal static async Task<int> Run(Action<string> write)
    {
        var sink = new MobileTestRunner(write);
        try
        {
            Xunit.TestContext.SetForInitialization(sink, false, false);
            await using var framework = new XunitTestFramework();
            var assembly = typeof(MobileTestRunner).Assembly;
            var tests = new List<ITestCase>();
            await framework
                .GetDiscoverer(assembly)
                .Find(
                    test =>
                    {
                        tests.Add(test);
                        return ValueTask.FromResult(true);
                    },
                    TestFrameworkOptions.ForDiscovery(new())
                );
            write($"Discovered {tests.Count} test cases");
            await framework
                .GetExecutor(assembly)
                .RunTestCases(tests, sink, TestFrameworkOptions.ForExecution(new()));
            return sink.finished && !sink.failed ? 0 : 1;
        }
        catch (Exception error)
        {
            write(error.ToString());
            return 1;
        }
    }

    public bool OnMessage(IMessageSinkMessage message)
    {
        lock (gate)
        {
            switch (message)
            {
                case ITestStarting starting:
                    names[starting.TestUniqueID] = starting.TestDisplayName;
                    break;
                case ITestFailed failure:
                    failed = true;
                    write($"FAIL {names.GetValueOrDefault(failure.TestUniqueID)}");
                    write(string.Join("\n", failure.Messages));
                    write(string.Join("\n", failure.StackTraces));
                    break;
                case IErrorMetadata error:
                    failed = true;
                    write($"{message.GetType().Name}: {string.Join("\n", error.Messages)}");
                    write(string.Join("\n", error.StackTraces));
                    break;
                case ITestAssemblyFinished summary:
                    finished = true;
                    failed |=
                        summary.TestsTotal == 0
                        || summary.TestsFailed != 0
                        || summary.TestsSkipped != 0
                        || summary.TestsNotRun != 0;
                    write(
                        $"Tests: {summary.TestsTotal}, failed: {summary.TestsFailed}, skipped: {summary.TestsSkipped}, not run: {summary.TestsNotRun}"
                    );
                    break;
            }
        }
        return true;
    }
}
