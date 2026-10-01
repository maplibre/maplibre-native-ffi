using System.Diagnostics;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>
/// A render session that the GLFW thread drives. The render loop requests frames, services driver
/// work after its wake, and drains frame results after theirs. Each mode composes a rendered frame
/// into the window in <see cref="Present" />.
/// </summary>
internal abstract class RenderTarget : IDisposable
{
    /// <summary>
    /// A session-owned texture ring deep enough to keep compositing while the map renders the next
    /// frame.
    /// </summary>
    protected const uint OwnedTextureRingDepth = 2;

    /// <summary>How long a frame that did not reach the window waits to retry, about one refresh.</summary>
    private const long RetryDelayMilliseconds = 16;

    private readonly GlfwWindow window;
    private bool released;

    protected RenderTarget(IGraphicsContext graphics, RenderSessionHandle session)
    {
        Graphics = graphics;
        Session = session;
        window = graphics.Window;
        try
        {
            AwaitDriverWork(session.Completion);
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

    protected IGraphicsContext Graphics { get; }

    protected RenderSessionHandle Session { get; }

    public static RenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        RenderTargetMode mode,
        LoopWakes wakes
    )
    {
        if (graphics is OpenGLContext openGl)
        {
            openGl.MakeCurrentForRendering();
        }
        var viewport = graphics.ReadViewport();
        return mode.Kind switch
        {
            RenderTargetModeKind.OwnedTexture => OwnedTextureRenderTarget.Attach(
                graphics,
                map,
                viewport,
                wakes
            ),
            RenderTargetModeKind.BorrowedTexture => BorrowedTextureRenderTarget.Attach(
                graphics,
                map,
                viewport,
                wakes
            ),
            RenderTargetModeKind.NativeSurface => NativeSurfaceRenderTarget.Attach(
                graphics,
                map,
                viewport,
                wakes
            ),
            _ => throw new ArgumentOutOfRangeException(nameof(mode)),
        };
    }

    /// <summary>Asks for a frame. A forced frame renders even when the map has no newer update.</summary>
    public void RequestFrame(bool force = false) =>
        Session.RequestFrame(
            FrameDemand.Default with
            {
                Flags = force
                    ? FrameDemandFlag.Present
                    : FrameDemandFlag.IfNeeded | FrameDemandFlag.Present,
            }
        );

    public void ServiceDriverWork() => Session.ServiceDriverWork(0);

    /// <summary>
    /// Drains every frame result and presents each rendered frame. A result that asks for another
    /// frame, as during a paint transition, requests it. A target that was not ready, or a frame
    /// that missed the window, consumed its map update, so a paced retry forces the next frame.
    /// </summary>
    /// <returns>Whether a frame reached the window.</returns>
    public bool DrainFrameResults()
    {
        RenderFrameBatchHandle results;
        try
        {
            results = Session.DrainFrameResults();
        }
        catch (MaplibreException error) when (error.Status == MaplibreStatus.NotReady)
        {
            return false;
        }

        var presented = false;
        var missed = false;
        var needsRepaint = false;
        using (results)
        {
            var count = results.Count();
            for (ulong index = 0; index < count; index++)
            {
                var result = results.Get(index);
                switch (result.Disposition)
                {
                    case RenderResult.Rendered when Present():
                        presented = true;
                        break;
                    case RenderResult.Rendered:
                    case RenderResult.TargetNotReady:
                        missed = true;
                        break;
                    default:
                        continue;
                }
                needsRepaint |= result.NeedsRepaint;
            }
        }

        if (missed)
        {
            RetryAt =
                Stopwatch.GetTimestamp() + Stopwatch.Frequency * RetryDelayMilliseconds / 1000;
        }
        else if (needsRepaint)
        {
            RequestFrame();
        }
        return presented;
    }

    /// <summary>
    /// When a paced retry is due, as a <see cref="Stopwatch.GetTimestamp" /> value, or null with
    /// none pending.
    /// </summary>
    public long? RetryAt { get; private set; }

    /// <summary>Forces the pending paced retry once it is due.</summary>
    public void RetryIfDue()
    {
        if (RetryAt is not { } due || Stopwatch.GetTimestamp() < due)
        {
            return;
        }
        RetryAt = null;
        RequestFrame(force: true);
    }

    /// <summary>
    /// Follows a resized host and keeps the session attached, so its renderer stays warm. The
    /// session resize carries the new extent to the map.
    /// </summary>
    public virtual void Resize(Viewport viewport) =>
        _ = Session.ResizeAsync(viewport.RenderTargetExtent);

    /// <summary>Detaches through the driver, abandoning the session if that fails, then closes it.</summary>
    public virtual void Dispose()
    {
        try
        {
            if (!released)
            {
                AwaitDriverWork(Session.DetachAsync());
            }
        }
        catch
        {
            try
            {
                Session.Abandon();
            }
            finally
            {
                Session.Close();
            }
            throw;
        }
        Session.Close();
    }

    /// <summary>Shows the frame the session just rendered, and reports whether it reached the window.</summary>
    protected abstract bool Present();

    /// <summary>
    /// Hands the session a replacement texture through <paramref name="handover" />, and waits for
    /// it. Frames that ran before the handover drew into the outgoing texture, so they are drained
    /// and presented while that texture is still current. A failed handover leaves it unknown which
    /// texture the session holds, so the session is detached before the caller releases either one.
    /// </summary>
    protected void HandOver(Func<Task> handover)
    {
        try
        {
            AwaitDriverWork(handover());
        }
        catch
        {
            released = true;
            try
            {
                AwaitDriverWork(Session.DetachAsync());
            }
            catch
            {
                Session.Abandon();
            }
            throw;
        }
        DrainFrameResults();
    }

    /// <summary>
    /// Services driver work on the GLFW thread until <paramref name="operation" /> finishes. Between
    /// services the thread waits for the driver-work wake or for the operation, each of which posts
    /// an empty event. This must not run inside a GLFW callback.
    /// </summary>
    private void AwaitDriverWork(Task operation)
    {
        _ = operation.ContinueWith(
            _ => window.Glfw.PostEmptyEvent(),
            TaskContinuationOptions.ExecuteSynchronously
        );
        while (true)
        {
            Session.ServiceDriverWork(0);
            if (operation.IsCompleted)
            {
                break;
            }
            window.WaitEvents();
        }
        operation.GetAwaiter().GetResult();
    }
}

internal sealed class OwnedTextureRenderTarget : RenderTarget
{
    private readonly ITextureCompositor compositor;

    private OwnedTextureRenderTarget(
        IGraphicsContext graphics,
        ITextureCompositor compositor,
        RenderSessionHandle session
    )
        : base(graphics, session)
    {
        this.compositor = compositor;
    }

    public static OwnedTextureRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport,
        LoopWakes wakes
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
            var options = wakes.AttachOptions(OwnedTextureRingDepth);
            var session = graphics switch
            {
                MetalContext metal => map.MetalOwnedTextureAttach(
                    new MetalOwnedTextureDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Context = metal.Descriptor(),
                    },
                    options
                ),
                VulkanContext vulkan => map.VulkanOwnedTextureAttach(
                    new VulkanOwnedTextureDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Context = vulkan.Descriptor(),
                    },
                    options
                ),
                OpenGLContext openGl => map.OpenglOwnedTextureAttach(
                    new OpenglOwnedTextureDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Context = openGl.Descriptor(requirePbufferConfig: true),
                    },
                    options
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

    protected override bool Present()
    {
        // An empty ring leaves the previously composited frame on screen, and nothing new reaches
        // the window.
        AcquiredFrameHandle frame;
        try
        {
            frame = Session.AcquireFrame();
        }
        catch (MaplibreException error) when (error.Status == MaplibreStatus.NotReady)
        {
            return false;
        }
        using (frame)
        {
            var presented = false;
            switch (Graphics)
            {
                case MetalContext:
                    frame.WithMetalTexture(view => presented = compositor.Draw(view));
                    break;
                case VulkanContext:
                    frame.WithVulkanTexture(view => presented = compositor.Draw(view));
                    break;
                case OpenGLContext openGl:
                    frame.WithOpenglTexture(view =>
                    {
                        presented = compositor.Draw(view);
                        openGl.FinishGpuWork();
                    });
                    break;
            }
            if (presented)
            {
                Graphics.FinishFrame();
            }
            frame.Release(GpuSync.Default);
            return presented;
        }
    }

    public override void Resize(Viewport viewport)
    {
        compositor.Resize(viewport);
        base.Resize(viewport);
    }

    public override void Dispose()
    {
        try
        {
            base.Dispose();
        }
        finally
        {
            compositor.Dispose();
        }
    }
}

internal sealed class BorrowedTextureRenderTarget : RenderTarget
{
    private readonly ITextureCompositor compositor;
    private readonly MapHandle map;
    private IDisposable texture;

    private BorrowedTextureRenderTarget(
        IGraphicsContext graphics,
        ITextureCompositor compositor,
        IDisposable texture,
        RenderSessionHandle session,
        MapHandle map
    )
        : base(graphics, session)
    {
        this.compositor = compositor;
        this.texture = texture;
        this.map = map;
    }

    public static BorrowedTextureRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport,
        LoopWakes wakes
    ) =>
        graphics switch
        {
            MetalContext metal => AttachMetal(metal, map, viewport, wakes),
            VulkanContext vulkan => AttachVulkan(vulkan, map, viewport, wakes),
            OpenGLContext openGl => AttachOpenGL(openGl, map, viewport, wakes),
            _ => throw new InvalidOperationException(
                $"Borrowed textures are not implemented for {graphics.Backend}."
            ),
        };

    protected override bool Present()
    {
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
        {
            Graphics.FinishFrame();
        }
        return presented;
    }

    /// <summary>Allocates a texture at the new size and hands it to the live session.</summary>
    public override void Resize(Viewport viewport)
    {
        compositor.Resize(viewport);
        IDisposable replacement;
        Func<Task> handover;
        switch (Graphics)
        {
            case MetalContext metal:
                var metalTexture = new MetalBorrowedTexture(metal, viewport);
                replacement = metalTexture;
                handover = () =>
                    Session.MetalBorrowedTextureSetTargetAsync(Describe(metalTexture, viewport));
                break;
            case VulkanContext vulkan:
                var vulkanImage = new VulkanBorrowedImage(vulkan, viewport);
                replacement = vulkanImage;
                handover = () =>
                    Session.VulkanBorrowedTextureSetTargetAsync(
                        Describe(vulkan, vulkanImage, viewport)
                    );
                break;
            case OpenGLContext openGl:
                var openGlTexture = new OpenGLBorrowedTexture(openGl, viewport);
                replacement = openGlTexture;
                handover = () =>
                    Session.OpenglBorrowedTextureSetTargetAsync(
                        Describe(openGl, openGlTexture, viewport)
                    );
                break;
            default:
                throw new InvalidOperationException(
                    $"Borrowed textures are not implemented for {Graphics.Backend}."
                );
        }

        try
        {
            HandOver(handover);
        }
        catch
        {
            replacement.Dispose();
            throw;
        }
        texture.Dispose();
        texture = replacement;
        // A handover replaces only the graphics resource, so the map still needs the extent.
        _ = map.ResizeAsync(
            new LogicalExtent(viewport.LogicalWidth, viewport.LogicalHeight, viewport.ScaleFactor)
        );
    }

    public override void Dispose()
    {
        try
        {
            base.Dispose();
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
        Viewport viewport,
        LoopWakes wakes
    )
    {
        var texture = new MetalBorrowedTexture(metal, viewport);
        try
        {
            var compositor = new MetalTextureCompositor(metal);
            try
            {
                var session = map.MetalBorrowedTextureAttach(
                    Describe(texture, viewport),
                    wakes.AttachOptions()
                );
                return new(metal, compositor, texture, session, map);
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
        Viewport viewport,
        LoopWakes wakes
    )
    {
        var texture = new VulkanBorrowedImage(vulkan, viewport);
        try
        {
            var compositor = new VulkanTextureCompositor(vulkan, viewport);
            try
            {
                var session = map.VulkanBorrowedTextureAttach(
                    Describe(vulkan, texture, viewport),
                    wakes.AttachOptions()
                );
                return new(vulkan, compositor, texture, session, map);
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
        Viewport viewport,
        LoopWakes wakes
    )
    {
        var texture = new OpenGLBorrowedTexture(openGl, viewport);
        try
        {
            var compositor = new OpenGLTextureCompositor(openGl, viewport);
            try
            {
                var session = map.OpenglBorrowedTextureAttach(
                    Describe(openGl, texture, viewport),
                    wakes.AttachOptions()
                );
                return new(openGl, compositor, texture, session, map);
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
            Image = image.ImageHandle,
            ImageView = image.ViewHandle,
            Format = (uint)VulkanBorrowedImage.ImageFormat,
            InitialLayout = (uint)VulkanBorrowedImage.InitialLayout,
            FinalLayout = (uint)VulkanBorrowedImage.FinalLayout,
        };

    private static OpenglBorrowedTextureDescriptor Describe(
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

/// <summary>A window surface, where the driver presents each frame it renders.</summary>
internal sealed class NativeSurfaceRenderTarget : RenderTarget
{
    private NativeSurfaceRenderTarget(IGraphicsContext graphics, RenderSessionHandle session)
        : base(graphics, session) { }

    public static NativeSurfaceRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport,
        LoopWakes wakes
    ) =>
        new(
            graphics,
            graphics switch
            {
                MetalContext metal => map.MetalSurfaceAttach(
                    new MetalSurfaceDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Layer = metal.LayerPointer(),
                        Context = metal.Descriptor(),
                    },
                    wakes.AttachOptions()
                ),
                VulkanContext vulkan => map.VulkanSurfaceAttach(
                    new VulkanSurfaceDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Surface = vulkan.SurfaceHandle(),
                        Context = vulkan.Descriptor(),
                    },
                    wakes.AttachOptions()
                ),
                OpenGLContext openGl => map.OpenglSurfaceAttach(
                    new OpenglSurfaceDescriptor
                    {
                        Extent = viewport.RenderTargetExtent,
                        Surface = openGl.SurfacePointer(),
                        Context = openGl.Descriptor(requirePbufferConfig: false),
                    },
                    wakes.AttachOptions()
                ),
                _ => throw new InvalidOperationException(
                    $"Native surfaces are not implemented for {graphics.Backend}."
                ),
            }
        );

    protected override bool Present() => true;
}
