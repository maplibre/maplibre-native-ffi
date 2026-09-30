using System.Text;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Tests that change process-global state: the log callback, the network status, standard
/// error, or the finalizer queue that a forced collection drains.
/// </summary>
/// <remarks>
/// xUnit runs this collection alone, after every parallel collection has finished, so no other
/// test's callbacks, leaks, or log records reach these tests.
/// </remarks>
[CollectionDefinition(nameof(GlobalState), DisableParallelization = true)]
public sealed class GlobalState;

/// <summary>Captures what the binding writes to standard error until disposed.</summary>
internal sealed class StandardErrorCapture : IDisposable
{
    private readonly TextWriter previous = Console.Error;
    private readonly StringBuilder text = new();
    private readonly TextWriter writer;

    internal StandardErrorCapture()
    {
        writer = TextWriter.Synchronized(new StringWriter(text));
        Console.SetError(writer);
    }

    // A synchronized writer locks itself around each write, so reading under that lock never
    // sees half a line from the finalizer thread.
    internal string Text
    {
        get
        {
            lock (writer)
                return text.ToString();
        }
    }

    public void Dispose() => Console.SetError(previous);
}
