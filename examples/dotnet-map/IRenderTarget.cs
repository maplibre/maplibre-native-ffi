using Maplibre.Native.Map;
using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal interface IRenderTarget : IDisposable
{
    bool NeedsReattachOnResize { get; }

    bool Render();

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

        return Attach(graphics, mapState.Map, mode);
    }

    public static IRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        RenderTargetMode mode
    )
    {
        ArgumentNullException.ThrowIfNull(graphics);
        ArgumentNullException.ThrowIfNull(map);

        return mode.Kind switch
        {
            RenderTargetModeKind.OwnedTexture => AttachOwnedTexture(graphics, map),
            RenderTargetModeKind.BorrowedTexture => AttachBorrowedTexture(graphics, map),
            RenderTargetModeKind.NativeSurface => AttachNativeSurface(graphics, map),
            _ => throw new ArgumentOutOfRangeException(nameof(mode)),
        };
    }

    private static IRenderTarget AttachOwnedTexture(IGraphicsContext graphics, MapHandle map)
    {
        return OwnedTextureRenderTarget.Attach(graphics, map, graphics.ReadViewport());
    }

    private static IRenderTarget AttachBorrowedTexture(IGraphicsContext graphics, MapHandle map)
    {
        return BorrowedTextureRenderTarget.Attach(graphics, map, graphics.ReadViewport());
    }

    private static IRenderTarget AttachNativeSurface(IGraphicsContext graphics, MapHandle map)
    {
        return NativeSurfaceRenderTarget.Attach(graphics, map, graphics.ReadViewport());
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
            MetalContext metal => AttachWithCleanup(
                metal,
                () =>
                    RenderSessionHandle.AttachMetalOwnedTexture(
                        map,
                        new MetalOwnedTextureDescriptor
                        {
                            Extent = viewport.RenderTargetExtent,
                            Context = metal.Descriptor(),
                        }
                    ),
                () => new MetalTextureCompositor(metal)
            ),
            VulkanContext vulkan => AttachWithCleanup(
                vulkan,
                () =>
                    RenderSessionHandle.AttachVulkanOwnedTexture(
                        map,
                        new VulkanOwnedTextureDescriptor
                        {
                            Extent = viewport.RenderTargetExtent,
                            Context = vulkan.Descriptor(),
                        }
                    ),
                () => new VulkanTextureCompositor(vulkan, viewport)
            ),
            OpenGLContext openGl => AttachWithCleanup(
                openGl,
                () =>
                    RenderSessionHandle.AttachOpenGLOwnedTexture(
                        map,
                        new OpenGLOwnedTextureDescriptor
                        {
                            Extent = viewport.RenderTargetExtent,
                            Context = openGl.Descriptor(requirePbufferConfig: true),
                        }
                    ),
                () => new OpenGLTextureCompositor(openGl, viewport)
            ),
            _ => throw new InvalidOperationException(
                $"Owned textures are not implemented for {graphics.Backend}."
            ),
        };
    }

    private static OwnedTextureRenderTarget AttachWithCleanup(
        IGraphicsContext graphics,
        Func<RenderSessionHandle> attachSession,
        Func<ITextureCompositor> createCompositor
    )
    {
        RenderSessionHandle? session = null;
        ITextureCompositor? compositor = null;
        try
        {
            session = attachSession();
            compositor = createCompositor();
            return new OwnedTextureRenderTarget(graphics, compositor, session);
        }
        catch
        {
            DisposeAfterFailure(compositor);
            DisposeAfterFailure(session);
            throw;
        }
    }

    public bool Render()
    {
        session.RenderUpdate();
        var presented = false;
        switch (graphics)
        {
            case MetalContext:
                using (var frame = session.AcquireMetalOwnedTextureFrame())
                {
                    presented = compositor.Draw(frame.Frame);
                }
                break;
            case VulkanContext:
                using (var frame = session.AcquireVulkanOwnedTextureFrame())
                {
                    presented = compositor.Draw(frame.Frame);
                }
                break;
            case OpenGLContext:
                using (var frame = session.AcquireOpenGLOwnedTextureFrame())
                {
                    presented = compositor.Draw(frame.Frame);
                }
                break;
            default:
                throw new InvalidOperationException(
                    $"Owned textures are not implemented for {graphics.Backend}."
                );
        }

        if (presented)
        {
            graphics.FinishFrame();
        }

        return presented;
    }

    public void Resize(Viewport viewport)
    {
        session.Resize(viewport.LogicalWidth, viewport.LogicalHeight, viewport.ScaleFactor);
        compositor.Resize(viewport);
    }

    public void Dispose()
    {
        try
        {
            compositor.Dispose();
        }
        finally
        {
            session.Dispose();
        }
    }

    private static void DisposeAfterFailure(IDisposable? disposable)
    {
        if (disposable is null)
        {
            return;
        }

        try
        {
            disposable.Dispose();
        }
        catch
        {
            // Preserve the setup failure that triggered cleanup.
        }
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

    public bool Render()
    {
        session.RenderUpdate();
        var presented = true;
        switch (texture)
        {
            case MetalBorrowedTexture metalTexture
                when compositor is MetalTextureCompositor metalCompositor:
                metalCompositor.DrawTexture(metalTexture.Texture);
                break;
            case VulkanBorrowedImage vulkanImage
                when compositor is VulkanTextureCompositor vulkanCompositor:
                presented = vulkanCompositor.DrawImageView(vulkanImage.View);
                break;
            case OpenGLBorrowedTexture openGlTexture
                when compositor is OpenGLTextureCompositor openGlCompositor:
                openGlCompositor.DrawTexture(openGlTexture.Texture);
                break;
            default:
                throw new InvalidOperationException("Unsupported borrowed texture compositor.");
        }

        if (presented)
        {
            graphics.FinishFrame();
        }

        return presented;
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
        try
        {
            compositor.Dispose();
        }
        finally
        {
            try
            {
                session.Dispose();
            }
            finally
            {
                texture.Dispose();
            }
        }
    }

    private static BorrowedTextureRenderTarget AttachVulkan(
        VulkanContext vulkan,
        MapHandle map,
        Viewport viewport
    )
    {
        VulkanBorrowedImage? texture = null;
        VulkanTextureCompositor? compositor = null;
        RenderSessionHandle? session = null;
        try
        {
            texture = new VulkanBorrowedImage(vulkan, viewport);
            compositor = new VulkanTextureCompositor(vulkan, viewport);
            session = RenderSessionHandle.AttachVulkanBorrowedTexture(
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
            DisposeAfterFailure(compositor);
            DisposeAfterFailure(session);
            DisposeAfterFailure(texture);
            throw;
        }
    }

    private static BorrowedTextureRenderTarget AttachMetal(
        MetalContext metal,
        MapHandle map,
        Viewport viewport
    )
    {
        MetalBorrowedTexture? texture = null;
        MetalTextureCompositor? compositor = null;
        RenderSessionHandle? session = null;
        try
        {
            texture = new MetalBorrowedTexture(metal, viewport);
            compositor = new MetalTextureCompositor(metal);
            session = RenderSessionHandle.AttachMetalBorrowedTexture(
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
            DisposeAfterFailure(compositor);
            DisposeAfterFailure(session);
            DisposeAfterFailure(texture);
            throw;
        }
    }

    private static BorrowedTextureRenderTarget AttachOpenGL(
        OpenGLContext openGl,
        MapHandle map,
        Viewport viewport
    )
    {
        OpenGLBorrowedTexture? texture = null;
        OpenGLTextureCompositor? compositor = null;
        RenderSessionHandle? session = null;
        try
        {
            texture = new OpenGLBorrowedTexture(openGl, viewport);
            compositor = new OpenGLTextureCompositor(openGl, viewport);
            session = RenderSessionHandle.AttachOpenGLBorrowedTexture(
                map,
                new OpenGLBorrowedTextureDescriptor
                {
                    Extent = viewport.RenderTargetExtent,
                    Context = openGl.Descriptor(requirePbufferConfig: true),
                    Texture = texture.Texture,
                    Target = texture.Target,
                }
            );
            return new BorrowedTextureRenderTarget(openGl, compositor, texture, session);
        }
        catch
        {
            DisposeAfterFailure(compositor);
            DisposeAfterFailure(session);
            DisposeAfterFailure(texture);
            throw;
        }
    }

    private static void DisposeAfterFailure(IDisposable? disposable)
    {
        if (disposable is null)
        {
            return;
        }

        try
        {
            disposable.Dispose();
        }
        catch
        {
            // Preserve the setup failure that triggered cleanup.
        }
    }
}

internal sealed class NativeSurfaceRenderTarget : IRenderTarget
{
    private readonly RenderSessionHandle session;

    private NativeSurfaceRenderTarget(RenderSessionHandle session)
    {
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
                RenderSessionHandle.AttachOpenGLSurface(
                    map,
                    new OpenGLSurfaceDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Surface = openGl.SurfacePointer(),
                        Context = openGl.Descriptor(requirePbufferConfig: false),
                    }
                )
            ),
            _ => throw new InvalidOperationException(
                $"Native surfaces are not implemented for {graphics.Backend}."
            ),
        };
    }

    public bool Render()
    {
        session.RenderUpdate();
        return true;
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
