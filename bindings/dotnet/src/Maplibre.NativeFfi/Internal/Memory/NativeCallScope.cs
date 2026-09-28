using System.Runtime.InteropServices;
using System.Text;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;

namespace Maplibre.NativeFfi.Internal.Memory;

internal sealed unsafe class NativeCallScope : IDisposable
{
    private readonly List<nint> allocations = [];
    private readonly List<NativeCallbackRoot> registrations = [];
    private bool accepted;

    internal void* Register(object descriptor)
    {
        var root = new NativeCallbackRoot(descriptor);
        try
        {
            registrations.Add(root);
        }
        catch
        {
            root.Dispose();
            throw;
        }
        return root.Pointer;
    }

    internal void Accept(NativeCallbackOwner? owner = null)
    {
        accepted = true;
        foreach (var root in registrations)
            root.Retain(owner ?? NativeCallbackOwner.Global);
    }

    private T* Allocate<T>(int count)
        where T : unmanaged
    {
        count = Math.Max(count, 1);
        var pointer = (T*)NativeMemory.Alloc((nuint)count, (nuint)sizeof(T));
        try
        {
            allocations.Add((nint)pointer);
        }
        catch
        {
            NativeMemory.Free(pointer);
            throw;
        }
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

    internal sbyte* CString(string text)
    {
        ArgumentNullException.ThrowIfNull(text);
        if (text.Contains('\0'))
            throw new ArgumentException("String contains a null character.", nameof(text));
        var count = Encoding.UTF8.GetByteCount(text);
        var pointer = Allocate<byte>(checked(count + 1));
        Encoding.UTF8.GetBytes(text, new Span<byte>(pointer, count));
        pointer[count] = 0;
        return (sbyte*)pointer;
    }

    internal static string CopyCString(sbyte* pointer) =>
        pointer == null
            ? throw new InvalidOperationException("Native string pointer is null.")
            : Marshal.PtrToStringUTF8((nint)pointer)!;

    internal mln_buffer_view Buffer(byte[] bytes)
    {
        ArgumentNullException.ThrowIfNull(bytes);
        var pointer = Allocate<byte>(bytes.Length);
        bytes.CopyTo(new Span<byte>(pointer, bytes.Length));
        return new mln_buffer_view { data = pointer, size = (nuint)bytes.Length };
    }

    internal mln_buffer_view Utf8(string text)
    {
        ArgumentNullException.ThrowIfNull(text);
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
        Func<TPublic, TNative> convert
    )
        where TNative : unmanaged
    {
        ArgumentNullException.ThrowIfNull(values);
        var pointer = Allocate<TNative>(values.Count);
        for (var index = 0; index < values.Count; index++)
            pointer[index] = convert(values[index]);
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
        if (!accepted)
            foreach (var root in registrations)
                root.Dispose();
        registrations.Clear();
        foreach (var pointer in allocations)
            NativeMemory.Free((void*)pointer);
        allocations.Clear();
    }
}
