// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Host lock on the graphics queue that a session shares with its host, copied
/// by a successful attach. The callback runs on the session's graphics thread
/// or on the thread that abandons the session.
/// </summary>
/// <remarks>
/// See <c>mln_queue_lock</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct QueueLock(QueueLockCallback? Lock, QueueLockCallback? Unlock)
{
    /// <summary>
    /// Creates a native registration from C functions and a context that the
    /// host supplies. The binding copies them unchanged, so native code calls
    /// each function without entering the .NET runtime.
    /// </summary>
    /// <remarks>
    /// The first call that carries this value or a copy of it consumes the
    /// registration. Native code releases the context of a call that it accepts,
    /// and the binding calls the release for a call that native code does not
    /// accept. A later call that carries the registration throws an
    /// invalid-state <see cref="MaplibreException"/> before it reaches
    /// native code.
    /// </remarks>
    /// <param name="lock">The C function that native code calls as <c>lock</c>.</param>
    /// <param name="unlock">The C function that native code calls as <c>unlock</c>.</param>
    /// <param name="userData">The context that native code passes to each function.</param>
    /// <param name="releaseUserData">The C function that releases <paramref name="userData"/>, or <see cref="NativePointer.Null"/>.</param>
    public static QueueLock Native(
        NativePointer @lock,
        NativePointer unlock,
        NativePointer userData,
        NativePointer releaseUserData
    ) =>
        default(QueueLock) with
        {
            NativeRegistration = new(
                [@lock.Address, unlock.Address],
                userData.Address,
                releaseUserData.Address
            ),
        };

    internal NativeRegistration? NativeRegistration { get; init; }
}
