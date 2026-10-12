using Silk.NET.GLFW;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>
/// A native wake for the GLFW thread. The wake marks its work and posts an empty GLFW event, so it
/// returns at once and the GLFW thread does the work after its wait ends.
/// </summary>
internal sealed class GlfwWake
{
    private int raised;

    public GlfwWake(Glfw glfw)
    {
        Wake = new Wake(() =>
        {
            Volatile.Write(ref raised, 1);
            glfw.PostEmptyEvent();
        });
    }

    public Wake Wake { get; }

    /// <summary>Reports whether the wake fired since the last call.</summary>
    public bool Consume() => Interlocked.Exchange(ref raised, 0) != 0;
}
