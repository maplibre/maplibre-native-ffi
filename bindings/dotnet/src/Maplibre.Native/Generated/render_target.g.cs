namespace Maplibre.Native.Internal.C
{
    internal partial struct mln_render_target_extent
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint width;

        [NativeTypeName("uint32_t")]
        public uint height;

        public double scale_factor;
    }

    internal unsafe partial struct mln_metal_context_descriptor
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        public void* device;
    }

    internal unsafe partial struct mln_vulkan_context_descriptor
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        public void* instance;

        public void* physical_device;

        public void* device;

        public void* graphics_queue;

        [NativeTypeName("uint32_t")]
        public uint graphics_queue_family_index;
    }
}
