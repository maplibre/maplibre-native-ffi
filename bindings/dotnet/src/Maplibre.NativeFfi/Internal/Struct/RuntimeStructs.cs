using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.Struct;

internal static unsafe class RuntimeStructs
{
    internal static string CopyUtf8(sbyte* pointer, nuint byteLength)
    {
        if (pointer is null || byteLength == 0)
        {
            return string.Empty;
        }

        return Marshal.PtrToStringUTF8((nint)pointer, checked((int)byteLength)) ?? string.Empty;
    }

    internal static string CopyUtf8(void* pointer, nuint byteLength) =>
        CopyUtf8((sbyte*)pointer, byteLength);
}
