// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A callback that tells the caller when work is ready.
/// </summary>
/// <remarks>
/// <para>Can run on any thread, including the caller's own thread during a
/// library call. Return quickly and don't block.</para>
/// <para>See <c>mln_wake</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Callback">
/// Null to turn the wake off.
/// </param>
public readonly partial record struct Wake(WakeCallback? Callback)
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
    /// <param name="callback">The C function that native code calls as <c>callback</c>.</param>
    /// <param name="userData">The context that native code passes to each function.</param>
    /// <param name="releaseUserData">The C function that releases <paramref name="userData"/>, or <see cref="NativePointer.Null"/>.</param>
    public static Wake Native(
        NativePointer callback,
        NativePointer userData,
        NativePointer releaseUserData
    ) =>
        default(Wake) with
        {
            NativeRegistration = new([callback.Address], userData.Address, releaseUserData.Address),
        };

    internal NativeRegistration? NativeRegistration { get; init; }
}
