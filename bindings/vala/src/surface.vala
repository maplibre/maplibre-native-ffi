namespace MaplibreNative {
    public class MetalSurfaceDescriptor {
        public uint32 width { get; set; default = 256; }
        public uint32 height { get; set; default = 256; }
        public double scale_factor { get; set; default = 1.0; }
        public NativePointer layer { get; set; }
        public NativePointer? device { get; set; }

        public MetalSurfaceDescriptor (NativePointer layer) {
            this.layer = layer;
        }

        public MetalSurfaceDescriptor copy () {
            var copied = new MetalSurfaceDescriptor (layer.copy ());
            copied.width = width;
            copied.height = height;
            copied.scale_factor = scale_factor;
            copied.device = device == null ? null : device.copy ();
            return copied;
        }

        public bool equal (MetalSurfaceDescriptor other) {
            return width == other.width
                && height == other.height
                && scale_factor == other.scale_factor
                && layer.equal (other.layer)
                && (device == null
                    ? other.device == null
                    : other.device != null && device.equal (other.device));
        }

        internal Raw.MetalSurfaceDescriptor to_native () throws Error {
            Raw.MetalSurfaceDescriptor descriptor = Raw.metal_surface_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.context.size = (uint32) sizeof (Raw.MetalContextDescriptor);
            if (device != null) {
                descriptor.context.device = device.to_native ();
            }
            descriptor.layer = layer.to_native ();
            return descriptor;
        }
    }

    public class OpenGLSurfaceDescriptor {
        public uint32 width { get; set; default = 256; }
        public uint32 height { get; set; default = 256; }
        public double scale_factor { get; set; default = 1.0; }
        public OpenGLContextDescriptor context { get; set; }
        public NativePointer surface { get; set; }

        public OpenGLSurfaceDescriptor (OpenGLContextDescriptor context, NativePointer surface) {
            this.context = context;
            this.surface = surface;
        }

        public OpenGLSurfaceDescriptor copy () {
            var copied = new OpenGLSurfaceDescriptor (context.copy (), surface.copy ());
            copied.width = width;
            copied.height = height;
            copied.scale_factor = scale_factor;
            return copied;
        }

        public bool equal (OpenGLSurfaceDescriptor other) {
            return width == other.width
                && height == other.height
                && scale_factor == other.scale_factor
                && context.equal (other.context)
                && surface.equal (other.surface);
        }

        internal Raw.OpenGLSurfaceDescriptor to_native () throws Error {
            Raw.OpenGLSurfaceDescriptor descriptor = Raw.opengl_surface_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.context = context.to_native ();
            descriptor.surface = surface.to_native ();
            return descriptor;
        }
    }

    public class VulkanSurfaceDescriptor {
        public uint32 width { get; set; default = 256; }
        public uint32 height { get; set; default = 256; }
        public double scale_factor { get; set; default = 1.0; }
        public VulkanContextDescriptor context { get; set; }
        public NativePointer surface { get; set; }

        public VulkanSurfaceDescriptor (VulkanContextDescriptor context, NativePointer surface) {
            this.context = context;
            this.surface = surface;
        }

        public VulkanSurfaceDescriptor copy () {
            var copied = new VulkanSurfaceDescriptor (context.copy (), surface.copy ());
            copied.width = width;
            copied.height = height;
            copied.scale_factor = scale_factor;
            return copied;
        }

        public bool equal (VulkanSurfaceDescriptor other) {
            return width == other.width
                && height == other.height
                && scale_factor == other.scale_factor
                && context.equal (other.context)
                && surface.equal (other.surface);
        }

        internal Raw.VulkanSurfaceDescriptor to_native () throws Error {
            Raw.VulkanSurfaceDescriptor descriptor = Raw.vulkan_surface_descriptor_default ();
            descriptor.extent = render_target_extent (width, height, scale_factor);
            descriptor.context = context.to_native ();
            descriptor.surface = surface.to_native ();
            return descriptor;
        }
    }
}
