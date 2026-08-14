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

/// <summary>One-bit signal that a frame is worth drawing.</summary>
internal sealed class RenderRequest
{
    private bool requested = true;

    public void Set()
    {
        requested = true;
    }

    public bool Consume()
    {
        var current = requested;
        requested = false;
        return current;
    }
}
