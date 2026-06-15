using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.Native;
using Silk.NET.GLFW;

namespace Maplibre.Native.Examples.DotnetMap;

internal static unsafe class NativeCallbacks
{
    private static readonly Glfw Glfw = Glfw.GetApi();

    public static NativePointer GlfwGetProcAddress =>
        new((nint)(delegate* unmanaged[Cdecl]<byte*, nint>)&GlfwGetProcAddressCallback);

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static nint GlfwGetProcAddressCallback(byte* name)
    {
        var symbol = Marshal.PtrToStringUTF8((nint)name);
        return symbol is null ? 0 : Glfw.GetProcAddress(symbol);
    }
}
