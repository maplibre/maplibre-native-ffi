namespace MaplibreNative {
    public class MetalSurfaceDescriptor {
        public uint32 width { get; set; default = 64; }
        public uint32 height { get; set; default = 64; }
        public double scale_factor { get; set; default = 1.0; }
        public NativePointer layer { get; set; }
        public NativePointer? device { get; set; }

        public MetalSurfaceDescriptor (NativePointer layer) {
            this.layer = layer;
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
        public uint32 width { get; set; default = 64; }
        public uint32 height { get; set; default = 64; }
        public double scale_factor { get; set; default = 1.0; }
        public OpenGLContextDescriptor context { get; set; }
        public NativePointer surface { get; set; }

        public OpenGLSurfaceDescriptor (OpenGLContextDescriptor context, NativePointer surface) {
            this.context = context;
            this.surface = surface;
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
        public uint32 width { get; set; default = 64; }
        public uint32 height { get; set; default = 64; }
        public double scale_factor { get; set; default = 1.0; }
        public VulkanContextDescriptor context { get; set; }
        public NativePointer surface { get; set; }

        public VulkanSurfaceDescriptor (VulkanContextDescriptor context, NativePointer surface) {
            this.context = context;
            this.surface = surface;
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
