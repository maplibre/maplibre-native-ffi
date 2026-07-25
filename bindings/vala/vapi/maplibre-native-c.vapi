/* Private raw VAPI for the MapLibre Native C API. */

[CCode (cprefix = "mln_", lower_case_cprefix = "mln_", cheader_filename = "maplibre_native_c/base.h,maplibre_native_c/diagnostics.h,maplibre_native_c/logging.h,maplibre_native_c/runtime.h,maplibre_native_c/map.h,maplibre_native_c/camera.h,maplibre_native_c/style.h,maplibre_native_c/query.h,maplibre_native_c/render_target.h,maplibre_native_c/texture.h,maplibre_native_c/surface.h,maplibre_native_c/render_session.h,maplibre_native_c/projection.h")]
namespace MaplibreNative.Raw {
    [CCode (cname = "mln_status", cprefix = "MLN_STATUS_", has_type_id = false)]
    public enum Status {
        OK,
        INVALID_ARGUMENT,
        INVALID_STATE,
        WRONG_THREAD,
        UNSUPPORTED,
        NATIVE_ERROR
    }

    [CCode (cname = "mln_render_backend_flag", cprefix = "MLN_RENDER_BACKEND_FLAG_", has_type_id = false)]
    [Flags]
    public enum RenderBackendFlag {
        METAL,
        VULKAN,
        OPENGL
    }

    [CCode (cname = "mln_opengl_context_provider_flag", cprefix = "MLN_OPENGL_CONTEXT_PROVIDER_FLAG_", has_type_id = false)]
    [Flags]
    public enum OpenGLContextProviderFlag {
        WGL,
        EGL
    }

    [CCode (cname = "mln_opengl_context_platform", cprefix = "MLN_OPENGL_CONTEXT_PLATFORM_", has_type_id = false)]
    public enum OpenGLContextPlatform {
        UNSPECIFIED,
        WGL,
        EGL
    }

    [CCode (cname = "mln_network_status", cprefix = "MLN_NETWORK_STATUS_", has_type_id = false)]
    public enum NetworkStatus {
        ONLINE,
        OFFLINE
    }

    [CCode (cname = "mln_map_mode", cprefix = "MLN_MAP_MODE_", has_type_id = false)]
    public enum MapMode {
        CONTINUOUS,
        STATIC,
        TILE
    }

    [CCode (cname = "mln_map_debug_option", cprefix = "MLN_MAP_DEBUG_", has_type_id = false)]
    [Flags]
    public enum MapDebugOption {
        TILE_BORDERS,
        PARSE_STATUS,
        TIMESTAMPS,
        COLLISION,
        OVERDRAW,
        STENCIL_CLIP,
        DEPTH_BUFFER
    }

    [CCode (cname = "mln_north_orientation", cprefix = "MLN_NORTH_ORIENTATION_", has_type_id = false)]
    public enum NorthOrientation {
        UP,
        RIGHT,
        DOWN,
        LEFT
    }

    [CCode (cname = "mln_constrain_mode", cprefix = "MLN_CONSTRAIN_MODE_", has_type_id = false)]
    public enum ConstrainMode {
        NONE,
        HEIGHT_ONLY,
        WIDTH_AND_HEIGHT,
        SCREEN
    }

    [CCode (cname = "mln_viewport_mode", cprefix = "MLN_VIEWPORT_MODE_", has_type_id = false)]
    public enum ViewportMode {
        DEFAULT,
        FLIPPED_Y
    }

    [CCode (cname = "mln_camera_option_field", cprefix = "MLN_CAMERA_OPTION_", has_type_id = false)]
    [Flags]
    public enum CameraOptionField {
        CENTER,
        ZOOM,
        BEARING,
        PITCH,
        CENTER_ALTITUDE,
        PADDING,
        ANCHOR,
        ROLL,
        FOV
    }

    [CCode (cname = "mln_animation_option_field", cprefix = "MLN_ANIMATION_OPTION_", has_type_id = false)]
    [Flags]
    public enum AnimationOptionField {
        DURATION,
        VELOCITY,
        MIN_ZOOM,
        EASING
    }

    [CCode (cname = "mln_camera_fit_option_field", cprefix = "MLN_CAMERA_FIT_OPTION_", has_type_id = false)]
    [Flags]
    public enum CameraFitOptionField {
        PADDING,
        BEARING,
        PITCH
    }

    [CCode (cname = "mln_bound_option_field", cprefix = "MLN_BOUND_OPTION_", has_type_id = false)]
    [Flags]
    public enum BoundOptionField {
        BOUNDS,
        MIN_ZOOM,
        MAX_ZOOM,
        MIN_PITCH,
        MAX_PITCH
    }

    [CCode (cname = "mln_free_camera_option_field", cprefix = "MLN_FREE_CAMERA_OPTION_", has_type_id = false)]
    [Flags]
    public enum FreeCameraOptionField {
        POSITION,
        ORIENTATION
    }

    [CCode (cname = "mln_projection_mode_field", cprefix = "MLN_PROJECTION_MODE_", has_type_id = false)]
    [Flags]
    public enum ProjectionModeField {
        AXONOMETRIC,
        X_SKEW,
        Y_SKEW
    }

    [CCode (cname = "mln_map_viewport_option_field", cprefix = "MLN_MAP_VIEWPORT_OPTION_", has_type_id = false)]
    [Flags]
    public enum MapViewportOptionField {
        NORTH_ORIENTATION,
        CONSTRAIN_MODE,
        VIEWPORT_MODE,
        FRUSTUM_OFFSET
    }

    [CCode (cname = "mln_tile_lod_mode", cprefix = "MLN_TILE_LOD_MODE_", has_type_id = false)]
    public enum TileLodMode {
        DEFAULT,
        DISTANCE
    }

    [CCode (cname = "mln_map_tile_option_field", cprefix = "MLN_MAP_TILE_OPTION_", has_type_id = false)]
    [Flags]
    public enum MapTileOptionField {
        PREFETCH_ZOOM_DELTA,
        LOD_MIN_RADIUS,
        LOD_SCALE,
        LOD_PITCH_THRESHOLD,
        LOD_ZOOM_SHIFT,
        LOD_MODE
    }

    [CCode (cname = "mln_runtime_option_flag", cprefix = "MLN_RUNTIME_OPTION_", has_type_id = false)]
    [Flags]
    public enum RuntimeOptionFlag {
        MAXIMUM_CACHE_SIZE
    }

    [CCode (cname = "mln_ambient_cache_operation", cprefix = "MLN_AMBIENT_CACHE_OPERATION_", has_type_id = false)]
    public enum AmbientCacheOperation {
        RESET_DATABASE,
        PACK_DATABASE,
        INVALIDATE,
        CLEAR
    }

    [SimpleType]
    [CCode (cname = "mln_offline_region_id", has_type_id = false)]
    public struct OfflineRegionId : int64 {
    }

    [SimpleType]
    [CCode (cname = "mln_offline_operation_id", has_type_id = false)]
    public struct OfflineOperationId : uint64 {
    }

    [CCode (cname = "mln_offline_region_definition_type", cprefix = "MLN_OFFLINE_REGION_DEFINITION_", has_type_id = false)]
    public enum OfflineRegionDefinitionType {
        TILE_PYRAMID,
        GEOMETRY
    }

    [CCode (cname = "mln_offline_region_download_state", cprefix = "MLN_OFFLINE_REGION_DOWNLOAD_", has_type_id = false)]
    public enum OfflineRegionDownloadState {
        INACTIVE,
        ACTIVE
    }

    [CCode (cname = "mln_offline_operation_kind", cprefix = "MLN_OFFLINE_OPERATION_", has_type_id = false)]
    public enum OfflineOperationKind {
        AMBIENT_CACHE,
        REGION_CREATE,
        REGION_GET,
        REGIONS_LIST,
        REGIONS_MERGE_DATABASE,
        REGION_UPDATE_METADATA,
        REGION_GET_STATUS,
        REGION_SET_OBSERVED,
        REGION_SET_DOWNLOAD_STATE,
        REGION_INVALIDATE,
        REGION_DELETE
    }

    [CCode (cname = "mln_offline_operation_result_kind", cprefix = "MLN_OFFLINE_OPERATION_RESULT_", has_type_id = false)]
    public enum OfflineOperationResultKind {
        NONE,
        REGION,
        OPTIONAL_REGION,
        REGION_LIST,
        REGION_STATUS
    }

    [CCode (cname = "mln_runtime_event_type", cprefix = "MLN_RUNTIME_EVENT_", has_type_id = false)]
    public enum RuntimeEventType {
        MAP_CAMERA_WILL_CHANGE,
        MAP_CAMERA_IS_CHANGING,
        MAP_CAMERA_DID_CHANGE,
        MAP_STYLE_LOADED,
        MAP_LOADING_STARTED,
        MAP_LOADING_FINISHED,
        MAP_LOADING_FAILED,
        MAP_IDLE,
        MAP_RENDER_UPDATE_AVAILABLE,
        MAP_RENDER_ERROR,
        MAP_STILL_IMAGE_FINISHED,
        MAP_STILL_IMAGE_FAILED,
        MAP_RENDER_FRAME_STARTED,
        MAP_RENDER_FRAME_FINISHED,
        MAP_RENDER_MAP_STARTED,
        MAP_RENDER_MAP_FINISHED,
        MAP_STYLE_IMAGE_MISSING,
        MAP_TILE_ACTION,
        OFFLINE_REGION_STATUS_CHANGED,
        OFFLINE_REGION_RESPONSE_ERROR,
        OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED,
        OFFLINE_OPERATION_COMPLETED
    }

    [CCode (cname = "mln_runtime_event_source_type", cprefix = "MLN_RUNTIME_EVENT_SOURCE_", has_type_id = false)]
    public enum RuntimeEventSourceType {
        RUNTIME,
        MAP
    }

    [CCode (cname = "mln_runtime_event_payload_type", cprefix = "MLN_RUNTIME_EVENT_PAYLOAD_", has_type_id = false)]
    public enum RuntimeEventPayloadType {
        NONE,
        RENDER_FRAME,
        RENDER_MAP,
        STYLE_IMAGE_MISSING,
        TILE_ACTION,
        OFFLINE_REGION_STATUS,
        OFFLINE_REGION_RESPONSE_ERROR,
        OFFLINE_REGION_TILE_COUNT_LIMIT,
        OFFLINE_OPERATION_COMPLETED
    }

    [CCode (cname = "mln_render_mode", cprefix = "MLN_RENDER_MODE_", has_type_id = false)]
    public enum RenderMode {
        PARTIAL,
        FULL
    }

    [CCode (cname = "mln_tile_operation", cprefix = "MLN_TILE_OPERATION_", has_type_id = false)]
    public enum TileOperation {
        REQUESTED_FROM_CACHE,
        REQUESTED_FROM_NETWORK,
        LOAD_FROM_NETWORK,
        LOAD_FROM_CACHE,
        START_PARSE,
        END_PARSE,
        ERROR,
        CANCELLED,
        NULL
    }

    [CCode (cname = "mln_log_severity", cprefix = "MLN_LOG_SEVERITY_", has_type_id = false)]
    public enum LogSeverity {
        INFO,
        WARNING,
        ERROR
    }

    [CCode (cname = "mln_log_severity_mask", cprefix = "MLN_LOG_SEVERITY_MASK_", has_type_id = false)]
    [Flags]
    public enum LogSeverityMask {
        INFO,
        WARNING,
        ERROR,
        DEFAULT,
        ALL
    }

    [CCode (cname = "mln_geometry_type", cprefix = "MLN_GEOMETRY_TYPE_", has_type_id = false)]
    public enum GeometryType {
        EMPTY,
        POINT,
        LINE_STRING,
        POLYGON,
        MULTI_POINT,
        MULTI_LINE_STRING,
        MULTI_POLYGON,
        GEOMETRY_COLLECTION
    }

    [CCode (cname = "mln_json_value_type", cprefix = "MLN_JSON_VALUE_TYPE_", has_type_id = false)]
    public enum JsonValueType {
        NULL,
        BOOL,
        UINT,
        INT,
        DOUBLE,
        STRING,
        ARRAY,
        OBJECT
    }

    [CCode (cname = "mln_feature_identifier_type", cprefix = "MLN_FEATURE_IDENTIFIER_TYPE_", has_type_id = false)]
    public enum FeatureIdentifierType {
        NULL,
        UINT,
        INT,
        DOUBLE,
        STRING
    }

    [CCode (cname = "mln_feature_state_selector_field", cprefix = "MLN_FEATURE_STATE_SELECTOR_", has_type_id = false)]
    [Flags]
    public enum FeatureStateSelectorField {
        SOURCE_LAYER_ID,
        FEATURE_ID,
        STATE_KEY
    }

    [CCode (cname = "mln_geojson_type", cprefix = "MLN_GEOJSON_TYPE_", has_type_id = false)]
    public enum GeoJsonType {
        GEOMETRY,
        FEATURE,
        FEATURE_COLLECTION
    }

    [CCode (cname = "mln_resource_kind", cprefix = "MLN_RESOURCE_KIND_", has_type_id = false)]
    public enum ResourceKind {
        UNKNOWN,
        STYLE,
        SOURCE,
        TILE,
        GLYPHS,
        SPRITE_IMAGE,
        SPRITE_JSON,
        IMAGE
    }

    [CCode (cname = "mln_resource_loading_method", cprefix = "MLN_RESOURCE_LOADING_METHOD_", has_type_id = false)]
    public enum ResourceLoadingMethod {
        ALL,
        CACHE_ONLY,
        NETWORK_ONLY
    }

    [CCode (cname = "mln_resource_priority", cprefix = "MLN_RESOURCE_PRIORITY_", has_type_id = false)]
    public enum ResourcePriority {
        REGULAR,
        LOW
    }

    [CCode (cname = "mln_resource_usage", cprefix = "MLN_RESOURCE_USAGE_", has_type_id = false)]
    public enum ResourceUsage {
        ONLINE,
        OFFLINE
    }

    [CCode (cname = "mln_resource_storage_policy", cprefix = "MLN_RESOURCE_STORAGE_POLICY_", has_type_id = false)]
    public enum ResourceStoragePolicy {
        PERMANENT,
        VOLATILE
    }

    [CCode (cname = "mln_resource_response_status", cprefix = "MLN_RESOURCE_RESPONSE_STATUS_", has_type_id = false)]
    public enum ResourceResponseStatus {
        OK,
        ERROR,
        NO_CONTENT,
        NOT_MODIFIED
    }

    [CCode (cname = "mln_resource_error_reason", cprefix = "MLN_RESOURCE_ERROR_REASON_", has_type_id = false)]
    public enum ResourceErrorReason {
        NONE,
        NOT_FOUND,
        SERVER,
        CONNECTION,
        RATE_LIMIT,
        OTHER
    }

    [CCode (cname = "mln_resource_provider_decision", cprefix = "MLN_RESOURCE_PROVIDER_DECISION_", has_type_id = false)]
    public enum ResourceProviderDecision {
        PASS_THROUGH,
        HANDLE
    }

    [CCode (cname = "mln_rendered_query_geometry_type", cprefix = "MLN_RENDERED_QUERY_GEOMETRY_TYPE_", has_type_id = false)]
    public enum RenderedQueryGeometryType {
        POINT,
        BOX,
        LINE_STRING
    }

    [CCode (cname = "mln_rendered_feature_query_option_field", cprefix = "MLN_RENDERED_FEATURE_QUERY_OPTION_", has_type_id = false)]
    [Flags]
    public enum RenderedFeatureQueryOptionField {
        LAYER_IDS
    }

    [CCode (cname = "mln_source_feature_query_option_field", cprefix = "MLN_SOURCE_FEATURE_QUERY_OPTION_", has_type_id = false)]
    [Flags]
    public enum SourceFeatureQueryOptionField {
        SOURCE_LAYER_IDS
    }

    [CCode (cname = "mln_queried_feature_field", cprefix = "MLN_QUERIED_FEATURE_", has_type_id = false)]
    [Flags]
    public enum QueriedFeatureField {
        SOURCE_ID,
        SOURCE_LAYER_ID,
        STATE
    }

    [CCode (cname = "mln_feature_extension_result_type", cprefix = "MLN_FEATURE_EXTENSION_RESULT_TYPE_", has_type_id = false)]
    public enum FeatureExtensionResultType {
        VALUE,
        FEATURE_COLLECTION
    }

    [CCode (cname = "mln_style_source_type", cprefix = "MLN_STYLE_SOURCE_TYPE_", has_type_id = false)]
    public enum StyleSourceType {
        UNKNOWN,
        VECTOR,
        RASTER,
        RASTER_DEM,
        GEOJSON,
        IMAGE,
        VIDEO,
        ANNOTATIONS,
        CUSTOM_VECTOR
    }

    [CCode (cname = "mln_style_tile_source_option_field", cprefix = "MLN_STYLE_TILE_SOURCE_OPTION_", has_type_id = false)]
    [Flags]
    public enum StyleTileSourceOptionField {
        MIN_ZOOM,
        MAX_ZOOM,
        ATTRIBUTION,
        SCHEME,
        BOUNDS,
        TILE_SIZE,
        VECTOR_ENCODING,
        RASTER_ENCODING
    }

    [CCode (cname = "mln_style_tile_scheme", cprefix = "MLN_STYLE_TILE_SCHEME_", has_type_id = false)]
    public enum StyleTileScheme {
        XYZ,
        TMS
    }

    [CCode (cname = "mln_style_vector_tile_encoding", cprefix = "MLN_STYLE_VECTOR_TILE_ENCODING_", has_type_id = false)]
    public enum StyleVectorTileEncoding {
        MVT,
        MLT
    }

    [CCode (cname = "mln_style_raster_dem_encoding", cprefix = "MLN_STYLE_RASTER_DEM_ENCODING_", has_type_id = false)]
    public enum StyleRasterDemEncoding {
        MAPBOX,
        TERRARIUM
    }

    [CCode (cname = "mln_custom_geometry_source_option_field", cprefix = "MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_", has_type_id = false)]
    [Flags]
    public enum CustomGeometrySourceOptionField {
        MIN_ZOOM,
        MAX_ZOOM,
        TOLERANCE,
        TILE_SIZE,
        BUFFER,
        CLIP,
        WRAP
    }

    [CCode (cname = "mln_style_image_option_field", cprefix = "MLN_STYLE_IMAGE_OPTION_", has_type_id = false)]
    [Flags]
    public enum StyleImageOptionField {
        PIXEL_RATIO,
        SDF
    }

    [CCode (cname = "mln_location_indicator_image_kind", cprefix = "MLN_LOCATION_INDICATOR_IMAGE_KIND_", has_type_id = false)]
    public enum LocationIndicatorImageKind {
        TOP,
        BEARING,
        SHADOW
    }

    [CCode (cname = "mln_log_event", cprefix = "MLN_LOG_EVENT_", has_type_id = false)]
    public enum LogEvent {
        GENERAL,
        SETUP,
        SHADER,
        PARSE_STYLE,
        PARSE_TILE,
        RENDER,
        STYLE,
        DATABASE,
        HTTP_REQUEST,
        SPRITE,
        IMAGE,
        OPENGL,
        JNI,
        ANDROID,
        CRASH,
        GLYPH,
        TIMING
    }

    [Compact]
    [CCode (cname = "mln_runtime", free_function = "")]
    public class Runtime {}

    [Compact]
    [CCode (cname = "mln_map", free_function = "")]
    public class Map {}

    [Compact]
    [CCode (cname = "mln_render_session", free_function = "")]
    public class RenderSession {}

    [Compact]
    [CCode (cname = "mln_map_projection", free_function = "")]
    public class MapProjection {}

    [Compact]
    [CCode (cname = "mln_style_id_list", free_function = "")]
    public class StyleIdList {}

    [Compact]
    [CCode (cname = "mln_feature_query_result", free_function = "")]
    public class FeatureQueryResult {}

    [Compact]
    [CCode (cname = "mln_feature_extension_result", free_function = "")]
    public class FeatureExtensionResult {}

    [Compact]
    [CCode (cname = "mln_offline_region_snapshot", free_function = "")]
    public class OfflineRegionSnapshot {}

    [Compact]
    [CCode (cname = "mln_offline_region_list", free_function = "")]
    public class OfflineRegionList {}

    [Compact]
    [CCode (cname = "mln_json_snapshot", free_function = "")]
    public class JsonSnapshot {}

    [Compact]
    [CCode (cname = "mln_resource_request_handle", free_function = "")]
    public class ResourceRequestHandle {}

    [CCode (cname = "mln_runtime_options", has_type_id = false)]
    public struct RuntimeOptions {
        public uint32 size;
        public uint32 flags;
        public unowned string? asset_path;
        public unowned string? cache_path;
        public uint64 maximum_cache_size;
    }

    [SimpleType]
    [CCode (cname = "mln_offline_region_status", has_type_id = false)]
    public struct OfflineRegionStatus {
        public uint32 size;
        public uint32 download_state;
        public uint64 completed_resource_count;
        public uint64 completed_resource_size;
        public uint64 completed_tile_count;
        public uint64 required_tile_count;
        public uint64 completed_tile_size;
        public uint64 required_resource_count;
        public bool required_resource_count_is_precise;
        public bool complete;
    }

    [SimpleType]
    [CCode (cname = "mln_offline_tile_pyramid_region_definition", has_type_id = false)]
    public struct OfflineTilePyramidRegionDefinition {
        public uint32 size;
        public unowned string? style_url;
        public LatLngBounds bounds;
        public double min_zoom;
        public double max_zoom;
        public float pixel_ratio;
        public bool include_ideographs;
    }

    [SimpleType]
    [CCode (cname = "mln_offline_geometry_region_definition", has_type_id = false)]
    public struct OfflineGeometryRegionDefinition {
        public uint32 size;
        public unowned string? style_url;
        public Geometry* geometry;
        public double min_zoom;
        public double max_zoom;
        public float pixel_ratio;
        public bool include_ideographs;
    }

    [SimpleType]
    [CCode (cname = "mln_offline_region_definition", has_type_id = false)]
    public struct OfflineRegionDefinition {
        public uint32 size;
        public uint32 type;
        [CCode (cname = "data.tile_pyramid")]
        public OfflineTilePyramidRegionDefinition tile_pyramid;
        [CCode (cname = "data.geometry")]
        public OfflineGeometryRegionDefinition geometry;
    }

    [SimpleType]
    [CCode (cname = "mln_offline_region_info", has_type_id = false)]
    public struct OfflineRegionInfo {
        public uint32 size;
        public int64 id;
        public OfflineRegionDefinition definition;
        public uint8* metadata;
        public size_t metadata_size;
    }

    [SimpleType]
    [CCode (cname = "mln_rendering_stats", has_type_id = false)]
    public struct RenderingStats {
        public uint32 size;
        public double encoding_time;
        public double rendering_time;
        public int64 frame_count;
        public int64 draw_call_count;
        public int64 total_draw_call_count;
    }

    [SimpleType]
    [CCode (cname = "mln_runtime_event_render_frame", has_type_id = false)]
    public struct RuntimeEventRenderFrame {
        public uint32 size;
        public uint32 mode;
        public bool needs_repaint;
        public bool placement_changed;
        public RenderingStats stats;
    }

    [SimpleType]
    [CCode (cname = "mln_runtime_event_render_map", has_type_id = false)]
    public struct RuntimeEventRenderMap {
        public uint32 size;
        public uint32 mode;
    }

    [SimpleType]
    [CCode (cname = "mln_runtime_event_style_image_missing", has_type_id = false)]
    public struct RuntimeEventStyleImageMissing {
        public uint32 size;
        public char* image_id;
        public size_t image_id_size;
    }

    [SimpleType]
    [CCode (cname = "mln_tile_id", has_type_id = false)]
    public struct TileId {
        public uint32 overscaled_z;
        public int32 wrap;
        public uint32 canonical_z;
        public uint32 canonical_x;
        public uint32 canonical_y;
    }

    [SimpleType]
    [CCode (cname = "mln_runtime_event_tile_action", has_type_id = false)]
    public struct RuntimeEventTileAction {
        public uint32 size;
        public uint32 operation;
        public TileId tile_id;
        public char* source_id;
        public size_t source_id_size;
    }

    [SimpleType]
    [CCode (cname = "mln_runtime_event_offline_region_status", has_type_id = false)]
    public struct RuntimeEventOfflineRegionStatus {
        public uint32 size;
        public OfflineRegionId region_id;
        public OfflineRegionStatus status;
    }

    [SimpleType]
    [CCode (cname = "mln_runtime_event_offline_region_response_error", has_type_id = false)]
    public struct RuntimeEventOfflineRegionResponseError {
        public uint32 size;
        public OfflineRegionId region_id;
        public uint32 reason;
    }

    [SimpleType]
    [CCode (cname = "mln_runtime_event_offline_region_tile_count_limit", has_type_id = false)]
    public struct RuntimeEventOfflineRegionTileCountLimit {
        public uint32 size;
        public OfflineRegionId region_id;
        public uint64 limit;
    }

    [SimpleType]
    [CCode (cname = "mln_runtime_event_offline_operation_completed", has_type_id = false)]
    public struct RuntimeEventOfflineOperationCompleted {
        public uint32 size;
        public OfflineOperationId operation_id;
        public uint32 operation_kind;
        public uint32 result_kind;
        public int32 result_status;
        public bool found;
    }

    [CCode (cname = "mln_map_options", has_type_id = false)]
    public struct MapOptions {
        public uint32 size;
        public uint32 width;
        public uint32 height;
        public double scale_factor;
        public uint32 map_mode;
    }

    [SimpleType]
    [CCode (cname = "mln_string_view", has_type_id = false)]
    public struct StringView {
        public char* data;
        public size_t size;
    }

    [SimpleType]
    [CCode (cname = "mln_lat_lng", has_type_id = false)]
    public struct LatLng {
        public double latitude;
        public double longitude;
    }

    [SimpleType]
    [CCode (cname = "mln_lat_lng_bounds", has_type_id = false)]
    public struct LatLngBounds {
        public LatLng southwest;
        public LatLng northeast;
    }

    [SimpleType]
    [CCode (cname = "mln_screen_point", has_type_id = false)]
    public struct ScreenPoint {
        public double x;
        public double y;
    }

    [SimpleType]
    [CCode (cname = "mln_vec3", has_type_id = false)]
    public struct Vec3 {
        public double x;
        public double y;
        public double z;
    }

    [SimpleType]
    [CCode (cname = "mln_quaternion", has_type_id = false)]
    public struct Quaternion {
        public double x;
        public double y;
        public double z;
        public double w;
    }

    [SimpleType]
    [CCode (cname = "mln_edge_insets", has_type_id = false)]
    public struct EdgeInsets {
        public double top;
        public double left;
        public double bottom;
        public double right;
    }

    [CCode (cname = "mln_map_viewport_options", has_type_id = false)]
    public struct MapViewportOptions {
        public uint32 size;
        public uint32 fields;
        public uint32 north_orientation;
        public uint32 constrain_mode;
        public uint32 viewport_mode;
        public EdgeInsets frustum_offset;
    }

    [CCode (cname = "mln_map_tile_options", has_type_id = false)]
    public struct MapTileOptions {
        public uint32 size;
        public uint32 fields;
        public uint32 prefetch_zoom_delta;
        public double lod_min_radius;
        public double lod_scale;
        public double lod_pitch_threshold;
        public double lod_zoom_shift;
        public uint32 lod_mode;
    }

    [SimpleType]
    [CCode (cname = "mln_projected_meters", has_type_id = false)]
    public struct ProjectedMeters {
        public double northing;
        public double easting;
    }

    [SimpleType]
    [CCode (cname = "mln_screen_box", has_type_id = false)]
    public struct ScreenBox {
        public ScreenPoint min;
        public ScreenPoint max;
    }

    [SimpleType]
    [CCode (cname = "mln_screen_line_string", has_type_id = false)]
    public struct ScreenLineString {
        public ScreenPoint* points;
        public size_t point_count;
    }

    [SimpleType]
    [CCode (cname = "mln_coordinate_span", has_type_id = false)]
    public struct CoordinateSpan {
        public LatLng* coordinates;
        public size_t coordinate_count;
    }

    [SimpleType]
    [CCode (cname = "mln_polygon_geometry", has_type_id = false)]
    public struct PolygonGeometry {
        public CoordinateSpan* rings;
        public size_t ring_count;
    }

    [SimpleType]
    [CCode (cname = "mln_multi_line_geometry", has_type_id = false)]
    public struct MultiLineGeometry {
        public CoordinateSpan* lines;
        public size_t line_count;
    }

    [SimpleType]
    [CCode (cname = "mln_multi_polygon_geometry", has_type_id = false)]
    public struct MultiPolygonGeometry {
        public PolygonGeometry* polygons;
        public size_t polygon_count;
    }

    [SimpleType]
    [CCode (cname = "mln_geometry_collection", has_type_id = false)]
    public struct GeometryCollection {
        public void* geometries;
        public size_t geometry_count;
    }

    [SimpleType]
    [CCode (cname = "mln_rendered_query_geometry", has_type_id = false)]
    public struct RenderedQueryGeometry {
        public uint32 size;
        public uint32 type;
        [CCode (cname = "data.point")]
        public ScreenPoint point;
        [CCode (cname = "data.box")]
        public ScreenBox box;
        [CCode (cname = "data.line_string")]
        public ScreenLineString line_string;
    }

    [SimpleType]
    [CCode (cname = "mln_rendered_feature_query_options", has_type_id = false)]
    public struct RenderedFeatureQueryOptions {
        public uint32 size;
        public uint32 fields;
        public StringView* layer_ids;
        public size_t layer_id_count;
        public JsonValue* filter;
    }

    [SimpleType]
    [CCode (cname = "mln_source_feature_query_options", has_type_id = false)]
    public struct SourceFeatureQueryOptions {
        public uint32 size;
        public uint32 fields;
        public StringView* source_layer_ids;
        public size_t source_layer_id_count;
        public JsonValue* filter;
    }

    [CCode (cname = "mln_geometry", has_type_id = false)]
    public struct Geometry {
        public uint32 size;
        public uint32 type;
        [CCode (cname = "data.point")]
        public LatLng point;
        [CCode (cname = "data.line_string")]
        public CoordinateSpan line_string;
        [CCode (cname = "data.polygon")]
        public PolygonGeometry polygon;
        [CCode (cname = "data.multi_point")]
        public CoordinateSpan multi_point;
        [CCode (cname = "data.multi_line_string")]
        public MultiLineGeometry multi_line_string;
        [CCode (cname = "data.multi_polygon")]
        public MultiPolygonGeometry multi_polygon;
        [CCode (cname = "data.geometry_collection")]
        public GeometryCollection geometry_collection;
    }

    [SimpleType]
    [CCode (cname = "mln_json_array", has_type_id = false)]
    public struct JsonArray {
        public void* values;
        public size_t value_count;
    }

    [SimpleType]
    [CCode (cname = "mln_json_member", has_type_id = false)]
    public struct JsonMember {
        public StringView key;
        public void* value;
    }

    [SimpleType]
    [CCode (cname = "mln_json_object", has_type_id = false)]
    public struct JsonObject {
        public JsonMember* members;
        public size_t member_count;
    }

    [CCode (cname = "mln_json_value", has_type_id = false)]
    public struct JsonValue {
        public uint32 size;
        public uint32 type;
        [CCode (cname = "data.bool_value")]
        public bool bool_value;
        [CCode (cname = "data.uint_value")]
        public uint64 uint_value;
        [CCode (cname = "data.int_value")]
        public int64 int_value;
        [CCode (cname = "data.double_value")]
        public double double_value;
        [CCode (cname = "data.string_value")]
        public StringView string_value;
        [CCode (cname = "data.array_value")]
        public JsonArray array_value;
        [CCode (cname = "data.object_value")]
        public JsonObject object_value;
    }

    [CCode (cname = "mln_feature_state_selector", has_type_id = false)]
    public struct FeatureStateSelector {
        public uint32 size;
        public uint32 fields;
        public StringView source_id;
        public StringView source_layer_id;
        public StringView feature_id;
        public StringView state_key;
    }

    [CCode (cname = "mln_feature", has_type_id = false)]
    public struct Feature {
        public uint32 size;
        public Geometry* geometry;
        public JsonMember* properties;
        public size_t property_count;
        public uint32 identifier_type;
        [CCode (cname = "identifier.uint_value")]
        public uint64 identifier_uint_value;
        [CCode (cname = "identifier.int_value")]
        public int64 identifier_int_value;
        [CCode (cname = "identifier.double_value")]
        public double identifier_double_value;
        [CCode (cname = "identifier.string_value")]
        public StringView identifier_string_value;
    }

    [CCode (cname = "mln_queried_feature", has_type_id = false)]
    public struct QueriedFeature {
        public uint32 size;
        public uint32 fields;
        public Feature feature;
        public StringView source_id;
        public StringView source_layer_id;
        public JsonValue* state;
    }

    [SimpleType]
    [CCode (cname = "mln_feature_collection", has_type_id = false)]
    public struct FeatureCollection {
        public Feature* features;
        public size_t feature_count;
    }

    [CCode (cname = "mln_feature_extension_result_info", has_type_id = false)]
    public struct FeatureExtensionResultInfo {
        public uint32 size;
        public uint32 type;
        [CCode (cname = "data.value")]
        public JsonValue* value;
        [CCode (cname = "data.feature_collection")]
        public FeatureCollection feature_collection;
    }

    [CCode (cname = "mln_geojson", has_type_id = false)]
    public struct GeoJson {
        public uint32 size;
        public uint32 type;
        [CCode (cname = "data.geometry")]
        public Geometry* geometry;
        [CCode (cname = "data.feature")]
        public Feature* feature;
        [CCode (cname = "data.feature_collection")]
        public FeatureCollection feature_collection;
    }

    [SimpleType]
    [CCode (cname = "mln_camera_options", has_type_id = false)]
    public struct CameraOptions {
        public uint32 size;
        public uint32 fields;
        public double latitude;
        public double longitude;
        public double center_altitude;
        public EdgeInsets padding;
        public ScreenPoint anchor;
        public double zoom;
        public double bearing;
        public double pitch;
        public double roll;
        public double field_of_view;
    }

    [SimpleType]
    [CCode (cname = "mln_unit_bezier", has_type_id = false)]
    public struct UnitBezier {
        public double x1;
        public double y1;
        public double x2;
        public double y2;
    }

    [CCode (cname = "mln_animation_options", has_type_id = false)]
    public struct AnimationOptions {
        public uint32 size;
        public uint32 fields;
        public double duration_ms;
        public double velocity;
        public double min_zoom;
        public UnitBezier easing;
    }

    [CCode (cname = "mln_camera_fit_options", has_type_id = false)]
    public struct CameraFitOptions {
        public uint32 size;
        public uint32 fields;
        public EdgeInsets padding;
        public double bearing;
        public double pitch;
    }

    [CCode (cname = "mln_bound_options", has_type_id = false)]
    public struct BoundOptions {
        public uint32 size;
        public uint32 fields;
        public LatLngBounds bounds;
        public double min_zoom;
        public double max_zoom;
        public double min_pitch;
        public double max_pitch;
    }

    [CCode (cname = "mln_free_camera_options", has_type_id = false)]
    public struct FreeCameraOptions {
        public uint32 size;
        public uint32 fields;
        public Vec3 position;
        public Quaternion orientation;
    }

    [CCode (cname = "mln_projection_mode", has_type_id = false)]
    public struct ProjectionMode {
        public uint32 size;
        public uint32 fields;
        public bool axonometric;
        public double x_skew;
        public double y_skew;
    }

    [SimpleType]
    [CCode (cname = "mln_render_target_extent", has_type_id = false)]
    public struct RenderTargetExtent {
        public uint32 size;
        public uint32 width;
        public uint32 height;
        public double scale_factor;
    }

    [SimpleType]
    [CCode (cname = "mln_metal_context_descriptor", has_type_id = false)]
    public struct MetalContextDescriptor {
        public uint32 size;
        public void* device;
    }

    [SimpleType]
    [CCode (cname = "mln_vulkan_context_descriptor", has_type_id = false)]
    public struct VulkanContextDescriptor {
        public uint32 size;
        public void* instance;
        public void* physical_device;
        public void* device;
        public void* graphics_queue;
        public uint32 graphics_queue_family_index;
        public void* get_instance_proc_addr;
        public void* get_device_proc_addr;
    }

    [SimpleType]
    [CCode (cname = "mln_wgl_context_descriptor", has_type_id = false)]
    public struct WglContextDescriptor {
        public uint32 size;
        public void* device_context;
        public void* share_context;
        public void* get_proc_address;
    }

    [SimpleType]
    [CCode (cname = "mln_egl_context_descriptor", has_type_id = false)]
    public struct EglContextDescriptor {
        public uint32 size;
        public void* display;
        public void* config;
        public void* share_context;
        public void* get_proc_address;
    }

    [SimpleType]
    [CCode (cname = "mln_opengl_context_descriptor", has_type_id = false)]
    public struct OpenGLContextDescriptor {
        public uint32 size;
        public OpenGLContextPlatform platform;
        [CCode (cname = "data.wgl")]
        public WglContextDescriptor wgl;
        [CCode (cname = "data.egl")]
        public EglContextDescriptor egl;
    }

    [SimpleType]
    [CCode (cname = "mln_metal_owned_texture_descriptor", has_type_id = false)]
    public struct MetalOwnedTextureDescriptor {
        public uint32 size;
        public RenderTargetExtent extent;
        public MetalContextDescriptor context;
    }

    [SimpleType]
    [CCode (cname = "mln_metal_borrowed_texture_descriptor", has_type_id = false)]
    public struct MetalBorrowedTextureDescriptor {
        public uint32 size;
        public RenderTargetExtent extent;
        public void* texture;
    }

    [SimpleType]
    [CCode (cname = "mln_vulkan_owned_texture_descriptor", has_type_id = false)]
    public struct VulkanOwnedTextureDescriptor {
        public uint32 size;
        public RenderTargetExtent extent;
        public VulkanContextDescriptor context;
    }

    [SimpleType]
    [CCode (cname = "mln_vulkan_borrowed_texture_descriptor", has_type_id = false)]
    public struct VulkanBorrowedTextureDescriptor {
        public uint32 size;
        public RenderTargetExtent extent;
        public VulkanContextDescriptor context;
        public void* image;
        public void* image_view;
        public uint32 format;
        public uint32 initial_layout;
        public uint32 final_layout;
    }

    [SimpleType]
    [CCode (cname = "mln_opengl_owned_texture_descriptor", has_type_id = false)]
    public struct OpenGLOwnedTextureDescriptor {
        public uint32 size;
        public RenderTargetExtent extent;
        public OpenGLContextDescriptor context;
    }

    [SimpleType]
    [CCode (cname = "mln_opengl_borrowed_texture_descriptor", has_type_id = false)]
    public struct OpenGLBorrowedTextureDescriptor {
        public uint32 size;
        public RenderTargetExtent extent;
        public OpenGLContextDescriptor context;
        public uint32 texture;
        public uint32 target;
    }

    [SimpleType]
    [CCode (cname = "mln_metal_surface_descriptor", has_type_id = false)]
    public struct MetalSurfaceDescriptor {
        public uint32 size;
        public RenderTargetExtent extent;
        public MetalContextDescriptor context;
        public void* layer;
    }

    [SimpleType]
    [CCode (cname = "mln_vulkan_surface_descriptor", has_type_id = false)]
    public struct VulkanSurfaceDescriptor {
        public uint32 size;
        public RenderTargetExtent extent;
        public VulkanContextDescriptor context;
        public void* surface;
    }

    [SimpleType]
    [CCode (cname = "mln_opengl_surface_descriptor", has_type_id = false)]
    public struct OpenGLSurfaceDescriptor {
        public uint32 size;
        public RenderTargetExtent extent;
        public OpenGLContextDescriptor context;
        public void* surface;
    }

    [SimpleType]
    [CCode (cname = "mln_texture_image_info", has_type_id = false)]
    public struct TextureImageInfo {
        public uint32 size;
        public uint32 width;
        public uint32 height;
        public uint32 stride;
        public size_t byte_length;
    }

    [SimpleType]
    [CCode (cname = "mln_canonical_tile_id", has_type_id = false)]
    public struct CanonicalTileId {
        public uint32 z;
        public uint32 x;
        public uint32 y;
    }

    [CCode (cname = "mln_custom_geometry_source_tile_callback", has_target = false)]
    public delegate void CustomGeometrySourceTileCallback (void* user_data, CanonicalTileId tile_id);

    [SimpleType]
    [CCode (cname = "mln_custom_geometry_source_options", has_type_id = false)]
    public struct CustomGeometrySourceOptions {
        public uint32 size;
        public uint32 fields;
        public CustomGeometrySourceTileCallback fetch_tile;
        public CustomGeometrySourceTileCallback cancel_tile;
        public void* user_data;
        public double min_zoom;
        public double max_zoom;
        public double tolerance;
        public uint32 tile_size;
        public uint32 buffer;
        public bool clip;
        public bool wrap;
    }

    [SimpleType]
    [CCode (cname = "mln_premultiplied_rgba8_image", has_type_id = false)]
    public struct PremultipliedRgba8Image {
        public uint32 size;
        public uint32 width;
        public uint32 height;
        public uint32 stride;
        public uint8* pixels;
        public size_t byte_length;
    }

    [CCode (cname = "mln_style_source_info", has_type_id = false)]
    public struct StyleSourceInfo {
        public uint32 size;
        public uint32 type;
        public size_t id_size;
        public bool is_volatile;
        public bool has_attribution;
        public size_t attribution_size;
    }

    [SimpleType]
    [CCode (cname = "mln_style_tile_source_options", has_type_id = false)]
    public struct StyleTileSourceOptions {
        public uint32 size;
        public uint32 fields;
        public double min_zoom;
        public double max_zoom;
        public StringView attribution;
        public uint32 scheme;
        public LatLngBounds bounds;
        public uint32 tile_size;
        public uint32 vector_encoding;
        public uint32 raster_encoding;
    }

    [SimpleType]
    [CCode (cname = "mln_style_image_options", has_type_id = false)]
    public struct StyleImageOptions {
        public uint32 size;
        public uint32 fields;
        public float pixel_ratio;
        public bool sdf;
    }

    [SimpleType]
    [CCode (cname = "mln_style_image_info", has_type_id = false)]
    public struct StyleImageInfo {
        public uint32 size;
        public uint32 width;
        public uint32 height;
        public uint32 stride;
        public size_t byte_length;
        public float pixel_ratio;
        public bool sdf;
    }

    [SimpleType]
    [CCode (cname = "mln_metal_owned_texture_frame", has_type_id = false)]
    public struct MetalOwnedTextureFrame {
        public uint32 size;
        public uint64 generation;
        public uint32 width;
        public uint32 height;
        public double scale_factor;
        public uint64 frame_id;
        public void* texture;
        public void* device;
        public uint64 pixel_format;
    }

    [SimpleType]
    [CCode (cname = "mln_vulkan_owned_texture_frame", has_type_id = false)]
    public struct VulkanOwnedTextureFrame {
        public uint32 size;
        public uint64 generation;
        public uint32 width;
        public uint32 height;
        public double scale_factor;
        public uint64 frame_id;
        public void* image;
        public void* image_view;
        public void* device;
        public uint32 format;
        public uint32 layout;
    }

    [SimpleType]
    [CCode (cname = "mln_opengl_owned_texture_frame", has_type_id = false)]
    public struct OpenGLOwnedTextureFrame {
        public uint32 size;
        public uint64 generation;
        public uint32 width;
        public uint32 height;
        public double scale_factor;
        public uint64 frame_id;
        public uint32 texture;
        public uint32 target;
        public uint32 internal_format;
        public uint32 format;
        public uint32 type;
    }

    [CCode (cname = "mln_resource_transform_response", has_type_id = false)]
    public struct ResourceTransformResponse {
        public uint32 size;
        public unowned string? url;
        public void* context;
    }

    [CCode (cname = "mln_resource_transform_callback", has_target = false)]
    public delegate Status ResourceTransformCallback (void* user_data, uint32 kind, string url, ResourceTransformResponse* out_response);

    [CCode (cname = "mln_resource_transform", has_type_id = false)]
    public struct ResourceTransform {
        public uint32 size;
        public ResourceTransformCallback callback;
        public void* user_data;
    }

    [CCode (cname = "mln_resource_request", has_type_id = false)]
    public struct ResourceRequest {
        public uint32 size;
        public unowned string? url;
        public uint32 kind;
        public uint32 loading_method;
        public uint32 priority;
        public uint32 usage;
        public uint32 storage_policy;
        public bool has_range;
        public uint64 range_start;
        public uint64 range_end;
        public bool has_prior_modified;
        public int64 prior_modified_unix_ms;
        public bool has_prior_expires;
        public int64 prior_expires_unix_ms;
        public unowned string? prior_etag;
        public uint8* prior_data;
        public size_t prior_data_size;
    }

    [CCode (cname = "mln_resource_response", has_type_id = false)]
    public struct ResourceResponse {
        public uint32 size;
        public uint32 status;
        public uint32 error_reason;
        public uint8* bytes;
        public size_t byte_count;
        public unowned string? error_message;
        public bool must_revalidate;
        public bool has_modified;
        public int64 modified_unix_ms;
        public bool has_expires;
        public int64 expires_unix_ms;
        public unowned string? etag;
        public bool has_retry_after;
        public int64 retry_after_unix_ms;
    }

    [CCode (cname = "mln_resource_provider_callback", has_target = false)]
    public delegate uint32 ResourceProviderCallback (void* user_data, ResourceRequest* request, ResourceRequestHandle handle);

    [CCode (cname = "mln_resource_provider", has_type_id = false)]
    public struct ResourceProvider {
        public uint32 size;
        public ResourceProviderCallback callback;
        public void* user_data;
    }

    [CCode (cname = "mln_runtime_event", has_type_id = false)]
    public struct RuntimeEvent {
        public uint32 size;
        public uint32 type;
        public uint32 source_type;
        public void* source;
        public int32 code;
        public uint32 payload_type;
        public void* payload;
        public size_t payload_size;
        public char* message;
        public size_t message_size;
    }

    [CCode (cname = "mln_log_callback", has_target = false)]
    public delegate uint32 LogCallback (void* user_data, uint32 severity, uint32 event, int64 code, string? message);

    [CCode (cname = "mln_c_version")]
    public static uint32 c_version ();

    [CCode (cname = "mln_supported_render_backend_mask")]
    public static uint32 supported_render_backend_mask ();

    [CCode (cname = "mln_thread_last_error_message")]
    public static unowned string thread_last_error_message ();

    [CCode (cname = "mln_network_status_get")]
    public static Status network_status_get (out uint32 out_status);

    [CCode (cname = "mln_network_status_set")]
    public static Status network_status_set (uint32 status);

    [CCode (cname = "mln_log_set_callback")]
    public static Status log_set_callback (LogCallback? callback, void* user_data);

    [CCode (cname = "mln_log_clear_callback")]
    public static Status log_clear_callback ();

    [CCode (cname = "mln_log_set_async_severity_mask")]
    public static Status log_set_async_severity_mask (uint32 mask);

    [CCode (cname = "mln_runtime_options_default")]
    public static RuntimeOptions runtime_options_default ();

    [CCode (cname = "mln_runtime_create")]
    public static Status runtime_create (RuntimeOptions* options, out Runtime runtime);

    [CCode (cname = "mln_runtime_set_resource_provider")]
    public static Status runtime_set_resource_provider (Runtime runtime, ResourceProvider* provider);

    [CCode (cname = "mln_resource_request_complete")]
    public static Status resource_request_complete (ResourceRequestHandle handle, ResourceResponse* response);

    [CCode (cname = "mln_resource_request_cancelled")]
    public static Status resource_request_cancelled (ResourceRequestHandle handle, out bool out_cancelled);

    [CCode (cname = "mln_resource_request_release")]
    public static void resource_request_release (ResourceRequestHandle handle);

    [CCode (cname = "mln_resource_transform_response_set_url")]
    public static Status resource_transform_response_set_url (ResourceTransformResponse* response, string? url, size_t url_size);

    [CCode (cname = "mln_runtime_destroy")]
    public static Status runtime_destroy (Runtime runtime);

    [CCode (cname = "mln_runtime_run_once")]
    public static Status runtime_run_once (Runtime runtime);

    [CCode (cname = "mln_runtime_poll_event")]
    public static Status runtime_poll_event (Runtime runtime, RuntimeEvent* out_event, out bool out_has_event);

    [CCode (cname = "mln_runtime_set_resource_transform")]
    public static Status runtime_set_resource_transform (Runtime runtime, ResourceTransform* transform);

    [CCode (cname = "mln_runtime_clear_resource_transform")]
    public static Status runtime_clear_resource_transform (Runtime runtime);

    [CCode (cname = "mln_runtime_run_ambient_cache_operation_start")]
    public static Status runtime_run_ambient_cache_operation_start (Runtime runtime, uint32 operation, out OfflineOperationId out_operation_id);

    [CCode (cname = "mln_runtime_offline_operation_discard")]
    public static Status runtime_offline_operation_discard (Runtime runtime, OfflineOperationId operation_id);

    [CCode (cname = "mln_runtime_offline_region_create_start")]
    public static Status runtime_offline_region_create_start (Runtime runtime, OfflineRegionDefinition* definition, uint8* metadata, size_t metadata_size, out OfflineOperationId out_operation_id);

    [CCode (cname = "mln_runtime_offline_region_get_start")]
    public static Status runtime_offline_region_get_start (Runtime runtime, OfflineRegionId region_id, out OfflineOperationId out_operation_id);

    [CCode (cname = "mln_runtime_offline_regions_list_start")]
    public static Status runtime_offline_regions_list_start (Runtime runtime, out OfflineOperationId out_operation_id);

    [CCode (cname = "mln_runtime_offline_regions_merge_database_start")]
    public static Status runtime_offline_regions_merge_database_start (Runtime runtime, string side_database_path, out OfflineOperationId out_operation_id);

    [CCode (cname = "mln_runtime_offline_region_update_metadata_start")]
    public static Status runtime_offline_region_update_metadata_start (Runtime runtime, OfflineRegionId region_id, uint8* metadata, size_t metadata_size, out OfflineOperationId out_operation_id);

    [CCode (cname = "mln_runtime_offline_region_get_status_start")]
    public static Status runtime_offline_region_get_status_start (Runtime runtime, OfflineRegionId region_id, out OfflineOperationId out_operation_id);

    [CCode (cname = "mln_runtime_offline_region_set_observed_start")]
    public static Status runtime_offline_region_set_observed_start (Runtime runtime, OfflineRegionId region_id, bool observed, out OfflineOperationId out_operation_id);

    [CCode (cname = "mln_runtime_offline_region_set_download_state_start")]
    public static Status runtime_offline_region_set_download_state_start (Runtime runtime, OfflineRegionId region_id, uint32 state, out OfflineOperationId out_operation_id);

    [CCode (cname = "mln_runtime_offline_region_invalidate_start")]
    public static Status runtime_offline_region_invalidate_start (Runtime runtime, OfflineRegionId region_id, out OfflineOperationId out_operation_id);

    [CCode (cname = "mln_runtime_offline_region_delete_start")]
    public static Status runtime_offline_region_delete_start (Runtime runtime, OfflineRegionId region_id, out OfflineOperationId out_operation_id);

    [CCode (cname = "mln_runtime_offline_region_create_take_result")]
    public static Status runtime_offline_region_create_take_result (Runtime runtime, OfflineOperationId operation_id, out OfflineRegionSnapshot? out_region);

    [CCode (cname = "mln_runtime_offline_region_get_take_result")]
    public static Status runtime_offline_region_get_take_result (Runtime runtime, OfflineOperationId operation_id, out OfflineRegionSnapshot? out_region, out bool out_found);

    [CCode (cname = "mln_runtime_offline_regions_list_take_result")]
    public static Status runtime_offline_regions_list_take_result (Runtime runtime, OfflineOperationId operation_id, out OfflineRegionList? out_regions);

    [CCode (cname = "mln_runtime_offline_regions_merge_database_take_result")]
    public static Status runtime_offline_regions_merge_database_take_result (Runtime runtime, OfflineOperationId operation_id, out OfflineRegionList? out_regions);

    [CCode (cname = "mln_runtime_offline_region_update_metadata_take_result")]
    public static Status runtime_offline_region_update_metadata_take_result (Runtime runtime, OfflineOperationId operation_id, out OfflineRegionSnapshot? out_region);

    [CCode (cname = "mln_runtime_offline_region_get_status_take_result")]
    public static Status runtime_offline_region_get_status_take_result (Runtime runtime, OfflineOperationId operation_id, OfflineRegionStatus* out_status);

    [CCode (cname = "mln_offline_region_snapshot_get")]
    public static Status offline_region_snapshot_get (OfflineRegionSnapshot snapshot, OfflineRegionInfo* out_info);

    [CCode (cname = "mln_offline_region_snapshot_destroy")]
    public static void offline_region_snapshot_destroy (OfflineRegionSnapshot snapshot);

    [CCode (cname = "mln_offline_region_list_count")]
    public static Status offline_region_list_count (OfflineRegionList list, out size_t out_count);

    [CCode (cname = "mln_offline_region_list_get")]
    public static Status offline_region_list_get (OfflineRegionList list, size_t index, OfflineRegionInfo* out_info);

    [CCode (cname = "mln_offline_region_list_destroy")]
    public static void offline_region_list_destroy (OfflineRegionList list);

    [CCode (cname = "mln_map_options_default")]
    public static MapOptions map_options_default ();

    [CCode (cname = "mln_map_viewport_options_default")]
    public static MapViewportOptions map_viewport_options_default ();

    [CCode (cname = "mln_map_tile_options_default")]
    public static MapTileOptions map_tile_options_default ();

    [CCode (cname = "mln_map_create")]
    public static Status map_create (Runtime runtime, MapOptions* options, out Map map);

    [CCode (cname = "mln_map_destroy")]
    public static Status map_destroy (Map map);

    [CCode (cname = "mln_map_request_repaint")]
    public static Status map_request_repaint (Map map);

    [CCode (cname = "mln_map_request_still_image")]
    public static Status map_request_still_image (Map map);

    [CCode (cname = "mln_map_set_style_url")]
    public static Status map_set_style_url (Map map, string url);

    [CCode (cname = "mln_map_set_style_json")]
    public static Status map_set_style_json (Map map, string json);

    [CCode (cname = "mln_map_set_debug_options")]
    public static Status map_set_debug_options (Map map, uint32 options);

    [CCode (cname = "mln_map_get_debug_options")]
    public static Status map_get_debug_options (Map map, out uint32 out_options);

    [CCode (cname = "mln_map_set_rendering_stats_view_enabled")]
    public static Status map_set_rendering_stats_view_enabled (Map map, bool enabled);

    [CCode (cname = "mln_map_get_rendering_stats_view_enabled")]
    public static Status map_get_rendering_stats_view_enabled (Map map, out bool out_enabled);

    [CCode (cname = "mln_map_is_fully_loaded")]
    public static Status map_is_fully_loaded (Map map, out bool out_loaded);

    [CCode (cname = "mln_map_dump_debug_logs")]
    public static Status map_dump_debug_logs (Map map);

    [CCode (cname = "mln_map_get_viewport_options")]
    public static Status map_get_viewport_options (Map map, MapViewportOptions* out_options);

    [CCode (cname = "mln_map_set_viewport_options")]
    public static Status map_set_viewport_options (Map map, MapViewportOptions* options);

    [CCode (cname = "mln_map_get_tile_options")]
    public static Status map_get_tile_options (Map map, MapTileOptions* out_options);

    [CCode (cname = "mln_map_set_tile_options")]
    public static Status map_set_tile_options (Map map, MapTileOptions* options);

    [CCode (cname = "mln_camera_options_default")]
    public static CameraOptions camera_options_default ();

    [CCode (cname = "mln_animation_options_default")]
    public static AnimationOptions animation_options_default ();

    [CCode (cname = "mln_camera_fit_options_default")]
    public static CameraFitOptions camera_fit_options_default ();

    [CCode (cname = "mln_bound_options_default")]
    public static BoundOptions bound_options_default ();

    [CCode (cname = "mln_free_camera_options_default")]
    public static FreeCameraOptions free_camera_options_default ();

    [CCode (cname = "mln_projection_mode_default")]
    public static ProjectionMode projection_mode_default ();

    [CCode (cname = "mln_map_get_camera")]
    public static Status map_get_camera (Map map, CameraOptions* out_camera);

    [CCode (cname = "mln_map_jump_to")]
    public static Status map_jump_to (Map map, CameraOptions* camera);

    [CCode (cname = "mln_map_ease_to")]
    public static Status map_ease_to (Map map, CameraOptions* camera, AnimationOptions* animation);

    [CCode (cname = "mln_map_fly_to")]
    public static Status map_fly_to (Map map, CameraOptions* camera, AnimationOptions* animation);

    [CCode (cname = "mln_map_move_by")]
    public static Status map_move_by (Map map, double delta_x, double delta_y);

    [CCode (cname = "mln_map_move_by_animated")]
    public static Status map_move_by_animated (Map map, double delta_x, double delta_y, AnimationOptions* animation);

    [CCode (cname = "mln_map_scale_by")]
    public static Status map_scale_by (Map map, double scale, ScreenPoint* anchor);

    [CCode (cname = "mln_map_scale_by_animated")]
    public static Status map_scale_by_animated (Map map, double scale, ScreenPoint* anchor, AnimationOptions* animation);

    [CCode (cname = "mln_map_rotate_by")]
    public static Status map_rotate_by (Map map, ScreenPoint first, ScreenPoint second);

    [CCode (cname = "mln_map_rotate_by_animated")]
    public static Status map_rotate_by_animated (Map map, ScreenPoint first, ScreenPoint second, AnimationOptions* animation);

    [CCode (cname = "mln_map_pitch_by")]
    public static Status map_pitch_by (Map map, double pitch);

    [CCode (cname = "mln_map_pitch_by_animated")]
    public static Status map_pitch_by_animated (Map map, double pitch, AnimationOptions* animation);

    [CCode (cname = "mln_map_cancel_transitions")]
    public static Status map_cancel_transitions (Map map);

    [CCode (cname = "mln_map_camera_for_geometry")]
    public static Status map_camera_for_geometry (Map map, Geometry* geometry, CameraFitOptions* fit_options, CameraOptions* out_camera);

    [CCode (cname = "mln_map_camera_for_lat_lng_bounds")]
    public static Status map_camera_for_lat_lng_bounds (Map map, LatLngBounds bounds, CameraFitOptions* fit_options, CameraOptions* out_camera);

    [CCode (cname = "mln_map_camera_for_lat_lngs")]
    public static Status map_camera_for_lat_lngs (Map map, LatLng* coordinates, size_t coordinate_count, CameraFitOptions* fit_options, CameraOptions* out_camera);

    [CCode (cname = "mln_map_lat_lng_bounds_for_camera")]
    public static Status map_lat_lng_bounds_for_camera (Map map, CameraOptions* camera, out LatLngBounds out_bounds);

    [CCode (cname = "mln_map_lat_lng_bounds_for_camera_unwrapped")]
    public static Status map_lat_lng_bounds_for_camera_unwrapped (Map map, CameraOptions* camera, out LatLngBounds out_bounds);

    [CCode (cname = "mln_map_get_bounds")]
    public static Status map_get_bounds (Map map, BoundOptions* out_options);

    [CCode (cname = "mln_map_set_bounds")]
    public static Status map_set_bounds (Map map, BoundOptions* options);

    [CCode (cname = "mln_map_get_free_camera_options")]
    public static Status map_get_free_camera_options (Map map, FreeCameraOptions* out_options);

    [CCode (cname = "mln_map_set_free_camera_options")]
    public static Status map_set_free_camera_options (Map map, FreeCameraOptions* options);

    [CCode (cname = "mln_map_get_projection_mode")]
    public static Status map_get_projection_mode (Map map, ProjectionMode* out_mode);

    [CCode (cname = "mln_map_set_projection_mode")]
    public static Status map_set_projection_mode (Map map, ProjectionMode* mode);

    [CCode (cname = "mln_map_pixel_for_lat_lng")]
    public static Status map_pixel_for_lat_lng (Map map, LatLng coordinate, out ScreenPoint out_point);

    [CCode (cname = "mln_map_lat_lng_for_pixel")]
    public static Status map_lat_lng_for_pixel (Map map, ScreenPoint point, out LatLng out_coordinate);

    [CCode (cname = "mln_map_pixels_for_lat_lngs")]
    public static Status map_pixels_for_lat_lngs (Map map, LatLng* coordinates, size_t coordinate_count, ScreenPoint* out_points);

    [CCode (cname = "mln_map_lat_lngs_for_pixels")]
    public static Status map_lat_lngs_for_pixels (Map map, ScreenPoint* points, size_t point_count, LatLng* out_coordinates);

    [CCode (cname = "mln_style_tile_source_options_default")]
    public static StyleTileSourceOptions style_tile_source_options_default ();

    [CCode (cname = "mln_custom_geometry_source_options_default")]
    public static CustomGeometrySourceOptions custom_geometry_source_options_default ();

    [CCode (cname = "mln_premultiplied_rgba8_image_default")]
    public static PremultipliedRgba8Image premultiplied_rgba8_image_default ();

    [CCode (cname = "mln_style_image_options_default")]
    public static StyleImageOptions style_image_options_default ();

    [CCode (cname = "mln_style_image_info_default")]
    public static StyleImageInfo style_image_info_default ();

    [CCode (cname = "mln_style_id_list_count")]
    public static Status style_id_list_count (StyleIdList list, out size_t out_count);

    [CCode (cname = "mln_style_id_list_get")]
    public static Status style_id_list_get (StyleIdList list, size_t index, out StringView out_id);

    [CCode (cname = "mln_style_id_list_destroy")]
    public static void style_id_list_destroy (StyleIdList list);

    [CCode (cname = "mln_map_add_style_source_json")]
    public static Status map_add_style_source_json (Map map, StringView source_id, JsonValue* source_json);

    [CCode (cname = "mln_map_add_geojson_source_url")]
    public static Status map_add_geojson_source_url (Map map, StringView source_id, StringView url);

    [CCode (cname = "mln_map_add_geojson_source_data")]
    public static Status map_add_geojson_source_data (Map map, StringView source_id, GeoJson* data);

    [CCode (cname = "mln_map_set_geojson_source_url")]
    public static Status map_set_geojson_source_url (Map map, StringView source_id, StringView url);

    [CCode (cname = "mln_map_set_geojson_source_data")]
    public static Status map_set_geojson_source_data (Map map, StringView source_id, GeoJson* data);

    [CCode (cname = "mln_map_add_vector_source_url")]
    public static Status map_add_vector_source_url (Map map, StringView source_id, StringView url, StyleTileSourceOptions* options);

    [CCode (cname = "mln_map_add_vector_source_tiles")]
    public static Status map_add_vector_source_tiles (Map map, StringView source_id, StringView* tiles, size_t tile_count, StyleTileSourceOptions* options);

    [CCode (cname = "mln_map_add_raster_source_url")]
    public static Status map_add_raster_source_url (Map map, StringView source_id, StringView url, StyleTileSourceOptions* options);

    [CCode (cname = "mln_map_add_raster_source_tiles")]
    public static Status map_add_raster_source_tiles (Map map, StringView source_id, StringView* tiles, size_t tile_count, StyleTileSourceOptions* options);

    [CCode (cname = "mln_map_add_raster_dem_source_url")]
    public static Status map_add_raster_dem_source_url (Map map, StringView source_id, StringView url, StyleTileSourceOptions* options);

    [CCode (cname = "mln_map_add_raster_dem_source_tiles")]
    public static Status map_add_raster_dem_source_tiles (Map map, StringView source_id, StringView* tiles, size_t tile_count, StyleTileSourceOptions* options);

    [CCode (cname = "mln_map_add_custom_geometry_source")]
    public static Status map_add_custom_geometry_source (Map map, StringView source_id, CustomGeometrySourceOptions* options);

    [CCode (cname = "mln_map_set_custom_geometry_source_tile_data")]
    public static Status map_set_custom_geometry_source_tile_data (Map map, StringView source_id, CanonicalTileId tile_id, GeoJson* data);

    [CCode (cname = "mln_map_invalidate_custom_geometry_source_tile")]
    public static Status map_invalidate_custom_geometry_source_tile (Map map, StringView source_id, CanonicalTileId tile_id);

    [CCode (cname = "mln_map_invalidate_custom_geometry_source_region")]
    public static Status map_invalidate_custom_geometry_source_region (Map map, StringView source_id, LatLngBounds bounds);

    [CCode (cname = "mln_map_remove_style_source")]
    public static Status map_remove_style_source (Map map, StringView source_id, out bool out_removed);

    [CCode (cname = "mln_map_style_source_exists")]
    public static Status map_style_source_exists (Map map, StringView source_id, out bool out_exists);

    [CCode (cname = "mln_map_get_style_source_type")]
    public static Status map_get_style_source_type (Map map, StringView source_id, out uint32 out_source_type, out bool out_found);

    [CCode (cname = "mln_map_get_style_source_info")]
    public static Status map_get_style_source_info (Map map, StringView source_id, StyleSourceInfo* out_info, out bool out_found);

    [CCode (cname = "mln_map_copy_style_source_attribution")]
    public static Status map_copy_style_source_attribution (Map map, StringView source_id, char* out_attribution, size_t attribution_capacity, out size_t out_attribution_size, out bool out_found);

    [CCode (cname = "mln_map_list_style_source_ids")]
    public static Status map_list_style_source_ids (Map map, out StyleIdList out_source_ids);

    [CCode (cname = "mln_map_add_hillshade_layer")]
    public static Status map_add_hillshade_layer (Map map, StringView layer_id, StringView source_id, StringView before_layer_id);

    [CCode (cname = "mln_map_add_color_relief_layer")]
    public static Status map_add_color_relief_layer (Map map, StringView layer_id, StringView source_id, StringView before_layer_id);

    [CCode (cname = "mln_map_add_location_indicator_layer")]
    public static Status map_add_location_indicator_layer (Map map, StringView layer_id, StringView before_layer_id);

    [CCode (cname = "mln_map_set_location_indicator_location")]
    public static Status map_set_location_indicator_location (Map map, StringView layer_id, LatLng coordinate, double altitude);

    [CCode (cname = "mln_map_set_location_indicator_bearing")]
    public static Status map_set_location_indicator_bearing (Map map, StringView layer_id, double bearing);

    [CCode (cname = "mln_map_set_location_indicator_accuracy_radius")]
    public static Status map_set_location_indicator_accuracy_radius (Map map, StringView layer_id, double radius);

    [CCode (cname = "mln_map_set_location_indicator_image_name")]
    public static Status map_set_location_indicator_image_name (Map map, StringView layer_id, uint32 image_kind, StringView image_id);

    [CCode (cname = "mln_map_add_style_layer_json")]
    public static Status map_add_style_layer_json (Map map, JsonValue* layer_json, StringView before_layer_id);

    [CCode (cname = "mln_map_remove_style_layer")]
    public static Status map_remove_style_layer (Map map, StringView layer_id, out bool out_removed);

    [CCode (cname = "mln_map_style_layer_exists")]
    public static Status map_style_layer_exists (Map map, StringView layer_id, out bool out_exists);

    [CCode (cname = "mln_map_get_style_layer_type")]
    public static Status map_get_style_layer_type (Map map, StringView layer_id, out StringView out_layer_type, out bool out_found);

    [CCode (cname = "mln_map_list_style_layer_ids")]
    public static Status map_list_style_layer_ids (Map map, out StyleIdList out_layer_ids);

    [CCode (cname = "mln_map_move_style_layer")]
    public static Status map_move_style_layer (Map map, StringView layer_id, StringView before_layer_id);

    [CCode (cname = "mln_map_get_style_layer_json")]
    public static Status map_get_style_layer_json (Map map, StringView layer_id, out JsonSnapshot out_layer, out bool out_found);

    [CCode (cname = "mln_map_set_style_light_json")]
    public static Status map_set_style_light_json (Map map, JsonValue* light_json);

    [CCode (cname = "mln_map_set_style_light_property")]
    public static Status map_set_style_light_property (Map map, StringView property_name, JsonValue* value);

    [CCode (cname = "mln_map_get_style_light_property")]
    public static Status map_get_style_light_property (Map map, StringView property_name, out JsonSnapshot out_value);

    [CCode (cname = "mln_map_set_layer_property")]
    public static Status map_set_layer_property (Map map, StringView layer_id, StringView property_name, JsonValue* value);

    [CCode (cname = "mln_map_get_layer_property")]
    public static Status map_get_layer_property (Map map, StringView layer_id, StringView property_name, out JsonSnapshot out_value);

    [CCode (cname = "mln_map_set_layer_filter")]
    public static Status map_set_layer_filter (Map map, StringView layer_id, JsonValue* filter);

    [CCode (cname = "mln_map_get_layer_filter")]
    public static Status map_get_layer_filter (Map map, StringView layer_id, out JsonSnapshot out_filter);

    [CCode (cname = "mln_map_set_style_image")]
    public static Status map_set_style_image (Map map, StringView image_id, PremultipliedRgba8Image* image, StyleImageOptions* options);

    [CCode (cname = "mln_map_remove_style_image")]
    public static Status map_remove_style_image (Map map, StringView image_id, out bool out_removed);

    [CCode (cname = "mln_map_style_image_exists")]
    public static Status map_style_image_exists (Map map, StringView image_id, out bool out_exists);

    [CCode (cname = "mln_map_get_style_image_info")]
    public static Status map_get_style_image_info (Map map, StringView image_id, StyleImageInfo* out_info, out bool out_found);

    [CCode (cname = "mln_map_copy_style_image_premultiplied_rgba8")]
    public static Status map_copy_style_image_premultiplied_rgba8 (Map map, StringView image_id, uint8* out_pixels, size_t pixel_capacity, out size_t out_byte_length, out bool out_found);

    [CCode (cname = "mln_map_add_image_source_url")]
    public static Status map_add_image_source_url (Map map, StringView source_id, LatLng* coordinates, size_t coordinate_count, StringView url);

    [CCode (cname = "mln_map_add_image_source_image")]
    public static Status map_add_image_source_image (Map map, StringView source_id, LatLng* coordinates, size_t coordinate_count, PremultipliedRgba8Image* image);

    [CCode (cname = "mln_map_set_image_source_url")]
    public static Status map_set_image_source_url (Map map, StringView source_id, StringView url);

    [CCode (cname = "mln_map_set_image_source_image")]
    public static Status map_set_image_source_image (Map map, StringView source_id, PremultipliedRgba8Image* image);

    [CCode (cname = "mln_map_set_image_source_coordinates")]
    public static Status map_set_image_source_coordinates (Map map, StringView source_id, LatLng* coordinates, size_t coordinate_count);

    [CCode (cname = "mln_map_get_image_source_coordinates")]
    public static Status map_get_image_source_coordinates (Map map, StringView source_id, LatLng* out_coordinates, size_t coordinate_capacity, out size_t out_coordinate_count, out bool out_found);

    [CCode (cname = "mln_rendered_feature_query_options_default")]
    public static RenderedFeatureQueryOptions rendered_feature_query_options_default ();

    [CCode (cname = "mln_source_feature_query_options_default")]
    public static SourceFeatureQueryOptions source_feature_query_options_default ();

    [CCode (cname = "mln_rendered_query_geometry_point")]
    public static RenderedQueryGeometry rendered_query_geometry_point (ScreenPoint point);

    [CCode (cname = "mln_rendered_query_geometry_box")]
    public static RenderedQueryGeometry rendered_query_geometry_box (ScreenBox box);

    [CCode (cname = "mln_rendered_query_geometry_line_string")]
    public static RenderedQueryGeometry rendered_query_geometry_line_string (ScreenPoint* points, size_t point_count);

    [CCode (cname = "mln_render_session_query_rendered_features")]
    public static Status render_session_query_rendered_features (RenderSession session, RenderedQueryGeometry* geometry, RenderedFeatureQueryOptions* options, out FeatureQueryResult out_result);

    [CCode (cname = "mln_render_session_query_source_features")]
    public static Status render_session_query_source_features (RenderSession session, StringView source_id, SourceFeatureQueryOptions* options, out FeatureQueryResult out_result);

    [CCode (cname = "mln_render_session_query_feature_extensions")]
    public static Status render_session_query_feature_extensions (RenderSession session, StringView source_id, Feature* feature, StringView extension, StringView extension_field, JsonValue* arguments, out FeatureExtensionResult out_result);

    [CCode (cname = "mln_render_session_set_feature_state")]
    public static Status render_session_set_feature_state (RenderSession session, FeatureStateSelector* selector, JsonValue* state);

    [CCode (cname = "mln_render_session_get_feature_state")]
    public static Status render_session_get_feature_state (RenderSession session, FeatureStateSelector* selector, out JsonSnapshot out_state);

    [CCode (cname = "mln_render_session_remove_feature_state")]
    public static Status render_session_remove_feature_state (RenderSession session, FeatureStateSelector* selector);

    [CCode (cname = "mln_json_snapshot_get")]
    public static Status json_snapshot_get (JsonSnapshot snapshot, out JsonValue* out_value);

    [CCode (cname = "mln_json_snapshot_destroy")]
    public static void json_snapshot_destroy (JsonSnapshot snapshot);

    [CCode (cname = "mln_feature_query_result_count")]
    public static Status feature_query_result_count (FeatureQueryResult result, out size_t out_count);

    [CCode (cname = "mln_feature_query_result_get")]
    public static Status feature_query_result_get (FeatureQueryResult result, size_t index, QueriedFeature* out_feature);

    [CCode (cname = "mln_feature_query_result_destroy")]
    public static void feature_query_result_destroy (FeatureQueryResult result);

    [CCode (cname = "mln_feature_extension_result_get")]
    public static Status feature_extension_result_get (FeatureExtensionResult result, FeatureExtensionResultInfo* out_info);

    [CCode (cname = "mln_feature_extension_result_destroy")]
    public static void feature_extension_result_destroy (FeatureExtensionResult result);

    [CCode (cname = "mln_metal_owned_texture_descriptor_default")]
    public static MetalOwnedTextureDescriptor metal_owned_texture_descriptor_default ();

    [CCode (cname = "mln_metal_borrowed_texture_descriptor_default")]
    public static MetalBorrowedTextureDescriptor metal_borrowed_texture_descriptor_default ();

    [CCode (cname = "mln_vulkan_owned_texture_descriptor_default")]
    public static VulkanOwnedTextureDescriptor vulkan_owned_texture_descriptor_default ();

    [CCode (cname = "mln_vulkan_borrowed_texture_descriptor_default")]
    public static VulkanBorrowedTextureDescriptor vulkan_borrowed_texture_descriptor_default ();

    [CCode (cname = "mln_opengl_owned_texture_descriptor_default")]
    public static OpenGLOwnedTextureDescriptor opengl_owned_texture_descriptor_default ();

    [CCode (cname = "mln_opengl_borrowed_texture_descriptor_default")]
    public static OpenGLBorrowedTextureDescriptor opengl_borrowed_texture_descriptor_default ();

    [CCode (cname = "mln_texture_image_info_default")]
    public static TextureImageInfo texture_image_info_default ();

    [CCode (cname = "mln_metal_surface_descriptor_default")]
    public static MetalSurfaceDescriptor metal_surface_descriptor_default ();

    [CCode (cname = "mln_vulkan_surface_descriptor_default")]
    public static VulkanSurfaceDescriptor vulkan_surface_descriptor_default ();

    [CCode (cname = "mln_opengl_surface_descriptor_default")]
    public static OpenGLSurfaceDescriptor opengl_surface_descriptor_default ();

    [CCode (cname = "mln_opengl_supported_context_provider_mask")]
    public static uint32 opengl_supported_context_provider_mask ();

    [CCode (cname = "mln_metal_owned_texture_attach")]
    public static Status metal_owned_texture_attach (Map map, MetalOwnedTextureDescriptor* descriptor, out RenderSession session);

    [CCode (cname = "mln_metal_borrowed_texture_attach")]
    public static Status metal_borrowed_texture_attach (Map map, MetalBorrowedTextureDescriptor* descriptor, out RenderSession session);

    [CCode (cname = "mln_vulkan_owned_texture_attach")]
    public static Status vulkan_owned_texture_attach (Map map, VulkanOwnedTextureDescriptor* descriptor, out RenderSession session);

    [CCode (cname = "mln_vulkan_borrowed_texture_attach")]
    public static Status vulkan_borrowed_texture_attach (Map map, VulkanBorrowedTextureDescriptor* descriptor, out RenderSession session);

    [CCode (cname = "mln_opengl_owned_texture_attach")]
    public static Status opengl_owned_texture_attach (Map map, OpenGLOwnedTextureDescriptor* descriptor, out RenderSession session);

    [CCode (cname = "mln_opengl_borrowed_texture_attach")]
    public static Status opengl_borrowed_texture_attach (Map map, OpenGLBorrowedTextureDescriptor* descriptor, out RenderSession session);

    [CCode (cname = "mln_metal_surface_attach")]
    public static Status metal_surface_attach (Map map, MetalSurfaceDescriptor* descriptor, out RenderSession session);

    [CCode (cname = "mln_vulkan_surface_attach")]
    public static Status vulkan_surface_attach (Map map, VulkanSurfaceDescriptor* descriptor, out RenderSession session);

    [CCode (cname = "mln_opengl_surface_attach")]
    public static Status opengl_surface_attach (Map map, OpenGLSurfaceDescriptor* descriptor, out RenderSession session);

    [CCode (cname = "mln_render_session_resize")]
    public static Status render_session_resize (RenderSession session, uint32 width, uint32 height, double scale_factor);

    [CCode (cname = "mln_render_session_render_update")]
    public static Status render_session_render_update (RenderSession session);

    [CCode (cname = "mln_render_session_detach")]
    public static Status render_session_detach (RenderSession session);

    [CCode (cname = "mln_render_session_destroy")]
    public static Status render_session_destroy (RenderSession session);

    [CCode (cname = "mln_render_session_reduce_memory_use")]
    public static Status render_session_reduce_memory_use (RenderSession session);

    [CCode (cname = "mln_render_session_clear_data")]
    public static Status render_session_clear_data (RenderSession session);

    [CCode (cname = "mln_render_session_dump_debug_logs")]
    public static Status render_session_dump_debug_logs (RenderSession session);

    [CCode (cname = "mln_texture_read_premultiplied_rgba8")]
    public static Status texture_read_premultiplied_rgba8 (RenderSession session, uint8* out_data, size_t out_data_capacity, TextureImageInfo* out_info);

    [CCode (cname = "mln_metal_owned_texture_acquire_frame")]
    public static Status metal_owned_texture_acquire_frame (RenderSession session, MetalOwnedTextureFrame* out_frame);

    [CCode (cname = "mln_metal_owned_texture_release_frame")]
    public static Status metal_owned_texture_release_frame (RenderSession session, MetalOwnedTextureFrame* frame);

    [CCode (cname = "mln_vulkan_owned_texture_acquire_frame")]
    public static Status vulkan_owned_texture_acquire_frame (RenderSession session, VulkanOwnedTextureFrame* out_frame);

    [CCode (cname = "mln_vulkan_owned_texture_release_frame")]
    public static Status vulkan_owned_texture_release_frame (RenderSession session, VulkanOwnedTextureFrame* frame);

    [CCode (cname = "mln_opengl_owned_texture_acquire_frame")]
    public static Status opengl_owned_texture_acquire_frame (RenderSession session, OpenGLOwnedTextureFrame* out_frame);

    [CCode (cname = "mln_opengl_owned_texture_release_frame")]
    public static Status opengl_owned_texture_release_frame (RenderSession session, OpenGLOwnedTextureFrame* frame);

    [CCode (cname = "mln_map_projection_create")]
    public static Status map_projection_create (Map map, out MapProjection projection);

    [CCode (cname = "mln_map_projection_destroy")]
    public static Status map_projection_destroy (MapProjection projection);

    [CCode (cname = "mln_map_projection_get_camera")]
    public static Status map_projection_get_camera (MapProjection projection, CameraOptions* out_camera);

    [CCode (cname = "mln_map_projection_set_camera")]
    public static Status map_projection_set_camera (MapProjection projection, CameraOptions* camera);

    [CCode (cname = "mln_map_projection_pixel_for_lat_lng")]
    public static Status map_projection_pixel_for_lat_lng (MapProjection projection, LatLng coordinate, out ScreenPoint out_point);

    [CCode (cname = "mln_map_projection_lat_lng_for_pixel")]
    public static Status map_projection_lat_lng_for_pixel (MapProjection projection, ScreenPoint point, out LatLng out_coordinate);

    [CCode (cname = "mln_map_projection_set_visible_coordinates")]
    public static Status map_projection_set_visible_coordinates (MapProjection projection, LatLng* coordinates, size_t coordinate_count, EdgeInsets padding);

    [CCode (cname = "mln_map_projection_set_visible_geometry")]
    public static Status map_projection_set_visible_geometry (MapProjection projection, Geometry* geometry, EdgeInsets padding);

    [CCode (cname = "mln_projected_meters_for_lat_lng")]
    public static Status projected_meters_for_lat_lng (LatLng coordinate, out ProjectedMeters out_meters);

    [CCode (cname = "mln_lat_lng_for_projected_meters")]
    public static Status lat_lng_for_projected_meters (ProjectedMeters meters, out LatLng out_coordinate);
}
