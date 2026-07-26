using Maplibre.Native.Geo;

namespace Maplibre.Native.Camera;

/// <summary>Camera change kinds reported by camera will-change and did-change events.</summary>
/// <remarks>
/// A camera will-change or did-change runtime event carries this value in its raw
/// <c>Code</c> field.
/// </remarks>
public enum CameraChangeMode : uint
{
    /// <summary>The camera reached its new value without an animated transition.</summary>
    Immediate = 0,

    /// <summary>The camera moved as part of an animated transition.</summary>
    Animated = 1,
}

/// <summary>Mutable camera descriptor used for camera snapshots and commands.</summary>
/// <remarks>
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record CameraOptions
{
    public LatLng? Center { get; set; }
    public double? CenterAltitude { get; set; }
    public EdgeInsets? Padding { get; set; }

    /// <summary>
    /// Input-only screen point the camera pivots around. Jump, ease, and fly honor
    /// it; MapLibre leaves it <see langword="null" /> on every read path, including
    /// camera snapshots and the camera-for-bounds helpers.
    /// </summary>
    public ScreenPoint? Anchor { get; set; }
    public double? Zoom { get; set; }
    public double? Bearing { get; set; }
    public double? Pitch { get; set; }
    public double? Roll { get; set; }
    public double? FieldOfView { get; set; }
}

/// <summary>Camera animation descriptor.</summary>
/// <remarks>
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record AnimationOptions
{
    public double? Duration { get; set; }
    public UnitBezier? Easing { get; set; }
    public double? MinimumZoom { get; set; }
    public double? Velocity { get; set; }

    /// <summary>Caller-chosen identity for the transition these options start.</summary>
    /// <remarks>
    /// <para>
    /// When set, the transition emits one map camera transition-finished runtime event carrying
    /// this value in its <c>RuntimeEventPayload.CameraTransitionFinished</c> payload. The value
    /// passes through to MapLibre Native uninterpreted, so callers pick their own scheme, such as
    /// a monotonically increasing counter.
    /// </para>
    /// <para>
    /// Each transition emits that event exactly once, whichever way it ends: running to
    /// completion, being superseded by a later camera command, being cancelled by
    /// <c>MapHandle.CancelTransitions</c>, completing instantly as a zero-duration jump, or exiting
    /// early because the requested camera contained a non-finite value. MapLibre Native reports the
    /// moment a transition releases the camera and does not report which of those outcomes
    /// occurred, so the event establishes transition identity rather than a completion reason. A
    /// host that needs to tell completion from cancellation compares the resulting camera against
    /// the requested one, or tracks which transition ID is current.
    /// </para>
    /// <para>Leaving this property null emits no such event.</para>
    /// </remarks>
    public ulong? TransitionId { get; set; }
}

/// <summary>Camera fitting descriptor.</summary>
/// <remarks>
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record CameraFitOptions
{
    public EdgeInsets? Padding { get; set; }
    public double? Bearing { get; set; }
    public double? Pitch { get; set; }
}

/// <summary>Geographic constraint applied to the map camera center.</summary>
public abstract record BoundsConstraint
{
    private BoundsConstraint() { }

    /// <summary>Keeps the camera center inside the given bounds.</summary>
    public sealed record Bounded(LatLngBounds Bounds) : BoundsConstraint;

    /// <summary>
    /// Leaves the camera center unconstrained, so the map pans freely across the antimeridian.
    /// This differs from world bounds of -90/-180 to 90/180, which clamp longitude to that range.
    /// </summary>
    public sealed record Unbounded : BoundsConstraint
    {
        public static Unbounded Instance { get; } = new();

        private Unbounded() { }
    }
}

/// <summary>Camera bound constraint descriptor.</summary>
/// <remarks>
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record BoundOptions
{
    public BoundsConstraint? Bounds { get; set; }
    public double? MinimumZoom { get; set; }
    public double? MaximumZoom { get; set; }
    public double? MinimumPitch { get; set; }
    public double? MaximumPitch { get; set; }
}

/// <summary>Free camera descriptor.</summary>
/// <remarks>
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record FreeCameraOptions
{
    public Vec3? Position { get; set; }
    public Quaternion? Orientation { get; set; }
}
