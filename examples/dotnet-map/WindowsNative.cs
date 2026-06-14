using System.ComponentModel;
using System.Runtime.InteropServices;

namespace Maplibre.Native.Examples.DotnetMap;

internal static partial class WindowsNative
{
    [LibraryImport("user32", EntryPoint = "GetDC")]
    public static partial nint GetDeviceContext(nint hwnd);

    [LibraryImport("user32", EntryPoint = "ReleaseDC")]
    private static partial int ReleaseDeviceContext(nint hwnd, nint hdc);

    public static void ReleaseDeviceContextOrThrow(nint hwnd, nint hdc)
    {
        if (hdc != 0 && ReleaseDeviceContext(hwnd, hdc) == 0)
        {
            throw new Win32Exception(Marshal.GetLastPInvokeError(), "ReleaseDC failed.");
        }
    }
}
