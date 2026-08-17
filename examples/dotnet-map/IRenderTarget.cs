using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

internal interface IRenderTarget : IDisposable
{
    bool Render();
    void Resize(Viewport viewport);
}

internal static class RenderTargetDriver
{
    internal static void Wait(RenderSessionHandle session, Task operation)
    {
        while (!operation.IsCompleted)
        {
            Service(session);
            Thread.Yield();
        }
        operation.GetAwaiter().GetResult();
    }

    internal static bool Render(RenderSessionHandle session, bool present)
    {
        session.RequestFrame(
            new FrameDemand(
                FrameDemandFlags.IfNeeded
                    | (present ? FrameDemandFlags.Present : FrameDemandFlags.None),
                0,
                0,
                0,
                0
            )
        );
        Service(session);
        using var batch = session.DrainFrameResults();
        return batch.Any(result => result.Disposition == RenderResult.Rendered);
    }

    internal static void Close(RenderSessionHandle session)
    {
        try
        {
            Wait(session, session.DetachAsync());
        }
        catch
        {
            try
            {
                session.Abandon();
            }
            finally
            {
                session.Close();
            }
            throw;
        }
        session.Close();
    }

    internal static void CompleteAttachment(RenderSessionHandle session)
    {
        try
        {
            Wait(session, session.Attachment);
        }
        catch
        {
            try
            {
                session.Abandon();
            }
            finally
            {
                session.Close();
            }
            throw;
        }
    }

    private static void Service(RenderSessionHandle session)
    {
        if (session.Capabilities.Driver == RenderDriverKind.CallerGraphicsThread)
        {
            session.ServiceDriverWork(0);
        }
    }
}

internal static class RenderTargetFactory
{
    public static IRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        RenderTargetMode mode
    )
    {
        if (graphics is OpenGLContext openGl)
        {
            openGl.MakeCurrentForRendering();
        }
        return mode.Kind switch
        {
            RenderTargetModeKind.OwnedTexture => OwnedTextureRenderTarget.Attach(
                graphics,
                map,
                graphics.ReadViewport()
            ),
            RenderTargetModeKind.BorrowedTexture => BorrowedTextureRenderTarget.Attach(
                graphics,
                map,
                graphics.ReadViewport()
            ),
            RenderTargetModeKind.NativeSurface => NativeSurfaceRenderTarget.Attach(
                graphics,
                map,
                graphics.ReadViewport()
            ),
            _ => throw new ArgumentOutOfRangeException(nameof(mode)),
        };
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
        RenderTargetDriver.CompleteAttachment(session);
    }

    public static OwnedTextureRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport
    )
    {
        ITextureCompositor compositor = graphics switch
        {
            MetalContext metal => new MetalTextureCompositor(metal),
            VulkanContext vulkan => new VulkanTextureCompositor(vulkan, viewport),
            OpenGLContext openGl => new OpenGLTextureCompositor(openGl, viewport),
            _ => throw new InvalidOperationException(
                $"Owned textures are not implemented for {graphics.Backend}."
            ),
        };
        try
        {
            var session = graphics switch
            {
                MetalContext metal => RenderSessionHandle.AttachMetalOwnedTexture(
                    map,
                    new MetalOwnedTextureDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Context = metal.Descriptor(),
                    },
                    null
                ),
                VulkanContext vulkan => RenderSessionHandle.AttachVulkanOwnedTexture(
                    map,
                    new VulkanOwnedTextureDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Context = vulkan.Descriptor(),
                    },
                    null
                ),
                OpenGLContext openGl => RenderSessionHandle.AttachOpenGLOwnedTexture(
                    map,
                    new OpenGLOwnedTextureDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Context = openGl.Descriptor(requirePbufferConfig: true),
                    },
                    null
                ),
                _ => throw new InvalidOperationException(
                    $"Owned textures are not implemented for {graphics.Backend}."
                ),
            };
            return new(graphics, compositor, session);
        }
        catch
        {
            compositor.Dispose();
            throw;
        }
    }

    public bool Render()
    {
        if (
            !RenderTargetDriver.Render(session, present: false)
            || !session.TryAcquireFrame(out var frame)
        )
        {
            return false;
        }
        using (frame)
        {
            var presented = graphics switch
            {
                MetalContext => compositor.Draw(frame.GetMetalTexture()),
                VulkanContext => compositor.Draw(frame.GetVulkanTexture()),
                OpenGLContext => compositor.Draw(frame.GetOpenGLTexture()),
                _ => false,
            };
            if (presented)
                graphics.FinishFrame();
            if (graphics is OpenGLContext openGl)
            {
                openGl.FinishGpuWork();
            }
            RenderTargetDriver.Wait(session, frame.ReleaseAsync(GpuSync.CpuComplete));
            return presented;
        }
    }

    public void Resize(Viewport viewport)
    {
        RenderTargetDriver.Wait(session, session.ResizeAsync(viewport.RenderTargetExtent));
        compositor.Resize(viewport);
    }

    public void Dispose()
    {
        try
        {
            RenderTargetDriver.Close(session);
        }
        finally
        {
            compositor.Dispose();
        }
    }
}

internal sealed class BorrowedTextureRenderTarget : IRenderTarget
{
    private readonly IGraphicsContext graphics;
    private readonly ITextureCompositor compositor;
    private readonly RenderSessionHandle session;
    private IDisposable texture;

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
        RenderTargetDriver.CompleteAttachment(session);
    }

    public static BorrowedTextureRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport
    ) =>
        graphics switch
        {
            MetalContext metal => AttachMetal(metal, map, viewport),
            VulkanContext vulkan => AttachVulkan(vulkan, map, viewport),
            OpenGLContext openGl => AttachOpenGL(openGl, map, viewport),
            _ => throw new InvalidOperationException(
                $"Borrowed textures are not implemented for {graphics.Backend}."
            ),
        };

    public bool Render()
    {
        if (!RenderTargetDriver.Render(session, present: false))
            return false;
        var presented = texture switch
        {
            MetalBorrowedTexture metalTexture
                when compositor is MetalTextureCompositor metalCompositor =>
                metalCompositor.DrawTexture(metalTexture.Texture),
            VulkanBorrowedImage vulkanImage
                when compositor is VulkanTextureCompositor vulkanCompositor =>
                vulkanCompositor.DrawImageView(vulkanImage.View),
            OpenGLBorrowedTexture openGlTexture
                when compositor is OpenGLTextureCompositor openGlCompositor => DrawOpenGL(
                openGlCompositor,
                openGlTexture
            ),
            _ => throw new InvalidOperationException("Unsupported borrowed texture compositor."),
        };
        if (presented)
            graphics.FinishFrame();
        return presented;
    }

    public void Resize(Viewport viewport)
    {
        IDisposable replacement;
        Task handover;
        switch (graphics)
        {
            case MetalContext metal:
                var metalTexture = new MetalBorrowedTexture(metal, viewport);
                replacement = metalTexture;
                handover = session.SetMetalBorrowedTextureAsync(Describe(metalTexture, viewport));
                break;
            case VulkanContext vulkan:
                var vulkanImage = new VulkanBorrowedImage(vulkan, viewport);
                replacement = vulkanImage;
                handover = session.SetVulkanBorrowedTextureAsync(
                    Describe(vulkan, vulkanImage, viewport)
                );
                break;
            case OpenGLContext openGl:
                var openGlTexture = new OpenGLBorrowedTexture(openGl, viewport);
                replacement = openGlTexture;
                handover = session.SetOpenGLBorrowedTextureAsync(
                    Describe(openGl, openGlTexture, viewport)
                );
                break;
            default:
                throw new InvalidOperationException(
                    $"Borrowed textures are not implemented for {graphics.Backend}."
                );
        }

        try
        {
            RenderTargetDriver.Wait(session, handover);
        }
        catch
        {
            replacement.Dispose();
            throw;
        }
        var outgoing = texture;
        texture = replacement;
        try
        {
            compositor.Resize(viewport);
        }
        finally
        {
            outgoing.Dispose();
        }
    }

    public void Dispose()
    {
        try
        {
            RenderTargetDriver.Close(session);
        }
        finally
        {
            try
            {
                compositor.Dispose();
            }
            finally
            {
                texture.Dispose();
            }
        }
    }

    private static BorrowedTextureRenderTarget AttachMetal(
        MetalContext metal,
        MapHandle map,
        Viewport viewport
    )
    {
        var texture = new MetalBorrowedTexture(metal, viewport);
        try
        {
            var compositor = new MetalTextureCompositor(metal);
            try
            {
                var session = RenderSessionHandle.AttachMetalBorrowedTexture(
                    map,
                    Describe(texture, viewport),
                    null
                );
                return new(metal, compositor, texture, session);
            }
            catch
            {
                compositor.Dispose();
                throw;
            }
        }
        catch
        {
            texture.Dispose();
            throw;
        }
    }

    private static BorrowedTextureRenderTarget AttachVulkan(
        VulkanContext vulkan,
        MapHandle map,
        Viewport viewport
    )
    {
        var texture = new VulkanBorrowedImage(vulkan, viewport);
        try
        {
            var compositor = new VulkanTextureCompositor(vulkan, viewport);
            try
            {
                var session = RenderSessionHandle.AttachVulkanBorrowedTexture(
                    map,
                    Describe(vulkan, texture, viewport),
                    null
                );
                return new(vulkan, compositor, texture, session);
            }
            catch
            {
                compositor.Dispose();
                throw;
            }
        }
        catch
        {
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
        try
        {
            var compositor = new OpenGLTextureCompositor(openGl, viewport);
            try
            {
                var session = RenderSessionHandle.AttachOpenGLBorrowedTexture(
                    map,
                    Describe(openGl, texture, viewport),
                    null
                );
                return new(openGl, compositor, texture, session);
            }
            catch
            {
                compositor.Dispose();
                throw;
            }
        }
        catch
        {
            texture.Dispose();
            throw;
        }
    }

    private static bool DrawOpenGL(
        OpenGLTextureCompositor compositor,
        OpenGLBorrowedTexture texture
    )
    {
        compositor.DrawTexture(texture.Texture);
        return true;
    }

    private static MetalBorrowedTextureDescriptor Describe(
        MetalBorrowedTexture texture,
        Viewport viewport
    ) =>
        new()
        {
            Extent = viewport.RenderTargetExtent,
            PhysicalWidth = viewport.PhysicalWidth,
            PhysicalHeight = viewport.PhysicalHeight,
            Texture = texture.Pointer,
        };

    private static VulkanBorrowedTextureDescriptor Describe(
        VulkanContext context,
        VulkanBorrowedImage image,
        Viewport viewport
    ) =>
        new()
        {
            Extent = viewport.RenderTargetExtent,
            PhysicalWidth = viewport.PhysicalWidth,
            PhysicalHeight = viewport.PhysicalHeight,
            Context = context.Descriptor(),
            Image = image.ImagePointer,
            ImageView = image.ViewPointer,
            Format = (uint)VulkanBorrowedImage.ImageFormat,
            InitialLayout = (uint)VulkanBorrowedImage.InitialLayout,
            FinalLayout = (uint)VulkanBorrowedImage.FinalLayout,
        };

    private static OpenGLBorrowedTextureDescriptor Describe(
        OpenGLContext context,
        OpenGLBorrowedTexture texture,
        Viewport viewport
    ) =>
        new()
        {
            Extent = viewport.RenderTargetExtent,
            PhysicalWidth = viewport.PhysicalWidth,
            PhysicalHeight = viewport.PhysicalHeight,
            Context = context.Descriptor(requirePbufferConfig: true),
            Texture = texture.Texture,
            Target = texture.Target,
        };
}

internal sealed class NativeSurfaceRenderTarget : IRenderTarget
{
    private readonly RenderSessionHandle session;

    private NativeSurfaceRenderTarget(RenderSessionHandle session)
    {
        this.session = session;
        RenderTargetDriver.CompleteAttachment(session);
    }

    public static NativeSurfaceRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport
    ) =>
        new(
            graphics switch
            {
                MetalContext metal => RenderSessionHandle.AttachMetalSurface(
                    map,
                    new MetalSurfaceDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Layer = metal.LayerPointer(),
                        Context = metal.Descriptor(),
                    },
                    null
                ),
                VulkanContext vulkan => RenderSessionHandle.AttachVulkanSurface(
                    map,
                    new VulkanSurfaceDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Surface = vulkan.SurfacePointer(),
                        Context = vulkan.Descriptor(),
                    },
                    null
                ),
                OpenGLContext openGl => RenderSessionHandle.AttachOpenGLSurface(
                    map,
                    new OpenGLSurfaceDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Surface = openGl.SurfacePointer(),
                        Context = openGl.Descriptor(requirePbufferConfig: false),
                    },
                    null
                ),
                _ => throw new InvalidOperationException(
                    $"Native surfaces are not implemented for {graphics.Backend}."
                ),
            }
        );

    public bool Render() => RenderTargetDriver.Render(session, present: true);

    public void Resize(Viewport viewport) =>
        RenderTargetDriver.Wait(session, session.ResizeAsync(viewport.RenderTargetExtent));

    public void Dispose() => RenderTargetDriver.Close(session);
}
