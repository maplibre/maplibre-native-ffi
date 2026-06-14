using Maplibre.Native;
using Maplibre.Native.Render;
using Silk.NET.GLFW;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed unsafe class MetalContext : IGraphicsContext
{
    private const ulong MtlPixelFormatBgra8Unorm = 80;
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

    public bool ShouldClose => window.ShouldClose;

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

    public MetalContextDescriptor Descriptor() => new() { Device = new NativePointer(device) };

    public NativePointer LayerPointer() => new(layer);

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
