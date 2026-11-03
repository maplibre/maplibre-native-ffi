// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// The result of a call or of asynchronous work.
/// </summary>
/// <remarks>
/// See <c>mln_status</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public enum Status : int
{
    Ok = 0,

    /// <summary>
    /// An argument is invalid, such as malformed JSON or a value out of range.
    /// </summary>
    InvalidArgument = -1,

    /// <summary>
    /// The call doesn't fit the current state, such as removing a source that a
    /// layer still uses.
    /// </summary>
    InvalidState = -2,

    /// <summary>
    /// The call came from a thread that can't make it.
    /// </summary>
    WrongThread = -3,

    /// <summary>
    /// The platform, the build, or the render target lacks the feature.
    /// </summary>
    Unsupported = -4,

    /// <summary>
    /// The library or the platform hit an error, such as a failed graphics
    /// driver call.
    /// </summary>
    NativeError = -5,

    /// <summary>
    /// The work stopped before it finished, such as when its handle was
    /// released.
    /// </summary>
    Cancelled = -6,

    /// <summary>
    /// Another call is still using the same object.
    /// </summary>
    Busy = -7,

    /// <summary>
    /// The render target can't be drawn to anymore.
    /// </summary>
    TargetLost = -8,

    /// <summary>
    /// Nothing has the given ID.
    /// </summary>
    NotFound = -10,
}
