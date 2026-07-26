namespace MaplibreNative {
    private NativePointer? copy_optional_pointer (NativePointer? value) {
        return value == null ? null : value.copy ();
    }

    private bool optional_pointer_equal (NativePointer? left, NativePointer? right) {
        return left == null
            ? right == null
            : right != null && left.equal (right);
    }

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

    public class PhysicalSize {
        public uint32 width { get; private set; }
        public uint32 height { get; private set; }

        internal PhysicalSize (uint32 width, uint32 height) {
            this.width = width;
            this.height = height;
        }

        public PhysicalSize copy () {
            return new PhysicalSize (width, height);
        }

        public bool equal (PhysicalSize other) {
            return width == other.width && height == other.height;
        }
    }

    public PhysicalSize render_target_extent_physical_size (uint32 width, uint32 height, double scale_factor) throws Error {
        Raw.RenderTargetExtent extent = render_target_extent (width, height, scale_factor);
        uint32 physical_width;
        uint32 physical_height;
        check_status (Raw.render_target_extent_physical_size (&extent, out physical_width, out physical_height));
        return new PhysicalSize (physical_width, physical_height);
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

        public TextureImageInfo copy () {
            Raw.TextureImageInfo native = {};
            native.width = width;
            native.height = height;
            native.stride = stride;
            native.byte_length = byte_length;
            return new TextureImageInfo (native);
        }

        public bool equal (TextureImageInfo other) {
            return width == other.width
                && height == other.height
                && stride == other.stride
                && byte_length == other.byte_length;
        }
    }

    public class MetalOwnedTextureDescriptor {
        public uint32 width { get; set; default = 256; }
        public uint32 height { get; set; default = 256; }
        public double scale_factor { get; set; default = 1.0; }
        public NativePointer device { get; set; }

        public MetalOwnedTextureDescriptor (NativePointer device) {
            this.device = device;
        }

        public MetalOwnedTextureDescriptor copy () {
            var copied = new MetalOwnedTextureDescriptor (device.copy ());
            copied.width = width;
            copied.height = height;
            copied.scale_factor = scale_factor;
            return copied;
        }

        public bool equal (MetalOwnedTextureDescriptor other) {
            return width == other.width
                && height == other.height
                && scale_factor == other.scale_factor
                && device.equal (other.device);
        }

        internal Raw.MetalOwnedTextureDescriptor to_native () throws Error {
            Raw.MetalOwnedTextureDescriptor descriptor = Raw.metal_owned_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.context = metal_context_descriptor (device);
            return descriptor;
        }
    }

    public class MetalBorrowedTextureDescriptor {
        public uint32 width { get; set; default = 256; }
        public uint32 height { get; set; default = 256; }
        public double scale_factor { get; set; default = 1.0; }
        public uint32 physical_width { get; set; default = 256; }
        public uint32 physical_height { get; set; default = 256; }
        public NativePointer texture { get; set; }

        public MetalBorrowedTextureDescriptor (NativePointer texture) {
            this.texture = texture;
        }

        public MetalBorrowedTextureDescriptor copy () {
            var copied = new MetalBorrowedTextureDescriptor (texture.copy ());
            copied.width = width;
            copied.height = height;
            copied.scale_factor = scale_factor;
            copied.physical_width = physical_width;
            copied.physical_height = physical_height;
            return copied;
        }

        public bool equal (MetalBorrowedTextureDescriptor other) {
            return width == other.width
                && height == other.height
                && scale_factor == other.scale_factor
                && physical_width == other.physical_width
                && physical_height == other.physical_height
                && texture.equal (other.texture);
        }

        internal Raw.MetalBorrowedTextureDescriptor to_native () throws Error {
            Raw.MetalBorrowedTextureDescriptor descriptor = Raw.metal_borrowed_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.physical_width = physical_width;
            descriptor.physical_height = physical_height;
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

        public VulkanContextDescriptor copy () {
            var copied = new VulkanContextDescriptor (
                instance.copy (),
                physical_device.copy (),
                device.copy (),
                graphics_queue.copy (),
                graphics_queue_family_index
            );
            copied.get_instance_proc_addr = copy_optional_pointer (get_instance_proc_addr);
            copied.get_device_proc_addr = copy_optional_pointer (get_device_proc_addr);
            return copied;
        }

        public bool equal (VulkanContextDescriptor other) {
            return instance.equal (other.instance)
                && physical_device.equal (other.physical_device)
                && device.equal (other.device)
                && graphics_queue.equal (other.graphics_queue)
                && graphics_queue_family_index == other.graphics_queue_family_index
                && optional_pointer_equal (get_instance_proc_addr, other.get_instance_proc_addr)
                && optional_pointer_equal (get_device_proc_addr, other.get_device_proc_addr);
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
        public abstract OpenGLContextDescriptor copy ();
        public abstract bool equal (OpenGLContextDescriptor other);
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

        public override OpenGLContextDescriptor copy () {
            var copied = new WglContextDescriptor (device_context.copy (), share_context.copy ());
            copied.get_proc_address = copy_optional_pointer (get_proc_address);
            return copied;
        }

        public override bool equal (OpenGLContextDescriptor other) {
            var wgl = other as WglContextDescriptor;
            return wgl != null
                && device_context.equal (wgl.device_context)
                && share_context.equal (wgl.share_context)
                && optional_pointer_equal (get_proc_address, wgl.get_proc_address);
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

        public override OpenGLContextDescriptor copy () {
            var copied = new EglContextDescriptor (display.copy (), config.copy (), share_context.copy ());
            copied.get_proc_address = copy_optional_pointer (get_proc_address);
            return copied;
        }

        public override bool equal (OpenGLContextDescriptor other) {
            var egl = other as EglContextDescriptor;
            return egl != null
                && display.equal (egl.display)
                && config.equal (egl.config)
                && share_context.equal (egl.share_context)
                && optional_pointer_equal (get_proc_address, egl.get_proc_address);
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
        public uint32 width { get; set; default = 256; }
        public uint32 height { get; set; default = 256; }
        public double scale_factor { get; set; default = 1.0; }
        public OpenGLContextDescriptor context { get; set; }

        public OpenGLOwnedTextureDescriptor (OpenGLContextDescriptor context) {
            this.context = context;
        }

        public OpenGLOwnedTextureDescriptor copy () {
            var copied = new OpenGLOwnedTextureDescriptor (context.copy ());
            copied.width = width;
            copied.height = height;
            copied.scale_factor = scale_factor;
            return copied;
        }

        public bool equal (OpenGLOwnedTextureDescriptor other) {
            return width == other.width
                && height == other.height
                && scale_factor == other.scale_factor
                && context.equal (other.context);
        }

        internal Raw.OpenGLOwnedTextureDescriptor to_native () throws Error {
            Raw.OpenGLOwnedTextureDescriptor descriptor = Raw.opengl_owned_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.context = context.to_native ();
            return descriptor;
        }
    }

    public class OpenGLBorrowedTextureDescriptor {
        public uint32 width { get; set; default = 256; }
        public uint32 height { get; set; default = 256; }
        public double scale_factor { get; set; default = 1.0; }
        public uint32 physical_width { get; set; default = 256; }
        public uint32 physical_height { get; set; default = 256; }
        public OpenGLContextDescriptor context { get; set; }
        public uint32 texture { get; set; }
        public uint32 target { get; set; }

        public OpenGLBorrowedTextureDescriptor (OpenGLContextDescriptor context, uint32 texture, uint32 target) {
            this.context = context;
            this.texture = texture;
            this.target = target;
        }

        public OpenGLBorrowedTextureDescriptor copy () {
            var copied = new OpenGLBorrowedTextureDescriptor (context.copy (), texture, target);
            copied.width = width;
            copied.height = height;
            copied.scale_factor = scale_factor;
            copied.physical_width = physical_width;
            copied.physical_height = physical_height;
            return copied;
        }

        public bool equal (OpenGLBorrowedTextureDescriptor other) {
            return width == other.width
                && height == other.height
                && scale_factor == other.scale_factor
                && physical_width == other.physical_width
                && physical_height == other.physical_height
                && context.equal (other.context)
                && texture == other.texture
                && target == other.target;
        }

        internal Raw.OpenGLBorrowedTextureDescriptor to_native () throws Error {
            Raw.OpenGLBorrowedTextureDescriptor descriptor = Raw.opengl_borrowed_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.physical_width = physical_width;
            descriptor.physical_height = physical_height;
            descriptor.context = context.to_native ();
            descriptor.texture = texture;
            descriptor.target = target;
            return descriptor;
        }
    }

    public class VulkanOwnedTextureDescriptor {
        public uint32 width { get; set; default = 256; }
        public uint32 height { get; set; default = 256; }
        public double scale_factor { get; set; default = 1.0; }
        public VulkanContextDescriptor context { get; set; }

        public VulkanOwnedTextureDescriptor (VulkanContextDescriptor context) {
            this.context = context;
        }

        public VulkanOwnedTextureDescriptor copy () {
            var copied = new VulkanOwnedTextureDescriptor (context.copy ());
            copied.width = width;
            copied.height = height;
            copied.scale_factor = scale_factor;
            return copied;
        }

        public bool equal (VulkanOwnedTextureDescriptor other) {
            return width == other.width
                && height == other.height
                && scale_factor == other.scale_factor
                && context.equal (other.context);
        }

        internal Raw.VulkanOwnedTextureDescriptor to_native () throws Error {
            Raw.VulkanOwnedTextureDescriptor descriptor = Raw.vulkan_owned_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.context = context.to_native ();
            return descriptor;
        }
    }

    public class VulkanBorrowedTextureDescriptor {
        public uint32 width { get; set; default = 256; }
        public uint32 height { get; set; default = 256; }
        public double scale_factor { get; set; default = 1.0; }
        public uint32 physical_width { get; set; default = 256; }
        public uint32 physical_height { get; set; default = 256; }
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
            final_layout = Raw.vulkan_borrowed_texture_descriptor_default ().final_layout;
        }

        public VulkanBorrowedTextureDescriptor copy () {
            var copied = new VulkanBorrowedTextureDescriptor (context.copy (), image.copy (), image_view.copy ());
            copied.width = width;
            copied.height = height;
            copied.scale_factor = scale_factor;
            copied.physical_width = physical_width;
            copied.physical_height = physical_height;
            copied.format = format;
            copied.initial_layout = initial_layout;
            copied.final_layout = final_layout;
            return copied;
        }

        public bool equal (VulkanBorrowedTextureDescriptor other) {
            return width == other.width
                && height == other.height
                && scale_factor == other.scale_factor
                && physical_width == other.physical_width
                && physical_height == other.physical_height
                && context.equal (other.context)
                && image.equal (other.image)
                && image_view.equal (other.image_view)
                && format == other.format
                && initial_layout == other.initial_layout
                && final_layout == other.final_layout;
        }

        internal Raw.VulkanBorrowedTextureDescriptor to_native () throws Error {
            Raw.VulkanBorrowedTextureDescriptor descriptor = Raw.vulkan_borrowed_texture_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.physical_width = physical_width;
            descriptor.physical_height = physical_height;
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
        private FrameAccessState state = new FrameAccessState ("opengl texture frame");

        internal OpenGLOwnedTextureFrameHandle (RenderSessionHandle session, Raw.OpenGLOwnedTextureFrame frame) {
            this.session = session;
            this.frame = frame;
        }

        ~OpenGLOwnedTextureFrameHandle () {
            if (!state.is_closed) {
                warning ("OpenGLOwnedTextureFrameHandle finalized while live; call close() on the owner thread");
            }
        }

        private FrameAccessLease require_live () throws Error {
            return state.acquire ();
        }

        public void close () throws Error {
            if (!state.begin_close ()) {
                return;
            }
            bool released = false;
            try {
                var lease = session.require_live ();
                check_status (Raw.opengl_owned_texture_release_frame (lease.native, &frame));
                released = true;
            } finally {
                state.finish_close (released);
                if (released) {
                    session.finish_frame_borrow ();
                }
            }
        }

        public uint32 get_width () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.width;
        }

        public uint32 get_height () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.height;
        }

        public double get_scale_factor () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.scale_factor;
        }

        public uint64 get_generation () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.generation;
        }

        public uint64 get_frame_id () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.frame_id;
        }

        public FrameUInt32 get_texture () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return new FrameUInt32 (frame.texture, () => {
                var checked_access = require_live ();
                checked_access.keep_alive ();
            });
        }

        public uint32 get_target () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.target;
        }

        public uint32 get_internal_format () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.internal_format;
        }

        public uint32 get_format () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.format;
        }

        public uint32 get_pixel_type () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.type;
        }
    }

    public class VulkanOwnedTextureFrameHandle {
        private RenderSessionHandle session;
        private Raw.VulkanOwnedTextureFrame frame;
        private FrameAccessState state = new FrameAccessState ("vulkan texture frame");

        internal VulkanOwnedTextureFrameHandle (RenderSessionHandle session, Raw.VulkanOwnedTextureFrame frame) {
            this.session = session;
            this.frame = frame;
        }

        ~VulkanOwnedTextureFrameHandle () {
            if (!state.is_closed) {
                warning ("VulkanOwnedTextureFrameHandle finalized while live; call close() on the owner thread");
            }
        }

        private FrameAccessLease require_live () throws Error {
            return state.acquire ();
        }

        public void close () throws Error {
            if (!state.begin_close ()) {
                return;
            }
            bool released = false;
            try {
                var lease = session.require_live ();
                check_status (Raw.vulkan_owned_texture_release_frame (lease.native, &frame));
                released = true;
            } finally {
                state.finish_close (released);
                if (released) {
                    session.finish_frame_borrow ();
                }
            }
        }

        public uint32 get_width () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.width;
        }

        public uint32 get_height () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.height;
        }

        public double get_scale_factor () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.scale_factor;
        }

        public uint64 get_generation () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.generation;
        }

        public uint64 get_frame_id () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.frame_id;
        }

        public FrameNativePointer get_image () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return new FrameNativePointer ((size_t) frame.image, () => {
                var checked_access = require_live ();
                checked_access.keep_alive ();
            });
        }

        public FrameNativePointer get_image_view () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return new FrameNativePointer ((size_t) frame.image_view, () => {
                var checked_access = require_live ();
                checked_access.keep_alive ();
            });
        }

        public FrameNativePointer get_device () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return new FrameNativePointer ((size_t) frame.device, () => {
                var checked_access = require_live ();
                checked_access.keep_alive ();
            });
        }

        public uint32 get_format () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.format;
        }

        public uint32 get_layout () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.layout;
        }
    }
}
