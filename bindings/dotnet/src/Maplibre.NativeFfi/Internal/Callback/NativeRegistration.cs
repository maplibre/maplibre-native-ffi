namespace Maplibre.NativeFfi.Internal.Callback;

/// <summary>
/// The native function pointers, context, and release of one native registration,
/// which the first call that carries it consumes.
/// </summary>
/// <remarks>
/// A registration record holds this object by reference, so every copy of the record
/// shares one claim. The binding copies the addresses unchanged, and native code
/// validates them.
/// </remarks>
internal sealed unsafe class NativeRegistration
{
    private readonly nint[] functions;
    private readonly nint userData;
    private readonly nint releaseUserData;
    private int consumed;

    internal NativeRegistration(nint[] functions, nint userData, nint releaseUserData)
    {
        this.functions = functions;
        this.userData = userData;
        this.releaseUserData = releaseUserData;
    }

    /// <summary>The function pointer of the registration's callback at <paramref name="index"/>.</summary>
    internal void* Function(int index) => (void*)functions[index];

    internal void* UserData => (void*)userData;

    internal void* ReleaseUserData => (void*)releaseUserData;

    /// <summary>
    /// Consumes the registration for one call. A registration that an earlier call
    /// consumed throws an invalid-state <see cref="MaplibreException"/>.
    /// </summary>
    internal void Claim()
    {
        if (Interlocked.Exchange(ref consumed, 1) != 0)
            throw new MaplibreException(
                global::Maplibre.NativeFfi.Status.InvalidState,
                "An earlier call already consumed this native registration."
            );
    }

    /// <summary>Releases the context of a call that native code did not accept.</summary>
    internal void Release()
    {
        if (releaseUserData != 0)
            ((delegate* unmanaged[Cdecl]<void*, void>)releaseUserData)((void*)userData);
    }
}
