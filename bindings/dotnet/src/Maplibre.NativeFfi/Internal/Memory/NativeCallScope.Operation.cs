using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Loader;
using Maplibre.NativeFfi.Internal.Pointer;

namespace Maplibre.NativeFfi.Internal.Memory;

/// <summary>
/// The scope of one generated operation. It keeps the receiver and handle
/// arguments reachable until disposal, and accepts its registrations once
/// native code admits the operation.
/// </summary>
internal readonly unsafe ref partial struct NativeCallScope
{
    internal NativeCallScope(INativeOwner? owner)
        : this()
    {
        if (owner is null)
            NativeLibraryLoader.EnsureLoaded();
        state.owner = owner;
    }

    /// <summary>
    /// Keeps an owner argument reachable for this call and returns its raw id.
    /// </summary>
    internal T Use<T>(
        INativeOwner<T>? argument,
        [CallerArgumentExpression(nameof(argument))] string? name = null
    )
        where T : unmanaged, IMlnHandle
    {
        ArgumentNullException.ThrowIfNull(argument, name);
        state.arguments.Add(argument);
        return argument.State.Handle;
    }

    // Native releases accepted registrations when their callbacks are quiescent.
    internal Task<T> Accepted<T>(Task<T> task)
    {
        Accept();
        return task;
    }
}
