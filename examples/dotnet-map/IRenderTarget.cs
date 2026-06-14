using Maplibre.Native.Map;
using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal interface IRenderTarget : IDisposable
{
    bool NeedsReattachOnResize { get; }

    void Render();

    void Resize(Viewport viewport);
}

internal static class RenderTargetFactory
{
    public static IRenderTarget Attach(
        IGraphicsContext graphics,
        MapState mapState,
        RenderTargetMode mode
    )
    {
        ArgumentNullException.ThrowIfNull(graphics);
        ArgumentNullException.ThrowIfNull(mapState);

        return mode.Kind switch
        {
            RenderTargetModeKind.OwnedTexture => AttachOwnedTexture(graphics, mapState),
            RenderTargetModeKind.BorrowedTexture => AttachBorrowedTexture(graphics, mapState),
            RenderTargetModeKind.NativeSurface => AttachNativeSurface(graphics, mapState),
            _ => throw new ArgumentOutOfRangeException(nameof(mode)),
        };
    }

    private static IRenderTarget AttachOwnedTexture(IGraphicsContext graphics, MapState mapState)
    {
        return OwnedTextureRenderTarget.Attach(graphics, mapState.Map, graphics.ReadViewport());
    }

    private static IRenderTarget AttachBorrowedTexture(IGraphicsContext graphics, MapState mapState)
    {
        return BorrowedTextureRenderTarget.Attach(graphics, mapState.Map, graphics.ReadViewport());
    }

    private static IRenderTarget AttachNativeSurface(IGraphicsContext graphics, MapState mapState)
    {
        return NativeSurfaceRenderTarget.Attach(graphics, mapState.Map, graphics.ReadViewport());
    }
}

internal sealed class OwnedTextureRenderTarget : IRenderTarget
{
    private readonly IGraphicsContext graphics;
    private readonly ITextureCompositor compositor;
    private readonly RenderSessionHandle session;

    private OwnedTextureRenderTarget(
        IGraphicsContext graphics,
        ITextureCompositor compositor,
        RenderSessionHandle session
    )
    {
        this.graphics = graphics;
        this.compositor = compositor;
        this.session = session;
    }

    public bool NeedsReattachOnResize => false;

    public static OwnedTextureRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport
    )
    {
        return graphics switch
        {
            MetalContext metal => new OwnedTextureRenderTarget(
                graphics,
                new MetalTextureCompositor(metal),
                RenderSessionHandle.AttachMetalOwnedTexture(
                    map,
                    new MetalOwnedTextureDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Context = metal.Descriptor(),
                    }
                )
            ),
            VulkanContext vulkan => new OwnedTextureRenderTarget(
                graphics,
                new VulkanTextureCompositor(vulkan, viewport),
                RenderSessionHandle.AttachVulkanOwnedTexture(
                    map,
                    new VulkanOwnedTextureDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Context = vulkan.Descriptor(),
                    }
                )
            ),
            OpenGLContext openGl => new OwnedTextureRenderTarget(
                graphics,
                new OpenGLTextureCompositor(openGl, viewport),
                RenderSessionHandle.AttachOpenGLOwnedTexture(
                    map,
                    new OpenGLOwnedTextureDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Context = openGl.Descriptor(),
                    }
                )
            ),
            _ => throw new InvalidOperationException(
                $"Owned textures are not implemented for {graphics.Backend}."
            ),
        };
    }

    public void Render()
    {
        session.RenderUpdate();
        switch (graphics)
        {
            case MetalContext:
                using (var frame = session.AcquireMetalOwnedTextureFrame())
                {
                    compositor.Draw(frame.Frame);
                }
                break;
            case VulkanContext:
                using (var frame = session.AcquireVulkanOwnedTextureFrame())
                {
                    compositor.Draw(frame.Frame);
                }
                break;
            case OpenGLContext:
                using (var frame = session.AcquireOpenGLOwnedTextureFrame())
                {
                    compositor.Draw(frame.Frame);
                }
                break;
            default:
                throw new InvalidOperationException(
                    $"Owned textures are not implemented for {graphics.Backend}."
                );
        }

        graphics.FinishFrame();
    }

    public void Resize(Viewport viewport)
    {
        session.Resize(viewport.LogicalWidth, viewport.LogicalHeight, viewport.ScaleFactor);
        compositor.Resize(viewport);
    }

    public void Dispose()
    {
        compositor.Dispose();
        session.Dispose();
    }
}

internal sealed class BorrowedTextureRenderTarget : IRenderTarget
{
    private readonly IGraphicsContext graphics;
    private readonly ITextureCompositor compositor;
    private readonly IDisposable texture;
    private readonly RenderSessionHandle session;

    private BorrowedTextureRenderTarget(
        IGraphicsContext graphics,
        ITextureCompositor compositor,
        IDisposable texture,
        RenderSessionHandle session
    )
    {
        this.graphics = graphics;
        this.compositor = compositor;
        this.texture = texture;
        this.session = session;
    }

    public bool NeedsReattachOnResize => true;

    public static BorrowedTextureRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport
    )
    {
        return graphics switch
        {
            MetalContext metal => AttachMetal(metal, map, viewport),
            VulkanContext vulkan => AttachVulkan(vulkan, map, viewport),
            OpenGLContext openGl => AttachOpenGL(openGl, map, viewport),
            _ => throw new InvalidOperationException(
                $"Borrowed textures are not implemented for {graphics.Backend}."
            ),
        };
    }

    public void Render()
    {
        session.RenderUpdate();
        switch (texture)
        {
            case MetalBorrowedTexture metalTexture
                when compositor is MetalTextureCompositor metalCompositor:
                metalCompositor.DrawTexture(metalTexture.Texture);
                break;
            case VulkanBorrowedImage vulkanImage
                when compositor is VulkanTextureCompositor vulkanCompositor:
                vulkanCompositor.DrawImageView(vulkanImage.View);
                break;
            case OpenGLBorrowedTexture openGlTexture
                when compositor is OpenGLTextureCompositor openGlCompositor:
                openGlCompositor.DrawTexture(openGlTexture.Texture);
                break;
            default:
                throw new InvalidOperationException("Unsupported borrowed texture compositor.");
        }

        graphics.FinishFrame();
    }

    public void Resize(Viewport viewport)
    {
        _ = viewport;
        throw new InvalidOperationException(
            "Borrowed textures are reattached instead of resized in place."
        );
    }

    public void Dispose()
    {
        compositor.Dispose();
        session.Dispose();
        texture.Dispose();
    }

    private static BorrowedTextureRenderTarget AttachVulkan(
        VulkanContext vulkan,
        MapHandle map,
        Viewport viewport
    )
    {
        var texture = new VulkanBorrowedImage(vulkan, viewport);
        var compositor = new VulkanTextureCompositor(vulkan, viewport);
        try
        {
            var session = RenderSessionHandle.AttachVulkanBorrowedTexture(
                map,
                new VulkanBorrowedTextureDescriptor
                {
                    Extent = viewport.RenderTargetExtent,
                    Context = vulkan.Descriptor(),
                    Image = texture.ImagePointer,
                    ImageView = texture.ViewPointer,
                    Format = (uint)VulkanBorrowedImage.ImageFormat,
                    InitialLayout = (uint)VulkanBorrowedImage.InitialLayout,
                    FinalLayout = (uint)VulkanBorrowedImage.FinalLayout,
                }
            );
            return new BorrowedTextureRenderTarget(vulkan, compositor, texture, session);
        }
        catch
        {
            compositor.Dispose();
            texture.Dispose();
            throw;
        }
    }

    private static BorrowedTextureRenderTarget AttachMetal(
        MetalContext metal,
        MapHandle map,
        Viewport viewport
    )
    {
        var texture = new MetalBorrowedTexture(metal, viewport);
        var compositor = new MetalTextureCompositor(metal);
        try
        {
            var session = RenderSessionHandle.AttachMetalBorrowedTexture(
                map,
                new MetalBorrowedTextureDescriptor
                {
                    Extent = viewport.RenderTargetExtent,
                    Texture = texture.Pointer,
                }
            );
            return new BorrowedTextureRenderTarget(metal, compositor, texture, session);
        }
        catch
        {
            compositor.Dispose();
            texture.Dispose();
            throw;
        }
    }

    private static BorrowedTextureRenderTarget AttachOpenGL(
        OpenGLContext openGl,
        MapHandle map,
        Viewport viewport
    )
    {
        var texture = new OpenGLBorrowedTexture(openGl, viewport);
        var compositor = new OpenGLTextureCompositor(openGl, viewport);
        try
        {
            var session = RenderSessionHandle.AttachOpenGLBorrowedTexture(
                map,
                new OpenGLBorrowedTextureDescriptor
                {
                    Extent = viewport.RenderTargetExtent,
                    Context = openGl.Descriptor(),
                    Texture = texture.Texture,
                    Target = texture.Target,
                }
            );
            return new BorrowedTextureRenderTarget(openGl, compositor, texture, session);
        }
        catch
        {
            compositor.Dispose();
            texture.Dispose();
            throw;
        }
    }
}

internal sealed class NativeSurfaceRenderTarget : IRenderTarget
{
    private readonly IGraphicsContext graphics;
    private readonly RenderSessionHandle session;

    private NativeSurfaceRenderTarget(IGraphicsContext graphics, RenderSessionHandle session)
    {
        this.graphics = graphics;
        this.session = session;
    }

    public bool NeedsReattachOnResize => false;

    public static NativeSurfaceRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport
    )
    {
        return graphics switch
        {
            MetalContext metal => new NativeSurfaceRenderTarget(
                graphics,
                RenderSessionHandle.AttachMetalSurface(
                    map,
                    new MetalSurfaceDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Layer = metal.LayerPointer(),
                        Context = metal.Descriptor(),
                    }
                )
            ),
            VulkanContext vulkan => new NativeSurfaceRenderTarget(
                graphics,
                RenderSessionHandle.AttachVulkanSurface(
                    map,
                    new VulkanSurfaceDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Surface = vulkan.SurfacePointer(),
                        Context = vulkan.Descriptor(),
                    }
                )
            ),
            OpenGLContext openGl => new NativeSurfaceRenderTarget(
                graphics,
                RenderSessionHandle.AttachOpenGLSurface(
                    map,
                    new OpenGLSurfaceDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Surface = openGl.SurfacePointer(),
                        Context = openGl.Descriptor(),
                    }
                )
            ),
            _ => throw new InvalidOperationException(
                $"Native surfaces are not implemented for {graphics.Backend}."
            ),
        };
    }

    public void Render()
    {
        session.RenderUpdate();
        graphics.FinishFrame();
    }

    public void Resize(Viewport viewport)
    {
        session.Resize(viewport.LogicalWidth, viewport.LogicalHeight, viewport.ScaleFactor);
    }

    public void Dispose()
    {
        session.Dispose();
    }
}
