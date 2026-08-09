using System.Runtime.InteropServices;
using System.Text;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Status;

namespace Maplibre.NativeFfi.Internal.Struct;

internal static unsafe class ValueStructs
{
    internal static byte[] CopyBufferView(mln_buffer_view view)
    {
        if (view.size == 0)
        {
            return [];
        }
        if (view.data is null)
        {
            throw new InvalidOperationException("Native buffer data was null with a nonzero size.");
        }
        var bytes = new byte[checked((int)view.size)];
        Marshal.Copy((nint)view.data, bytes, 0, bytes.Length);
        return bytes;
    }

    internal static byte[]? ReadOptionalBuffer(ulong buffer)
    {
        if (buffer == 0)
        {
            return null;
        }
        return ReadBuffer(buffer);
    }

    internal static byte[] ReadBuffer(ulong buffer)
    {
        if (buffer == 0)
        {
            return [];
        }
        try
        {
            mln_buffer_view view = default;
            NativeStatus.Check(NativeMethods.mln_buffer_get(buffer, &view));
            if (view.size == 0)
            {
                return [];
            }
            if (view.data is null)
            {
                throw new InvalidOperationException(
                    "Native buffer data was null with a nonzero size."
                );
            }
            var bytes = new byte[checked((int)view.size)];
            Marshal.Copy((nint)view.data, bytes, 0, bytes.Length);
            return bytes;
        }
        finally
        {
            NativeMethods.mln_buffer_destroy(buffer);
        }
    }
}

internal sealed unsafe class NativeStringView : IDisposable
{
    private readonly nint allocation;
    private readonly mln_buffer_view* pointer;

    private NativeStringView(mln_buffer_view value, nint allocation)
    {
        Value = value;
        this.allocation = allocation;
        pointer = (mln_buffer_view*)NativeMemory.Alloc((nuint)sizeof(mln_buffer_view));
        *pointer = value;
    }

    internal mln_buffer_view Value { get; }
    internal mln_buffer_view* Pointer => pointer;

    internal static NativeStringView From(string value, string parameterName)
    {
        ArgumentNullException.ThrowIfNull(value, parameterName);
        return From(Encoding.UTF8.GetBytes(value), parameterName);
    }

    internal static NativeStringView From(byte[] value, string parameterName)
    {
        ArgumentNullException.ThrowIfNull(value, parameterName);
        var allocation = value.Length == 0 ? 0 : (nint)NativeMemory.Alloc((nuint)value.Length);
        if (allocation != 0)
        {
            Marshal.Copy(value, 0, allocation, value.Length);
        }
        return new NativeStringView(
            new mln_buffer_view { data = (void*)allocation, size = (nuint)value.Length },
            allocation
        );
    }

    public void Dispose()
    {
        if (allocation != 0)
        {
            NativeMemory.Free((void*)allocation);
        }
        NativeMemory.Free(pointer);
    }
}
