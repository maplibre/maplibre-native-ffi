using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Silk.NET.GLFW;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>
/// The native wakes that bring the GLFW thread back from its wait. Each one marks its receiver's
/// work and posts an empty GLFW event, so the wake returns at once and the GLFW thread does the
/// work.
/// </summary>
internal sealed class LoopWakes(Glfw glfw)
{
    /// <summary>The runtime has events to drain.</summary>
    public LoopWake Events { get; } = new(glfw);

    /// <summary>The render session has frame results to drain.</summary>
    public LoopWake Frames { get; } = new(glfw);

    /// <summary>The render session has driver work for the GLFW thread.</summary>
    public LoopWake DriverWork { get; } = new(glfw);

    /// <summary>Attach options for a session that the GLFW thread drives.</summary>
    public RenderSessionAttachOptions AttachOptions(uint ringDepth = 0) =>
        new()
        {
            Driver = RenderDriverKind.CallerGraphicsThread,
            RequestedTextureRingDepth = ringDepth,
            FrameWake = Frames.Wake,
            DriverWorkWake = DriverWork.Wake,
        };
}

internal sealed class LoopWake
{
    private int raised;

    public LoopWake(Glfw glfw)
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
