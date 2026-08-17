using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    [NativeTypeName("uint32_t")]
    internal enum mln_render_result : uint
    {
        MLN_RENDER_RESULT_RENDERED = 0,
        MLN_RENDER_RESULT_NO_UPDATE = 1,
        MLN_RENDER_RESULT_SIZE_PENDING = 2,
        MLN_RENDER_RESULT_TARGET_NOT_READY = 3,
        MLN_RENDER_RESULT_SUPERSEDED = 4,
        MLN_RENDER_RESULT_DEADLINE_MISSED = 5,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_render_session_state : uint
    {
        MLN_RENDER_SESSION_STATE_ATTACHING = 1U,
        MLN_RENDER_SESSION_STATE_ATTACHED = 2U,
        MLN_RENDER_SESSION_STATE_DETACHING = 3U,
        MLN_RENDER_SESSION_STATE_DETACHED = 4U,
        MLN_RENDER_SESSION_STATE_TARGET_LOST = 5U,
        MLN_RENDER_SESSION_STATE_ABANDONED = 6U,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_frame_demand_flag : uint
    {
        MLN_FRAME_DEMAND_IF_NEEDED = 1U << 0,
        MLN_FRAME_DEMAND_PRESENT = 1U << 1,
    }

    internal partial struct mln_frame_demand
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint flags;

        [NativeTypeName("uint64_t")]
        public ulong token;

        [NativeTypeName("uint64_t")]
        public ulong coalescing_boundary;

        [NativeTypeName("uint64_t")]
        public ulong timeout_ns;
    }

    internal partial struct mln_render_frame_result
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint disposition;

        [NativeTypeName("uint64_t")]
        public ulong token;

        [NativeTypeName("uint64_t")]
        public ulong map_update_generation;

        [NativeTypeName("uint64_t")]
        public ulong extent_generation;

        [NativeTypeName("uint64_t")]
        public ulong frame_generation;

        [NativeTypeName("bool")]
        public byte needs_repaint;
    }

    internal partial struct mln_render_session_snapshot
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint state;

        [NativeTypeName("uint32_t")]
        public uint driver;

        [NativeTypeName("uint32_t")]
        public uint latest_result;

        public mln_render_target_extent extent;

        [NativeTypeName("uint64_t")]
        public ulong generation;

        [NativeTypeName("uint64_t")]
        public ulong map_update_generation;

        [NativeTypeName("uint64_t")]
        public ulong rendered_update_generation;

        [NativeTypeName("uint64_t")]
        public ulong extent_generation;

        [NativeTypeName("uint64_t")]
        public ulong frame_generation;

        [NativeTypeName("uint64_t")]
        public ulong latest_demand_token;

        [NativeTypeName("uint32_t")]
        public uint pending_demand_count;

        [NativeTypeName("uint32_t")]
        public uint acquired_frame_count;

        [NativeTypeName("bool")]
        public byte target_ready;

        [NativeTypeName("bool")]
        public byte pending_changes;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_render_abandon_disposition : uint
    {
        MLN_RENDER_ABANDON_DISPOSITION_CLEAN = 0U,
        MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED = 1U,
    }

    internal partial struct mln_render_abandon_result
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint disposition;

        [NativeTypeName("uint32_t")]
        public uint quarantined_resource_count;

        [NativeTypeName("uint32_t")]
        public uint reserved;
    }

    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_frame_demand mln_frame_demand_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_get_capabilities([NativeTypeName("mln_render_session")] MlnRenderSession session, mln_render_session_capabilities* out_capabilities);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_get_snapshot([NativeTypeName("mln_render_session")] MlnRenderSession session, mln_render_session_snapshot* out_snapshot);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_request_frame([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("const mln_frame_demand *")] mln_frame_demand* demand);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_drain_frame_results([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("mln_render_frame_batch *")] MlnRenderFrameBatch* out_batch);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_frame_batch_count([NativeTypeName("mln_render_frame_batch")] MlnRenderFrameBatch batch, [NativeTypeName("size_t *")] nuint* out_count);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_frame_batch_get([NativeTypeName("mln_render_frame_batch")] MlnRenderFrameBatch batch, [NativeTypeName("size_t")] nuint index, mln_render_frame_result* out_result);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_render_frame_batch_release([NativeTypeName("mln_render_frame_batch")] MlnRenderFrameBatch batch);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_acquire_frame([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("mln_acquired_frame *")] MlnAcquiredFrame* out_frame);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_acquired_frame_get_result([NativeTypeName("mln_acquired_frame")] MlnAcquiredFrame frame, mln_render_frame_result* out_result);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_acquired_frame_get_producer_sync([NativeTypeName("mln_acquired_frame")] MlnAcquiredFrame frame, mln_gpu_sync* out_sync);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_acquired_frame_release_start([NativeTypeName("mln_acquired_frame *")] MlnAcquiredFrame* frame, [NativeTypeName("const mln_gpu_sync *")] mln_gpu_sync* consumer_completion, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_resize_start([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("const mln_render_target_extent *")] mln_render_target_extent* extent, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_barrier_start([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_reduce_memory_use_start([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_clear_data_start([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_dump_debug_logs_start([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_set_feature_state_start([NativeTypeName("mln_render_session")] MlnRenderSession session, mln_buffer_view source_id, mln_buffer_view source_layer_id, mln_buffer_view feature_id, mln_buffer_view state_json, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_get_feature_state_start([NativeTypeName("mln_render_session")] MlnRenderSession session, mln_buffer_view source_id, mln_buffer_view source_layer_id, mln_buffer_view feature_id, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_get_feature_state_take_result([NativeTypeName("mln_operation")] MlnOperation operation, [NativeTypeName("mln_buffer *")] MlnBuffer* out_state_json);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_remove_feature_state_start([NativeTypeName("mln_render_session")] MlnRenderSession session, mln_buffer_view source_id, mln_buffer_view source_layer_id, mln_buffer_view feature_id, mln_buffer_view state_key, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_service_driver_work([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("size_t")] nuint max_work, [NativeTypeName("size_t *")] nuint* out_serviced);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_detach_start([NativeTypeName("mln_render_session")] MlnRenderSession session, [NativeTypeName("mln_operation *")] MlnOperation* out_operation);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_abandon([NativeTypeName("mln_render_session")] MlnRenderSession session, mln_render_abandon_result* out_result);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_render_session_destroy([NativeTypeName("mln_render_session")] MlnRenderSession session);
    }
}
