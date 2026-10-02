using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;

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

    internal static string CopyUtf8View(mln_buffer_view view) =>
        RuntimeStructs.CopyUtf8(view.data, view.size);

    /// <summary>Copies a UTF-8 view whose empty value means that no string is present.</summary>
    internal static string? CopyOptionalUtf8View(mln_buffer_view view) =>
        view.size == 0 ? null : CopyUtf8View(view);
}
