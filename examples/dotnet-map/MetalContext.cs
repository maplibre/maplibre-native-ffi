using System.Runtime.InteropServices;
using Maplibre.Native;
using Maplibre.Native.Render;
using Silk.NET.GLFW;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed unsafe class MetalContext : IGraphicsContext
{
    private const ulong MtlPixelFormatRgba8Unorm = 70;
    private const ulong MtlPixelFormatBgra8Unorm = 80;
    private const ulong MtlTextureType2D = 2;
    private const ulong MtlTextureUsageShaderRead = 1;
    private const ulong MtlTextureUsageRenderTarget = 4;
    private readonly GlfwWindow window;
    private nint view;
    private nint device;
    private nint layer;
    private bool closed;

    private MetalContext(GlfwWindow window, nint view, nint device, nint layer)
    {
        this.window = window;
        this.view = view;
        this.device = device;
        this.layer = layer;
    }

    public RenderBackend Backend => RenderBackend.Metal;

    public nint WindowHandle => window.NativeHandle;

    public GlfwWindow Window => window;

    public bool ShouldClose => window.ShouldClose;

    public bool CanRenderFrame => window.CanRenderFrame();

    public static MetalContext Create(string title, int width, int height)
    {
        if (!OperatingSystem.IsMacOS())
        {
            throw new InvalidOperationException("Metal context creation requires macOS.");
        }

        var window = GlfwWindow.Create(
            title,
            width,
            height,
            glfw => glfw.WindowHint(WindowHintClientApi.ClientApi, ClientApi.NoApi)
        );
        nint retainedView = 0;
        nint device = 0;
        nint layer = 0;

        try
        {
            using var pool = MacObjectiveC.AutoreleasePool();
            var cocoaView = GlfwNativeAccess.GetCocoaView(window.Handle);
            if (cocoaView == 0)
            {
                throw new InvalidOperationException("GLFW did not expose a Cocoa NSView.");
            }

            retainedView = MacObjectiveC.Retain(cocoaView);
            device = MacObjectiveC.MetalSystemDefaultDevice();
            if (device == 0)
            {
                throw new InvalidOperationException("MTLCreateSystemDefaultDevice returned nil.");
            }

            layer = MacObjectiveC.AllocInit("CAMetalLayer");
            MacObjectiveC.SendVoid(layer, "setDevice:", device);
            MacObjectiveC.SendVoid(layer, "setPixelFormat:", MtlPixelFormatBgra8Unorm);
            MacObjectiveC.SendVoid(layer, "setOpaque:", true);
            MacObjectiveC.SendVoid(retainedView, "setWantsLayer:", true);
            MacObjectiveC.SendVoid(retainedView, "setLayer:", layer);

            var context = new MetalContext(window, retainedView, device, layer);
            context.Resize(context.ReadViewport());
            Console.WriteLine($"GLFW {window.Glfw.GetVersionString()}, Metal, Cocoa");
            return context;
        }
        catch
        {
            MacObjectiveC.Release(layer);
            MacObjectiveC.Release(device);
            MacObjectiveC.Release(retainedView);
            window.Dispose();
            throw;
        }
    }

    public MetalContextDescriptor Descriptor() =>
        new() { Device = NativePointer.FromBorrowedAddress(device) };

    public NativePointer LayerPointer() => NativePointer.FromBorrowedAddress(layer);

    public NativePointer DevicePointer() => NativePointer.FromBorrowedAddress(device);

    public nint CreateBorrowedTexture(Viewport viewport)
    {
        nint descriptor = 0;
        try
        {
            using var pool = MacObjectiveC.AutoreleasePool();
            descriptor = MacObjectiveC.AllocInit("MTLTextureDescriptor");
            MacObjectiveC.SendVoid(descriptor, "setTextureType:", MtlTextureType2D);
            MacObjectiveC.SendVoid(descriptor, "setPixelFormat:", MtlPixelFormatRgba8Unorm);
            MacObjectiveC.SendVoid(descriptor, "setWidth:", (ulong)viewport.PhysicalWidth);
            MacObjectiveC.SendVoid(descriptor, "setHeight:", (ulong)viewport.PhysicalHeight);
            MacObjectiveC.SendVoid(descriptor, "setDepth:", 1UL);
            MacObjectiveC.SendVoid(descriptor, "setMipmapLevelCount:", 1UL);
            MacObjectiveC.SendVoid(descriptor, "setArrayLength:", 1UL);
            MacObjectiveC.SendVoid(descriptor, "setSampleCount:", 1UL);
            MacObjectiveC.SendVoid(
                descriptor,
                "setUsage:",
                MtlTextureUsageShaderRead | MtlTextureUsageRenderTarget
            );
            var texture = MacObjectiveC.SendPointer(
                device,
                "newTextureWithDescriptor:",
                descriptor
            );
            if (texture == 0)
            {
                throw new InvalidOperationException("Metal borrowed texture creation failed.");
            }

            return texture;
        }
        finally
        {
            MacObjectiveC.Release(descriptor);
        }
    }

    public nint CreateCommandQueue()
    {
        var queue = MacObjectiveC.SendPointer(device, "newCommandQueue");
        if (queue == 0)
        {
            throw new InvalidOperationException("Metal command queue creation failed.");
        }

        return queue;
    }

    public nint CreateRenderPipeline()
    {
        nint source = 0;
        nint vertexName = 0;
        nint fragmentName = 0;
        nint library = 0;
        nint vertex = 0;
        nint fragment = 0;
        nint descriptor = 0;
        var errorOut = Marshal.AllocHGlobal(nint.Size);
        try
        {
            using var pool = MacObjectiveC.AutoreleasePool();
            Marshal.WriteIntPtr(errorOut, 0);
            source = MacObjectiveC.CfString(MetalShaders.TextureCompositor);
            vertexName = MacObjectiveC.CfString("vertex_main");
            fragmentName = MacObjectiveC.CfString("fragment_main");
            library = MacObjectiveC.SendPointer(
                device,
                "newLibraryWithSource:options:error:",
                source,
                0,
                errorOut
            );
            if (library == 0)
            {
                throw new InvalidOperationException(
                    "Metal shader library creation failed: "
                        + MacObjectiveC.ErrorDescription(Marshal.ReadIntPtr(errorOut))
                );
            }

            vertex = MacObjectiveC.SendPointer(library, "newFunctionWithName:", vertexName);
            fragment = MacObjectiveC.SendPointer(library, "newFunctionWithName:", fragmentName);
            if (vertex == 0 || fragment == 0)
            {
                throw new InvalidOperationException("Metal shader function lookup failed.");
            }

            descriptor = MacObjectiveC.AllocInit("MTLRenderPipelineDescriptor");
            MacObjectiveC.SendVoid(descriptor, "setVertexFunction:", vertex);
            MacObjectiveC.SendVoid(descriptor, "setFragmentFunction:", fragment);
            var attachment = MacObjectiveC.SendPointer(
                MacObjectiveC.SendPointer(descriptor, "colorAttachments"),
                "objectAtIndexedSubscript:",
                0
            );
            MacObjectiveC.SendVoid(attachment, "setPixelFormat:", MtlPixelFormatBgra8Unorm);

            Marshal.WriteIntPtr(errorOut, 0);
            var pipeline = MacObjectiveC.SendPointer(
                device,
                "newRenderPipelineStateWithDescriptor:error:",
                descriptor,
                errorOut
            );
            if (pipeline == 0)
            {
                throw new InvalidOperationException(
                    "Metal render pipeline creation failed: "
                        + MacObjectiveC.ErrorDescription(Marshal.ReadIntPtr(errorOut))
                );
            }

            return pipeline;
        }
        finally
        {
            Marshal.FreeHGlobal(errorOut);
            MacObjectiveC.Release(descriptor);
            MacObjectiveC.Release(fragment);
            MacObjectiveC.Release(vertex);
            MacObjectiveC.Release(library);
            if (fragmentName != 0)
            {
                MacObjectiveC.CFRelease(fragmentName);
            }

            if (vertexName != 0)
            {
                MacObjectiveC.CFRelease(vertexName);
            }

            if (source != 0)
            {
                MacObjectiveC.CFRelease(source);
            }
        }
    }

    public nint NextDrawable() => MacObjectiveC.SendPointer(layer, "nextDrawable");

    public nint DrawableTexture(nint drawable) => MacObjectiveC.SendPointer(drawable, "texture");

    public void ReleaseObject(nint obj) => MacObjectiveC.Release(obj);

    public Viewport ReadViewport() => window.ReadViewport();

    public void Resize(Viewport viewport)
    {
        MacObjectiveC.SendSize(
            layer,
            "setDrawableSize:",
            viewport.PhysicalWidth,
            viewport.PhysicalHeight
        );
    }

    public void PollEvents()
    {
        window.PollEvents();
    }

    public void FinishFrame() { }

    public void Dispose()
    {
        if (closed)
        {
            return;
        }

        closed = true;
        if (view != 0)
        {
            MacObjectiveC.SendVoid(view, "setLayer:", 0);
        }

        MacObjectiveC.Release(layer);
        MacObjectiveC.Release(device);
        MacObjectiveC.Release(view);
        layer = 0;
        device = 0;
        view = 0;
        window.Dispose();
    }
}
