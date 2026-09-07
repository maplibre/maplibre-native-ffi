using Maplibre.NativeFfi.Geo;

namespace Maplibre.NativeFfi.Camera;

/// <summary>Camera change kinds reported in the raw <c>Code</c> field of camera
/// will-change and did-change runtime events.</summary>
public enum CameraChangeMode : uint
{
    /// <summary>The camera reached its new value without an animated transition.</summary>
    Immediate = 0,

    /// <summary>The camera moved as part of an animated transition.</summary>
    Animated = 1,
}

/// <summary>Mutable camera descriptor used for camera snapshots and commands.</summary>
/// <remarks>
/// Compares and hashes by property value; keep an instance unmodified while it is a key in a
/// hash-based collection.
/// </remarks>
public sealed record CameraOptions
{
    public LatLng? Center { get; set; }
    public double? CenterAltitude { get; set; }
    public EdgeInsets? Padding { get; set; }

    /// <summary>
    /// Input-only screen point the camera pivots around. Jump, ease, and fly honor
    /// it; every read path leaves it <see langword="null" />.
    /// </summary>
    public ScreenPoint? Anchor { get; set; }
    public double? Zoom { get; set; }
    public double? Bearing { get; set; }
    public double? Pitch { get; set; }
    public double? Roll { get; set; }
    public double? FieldOfView { get; set; }
}

public enum CameraUpdateMode : uint
{
    Jump = 0,
    Ease = 1,
    Fly = 2,
}

public enum GesturePhase : uint
{
    None = 0,
    Begin = 1,
    Update = 2,
    End = 3,
    Cancel = 4,
}

/// <summary>One copied camera command submitted to a map.</summary>
public sealed record CameraUpdate
{
    public CameraUpdateMode Mode { get; set; }
    public CameraOptions Camera { get; set; } = new();
    public AnimationOptions Animation { get; set; } = new();
    public GesturePhase GesturePhase { get; set; }
}

public enum CameraDeltaKind : uint
{
    Move = 0,
    Scale = 1,
    Bearing = 2,
    Pitch = 3,
}

/// <summary>One relative camera operation.</summary>
public sealed record CameraDelta
{
    public CameraDeltaKind Kind { get; set; }
    public ScreenPoint Offset { get; set; }
    public double Amount { get; set; }
    public ScreenPoint? Anchor { get; set; }
    public AnimationOptions Animation { get; set; } = new();
}

/// <summary>A camera snapshot and its committed generation.</summary>
public readonly record struct CameraSnapshot(CameraOptions Camera, ulong Generation);

/// <summary>Camera animation descriptor.</summary>
/// <remarks>
/// Compares and hashes by property value; keep an instance unmodified while it is a key in a
/// hash-based collection.
/// </remarks>
public sealed record AnimationOptions
{
    public double? Duration { get; set; }
    public UnitBezier? Easing { get; set; }
    public double? MinimumZoom { get; set; }
    public double? Velocity { get; set; }

    /// <summary>Caller-chosen identity for the transition these options start.</summary>
    /// <remarks>
    /// When set, the transition emits exactly one map camera transition-finished runtime event
    /// carrying this value in its <c>RuntimeEventPayload.CameraTransitionFinished</c> payload,
    /// whether it completes, is superseded, or is cancelled; the event establishes transition
    /// identity rather than a completion reason. A rejected camera command starts no transition
    /// and emits no event, and neither does leaving this property null. The value passes through
    /// uninterpreted, so callers pick their own scheme.
    /// </remarks>
    public ulong? TransitionId { get; set; }
}

/// <summary>Camera fitting descriptor.</summary>
/// <remarks>
/// Compares and hashes by property value; keep an instance unmodified while it is a key in a
/// hash-based collection.
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
    /// This differs from world bounds of -90/-180 to 90/180, which clamp longitude.
    /// </summary>
    public sealed record Unbounded : BoundsConstraint
    {
        public static Unbounded Instance { get; } = new();

        private Unbounded() { }
    }
}

/// <summary>Camera bound constraint descriptor.</summary>
/// <remarks>
/// Compares and hashes by property value; keep an instance unmodified while it is a key in a
/// hash-based collection.
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
/// Compares and hashes by property value; keep an instance unmodified while it is a key in a
/// hash-based collection.
/// </remarks>
public sealed record FreeCameraOptions
{
    public Vec3? Position { get; set; }
    public Quaternion? Orientation { get; set; }
}
