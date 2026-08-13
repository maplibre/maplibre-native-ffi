using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    [NativeTypeName("uint32_t")]
    internal enum mln_notification_endpoint_kind : uint
    {
        MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS = 1,
        MLN_NOTIFICATION_ENDPOINT_OPERATION = 2,
        MLN_NOTIFICATION_ENDPOINT_ADAPTER_RESOURCE_REQUESTS = 3,
        MLN_NOTIFICATION_ENDPOINT_ADAPTER_LOG_RECORDS = 4,
        MLN_NOTIFICATION_ENDPOINT_RENDER_FRAMES = 5,
        MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK = 6,
    }

    internal partial struct mln_ready_endpoint
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint kind;

        [NativeTypeName("uint64_t")]
        public ulong id;
    }

    internal unsafe partial struct mln_ready_batch_view
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint endpoint_size;

        [NativeTypeName("const mln_ready_endpoint *")]
        public mln_ready_endpoint* endpoints;

        [NativeTypeName("size_t")]
        public nuint endpoint_count;
    }

    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_notification_source_create([NativeTypeName("mln_notification_source *")] MlnNotificationSource* out_source);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_notification_source_set_callback([NativeTypeName("mln_notification_source")] MlnNotificationSource source, [NativeTypeName("mln_notification_callback")] delegate* unmanaged[Cdecl]<void*, void> callback, void* user_data);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_notification_source_clear_callback([NativeTypeName("mln_notification_source")] MlnNotificationSource source);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_notification_source_drain_ready([NativeTypeName("mln_notification_source")] MlnNotificationSource source, [NativeTypeName("mln_ready_batch *")] MlnReadyBatch* out_batch);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_ready_batch_get([NativeTypeName("mln_ready_batch")] MlnReadyBatch batch, mln_ready_batch_view* out_view);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_ready_batch_release([NativeTypeName("mln_ready_batch")] MlnReadyBatch batch);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_notification_source_close([NativeTypeName("mln_notification_source")] MlnNotificationSource source);
    }
}
