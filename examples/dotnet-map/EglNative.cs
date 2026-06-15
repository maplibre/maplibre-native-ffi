using System.Runtime.InteropServices;

namespace Maplibre.Native.Examples.DotnetMap;

internal static unsafe partial class EglNative
{
    private const int EglConfigId = 0x3028;
    private const int EglNone = 0x3038;

    static EglNative()
    {
        NativeLibraryResolver.Register();
    }

    public static nint GetSurfaceConfig(nint display, nint surface)
    {
        if (display == 0 || surface == 0)
        {
            throw new InvalidOperationException("EGL display and surface are required.");
        }

        if (eglQuerySurface(display, surface, EglConfigId, out var configId) == 0)
        {
            throw new InvalidOperationException(
                $"eglQuerySurface(EGL_CONFIG_ID) failed with EGL error 0x{eglGetError():x}."
            );
        }

        var attributes = stackalloc[] { EglConfigId, configId, EglNone };
        nint config = 0;
        var configCount = 0;
        if (
            eglChooseConfig(display, attributes, &config, 1, &configCount) == 0
            || configCount == 0
            || config == 0
        )
        {
            throw new InvalidOperationException(
                $"eglChooseConfig(EGL_CONFIG_ID={configId}) failed with EGL error 0x{eglGetError():x}."
            );
        }

        return config;
    }

    [LibraryImport("EGL", EntryPoint = "eglQuerySurface")]
    private static partial int eglQuerySurface(
        nint display,
        nint surface,
        int attribute,
        out int value
    );

    [LibraryImport("EGL", EntryPoint = "eglChooseConfig")]
    private static partial int eglChooseConfig(
        nint display,
        int* attributes,
        nint* configs,
        int configSize,
        int* configCount
    );

    [LibraryImport("EGL", EntryPoint = "eglGetError")]
    private static partial int eglGetError();
}
