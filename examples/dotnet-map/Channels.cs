using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Geo;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

internal abstract record CameraCommand;

internal sealed record CancelTransitionsCommand : CameraCommand;

internal sealed record SetGestureInProgressCommand(bool InProgress) : CameraCommand;

internal sealed record MoveByCommand(double DeltaX, double DeltaY, AnimationOptions? Animation)
    : CameraCommand;

internal sealed record ScaleByCommand(
    double Scale,
    ScreenPoint? Anchor,
    AnimationOptions? Animation
) : CameraCommand;

internal sealed record AdjustBearingCommand(double Delta, AnimationOptions? Animation)
    : CameraCommand;

internal sealed record AdjustPitchCommand(double Delta, AnimationOptions? Animation)
    : CameraCommand;

internal sealed record ResetOrientationCommand(AnimationOptions Animation) : CameraCommand;

/// <summary>Submits decoded input directly through the map's any-thread command API.</summary>
internal sealed class CommandQueue(MapState state)
{
    public void Push(CameraCommand command)
    {
        _ = state.Apply(command);
    }
}

/// <summary>One-bit signal that a frame is worth drawing.</summary>
internal sealed class RenderRequest
{
    private int value = 1;

    public void Set()
    {
        Volatile.Write(ref value, 1);
    }

    public bool Consume()
    {
        return Interlocked.Exchange(ref value, 0) == 1;
    }
}
