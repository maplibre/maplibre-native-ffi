using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi;
using Silk.NET.GLFW;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

internal static unsafe class NativeCallbacks
{
    private static readonly Glfw Glfw = CreateGlfw();

    private static Glfw CreateGlfw()
    {
        NativeLibraryResolver.PreloadGlfw();
        return Glfw.GetApi();
    }

    public static NativePointer GlfwGetProcAddress =>
        NativePointer.FromBorrowedAddress(
            (nint)(delegate* unmanaged[Cdecl]<byte*, nint>)&GlfwGetProcAddressCallback
        );

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static nint GlfwGetProcAddressCallback(byte* name)
    {
        var symbol = Marshal.PtrToStringUTF8((nint)name);
        return symbol is null ? 0 : Glfw.GetProcAddress(symbol);
    }
}
