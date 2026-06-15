using System.Runtime.InteropServices;

namespace Maplibre.Native.Examples.DotnetMap;

internal static unsafe partial class EglNative
{
    private const int EglConfigId = 0x3028;
    private const int EglNone = 0x3038;
    private const int EglRedSize = 0x3024;
    private const int EglGreenSize = 0x3023;
    private const int EglBlueSize = 0x3022;
    private const int EglAlphaSize = 0x3021;
    private const int EglDepthSize = 0x3025;
    private const int EglStencilSize = 0x3026;
    private const int EglSurfaceType = 0x3033;
    private const int EglPbufferBit = 0x0001;
    private const int EglRenderableType = 0x3040;
    private const int EglOpenGles3Bit = 0x00000040;

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

    public static nint GetTextureConfig(nint display, nint surface)
    {
        var surfaceConfig = GetSurfaceConfig(display, surface);
        if (SupportsPbuffer(display, surfaceConfig))
        {
            return surfaceConfig;
        }

        return ChoosePbufferConfig(display);
    }

    private static bool SupportsPbuffer(nint display, nint config)
    {
        if (eglGetConfigAttrib(display, config, EglSurfaceType, out var surfaceType) == 0)
        {
            throw new InvalidOperationException(
                $"eglGetConfigAttrib(EGL_SURFACE_TYPE) failed with EGL error 0x{eglGetError():x}."
            );
        }

        return (surfaceType & EglPbufferBit) == EglPbufferBit;
    }

    private static nint ChoosePbufferConfig(nint display)
    {
        if (display == 0)
        {
            throw new InvalidOperationException("EGL display is required.");
        }

        var attributes = stackalloc[] {
            EglSurfaceType,
            EglPbufferBit,
            EglRenderableType,
            EglOpenGles3Bit,
            EglRedSize,
            8,
            EglGreenSize,
            8,
            EglBlueSize,
            8,
            EglAlphaSize,
            8,
            EglDepthSize,
            24,
            EglStencilSize,
            8,
            EglNone,
        };
        nint config = 0;
        var configCount = 0;
        if (
            eglChooseConfig(display, attributes, &config, 1, &configCount) == 0
            || configCount == 0
            || config == 0
        )
        {
            throw new InvalidOperationException(
                $"eglChooseConfig(EGL_PBUFFER_BIT) failed with EGL error 0x{eglGetError():x}."
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

    [LibraryImport("EGL", EntryPoint = "eglGetConfigAttrib")]
    private static partial int eglGetConfigAttrib(
        nint display,
        nint config,
        int attribute,
        out int value
    );

    [LibraryImport("EGL", EntryPoint = "eglGetError")]
    private static partial int eglGetError();
}
