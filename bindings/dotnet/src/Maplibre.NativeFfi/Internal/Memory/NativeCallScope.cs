using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;

namespace Maplibre.NativeFfi.Internal.Memory;

/// <summary>
/// The native storage and callback registrations of one native call, released
/// when the call returns.
/// </summary>
/// <remarks>
/// A registration becomes permanent only after <see cref="Accept"/>. Without it, the
/// scope releases the managed registrations, and the contexts of the native
/// registrations that the call consumed.
/// <para>
/// The scope is a ref struct, so no closure can capture it, and its bookkeeping
/// lives in state that each thread reuses, so a call allocates nothing for it.
/// Copies of a scope share that state, and only the declaring call disposes it.
/// </para>
/// </remarks>
internal readonly unsafe ref partial struct NativeCallScope
{
    private readonly State state;

    public NativeCallScope()
    {
        state = State.Rent();
    }

    private sealed class State
    {
        // Calls nest when a synchronous callback calls native, so each thread
        // keeps a stack of free states rather than one.
        [ThreadStatic]
        private static State? free;

        private State? next;
        internal bool rented;
        internal readonly List<nint> allocations = [];
        internal readonly List<NativeCallbackRoot> registrations = [];
        internal readonly List<NativeRegistration> natives = [];
        internal readonly List<object> arguments = [];
        internal object? owner;
        internal bool accepted;

        internal static State Rent()
        {
            var state = free ?? new State();
            free = state.next;
            state.next = null;
            state.rented = true;
            return state;
        }

        internal void Return()
        {
            rented = false;
            owner = null;
            accepted = false;
            next = free;
            free = this;
        }
    }

    internal void* Register(object descriptor)
    {
        var root = new NativeCallbackRoot(descriptor);
        try
        {
            state.registrations.Add(root);
        }
        catch
        {
            root.Dispose();
            throw;
        }
        return root.Pointer;
    }

    /// <summary>
    /// Consumes a native registration for this call. The scope releases its context
    /// unless native code accepts the call.
    /// </summary>
    internal void Consume(NativeRegistration registration)
    {
        registration.Claim();
        try
        {
            state.natives.Add(registration);
        }
        catch
        {
            registration.Release();
            throw;
        }
    }

    internal void Accept() => state.accepted = true;

    private T* Allocate<T>(int count)
        where T : unmanaged
    {
        count = Math.Max(count, 1);
        var pointer = (T*)NativeMemory.Alloc((nuint)count, (nuint)sizeof(T));
        try
        {
            state.allocations.Add((nint)pointer);
        }
        catch
        {
            NativeMemory.Free(pointer);
            throw;
        }
        return pointer;
    }

    internal T* Out<T>()
        where T : unmanaged => Value(default(T));

    internal T* OutputArray<T>(int count)
        where T : unmanaged
    {
        var pointer = Allocate<T>(count);
        new Span<T>(pointer, Math.Max(count, 1)).Clear();
        return pointer;
    }

    internal T* Value<T>(T value)
        where T : unmanaged
    {
        var pointer = Allocate<T>(1);
        *pointer = value;
        return pointer;
    }

    internal static T Read<T>(T* pointer)
        where T : unmanaged
    {
        if (pointer == null)
            throw new InvalidOperationException("Native value pointer is null.");
        return *pointer;
    }

    internal mln_buffer_view Buffer(
        byte[] bytes,
        [CallerArgumentExpression(nameof(bytes))] string? name = null
    )
    {
        ArgumentNullException.ThrowIfNull(bytes, name);
        var pointer = Allocate<byte>(bytes.Length);
        bytes.CopyTo(new Span<byte>(pointer, bytes.Length));
        return new mln_buffer_view { data = pointer, size = (nuint)bytes.Length };
    }

    internal mln_buffer_view Utf8(
        string text,
        [CallerArgumentExpression(nameof(text))] string? name = null
    )
    {
        ArgumentNullException.ThrowIfNull(text, name);
        var count = Encoding.UTF8.GetByteCount(text);
        var pointer = Allocate<byte>(count);
        Encoding.UTF8.GetBytes(text, new Span<byte>(pointer, count));
        return new mln_buffer_view { data = pointer, size = (nuint)count };
    }

    internal T* Array<T>(ReadOnlySpan<T> values)
        where T : unmanaged
    {
        var pointer = Allocate<T>(values.Length);
        values.CopyTo(new Span<T>(pointer, values.Length));
        return pointer;
    }

    internal static T[] CopyArray<T>(T* pointer, nuint count)
        where T : unmanaged
    {
        if (count != 0 && pointer == null)
            throw new InvalidOperationException("Native array is null with a nonzero count.");
        return new ReadOnlySpan<T>(pointer, checked((int)count)).ToArray();
    }

    internal static byte[] CopyValueBytes<T>(T value)
        where T : unmanaged => new ReadOnlySpan<byte>(&value, sizeof(T)).ToArray();

    internal TNative* Array<TNative, TPublic>(
        IReadOnlyList<TPublic> values,
        ScopedConverter<TPublic, TNative> convert,
        [CallerArgumentExpression(nameof(values))] string? name = null
    )
        where TNative : unmanaged
    {
        ArgumentNullException.ThrowIfNull(values, name);
        var pointer = Allocate<TNative>(values.Count);
        for (var index = 0; index < values.Count; index++)
            pointer[index] = convert(values[index], this);
        return pointer;
    }

    internal static TPublic[] CopyArray<TNative, TPublic>(
        TNative* pointer,
        nuint count,
        Func<TNative, TPublic> copy
    )
        where TNative : unmanaged
    {
        if (count != 0 && pointer == null)
            throw new InvalidOperationException("Native array is null with a nonzero count.");
        var result = new TPublic[checked((int)count)];
        for (var index = 0; index < result.Length; index++)
            result[index] = copy(pointer[index]);
        return result;
    }

    public void Dispose()
    {
        if (!state.rented)
            return;
        if (!state.accepted)
        {
            foreach (var root in state.registrations)
                root.Dispose();
            foreach (var registration in state.natives)
                registration.Release();
        }
        state.registrations.Clear();
        state.natives.Clear();
        foreach (var pointer in state.allocations)
            NativeMemory.Free((void*)pointer);
        state.allocations.Clear();
        // The owner and handle arguments stay reachable until the call is over.
        GC.KeepAlive(state.owner);
        state.arguments.Clear();
        state.Return();
    }
}

/// <summary>Converts a value with memory from the scope that it receives.</summary>
internal delegate TNative ScopedConverter<TPublic, TNative>(TPublic value, NativeCallScope scope)
    where TNative : unmanaged;
