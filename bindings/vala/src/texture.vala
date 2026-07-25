namespace MaplibreNative {
    internal Raw.RenderTargetExtent render_target_extent (uint32 width, uint32 height, double scale_factor) {
        Raw.RenderTargetExtent extent = {};
        extent.size = (uint32) sizeof (Raw.RenderTargetExtent);
        extent.width = width;
        extent.height = height;
        extent.scale_factor = scale_factor;
        return extent;
    }

    internal Raw.MetalContextDescriptor metal_context_descriptor (NativePointer device) throws Error {
        Raw.MetalContextDescriptor context = {};
        context.size = (uint32) sizeof (Raw.MetalContextDescriptor);
        context.device = device.to_native ();
        return context;
    }

    public class TextureImageInfo {
        public uint32 width { get; private set; }
        public uint32 height { get; private set; }
        public uint32 stride { get; private set; }
        public size_t byte_length { get; private set; }

        internal TextureImageInfo (Raw.TextureImageInfo native) {
            width = native.width;
            height = native.height;
            stride = native.stride;
            byte_length = native.byte_length;
        }
    }

    public class MetalOwnedTextureDescriptor {
        public uint32 width { get; set; default = 64; }
        public uint32 height { get; set; default = 64; }
        public double scale_factor { get; set; default = 1.0; }
        public NativePointer device { get; set; }

        public MetalOwnedTextureDescriptor (NativePointer device) {
            this.device = device;
        }

        internal Raw.MetalOwnedTextureDescriptor to_native () throws Error {
            Raw.MetalOwnedTextureDescriptor descriptor = Raw.metal_owned_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.context = metal_context_descriptor (device);
            return descriptor;
        }
    }

    public class MetalBorrowedTextureDescriptor {
        public uint32 width { get; set; default = 64; }
        public uint32 height { get; set; default = 64; }
        public double scale_factor { get; set; default = 1.0; }
        public NativePointer texture { get; set; }

        public MetalBorrowedTextureDescriptor (NativePointer texture) {
            this.texture = texture;
        }

        internal Raw.MetalBorrowedTextureDescriptor to_native () throws Error {
            Raw.MetalBorrowedTextureDescriptor descriptor = Raw.metal_borrowed_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.texture = texture.to_native ();
            return descriptor;
        }
    }

    public class VulkanContextDescriptor {
        public NativePointer instance { get; set; }
        public NativePointer physical_device { get; set; }
        public NativePointer device { get; set; }
        public NativePointer graphics_queue { get; set; }
        public uint32 graphics_queue_family_index { get; set; }
        public NativePointer? get_instance_proc_addr { get; set; }
        public NativePointer? get_device_proc_addr { get; set; }

        public VulkanContextDescriptor (NativePointer instance, NativePointer physical_device, NativePointer device, NativePointer graphics_queue, uint32 graphics_queue_family_index) {
            this.instance = instance;
            this.physical_device = physical_device;
            this.device = device;
            this.graphics_queue = graphics_queue;
            this.graphics_queue_family_index = graphics_queue_family_index;
        }

        internal Raw.VulkanContextDescriptor to_native () throws Error {
            Raw.VulkanContextDescriptor context = {};
            context.size = (uint32) sizeof (Raw.VulkanContextDescriptor);
            context.instance = instance.to_native ();
            context.physical_device = physical_device.to_native ();
            context.device = device.to_native ();
            context.graphics_queue = graphics_queue.to_native ();
            context.graphics_queue_family_index = graphics_queue_family_index;
            if (get_instance_proc_addr != null) {
                context.get_instance_proc_addr = get_instance_proc_addr.to_native ();
            }
            if (get_device_proc_addr != null) {
                context.get_device_proc_addr = get_device_proc_addr.to_native ();
            }
            return context;
        }
    }

    public abstract class OpenGLContextDescriptor {
        internal abstract Raw.OpenGLContextDescriptor to_native () throws Error;
    }

    public class WglContextDescriptor : OpenGLContextDescriptor {
        public NativePointer device_context { get; set; }
        public NativePointer share_context { get; set; }
        public NativePointer? get_proc_address { get; set; }

        public WglContextDescriptor (NativePointer device_context, NativePointer share_context) {
            this.device_context = device_context;
            this.share_context = share_context;
        }

        internal override Raw.OpenGLContextDescriptor to_native () throws Error {
            Raw.OpenGLContextDescriptor context = {};
            context.size = (uint32) sizeof (Raw.OpenGLContextDescriptor);
            context.platform = Raw.OpenGLContextPlatform.WGL;
            context.wgl.size = (uint32) sizeof (Raw.WglContextDescriptor);
            context.wgl.device_context = device_context.to_native ();
            context.wgl.share_context = share_context.to_native ();
            if (get_proc_address != null) {
                context.wgl.get_proc_address = get_proc_address.to_native ();
            }
            return context;
        }
    }

    public class EglContextDescriptor : OpenGLContextDescriptor {
        public NativePointer display { get; set; }
        public NativePointer config { get; set; }
        public NativePointer share_context { get; set; }
        public NativePointer? get_proc_address { get; set; }

        public EglContextDescriptor (NativePointer display, NativePointer config, NativePointer share_context) {
            this.display = display;
            this.config = config;
            this.share_context = share_context;
        }

        internal override Raw.OpenGLContextDescriptor to_native () throws Error {
            Raw.OpenGLContextDescriptor context = {};
            context.size = (uint32) sizeof (Raw.OpenGLContextDescriptor);
            context.platform = Raw.OpenGLContextPlatform.EGL;
            context.egl.size = (uint32) sizeof (Raw.EglContextDescriptor);
            context.egl.display = display.to_native ();
            context.egl.config = config.to_native ();
            context.egl.share_context = share_context.to_native ();
            if (get_proc_address != null) {
                context.egl.get_proc_address = get_proc_address.to_native ();
            }
            return context;
        }
    }

    public class OpenGLOwnedTextureDescriptor {
        public uint32 width { get; set; default = 64; }
        public uint32 height { get; set; default = 64; }
        public double scale_factor { get; set; default = 1.0; }
        public OpenGLContextDescriptor context { get; set; }

        public OpenGLOwnedTextureDescriptor (OpenGLContextDescriptor context) {
            this.context = context;
        }

        internal Raw.OpenGLOwnedTextureDescriptor to_native () throws Error {
            Raw.OpenGLOwnedTextureDescriptor descriptor = Raw.opengl_owned_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.context = context.to_native ();
            return descriptor;
        }
    }

    public class OpenGLBorrowedTextureDescriptor {
        public uint32 width { get; set; default = 64; }
        public uint32 height { get; set; default = 64; }
        public double scale_factor { get; set; default = 1.0; }
        public OpenGLContextDescriptor context { get; set; }
        public uint32 texture { get; set; }
        public uint32 target { get; set; }

        public OpenGLBorrowedTextureDescriptor (OpenGLContextDescriptor context, uint32 texture, uint32 target) {
            this.context = context;
            this.texture = texture;
            this.target = target;
        }

        internal Raw.OpenGLBorrowedTextureDescriptor to_native () throws Error {
            Raw.OpenGLBorrowedTextureDescriptor descriptor = Raw.opengl_borrowed_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.context = context.to_native ();
            descriptor.texture = texture;
            descriptor.target = target;
            return descriptor;
        }
    }

    public class VulkanOwnedTextureDescriptor {
        public uint32 width { get; set; default = 64; }
        public uint32 height { get; set; default = 64; }
        public double scale_factor { get; set; default = 1.0; }
        public VulkanContextDescriptor context { get; set; }

        public VulkanOwnedTextureDescriptor (VulkanContextDescriptor context) {
            this.context = context;
        }

        internal Raw.VulkanOwnedTextureDescriptor to_native () throws Error {
            Raw.VulkanOwnedTextureDescriptor descriptor = Raw.vulkan_owned_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.context = context.to_native ();
            return descriptor;
        }
    }

    public class VulkanBorrowedTextureDescriptor {
        public uint32 width { get; set; default = 64; }
        public uint32 height { get; set; default = 64; }
        public double scale_factor { get; set; default = 1.0; }
        public VulkanContextDescriptor context { get; set; }
        public NativePointer image { get; set; }
        public NativePointer image_view { get; set; }
        public uint32 format { get; set; }
        public uint32 initial_layout { get; set; }
        public uint32 final_layout { get; set; }

        public VulkanBorrowedTextureDescriptor (VulkanContextDescriptor context, NativePointer image, NativePointer image_view) {
            this.context = context;
            this.image = image;
            this.image_view = image_view;
        }

        internal Raw.VulkanBorrowedTextureDescriptor to_native () throws Error {
            Raw.VulkanBorrowedTextureDescriptor descriptor = Raw.vulkan_borrowed_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.context = context.to_native ();
            descriptor.image = image.to_native ();
            descriptor.image_view = image_view.to_native ();
            descriptor.format = format;
            descriptor.initial_layout = initial_layout;
            descriptor.final_layout = final_layout;
            return descriptor;
        }
    }

    public class OpenGLOwnedTextureFrameHandle {
        private RenderSessionHandle session;
        private Raw.OpenGLOwnedTextureFrame frame;
        private bool closed;

        internal OpenGLOwnedTextureFrameHandle (RenderSessionHandle session, Raw.OpenGLOwnedTextureFrame frame) {
            this.session = session;
            this.frame = frame;
        }

        ~OpenGLOwnedTextureFrameHandle () {
            if (!closed) {
                warning ("OpenGLOwnedTextureFrameHandle finalized while live; call close() on the owner thread");
            }
        }

        private void require_live () throws Error {
            if (closed) {
                throw new Error.INVALID_STATE ("opengl texture frame is closed");
            }
        }

        public void close () throws Error {
            if (closed) {
                return;
            }
            check_status (Raw.opengl_owned_texture_release_frame (session.require_live (), &frame));
            closed = true;
            session.finish_frame_borrow ();
        }

        public uint32 get_width () throws Error {
            require_live ();
            return frame.width;
        }

        public uint32 get_height () throws Error {
            require_live ();
            return frame.height;
        }

        public double get_scale_factor () throws Error {
            require_live ();
            return frame.scale_factor;
        }

        public uint64 get_generation () throws Error {
            require_live ();
            return frame.generation;
        }

        public uint64 get_frame_id () throws Error {
            require_live ();
            return frame.frame_id;
        }

        public FrameUInt32 get_texture () throws Error {
            require_live ();
            return new FrameUInt32 (frame.texture, () => require_live ());
        }

        public uint32 get_target () throws Error {
            require_live ();
            return frame.target;
        }

        public uint32 get_internal_format () throws Error {
            require_live ();
            return frame.internal_format;
        }

        public uint32 get_format () throws Error {
            require_live ();
            return frame.format;
        }

        public uint32 get_pixel_type () throws Error {
            require_live ();
            return frame.type;
        }
    }

    public class VulkanOwnedTextureFrameHandle {
        private RenderSessionHandle session;
        private Raw.VulkanOwnedTextureFrame frame;
        private bool closed;

        internal VulkanOwnedTextureFrameHandle (RenderSessionHandle session, Raw.VulkanOwnedTextureFrame frame) {
            this.session = session;
            this.frame = frame;
        }

        ~VulkanOwnedTextureFrameHandle () {
            if (!closed) {
                warning ("VulkanOwnedTextureFrameHandle finalized while live; call close() on the owner thread");
            }
        }

        private void require_live () throws Error {
            if (closed) {
                throw new Error.INVALID_STATE ("vulkan texture frame is closed");
            }
        }

        public void close () throws Error {
            if (closed) {
                return;
            }
            check_status (Raw.vulkan_owned_texture_release_frame (session.require_live (), &frame));
            closed = true;
            session.finish_frame_borrow ();
        }

        public uint32 get_width () throws Error {
            require_live ();
            return frame.width;
        }

        public uint32 get_height () throws Error {
            require_live ();
            return frame.height;
        }

        public double get_scale_factor () throws Error {
            require_live ();
            return frame.scale_factor;
        }

        public uint64 get_generation () throws Error {
            require_live ();
            return frame.generation;
        }

        public uint64 get_frame_id () throws Error {
            require_live ();
            return frame.frame_id;
        }

        public FrameNativePointer get_image () throws Error {
            require_live ();
            return new FrameNativePointer ((size_t) frame.image, () => require_live ());
        }

        public FrameNativePointer get_image_view () throws Error {
            require_live ();
            return new FrameNativePointer ((size_t) frame.image_view, () => require_live ());
        }

        public FrameNativePointer get_device () throws Error {
            require_live ();
            return new FrameNativePointer ((size_t) frame.device, () => require_live ());
        }

        public uint32 get_format () throws Error {
            require_live ();
            return frame.format;
        }

        public uint32 get_layout () throws Error {
            require_live ();
            return frame.layout;
        }
    }
}
