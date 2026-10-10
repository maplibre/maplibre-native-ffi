using System.Diagnostics;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>
/// Where a session's graphics work runs. A core worker runs it on its own thread. A caller driver
/// queues it for the GLFW thread, which services it after the driver-work wake.
/// <paramref name="queueLock" /> is the host's lock on a queue that the session shares with it.
/// </summary>
internal sealed class SessionDriver(
    RenderDriverKind kind,
    GlfwWindow window,
    QueueLock queueLock = default
)
{
    /// <summary>The driver-work wake, which only a caller driver has.</summary>
    private readonly GlfwWake? work =
        kind == RenderDriverKind.CallerGraphicsThread ? new(window.Glfw) : null;

    public RenderDriverKind Kind => kind;

    public string Label =>
        kind == RenderDriverKind.CoreWorker ? "core-worker" : "caller-graphics-thread";

    private bool Caller => kind == RenderDriverKind.CallerGraphicsThread;

    public RenderSessionAttachOptions AttachOptions(GlfwWake frames, uint ringDepth) =>
        new()
        {
            Driver = kind,
            RequestedTextureRingDepth = ringDepth,
            FrameWake = frames.Wake,
            DriverWorkWake = work?.Wake ?? default,
            QueueLock = queueLock,
        };

    /// <summary>Runs every queued item after the driver-work wake.</summary>
    public void Service(RenderSessionHandle session)
    {
        if (work?.Consume() == true)
        {
            session.ServiceDriverWork(0);
        }
    }

    /// <summary>
    /// Waits for <paramref name="operation" />. A caller driver's operation progresses only through
    /// driver service, so the GLFW thread services the session between waits for the driver-work
    /// wake or for the operation. This must not run inside a GLFW callback.
    /// </summary>
    public void Await(RenderSessionHandle session, Task operation)
    {
        if (Caller)
        {
            _ = operation.ContinueWith(
                _ => window.Glfw.PostEmptyEvent(),
                TaskContinuationOptions.ExecuteSynchronously
            );
            while (true)
            {
                session.ServiceDriverWork(0);
                if (operation.IsCompleted)
                {
                    break;
                }
                window.WaitEvents();
            }
        }
        operation.GetAwaiter().GetResult();
    }

    /// <summary>
    /// Selects the driver from the graphics API: a core worker wherever the target accepts one.
    /// OpenGL on a WGL or EGL context requires the caller driver. A Vulkan core worker shares the
    /// host's queue and takes the host's queue lock around each call on it.
    /// </summary>
    public static SessionDriver For(IGraphicsContext graphics) =>
        graphics switch
        {
            MetalContext => new(RenderDriverKind.CoreWorker, graphics.Window),
            VulkanContext vulkan => new(
                RenderDriverKind.CoreWorker,
                graphics.Window,
                vulkan.QueueLock
            ),
            _ => new(RenderDriverKind.CallerGraphicsThread, graphics.Window),
        };
}

/// <summary>
/// A render session and the frames it shows. The shell demands a frame for each map update, and
/// after the frame wake <see cref="HandleWakes" /> drains the results and shows the newest rendered
/// frame. A native surface presents in the session; the texture modes compose the frame into the
/// window in <see cref="Present" />.
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

    private readonly GlfwWake frames;
    private ulong nextToken;
    private bool demandOutstanding;
    private bool? wanted;
    private bool disposed;

    /// <summary>Starts an attachment with <paramref name="attach" /> and awaits it.</summary>
    protected RenderTarget(
        IGraphicsContext graphics,
        SessionDriver driver,
        uint ringDepth,
        Func<RenderSessionAttachOptions, RenderSessionHandle> attach
    )
    {
        Graphics = graphics;
        Driver = driver;
        frames = new GlfwWake(graphics.Window.Glfw);
        Session = attach(driver.AttachOptions(frames, ringDepth));
        try
        {
            driver.Await(Session, Session.Completion);
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
    }

    protected IGraphicsContext Graphics { get; }

    protected RenderSessionHandle Session { get; }

    public SessionDriver Driver { get; }

    /// <summary>
    /// When a paced retry is due, as a <see cref="Stopwatch.GetTimestamp" /> value, or null with
    /// none pending.
    /// </summary>
    public long? RetryAt { get; private set; }

    /// <summary>
    /// Set by a target whose texture the session and the compositor take turns on. Such a target
    /// keeps at most one demand outstanding, and holds a wanted frame until the outstanding result
    /// arrives.
    /// </summary>
    protected virtual bool ExclusiveTexture => false;

    /// <summary>
    /// Set by a native surface, the only target whose demands ask the session to present.
    /// </summary>
    protected virtual bool Presents => false;

    public static RenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        RenderTargetMode mode
    )
    {
        var driver = SessionDriver.For(graphics);
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
                driver
            ),
            RenderTargetModeKind.BorrowedTexture => BorrowedTextureRenderTarget.Attach(
                graphics,
                map,
                viewport,
                driver
            ),
            RenderTargetModeKind.NativeSurface => NativeSurfaceRenderTarget.Attach(
                graphics,
                map,
                viewport,
                driver
            ),
            _ => throw new ArgumentOutOfRangeException(nameof(mode)),
        };
    }

    /// <summary>Demands a frame. A forced frame renders even when the map has no newer update.</summary>
    public void RequestFrame(bool force = false)
    {
        if (ExclusiveTexture && demandOutstanding)
        {
            wanted = force || wanted == true;
            return;
        }
        demandOutstanding = true;
        FrameDemandFlag flags = force ? 0 : FrameDemandFlag.IfNeeded;
        if (Presents)
        {
            flags |= FrameDemandFlag.Present;
        }
        Session.RequestFrame(FrameDemand.Default with { Flags = flags, Token = ++nextToken });
    }

    /// <summary>
    /// Runs the session's work after its wakes: queued driver work, a due retry, and the frame
    /// results.
    /// </summary>
    /// <returns>Whether a frame reached the window.</returns>
    public bool HandleWakes()
    {
        Driver.Service(Session);
        if (RetryAt is { } due && Stopwatch.GetTimestamp() >= due)
        {
            RetryAt = null;
            RequestFrame(force: true);
        }
        return frames.Consume() && DrainFrameResults();
    }

    /// <summary>Follows a resized host. The session resize carries the new extent to the map.</summary>
    public virtual void Resize(Viewport viewport) =>
        Session.ResizeAsync(viewport.RenderTargetExtent).ReportFailure("render session resize");

    /// <summary>
    /// Releases held frames, detaches, and destroys the session, then releases the mode's host
    /// resources. A failed detach abandons the session, which ends its graphics calls at once.
    /// </summary>
    public void Dispose()
    {
        if (disposed)
        {
            return;
        }
        disposed = true;
        try
        {
            ReleaseFrames();
            Driver.Await(Session, Session.DetachAsync());
        }
        catch (Exception error)
        {
            Console.Error.WriteLine($"render session detach failed, abandoning: {error.Message}");
            var abandoned = Session.Abandon();
            if (abandoned.QuarantinedResourceCount > 0)
            {
                Console.Error.WriteLine(
                    $"render session quarantined {abandoned.QuarantinedResourceCount} resources"
                );
            }
        }
        finally
        {
            Session.Close();
            DisposeHost();
        }
    }

    /// <summary>
    /// Drains every frame result and shows the newest rendered frame. A rendered frame that asks
    /// for another, as during a paint transition, demands it. Neither a target that was not ready
    /// nor a frame that missed the window causes a map-update event, so a retry follows after about
    /// one refresh. The retry is forced, because a frame that missed the window consumed its
    /// update.
    /// </summary>
    /// <returns>Whether a frame reached the window.</returns>
    protected bool DrainFrameResults()
    {
        var results = Session.DrainFrameResults();
        if (results is null)
        {
            return false;
        }

        var rendered = false;
        var retry = false;
        var repaint = false;
        using (results)
        {
            var count = results.Count();
            for (ulong index = 0; index < count; index++)
            {
                var result = results.Get(index);
                demandOutstanding = false;
                switch (result.Disposition)
                {
                    case RenderResult.Rendered:
                        rendered = true;
                        repaint |= result.NeedsRepaint;
                        break;
                    case RenderResult.TargetNotReady:
                        retry = true;
                        break;
                }
            }
        }

        var shown = rendered && Present();
        retry |= rendered && !shown;
        if (retry)
        {
            RetryAt =
                Stopwatch.GetTimestamp() + Stopwatch.Frequency * RetryDelayMilliseconds / 1000;
        }
        else if (repaint)
        {
            RequestFrame();
        }
        if (wanted is { } force && !demandOutstanding)
        {
            wanted = null;
            RequestFrame(force);
        }
        return shown;
    }

    /// <summary>Shows the frame the session rendered, and reports whether it reached the window.</summary>
    protected abstract bool Present();

    /// <summary>Releases the acquired frames that the target holds.</summary>
    protected virtual void ReleaseFrames() { }

    /// <summary>Releases the host resources of the target's mode, after the session detached.</summary>
    protected virtual void DisposeHost() { }

    protected void Await(Task operation) => Driver.Await(Session, operation);
}

/// <summary>
/// A session-owned texture ring. After a rendered result, the target acquires every ready frame,
/// keeps the newest, and composes it. It holds that frame until a newer one replaces it, so the
/// session renders into the ring's other slots meanwhile.
/// </summary>
internal sealed class OwnedTextureRenderTarget : RenderTarget
{
    private readonly ITextureCompositor compositor;
    private AcquiredFrameHandle? held;

    private OwnedTextureRenderTarget(
        IGraphicsContext graphics,
        SessionDriver driver,
        ITextureCompositor compositor,
        Func<RenderSessionAttachOptions, RenderSessionHandle> attach
    )
        : base(graphics, driver, OwnedTextureRingDepth, attach)
    {
        this.compositor = compositor;
    }

    public static OwnedTextureRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport,
        SessionDriver driver
    )
    {
        ITextureCompositor compositor = graphics switch
        {
            MetalContext metal => new MetalTextureCompositor(metal, viewport),
            VulkanContext vulkan => new VulkanTextureCompositor(vulkan, viewport),
            OpenGLContext openGl => new OpenGLTextureCompositor(openGl, viewport),
            _ => throw new InvalidOperationException(
                $"Owned textures are not implemented for {graphics.Backend}."
            ),
        };
        try
        {
            return new(
                graphics,
                driver,
                compositor,
                options =>
                    graphics switch
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
                    }
            );
        }
        catch
        {
            compositor.Dispose();
            throw;
        }
    }

    protected override bool Present()
    {
        AcquiredFrameHandle? newest = null;
        while (Session.AcquireFrame() is { } frame)
        {
            // Nothing read an older frame, so it releases CPU-complete.
            Release(newest);
            newest = frame;
        }
        if (newest is null)
        {
            return held is not null;
        }

        // The compositors finish their reads before they return, so a frame releases CPU-complete.
        var presented = false;
        switch (Graphics)
        {
            case MetalContext:
                newest.WithMetalTexture(view => presented = compositor.Draw(view));
                break;
            case VulkanContext:
                newest.WithVulkanTexture(view => presented = compositor.Draw(view));
                break;
            case OpenGLContext openGl:
                newest.WithOpenglTexture(view =>
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
        Release(held);
        held = newest;
        return presented;
    }

    /// <summary>A session resize needs every frame released, so the held frame goes first.</summary>
    public override void Resize(Viewport viewport)
    {
        ReleaseFrames();
        compositor.Resize(viewport);
        base.Resize(viewport);
    }

    protected override void ReleaseFrames()
    {
        Release(held);
        held = null;
    }

    protected override void DisposeHost() => compositor.Dispose();

    private static void Release(AcquiredFrameHandle? frame)
    {
        if (frame is null)
        {
            return;
        }
        using (frame)
        {
            frame.Release(GpuSync.Default);
        }
    }
}

/// <summary>
/// A caller-owned texture that the session renders into and the compositor samples. The texture
/// belongs to the session from a demand until its result, and to the compositor from a rendered
/// result until the draw returns, so the target keeps one demand outstanding.
/// </summary>
internal sealed class BorrowedTextureRenderTarget : RenderTarget
{
    private readonly ITextureCompositor compositor;
    private readonly MapHandle map;
    private IDisposable texture;

    private BorrowedTextureRenderTarget(
        IGraphicsContext graphics,
        SessionDriver driver,
        ITextureCompositor compositor,
        IDisposable texture,
        MapHandle map,
        Func<RenderSessionAttachOptions, RenderSessionHandle> attach
    )
        : base(graphics, driver, 0, attach)
    {
        this.compositor = compositor;
        this.texture = texture;
        this.map = map;
    }

    protected override bool ExclusiveTexture => true;

    public static BorrowedTextureRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport,
        SessionDriver driver
    )
    {
        var texture = Allocate(graphics, viewport);
        try
        {
            ITextureCompositor compositor = graphics switch
            {
                MetalContext metal => new MetalTextureCompositor(metal, viewport),
                VulkanContext vulkan => new VulkanTextureCompositor(vulkan, viewport),
                OpenGLContext openGl => new OpenGLTextureCompositor(openGl, viewport),
                _ => throw new InvalidOperationException(
                    $"Borrowed textures are not implemented for {graphics.Backend}."
                ),
            };
            try
            {
                return new(
                    graphics,
                    driver,
                    compositor,
                    texture,
                    map,
                    options =>
                        texture switch
                        {
                            MetalBorrowedTexture metal => map.MetalBorrowedTextureAttach(
                                Describe(metal, viewport),
                                options
                            ),
                            VulkanBorrowedImage vulkan => map.VulkanBorrowedTextureAttach(
                                Describe((VulkanContext)graphics, driver, vulkan, viewport),
                                options
                            ),
                            OpenGLBorrowedTexture openGl => map.OpenglBorrowedTextureAttach(
                                Describe((OpenGLContext)graphics, openGl, viewport),
                                options
                            ),
                            _ => throw new InvalidOperationException(
                                $"Borrowed textures are not implemented for {graphics.Backend}."
                            ),
                        }
                );
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

    /// <summary>
    /// Replaces the texture, because its owner sets its size. The outgoing texture stays current
    /// until the replacement completes, and the frames rendered before it are drawn from it. A
    /// replacement that fails once started leaves it unknown which texture the session holds, so
    /// the session detaches before either texture is released.
    /// </summary>
    public override void Resize(Viewport viewport)
    {
        compositor.Resize(viewport);
        var replacement = Allocate(Graphics, viewport);
        Task handover;
        try
        {
            handover = replacement switch
            {
                MetalBorrowedTexture metal => Session.MetalBorrowedTextureSetTargetAsync(
                    Describe(metal, viewport)
                ),
                VulkanBorrowedImage vulkan => Session.VulkanBorrowedTextureSetTargetAsync(
                    Describe((VulkanContext)Graphics, Driver, vulkan, viewport)
                ),
                OpenGLBorrowedTexture openGl => Session.OpenglBorrowedTextureSetTargetAsync(
                    Describe((OpenGLContext)Graphics, openGl, viewport)
                ),
                _ => throw new InvalidOperationException(
                    $"Borrowed textures are not implemented for {Graphics.Backend}."
                ),
            };
        }
        catch
        {
            replacement.Dispose();
            throw;
        }
        // A target replacement leaves the map's extent unchanged.
        map.ResizeAsync(
                new LogicalExtent(
                    viewport.LogicalWidth,
                    viewport.LogicalHeight,
                    viewport.ScaleFactor
                )
            )
            .ReportFailure("map resize");
        try
        {
            Await(handover);
        }
        catch
        {
            Dispose();
            replacement.Dispose();
            throw;
        }
        DrainFrameResults();
        texture.Dispose();
        texture = replacement;
        RequestFrame(force: true);
    }

    protected override void DisposeHost()
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

    private static IDisposable Allocate(IGraphicsContext graphics, Viewport viewport) =>
        graphics switch
        {
            MetalContext metal => new MetalBorrowedTexture(metal, viewport),
            VulkanContext vulkan => new VulkanBorrowedImage(vulkan, viewport),
            OpenGLContext openGl => new OpenGLBorrowedTexture(openGl, viewport),
            _ => throw new InvalidOperationException(
                $"Borrowed textures are not implemented for {graphics.Backend}."
            ),
        };

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
        SessionDriver driver,
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

/// <summary>A window surface, where the session presents each frame that it renders.</summary>
internal sealed class NativeSurfaceRenderTarget : RenderTarget
{
    private NativeSurfaceRenderTarget(
        IGraphicsContext graphics,
        SessionDriver driver,
        Func<RenderSessionAttachOptions, RenderSessionHandle> attach
    )
        : base(graphics, driver, 0, attach) { }

    public static NativeSurfaceRenderTarget Attach(
        IGraphicsContext graphics,
        MapHandle map,
        Viewport viewport,
        SessionDriver driver
    ) =>
        new(
            graphics,
            driver,
            options =>
                graphics switch
                {
                    MetalContext metal => map.MetalSurfaceAttach(
                        new MetalSurfaceDescriptor
                        {
                            Extent = viewport.RenderTargetExtent,
                            Layer = metal.LayerPointer(),
                            Context = metal.Descriptor(),
                        },
                        options
                    ),
                    // The host submits nothing in this mode, so the session shares its queue.
                    VulkanContext vulkan => map.VulkanSurfaceAttach(
                        new VulkanSurfaceDescriptor
                        {
                            Extent = viewport.RenderTargetExtent,
                            Surface = vulkan.SurfaceHandle(),
                            Context = vulkan.Descriptor(),
                        },
                        options
                    ),
                    OpenGLContext openGl => map.OpenglSurfaceAttach(
                        new OpenglSurfaceDescriptor
                        {
                            Extent = viewport.RenderTargetExtent,
                            Surface = openGl.SurfacePointer(),
                            Context = openGl.Descriptor(requirePbufferConfig: false),
                        },
                        options
                    ),
                    _ => throw new InvalidOperationException(
                        $"Native surfaces are not implemented for {graphics.Backend}."
                    ),
                }
        );

    protected override bool Presents => true;

    protected override bool Present() => true;
}

internal static class TaskReporting
{
    /// <summary>Reports a failed command to diagnostics. Nothing waits on its completion.</summary>
    public static void ReportFailure(this Task task, string operation) =>
        _ = task.ContinueWith(
            completed =>
                Console.Error.WriteLine(
                    $"{operation} failed: {completed.Exception?.GetBaseException().Message}"
                ),
            TaskContinuationOptions.OnlyOnFaulted | TaskContinuationOptions.ExecuteSynchronously
        );
}
