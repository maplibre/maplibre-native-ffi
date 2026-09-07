using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Attaches a session-owned offscreen texture over whichever backend the loaded native library
/// compiled, so one render-session suite covers every preset.
/// </summary>
/// <remarks>
/// Metal takes the system default device; OpenGL takes an EGL display and pbuffer config and lets
/// the core worker own the context, matching the C suite's dedicated-EGL texture fixture.
/// </remarks>
internal abstract partial class OwnedTextureFixture : IDisposable
{
    internal static bool IsAvailable => MetalDevice.IsSupported || EglDisplay.IsSupported;

    /// <summary>Attaches a session over an offscreen texture the session owns.</summary>
    internal abstract RenderSessionHandle Attach(MapHandle map, RenderTargetExtent extent);

    /// <summary>Replaces the session's target, which an owned texture never supports.</summary>
    internal abstract Task SetTargetAsync(RenderSessionHandle session, RenderTargetExtent extent);

    public abstract void Dispose();

    internal static OwnedTextureFixture Create()
    {
        if (MetalDevice.IsSupported)
        {
            return new MetalDevice();
        }
        if (EglDisplay.IsSupported)
        {
            return new EglDisplay();
        }
        throw new InvalidOperationException(
            "No offscreen render fixture exists for the native library's backends."
        );
    }

    private sealed partial class MetalDevice : OwnedTextureFixture
    {
        private readonly nint device = MetalCreateSystemDefaultDevice();

        internal static bool IsSupported =>
            OperatingSystem.IsMacOS()
            && Maplibre.SupportedRenderBackends().HasFlag(RenderBackend.Metal);

        internal override RenderSessionHandle Attach(MapHandle map, RenderTargetExtent extent)
        {
            if (device == 0)
            {
                throw new InvalidOperationException("Metal reported no system default device.");
            }
            return RenderSessionHandle.AttachMetalOwnedTexture(
                map,
                new MetalOwnedTextureDescriptor
                {
                    Extent = extent,
                    Context = new MetalContextDescriptor
                    {
                        Device = NativePointer.FromBorrowedAddress(device),
                    },
                },
                new RenderSessionAttachOptions { Driver = RenderDriverKind.CoreWorker }
            );
        }

        internal override Task SetTargetAsync(
            RenderSessionHandle session,
            RenderTargetExtent extent
        ) =>
            // The owned-texture session rejects a retarget before it reads the layer, so a
            // stand-in pointer reaches that rejection without a CAMetalLayer.
            session.SetMetalSurfaceAsync(
                new MetalSurfaceDescriptor
                {
                    Extent = extent,
                    Layer = NativePointer.FromBorrowedAddress(device),
                    Context = new MetalContextDescriptor
                    {
                        Device = NativePointer.FromBorrowedAddress(device),
                    },
                },
                TestContext.Current.CancellationToken
            );

        public override void Dispose()
        {
            if (device != 0)
            {
                ObjectiveCRelease(device);
            }
        }

        [LibraryImport(
            "/System/Library/Frameworks/Metal.framework/Metal",
            EntryPoint = "MTLCreateSystemDefaultDevice"
        )]
        private static partial nint MetalCreateSystemDefaultDevice();

        [LibraryImport("/usr/lib/libobjc.A.dylib", EntryPoint = "objc_release")]
        private static partial void ObjectiveCRelease(nint value);
    }

    private sealed partial class EglDisplay : OwnedTextureFixture
    {
        private const int EglNone = 0x3038;
        private const int EglSurfaceType = 0x3033;
        private const int EglPbufferBit = 0x0001;
        private const int EglRenderableType = 0x3040;
        private const int EglOpenGLEs3Bit = 0x0040;
        private const int EglRedSize = 0x3024;
        private const int EglGreenSize = 0x3023;
        private const int EglBlueSize = 0x3022;
        private const int EglAlphaSize = 0x3021;
        private const int EglDepthSize = 0x3025;
        private const int EglStencilSize = 0x3026;
        private const int EglOpenGLEsApi = 0x30A0;
        private const int EglPlatformSurfacelessMesa = 0x31DD;

        private readonly nint display;
        private readonly nint config;

        internal EglDisplay()
        {
            display = OpenDisplay();
            if (display == 0)
            {
                throw new InvalidOperationException("No EGL display initialized.");
            }
            if (!eglBindAPI(EglOpenGLEsApi))
            {
                throw new InvalidOperationException("EGL rejected the OpenGL ES API.");
            }
            config = ChooseConfig(display);
        }

        internal static bool IsSupported =>
            Maplibre.SupportedRenderBackends().HasFlag(RenderBackend.OpenGL)
            && Maplibre.SupportedOpenGLContextProviders().HasFlag(OpenGLContextProvider.Egl);

        internal override RenderSessionHandle Attach(MapHandle map, RenderTargetExtent extent) =>
            RenderSessionHandle.AttachOpenGLOwnedTexture(
                map,
                new OpenGLOwnedTextureDescriptor
                {
                    Extent = extent,
                    Context = new EglContextDescriptor
                    {
                        Ownership = OpenGLContextOwnership.Dedicated,
                        Display = NativePointer.FromBorrowedAddress(display),
                        Config = NativePointer.FromBorrowedAddress(config),
                        ClientApi = OpenGLClientApi.Gles,
                    },
                },
                new RenderSessionAttachOptions { Driver = RenderDriverKind.CoreWorker }
            );

        internal override Task SetTargetAsync(
            RenderSessionHandle session,
            RenderTargetExtent extent
        ) =>
            session.SetOpenGLSurfaceAsync(
                new OpenGLSurfaceDescriptor
                {
                    Extent = extent,
                    Context = new EglContextDescriptor
                    {
                        Ownership = OpenGLContextOwnership.Dedicated,
                        Display = NativePointer.FromBorrowedAddress(display),
                        Config = NativePointer.FromBorrowedAddress(config),
                        ClientApi = OpenGLClientApi.Gles,
                    },
                },
                TestContext.Current.CancellationToken
            );

        public override void Dispose()
        {
            if (display != 0)
            {
                eglTerminate(display);
            }
        }

        private static nint OpenDisplay()
        {
            // A headless CI host has no X11 display, so the surfaceless platform comes first and
            // the default display is the fallback for hosts whose EGL lacks it.
            var surfaceless = eglGetPlatformDisplay(EglPlatformSurfacelessMesa, 0, 0);
            if (surfaceless != 0 && eglInitialize(surfaceless, 0, 0))
            {
                return surfaceless;
            }
            var fallback = eglGetDisplay(0);
            if (fallback != 0 && fallback != surfaceless && eglInitialize(fallback, 0, 0))
            {
                return fallback;
            }
            return 0;
        }

        private static unsafe nint ChooseConfig(nint display)
        {
            int[] attributes =
            [
                EglSurfaceType,
                EglPbufferBit,
                EglRenderableType,
                EglOpenGLEs3Bit,
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
            ];
            nint chosen = 0;
            var count = 0;
            fixed (int* attributePointer = attributes)
            {
                if (
                    !eglChooseConfig(display, (nint)attributePointer, ref chosen, 1, ref count)
                    || count == 0
                )
                {
                    throw new InvalidOperationException(
                        "No EGL config supports OpenGL ES 3 pbuffer rendering."
                    );
                }
            }
            return chosen;
        }

        [LibraryImport("libEGL.so.1")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static partial bool eglBindAPI(int api);

        [LibraryImport("libEGL.so.1")]
        private static partial nint eglGetDisplay(nint displayId);

        [LibraryImport("libEGL.so.1")]
        private static partial nint eglGetPlatformDisplay(
            int platform,
            nint nativeDisplay,
            nint attributes
        );

        [LibraryImport("libEGL.so.1")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static partial bool eglInitialize(nint display, nint major, nint minor);

        [LibraryImport("libEGL.so.1")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static partial bool eglChooseConfig(
            nint display,
            nint attributes,
            ref nint configs,
            int configSize,
            ref int configCount
        );

        [LibraryImport("libEGL.so.1")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static partial bool eglTerminate(nint display);
    }
}
