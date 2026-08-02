using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

internal static partial class WindowsNative
{
    [LibraryImport("user32", EntryPoint = "GetDC")]
    public static partial nint GetDeviceContext(nint hwnd);
}
