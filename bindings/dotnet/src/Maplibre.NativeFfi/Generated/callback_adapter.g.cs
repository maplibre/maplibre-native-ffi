using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.C
{
    internal unsafe partial struct mln_adapter_completion_record
    {
        public void* owner;

        public mln_completion_result result;
    }

    internal unsafe partial struct mln_adapter_deferred_call_record
    {
        public void* owner;

        [NativeTypeName("uint32_t")]
        public uint callback;

        [NativeTypeName("const void *")]
        public void* arguments;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_adapter_url_match_flags : uint
    {
        MLN_ADAPTER_URL_MATCH_FLAGS_NONE = 0U,
        MLN_ADAPTER_URL_MATCH_GLOB = 1U << 0,
    }

    internal unsafe partial struct mln_adapter_resource_rewrite_rule
    {
        [NativeTypeName("uint32_t")]
        public uint kind;

        [NativeTypeName("uint32_t")]
        public uint flags;

        [NativeTypeName("const char *")]
        public sbyte* url;

        [NativeTypeName("const char *")]
        public sbyte* replacement_url;
    }

    internal unsafe partial struct mln_adapter_resource_rewrite_rules
    {
        [NativeTypeName("const mln_adapter_resource_rewrite_rule *")]
        public mln_adapter_resource_rewrite_rule* rules;

        [NativeTypeName("size_t")]
        public nuint count;
    }

    internal unsafe partial struct mln_adapter_http_header
    {
        [NativeTypeName("const char *")]
        public sbyte* name;

        [NativeTypeName("const char *")]
        public sbyte* value;
    }

    internal unsafe partial struct mln_adapter_http_header_transform_rule
    {
        [NativeTypeName("uint32_t")]
        public uint kind;

        [NativeTypeName("uint32_t")]
        public uint flags;

        [NativeTypeName("const char *")]
        public sbyte* url;

        [NativeTypeName("const mln_adapter_http_header *")]
        public mln_adapter_http_header* headers;

        [NativeTypeName("size_t")]
        public nuint header_count;
    }

    internal unsafe partial struct mln_adapter_http_header_transform_rules
    {
        [NativeTypeName("const mln_adapter_http_header_transform_rule *")]
        public mln_adapter_http_header_transform_rule* rules;

        [NativeTypeName("size_t")]
        public nuint count;
    }

    internal unsafe partial struct mln_adapter_resource_provider_rule
    {
        [NativeTypeName("uint32_t")]
        public uint kind;

        [NativeTypeName("uint32_t")]
        public uint flags;

        [NativeTypeName("const char *")]
        public sbyte* requested_url;

        public mln_resource_response response;
    }

    internal unsafe partial struct mln_adapter_resource_provider_rules
    {
        [NativeTypeName("const mln_adapter_resource_provider_rule *")]
        public mln_adapter_resource_provider_rule* rules;

        [NativeTypeName("size_t")]
        public nuint count;
    }

    [NativeTypeName("uint32_t")]
    internal enum mln_adapter_resource_route_flags : uint
    {
        MLN_ADAPTER_RESOURCE_ROUTE_FLAGS_NONE = 0U,
        MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB = 1U << 0,
        MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL = 1U << 1,
    }

    internal unsafe partial struct mln_adapter_resource_route
    {
        [NativeTypeName("uint32_t")]
        public uint kind;

        [NativeTypeName("uint32_t")]
        public uint flags;

        [NativeTypeName("const char *")]
        public sbyte* url;
    }

    internal unsafe partial struct mln_adapter_routed_resource_provider
    {
        [NativeTypeName("const mln_adapter_resource_route *")]
        public mln_adapter_resource_route* routes;

        [NativeTypeName("size_t")]
        public nuint route_count;

        [NativeTypeName("mln_resource_provider_callback")]
        public delegate* unmanaged[Cdecl]<void*, mln_resource_request*, MlnResourceRequest, uint> callback;

        public void* user_data;
    }

    internal static unsafe partial class NativeMethods
    {
        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_completion_create([NativeTypeName("uint32_t")] uint copy_kind, [NativeTypeName("size_t")] nuint element_size, [NativeTypeName("mln_adapter_completion_listener")] delegate* unmanaged[Cdecl]<void*, mln_adapter_completion_record*, void> listener, void* user_data, mln_completion* out_completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_completion_reject(mln_completion* completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_completion_record_adopt(mln_adapter_completion_record* record);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_completion_record_destroy(mln_adapter_completion_record* record);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_deferred_callback_create([NativeTypeName("uint32_t")] uint callback, [NativeTypeName("mln_adapter_deferred_call_listener")] delegate* unmanaged[Cdecl]<void*, mln_adapter_deferred_call_record*, void> listener, void* listener_user_data, void** out_context);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_dart_deferred_callback_create([NativeTypeName("uint32_t")] uint callback, void* post_cobject, [NativeTypeName("int64_t")] long port, void** out_context);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void* mln_adapter_deferred_callback_function([NativeTypeName("uint32_t")] uint callback);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_deferred_callback_release(void* context);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_deferred_call_record_adopt(mln_adapter_deferred_call_record* record);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_deferred_call_record_destroy(mln_adapter_deferred_call_record* record);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_dart_wake_create(void* post_cobject, [NativeTypeName("int64_t")] long port, mln_wake* out_wake);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_dart_completion_create([NativeTypeName("uint32_t")] uint copy_kind, [NativeTypeName("size_t")] nuint element_size, void* post_cobject, [NativeTypeName("int64_t")] long port, [NativeTypeName("int64_t")] long token, mln_completion* out_completion);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void* mln_adapter_dart_port_create(void* post_cobject, [NativeTypeName("int64_t")] long port);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void* mln_adapter_dart_port_function([NativeTypeName("uint32_t")] uint id);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_dart_port_release(void* context);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void* mln_adapter_arena_create();

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void* mln_adapter_arena_allocate(void* arena, [NativeTypeName("size_t")] nuint size, [NativeTypeName("size_t")] nuint alignment);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_arena_destroy(void* arena);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_arena_adopt_handle(void* arena, [NativeTypeName("uint64_t")] ulong handle);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_arena_adopt_release(void* arena, [NativeTypeName("mln_runtime_callback_release")] delegate* unmanaged[Cdecl]<void*, void> release, void* context);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_dart_release_register(void* post_cobject, [NativeTypeName("int64_t")] long port, void* context, void* arena, [NativeTypeName("uint64_t *")] ulong* out_registration);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_dart_release(void* context);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void* mln_adapter_owner_token_create([NativeTypeName("uint64_t")] ulong handle);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_owner_token_destroy(void* token);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_owner_finalize(void* token);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_resource_transform_rewrite_callback(void* user_data, [NativeTypeName("uint32_t")] uint kind, [NativeTypeName("const char *")] sbyte* url, mln_resource_transform_response* out_response);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_http_header_transform_callback(void* user_data, [NativeTypeName("uint32_t")] uint kind, [NativeTypeName("const char *")] sbyte* url, mln_http_header_transform_response* out_response);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_http_header_validate([NativeTypeName("const char *")] sbyte* name, [NativeTypeName("const char *")] sbyte* value);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint mln_adapter_resource_provider_rules_callback(void* user_data, [NativeTypeName("const mln_resource_request *")] mln_resource_request* request, [NativeTypeName("mln_resource_request_handle")] MlnResourceRequest handle);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint32_t")]
        public static extern uint mln_adapter_routed_resource_provider_callback(void* user_data, [NativeTypeName("const mln_resource_request *")] mln_resource_request* request, [NativeTypeName("mln_resource_request_handle")] MlnResourceRequest handle);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_custom_geometry_callbacks_retire([NativeTypeName("mln_custom_geometry_source_tile_callback")] delegate* unmanaged[Cdecl]<void*, mln_canonical_tile_id, void> fetch_tile, [NativeTypeName("mln_custom_geometry_source_tile_callback")] delegate* unmanaged[Cdecl]<void*, mln_canonical_tile_id, void> cancel_tile, void* user_data);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_custom_mvt_vector_callbacks_retire([NativeTypeName("mln_custom_mvt_vector_source_tile_callback")] delegate* unmanaged[Cdecl]<void*, mln_canonical_tile_id, void> fetch_tile, [NativeTypeName("mln_custom_mvt_vector_source_tile_callback")] delegate* unmanaged[Cdecl]<void*, mln_canonical_tile_id, void> cancel_tile, void* user_data);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern mln_status mln_adapter_acquired_frame_view_begin([NativeTypeName("mln_acquired_frame")] MlnAcquiredFrame frame, void** out_scope);

        [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void mln_adapter_acquired_frame_view_end(void* scope);
    }
}
