using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
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

    [NativeTypeName("uint32_t")]
    internal enum mln_render_driver_kind : uint
    {
        MLN_RENDER_DRIVER_CORE_WORKER = 1U,
        MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD = 2U,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_render_session_capability_flag : uint
    {
        MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION = 1U << 0,
        MLN_RENDER_SESSION_CAPABILITY_READBACK = 1U << 1,
        MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC = 1U << 2,
        MLN_RENDER_SESSION_CAPABILITY_PRESENTATION = 1U << 3,
    }

    internal partial struct mln_render_session_attach_options
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint driver;

        [NativeTypeName("uint32_t")]
        public uint requested_texture_ring_depth;

        [NativeTypeName("uint32_t")]
        public uint reserved;

        [NativeTypeName("mln_notification_source")]
        public MlnNotificationSource operation_source;

        [NativeTypeName("mln_notification_source")]
        public MlnNotificationSource frame_source;

        [NativeTypeName("mln_notification_source")]
        public MlnNotificationSource driver_work_source;
    }

    internal partial struct mln_render_session_capabilities
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint driver;

        [NativeTypeName("uint32_t")]
        public uint texture_ring_depth;

        [NativeTypeName("uint32_t")]
        public uint flags;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_gpu_sync_kind : uint
    {
        MLN_GPU_SYNC_CPU_COMPLETE = 0U,
        MLN_GPU_SYNC_METAL_SHARED_EVENT = 1U,
        MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE = 2U,
        MLN_GPU_SYNC_OPENGL_FENCE = 3U,
        MLN_GPU_SYNC_WEBGPU_TOKEN = 4U,
    }

    internal unsafe partial struct mln_gpu_sync
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint kind;

        public void* @object;

        [NativeTypeName("uint64_t")]
        public ulong value;
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

        public void* get_instance_proc_addr;

        public void* get_device_proc_addr;
    }

    internal unsafe partial struct mln_webgpu_context_descriptor
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        public void* instance;

        public void* device;

        public void* queue;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_opengl_context_provider_flag : uint
    {
        MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL = 1U << 0,
        MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL = 1U << 1,
        MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WEBGL = 1U << 2,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_opengl_context_platform : uint
    {
        MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED = 0U,
        MLN_OPENGL_CONTEXT_PLATFORM_WGL = 1U,
        MLN_OPENGL_CONTEXT_PLATFORM_EGL = 2U,
        MLN_OPENGL_CONTEXT_PLATFORM_WEBGL = 3U,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_opengl_context_ownership : uint
    {
        MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED = 0U,
        MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED = 1U,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_opengl_client_api : uint
    {
        MLN_OPENGL_CLIENT_API_UNSPECIFIED = 0U,
        MLN_OPENGL_CLIENT_API_GL = 1U,
        MLN_OPENGL_CLIENT_API_GLES = 2U,
    }

    internal unsafe partial struct mln_wgl_context_descriptor
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        public void* device_context;

        public void* share_context;

        public void* get_proc_address;
    }

    internal unsafe partial struct mln_egl_context_descriptor
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        public void* display;

        public void* config;

        public void* share_context;

        public mln_opengl_client_api client_api;

        public void* get_proc_address;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_webgl_context_kind : uint
    {
        MLN_WEBGL_CONTEXT_EXISTING = 0U,
        MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS = 1U,
    }

    internal partial struct mln_webgl_context_descriptor
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint kind;

        [NativeTypeName("int32_t")]
        public int context;

        public mln_buffer_view canvas_selector;
    }

    internal partial struct mln_opengl_context_descriptor
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        public mln_opengl_context_platform platform;

        public mln_opengl_context_ownership ownership;

        [NativeTypeName("__AnonymousRecord_render_target_L285_C3")]
        public _data_e__Union data;

        [StructLayout(LayoutKind.Explicit)]
        internal partial struct _data_e__Union
        {
            [FieldOffset(0)]
            public mln_wgl_context_descriptor wgl;

            [FieldOffset(0)]
            public mln_egl_context_descriptor egl;

            [FieldOffset(0)]
            public mln_webgl_context_descriptor webgl;
        }
    }

    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_gpu_sync mln_gpu_sync_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_render_session_attach_options mln_render_session_attach_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_target_extent_physical_size([NativeTypeName("const mln_render_target_extent *")] mln_render_target_extent* extent, [NativeTypeName("uint32_t *")] uint* out_width, [NativeTypeName("uint32_t *")] uint* out_height);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint mln_opengl_supported_context_provider_mask();
    }
}
