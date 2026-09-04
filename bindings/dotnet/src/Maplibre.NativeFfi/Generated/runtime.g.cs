using System.Runtime.InteropServices;
using static Maplibre.NativeFfi.Internal.C.mln_runtime_event_type;

namespace Maplibre.NativeFfi.Internal.C
{
    [NativeTypeName("uint32_t")]
    internal enum mln_network_status : uint
    {
        MLN_NETWORK_STATUS_ONLINE = 1,
        MLN_NETWORK_STATUS_OFFLINE = 2,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_ambient_cache_operation : uint
    {
        MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE = 1,
        MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE = 2,
        MLN_AMBIENT_CACHE_OPERATION_INVALIDATE = 3,
        MLN_AMBIENT_CACHE_OPERATION_CLEAR = 4,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_offline_region_definition_type : uint
    {
        MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID = 1,
        MLN_OFFLINE_REGION_DEFINITION_GEOMETRY = 2,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_offline_region_download_state : uint
    {
        MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE = 0,
        MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE = 1,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_offline_operation_kind : uint
    {
        MLN_OFFLINE_OPERATION_AMBIENT_CACHE = 1,
        MLN_OFFLINE_OPERATION_REGION_CREATE = 2,
        MLN_OFFLINE_OPERATION_REGION_GET = 3,
        MLN_OFFLINE_OPERATION_REGIONS_LIST = 4,
        MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE = 5,
        MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA = 6,
        MLN_OFFLINE_OPERATION_REGION_GET_STATUS = 7,
        MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED = 8,
        MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE = 9,
        MLN_OFFLINE_OPERATION_REGION_INVALIDATE = 10,
        MLN_OFFLINE_OPERATION_REGION_DELETE = 11,
        MLN_OFFLINE_OPERATION_SET_MAXIMUM_AMBIENT_CACHE_SIZE = 12,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_offline_operation_result_kind : uint
    {
        MLN_OFFLINE_OPERATION_RESULT_NONE = 0,
        MLN_OFFLINE_OPERATION_RESULT_REGION = 1,
        MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION = 2,
        MLN_OFFLINE_OPERATION_RESULT_REGION_LIST = 3,
        MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS = 4,
    }

    internal partial struct mln_offline_region_status
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint download_state;

        [NativeTypeName("uint64_t")]
        public ulong completed_resource_count;

        [NativeTypeName("uint64_t")]
        public ulong completed_resource_size;

        [NativeTypeName("uint64_t")]
        public ulong completed_tile_count;

        [NativeTypeName("uint64_t")]
        public ulong required_tile_count;

        [NativeTypeName("uint64_t")]
        public ulong completed_tile_size;

        [NativeTypeName("uint64_t")]
        public ulong required_resource_count;

        [NativeTypeName("bool")]
        public byte required_resource_count_is_precise;

        [NativeTypeName("bool")]
        public byte complete;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_runtime_event_type : uint
    {
        MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE = 1,
        MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING = 2,
        MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE = 3,
        MLN_RUNTIME_EVENT_MAP_STYLE_LOADED = 4,
        MLN_RUNTIME_EVENT_MAP_LOADING_STARTED = 5,
        MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED = 6,
        MLN_RUNTIME_EVENT_MAP_LOADING_FAILED = 7,
        MLN_RUNTIME_EVENT_MAP_IDLE = 8,
        MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE = 9,
        MLN_RUNTIME_EVENT_MAP_RENDER_ERROR = 10,
        MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED = 11,
        MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED = 12,
        MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED = 13,
        MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED = 14,
        MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED = 15,
        MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED = 16,
        MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING = 17,
        MLN_RUNTIME_EVENT_MAP_TILE_ACTION = 18,
        MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED = 19,
        MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR = 20,
        MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 21,
        MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED = 22,
        MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED = 23,
    }

    [NativeTypeName("uint64_t")]
    internal enum mln_runtime_event_mask : ulong
    {
        MLN_RUNTIME_EVENT_MASK_NONE = 0,
        MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_WILL_CHANGE = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE),
        MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_IS_CHANGING = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING),
        MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE),
        MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_STYLE_LOADED),
        MLN_RUNTIME_EVENT_MASK_MAP_LOADING_STARTED = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_LOADING_STARTED),
        MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FINISHED = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED),
        MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_LOADING_FAILED),
        MLN_RUNTIME_EVENT_MASK_MAP_IDLE = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_IDLE),
        MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE),
        MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_RENDER_ERROR),
        MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FINISHED = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED),
        MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED),
        MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_STARTED = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED),
        MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED),
        MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_STARTED = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED),
        MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_FINISHED = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED),
        MLN_RUNTIME_EVENT_MASK_MAP_STYLE_IMAGE_MISSING = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING),
        MLN_RUNTIME_EVENT_MASK_MAP_TILE_ACTION = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_TILE_ACTION),
        MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED = 1UL << (int)(MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED),
        MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED = 1UL << (int)(MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED),
        MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_RESPONSE_ERROR = 1UL << (int)(MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR),
        MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 1UL << (int)(MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED),
        MLN_RUNTIME_EVENT_MASK_OFFLINE_OPERATION_COMPLETED = 1UL << (int)(MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED),
        MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS = MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_WILL_CHANGE | MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_IS_CHANGING | MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE | MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED | MLN_RUNTIME_EVENT_MASK_MAP_LOADING_STARTED | MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FINISHED | MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED | MLN_RUNTIME_EVENT_MASK_MAP_IDLE | MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE | MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR | MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FINISHED | MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED | MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_STARTED | MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED | MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_STARTED | MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_FINISHED | MLN_RUNTIME_EVENT_MASK_MAP_STYLE_IMAGE_MISSING | MLN_RUNTIME_EVENT_MASK_MAP_TILE_ACTION | MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED,
        MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS = MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED | MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_RESPONSE_ERROR | MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED | MLN_RUNTIME_EVENT_MASK_OFFLINE_OPERATION_COMPLETED,
        MLN_RUNTIME_EVENT_MASK_ALL = MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS | MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_runtime_event_source_type : uint
    {
        MLN_RUNTIME_EVENT_SOURCE_RUNTIME = 0,
        MLN_RUNTIME_EVENT_SOURCE_MAP = 1,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_runtime_event_payload_type : uint
    {
        MLN_RUNTIME_EVENT_PAYLOAD_NONE = 0,
        MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME = 1,
        MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP = 2,
        MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION = 4,
        MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS = 5,
        MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR = 6,
        MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT = 7,
        MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED = 8,
        MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED = 9,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_camera_change_mode : uint
    {
        MLN_CAMERA_CHANGE_MODE_IMMEDIATE = 0,
        MLN_CAMERA_CHANGE_MODE_ANIMATED = 1,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_render_mode : uint
    {
        MLN_RENDER_MODE_PARTIAL = 0,
        MLN_RENDER_MODE_FULL = 1,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_tile_operation : uint
    {
        MLN_TILE_OPERATION_REQUESTED_FROM_CACHE = 0,
        MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK = 1,
        MLN_TILE_OPERATION_LOAD_FROM_NETWORK = 2,
        MLN_TILE_OPERATION_LOAD_FROM_CACHE = 3,
        MLN_TILE_OPERATION_START_PARSE = 4,
        MLN_TILE_OPERATION_END_PARSE = 5,
        MLN_TILE_OPERATION_ERROR = 6,
        MLN_TILE_OPERATION_CANCELLED = 7,
        MLN_TILE_OPERATION_NULL = 8,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_resource_kind : uint
    {
        MLN_RESOURCE_KIND_UNKNOWN = 0,
        MLN_RESOURCE_KIND_STYLE = 1,
        MLN_RESOURCE_KIND_SOURCE = 2,
        MLN_RESOURCE_KIND_TILE = 3,
        MLN_RESOURCE_KIND_GLYPHS = 4,
        MLN_RESOURCE_KIND_SPRITE_IMAGE = 5,
        MLN_RESOURCE_KIND_SPRITE_JSON = 6,
        MLN_RESOURCE_KIND_IMAGE = 7,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_resource_loading_method : uint
    {
        MLN_RESOURCE_LOADING_METHOD_ALL = 0,
        MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY = 1,
        MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY = 2,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_resource_priority : uint
    {
        MLN_RESOURCE_PRIORITY_REGULAR = 0,
        MLN_RESOURCE_PRIORITY_LOW = 1,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_resource_usage : uint
    {
        MLN_RESOURCE_USAGE_ONLINE = 0,
        MLN_RESOURCE_USAGE_OFFLINE = 1,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_resource_storage_policy : uint
    {
        MLN_RESOURCE_STORAGE_POLICY_PERMANENT = 0,
        MLN_RESOURCE_STORAGE_POLICY_VOLATILE = 1,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_resource_response_status : uint
    {
        MLN_RESOURCE_RESPONSE_STATUS_OK = 0,
        MLN_RESOURCE_RESPONSE_STATUS_ERROR = 1,
        MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT = 2,
        MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED = 3,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_resource_error_reason : uint
    {
        MLN_RESOURCE_ERROR_REASON_NONE = 0,
        MLN_RESOURCE_ERROR_REASON_NOT_FOUND = 1,
        MLN_RESOURCE_ERROR_REASON_SERVER = 2,
        MLN_RESOURCE_ERROR_REASON_CONNECTION = 3,
        MLN_RESOURCE_ERROR_REASON_RATE_LIMIT = 4,
        MLN_RESOURCE_ERROR_REASON_OTHER = 5,
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_resource_provider_decision : uint
    {
        MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH = 0,
        MLN_RESOURCE_PROVIDER_DECISION_HANDLE = 1,
    }

    internal unsafe partial struct mln_runtime_options
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint flags;

        [NativeTypeName("const char *")]
        public sbyte* asset_path;

        [NativeTypeName("const char *")]
        public sbyte* cache_path;

        [NativeTypeName("uint64_t")]
        public ulong event_mask;
    }

    internal partial struct mln_rendering_stats
    {
        public double encoding_time;

        public double rendering_time;

        [NativeTypeName("int64_t")]
        public long frame_count;

        [NativeTypeName("int64_t")]
        public long draw_call_count;

        [NativeTypeName("int64_t")]
        public long total_draw_call_count;
    }

    internal partial struct mln_runtime_event_render_frame
    {
        [NativeTypeName("uint32_t")]
        public uint mode;

        [NativeTypeName("bool")]
        public byte needs_repaint;

        [NativeTypeName("bool")]
        public byte placement_changed;

        public mln_rendering_stats stats;
    }

    internal partial struct mln_runtime_event_render_map
    {
        [NativeTypeName("uint32_t")]
        public uint mode;
    }

    internal partial struct mln_tile_id
    {
        [NativeTypeName("uint32_t")]
        public uint overscaled_z;

        [NativeTypeName("int32_t")]
        public int wrap;

        [NativeTypeName("uint32_t")]
        public uint canonical_z;

        [NativeTypeName("uint32_t")]
        public uint canonical_x;

        [NativeTypeName("uint32_t")]
        public uint canonical_y;
    }

    internal partial struct mln_runtime_event_tile_action
    {
        [NativeTypeName("uint32_t")]
        public uint operation;

        public mln_tile_id tile_id;
    }

    internal partial struct mln_runtime_event_camera_transition_finished
    {
        [NativeTypeName("uint64_t")]
        public ulong transition_id;
    }

    internal partial struct mln_runtime_event_offline_region_status
    {
        [NativeTypeName("mln_offline_region_id")]
        public long region_id;

        public mln_offline_region_status status;
    }

    internal partial struct mln_runtime_event_offline_region_response_error
    {
        [NativeTypeName("mln_offline_region_id")]
        public long region_id;

        [NativeTypeName("uint32_t")]
        public uint reason;
    }

    internal partial struct mln_runtime_event_offline_region_tile_count_limit
    {
        [NativeTypeName("mln_offline_region_id")]
        public long region_id;

        [NativeTypeName("uint64_t")]
        public ulong limit;
    }

    internal partial struct mln_runtime_event_offline_operation_completed
    {
        [NativeTypeName("mln_offline_operation_id")]
        public ulong operation_id;

        [NativeTypeName("uint32_t")]
        public uint operation_kind;

        [NativeTypeName("uint32_t")]
        public uint result_kind;

        [NativeTypeName("int32_t")]
        public int result_status;

        [NativeTypeName("bool")]
        public byte found;
    }

    [StructLayout(LayoutKind.Explicit)]
    internal partial struct mln_runtime_event_payload
    {
        [FieldOffset(0)]
        public mln_runtime_event_render_frame render_frame;

        [FieldOffset(0)]
        public mln_runtime_event_render_map render_map;

        [FieldOffset(0)]
        public mln_runtime_event_tile_action tile_action;

        [FieldOffset(0)]
        public mln_runtime_event_offline_region_status offline_region_status;

        [FieldOffset(0)]
        public mln_runtime_event_offline_region_response_error offline_region_response_error;

        [FieldOffset(0)]
        public mln_runtime_event_offline_region_tile_count_limit offline_region_tile_count_limit;

        [FieldOffset(0)]
        public mln_runtime_event_offline_operation_completed offline_operation_completed;

        [FieldOffset(0)]
        public mln_runtime_event_camera_transition_finished camera_transition_finished;
    }

    internal partial struct mln_runtime_event
    {
        [NativeTypeName("uint32_t")]
        public uint type;

        [NativeTypeName("uint32_t")]
        public uint source_type;

        [NativeTypeName("uint64_t")]
        public ulong source;

        [NativeTypeName("int32_t")]
        public int code;

        [NativeTypeName("uint32_t")]
        public uint payload_type;

        [NativeTypeName("uint32_t")]
        public uint message_offset;

        [NativeTypeName("uint32_t")]
        public uint message_size;

        public mln_runtime_event_payload payload;
    }

    internal unsafe partial struct mln_runtime_event_batch
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint event_size;

        [NativeTypeName("const mln_runtime_event *")]
        public mln_runtime_event* events;

        [NativeTypeName("size_t")]
        public nuint event_count;

        [NativeTypeName("const char *")]
        public sbyte* messages;

        [NativeTypeName("size_t")]
        public nuint messages_size;

        [NativeTypeName("size_t")]
        public nuint remaining_count;
    }

    internal unsafe partial struct mln_resource_transform_response
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("const char *")]
        public sbyte* url;

        public void* context;
    }

    internal unsafe partial struct mln_resource_transform
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("mln_resource_transform_callback")]
        public delegate* unmanaged[Cdecl]<void*, uint, sbyte*, mln_resource_transform_response*, mln_status> callback;

        public void* user_data;
    }

    internal unsafe partial struct mln_http_header_transform_response
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        public void* context;
    }

    internal unsafe partial struct mln_http_header_transform
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("mln_http_header_transform_callback")]
        public delegate* unmanaged[Cdecl]<void*, uint, sbyte*, mln_http_header_transform_response*, mln_status> callback;

        public void* user_data;
    }

    internal unsafe partial struct mln_resource_request
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("const char *")]
        public sbyte* requested_url;

        [NativeTypeName("const char *")]
        public sbyte* resolved_url;

        [NativeTypeName("uint32_t")]
        public uint kind;

        [NativeTypeName("uint32_t")]
        public uint loading_method;

        [NativeTypeName("uint32_t")]
        public uint priority;

        [NativeTypeName("uint32_t")]
        public uint usage;

        [NativeTypeName("uint32_t")]
        public uint storage_policy;

        [NativeTypeName("bool")]
        public byte has_range;

        [NativeTypeName("uint64_t")]
        public ulong range_start;

        [NativeTypeName("uint64_t")]
        public ulong range_end;

        [NativeTypeName("bool")]
        public byte has_prior_modified;

        [NativeTypeName("int64_t")]
        public long prior_modified_unix_ms;

        [NativeTypeName("bool")]
        public byte has_prior_expires;

        [NativeTypeName("int64_t")]
        public long prior_expires_unix_ms;

        [NativeTypeName("const char *")]
        public sbyte* prior_etag;

        [NativeTypeName("const uint8_t *")]
        public byte* prior_data;

        [NativeTypeName("size_t")]
        public nuint prior_data_size;
    }

    internal unsafe partial struct mln_resource_response
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("uint32_t")]
        public uint status;

        [NativeTypeName("uint32_t")]
        public uint error_reason;

        [NativeTypeName("const uint8_t *")]
        public byte* bytes;

        [NativeTypeName("size_t")]
        public nuint byte_count;

        [NativeTypeName("const char *")]
        public sbyte* error_message;

        [NativeTypeName("bool")]
        public byte must_revalidate;

        [NativeTypeName("bool")]
        public byte has_modified;

        [NativeTypeName("int64_t")]
        public long modified_unix_ms;

        [NativeTypeName("bool")]
        public byte has_expires;

        [NativeTypeName("int64_t")]
        public long expires_unix_ms;

        [NativeTypeName("const char *")]
        public sbyte* etag;

        [NativeTypeName("bool")]
        public byte has_retry_after;

        [NativeTypeName("int64_t")]
        public long retry_after_unix_ms;
    }

    internal unsafe partial struct mln_resource_provider
    {
        [NativeTypeName("uint32_t")]
        public uint size;

        [NativeTypeName("mln_resource_provider_callback")]
        public delegate* unmanaged[Cdecl]<void*, mln_resource_request*, MlnResourceRequest, uint> callback;

        public void* user_data;
    }

    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_network_status_get([NativeTypeName("uint32_t *")] uint* out_status);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_network_status_set([NativeTypeName("uint32_t")] uint status);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_resource_transform_response_set_url(mln_resource_transform_response* response, [NativeTypeName("const char *")] sbyte* url, [NativeTypeName("size_t")] nuint url_size);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_http_header_transform_response_set(mln_http_header_transform_response* response, [NativeTypeName("const char *")] sbyte* name, [NativeTypeName("size_t")] nuint name_size, [NativeTypeName("const char *")] sbyte* value, [NativeTypeName("size_t")] nuint value_size);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_runtime_options mln_runtime_options_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_create([NativeTypeName("const mln_runtime_options *")] mln_runtime_options* options, [NativeTypeName("mln_runtime *")] MlnRuntime* out_runtime);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_set_resource_provider([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("const mln_resource_provider *")] mln_resource_provider* provider);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_clear_resource_provider([NativeTypeName("mln_runtime")] MlnRuntime runtime);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_resource_request_complete([NativeTypeName("mln_resource_request_handle")] MlnResourceRequest handle, [NativeTypeName("const mln_resource_response *")] mln_resource_response* response);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_resource_request_cancelled([NativeTypeName("mln_resource_request_handle")] MlnResourceRequest handle, bool* out_cancelled);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_resource_request_set_cancel_callback([NativeTypeName("mln_resource_request_handle")] MlnResourceRequest handle, [NativeTypeName("mln_resource_request_cancel_callback")] delegate* unmanaged[Cdecl]<void*, void> callback, void* user_data, bool* out_cancelled);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_resource_request_release([NativeTypeName("mln_resource_request_handle")] MlnResourceRequest handle);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_resource_request_wait_until_retired([NativeTypeName("mln_resource_request_handle")] MlnResourceRequest handle);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_set_resource_transform([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("const mln_resource_transform *")] mln_resource_transform* transform);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_clear_resource_transform([NativeTypeName("mln_runtime")] MlnRuntime runtime);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_set_http_header_transform([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("const mln_http_header_transform *")] mln_http_header_transform* transform);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_clear_http_header_transform([NativeTypeName("mln_runtime")] MlnRuntime runtime);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_run_ambient_cache_operation_start([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("uint32_t")] uint operation, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_set_maximum_ambient_cache_size_start([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("uint64_t")] ulong size, [NativeTypeName("mln_offline_operation_id *")] ulong* out_operation_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_offline_operation_discard([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("mln_offline_operation_id")] ulong operation_id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_destroy([NativeTypeName("mln_runtime")] MlnRuntime runtime);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_pump([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("int64_t")] long timeout_ms, [NativeTypeName("int64_t")] long budget_ms);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_wake_source_acquire([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("mln_wake_source *")] MlnWakeSource* out_source);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_wake_source_signal([NativeTypeName("mln_wake_source")] MlnWakeSource source);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_wake_source_destroy([NativeTypeName("mln_wake_source")] MlnWakeSource source);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_runtime_event_batch mln_runtime_event_batch_default();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_drain_events([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("size_t")] nuint max_events, mln_runtime_event_batch* out_batch);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_set_event_mask([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("uint64_t")] ulong mask);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_runtime_get_event_mask([NativeTypeName("mln_runtime")] MlnRuntime runtime, [NativeTypeName("uint64_t *")] ulong* out_mask);
    }
}
