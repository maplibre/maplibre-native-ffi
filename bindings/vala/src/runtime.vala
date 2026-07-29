namespace MaplibreNative {
    private const uint32 EXPECTED_C_ABI_VERSION = 0;

    public enum ResourceLoadingMethod {
        ALL = 0,
        CACHE_ONLY = 1,
        NETWORK_ONLY = 2,
        UNKNOWN = 255
    }

    public enum ResourcePriority {
        REGULAR = 0,
        LOW = 1,
        UNKNOWN = 255
    }

    public enum ResourceUsage {
        ONLINE = 0,
        OFFLINE = 1,
        UNKNOWN = 255
    }

    public enum ResourceStoragePolicy {
        PERMANENT = 0,
        VOLATILE = 1,
        UNKNOWN = 255
    }

    public enum ResourceResponseStatus {
        OK = 0,
        ERROR = 1,
        NO_CONTENT = 2,
        NOT_MODIFIED = 3
    }

    public enum ResourceErrorReason {
        NONE = 0,
        NOT_FOUND = 1,
        SERVER = 2,
        CONNECTION = 3,
        RATE_LIMIT = 4,
        OTHER = 5,
        UNKNOWN = 255
    }

    public enum RuntimeEventPayloadType {
        NONE = 0,
        RENDER_FRAME = 1,
        RENDER_MAP = 2,
        STYLE_IMAGE_MISSING = 3,
        TILE_ACTION = 4,
        OFFLINE_REGION_STATUS = 5,
        OFFLINE_REGION_RESPONSE_ERROR = 6,
        OFFLINE_REGION_TILE_COUNT_LIMIT = 7,
        OFFLINE_OPERATION_COMPLETED = 8,
        CAMERA_TRANSITION_FINISHED = 9,
        UNKNOWN = 255
    }

    public enum RenderMode {
        PARTIAL = 0,
        FULL = 1,
        UNKNOWN = 255
    }

    public enum TileOperation {
        REQUESTED_FROM_CACHE = 0,
        REQUESTED_FROM_NETWORK = 1,
        LOAD_FROM_NETWORK = 2,
        LOAD_FROM_CACHE = 3,
        START_PARSE = 4,
        END_PARSE = 5,
        ERROR = 6,
        CANCELLED = 7,
        NULL = 8,
        UNKNOWN = 255
    }

    public enum OfflineRegionDownloadState {
        INACTIVE = 0,
        ACTIVE = 1,
        UNKNOWN = 255
    }

    public enum OfflineOperationKind {
        AMBIENT_CACHE = 1,
        REGION_CREATE = 2,
        REGION_GET = 3,
        REGIONS_LIST = 4,
        REGIONS_MERGE_DATABASE = 5,
        REGION_UPDATE_METADATA = 6,
        REGION_GET_STATUS = 7,
        REGION_SET_OBSERVED = 8,
        REGION_SET_DOWNLOAD_STATE = 9,
        REGION_INVALIDATE = 10,
        REGION_DELETE = 11,
        UNKNOWN = 255
    }

    public enum OfflineOperationResultKind {
        NONE = 0,
        REGION = 1,
        OPTIONAL_REGION = 2,
        REGION_LIST = 3,
        REGION_STATUS = 4,
        UNKNOWN = 255
    }

    public struct OfflineRegionId {
        public int64 value;

        public OfflineRegionId (int64 value) {
            this.value = value;
        }

        internal int64 to_native () {
            return value;
        }
    }

    public class OfflineOperationHandle {
        private RuntimeHandle runtime;
        private uint64 native_id;
        private OfflineOperationKind operation_kind;
        private OfflineOperationResultKind result_kind;
        private bool consumed;
        private bool consuming;
        private Mutex state_mutex;

        public bool closed {
            get {
                state_mutex.lock ();
                var value = consumed || consuming;
                state_mutex.unlock ();
                return value;
            }
        }

        internal OfflineOperationHandle (RuntimeHandle runtime, uint64 native_id, OfflineOperationKind operation_kind, OfflineOperationResultKind result_kind) {
            this.runtime = runtime;
            this.native_id = native_id;
            this.operation_kind = operation_kind;
            this.result_kind = result_kind;
        }

        ~OfflineOperationHandle () {
            state_mutex.lock ();
            var live = !consumed && !consuming;
            state_mutex.unlock ();
            if (live) {
                try {
                    runtime.discard_offline_operation (this);
                } catch (Error error) {
                    warning ("OfflineOperationHandle finalized while live and could not be discarded: %s", error.message);
                }
            }
        }

        public void close () throws Error {
            state_mutex.lock ();
            var already_closed = consumed;
            state_mutex.unlock ();
            if (already_closed) {
                return;
            }
            runtime.discard_offline_operation (this);
        }

        internal uint64 begin_consume (RuntimeHandle expected_runtime) throws Error {
            state_mutex.lock ();
            if (consumed || consuming) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("offline operation handle is closed");
            }
            if (runtime != expected_runtime) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("offline operation belongs to a different runtime");
            }
            consuming = true;
            state_mutex.unlock ();
            return native_id;
        }

        internal uint64 begin_result (RuntimeHandle expected_runtime, OfflineOperationKind expected_kind, OfflineOperationResultKind expected_result_kind) throws Error {
            state_mutex.lock ();
            if (consumed || consuming) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("offline operation handle is closed");
            }
            if (runtime != expected_runtime) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("offline operation belongs to a different runtime");
            }
            if (operation_kind != expected_kind) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("offline operation has the wrong operation kind");
            }
            if (result_kind != expected_result_kind) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("offline operation has the wrong result kind");
            }
            consuming = true;
            state_mutex.unlock ();
            return native_id;
        }

        internal void finish_consume () {
            state_mutex.lock ();
            consumed = true;
            consuming = false;
            state_mutex.unlock ();
        }

        internal void cancel_consume () {
            state_mutex.lock ();
            consuming = false;
            state_mutex.unlock ();
        }
    }

    public enum OfflineRegionDefinitionType {
        TILE_PYRAMID = 1,
        GEOMETRY = 2,
        UNKNOWN = 255
    }

    public class OfflineRegionDefinition {
        public OfflineRegionDefinitionType definition_type { get; private set; }
        public string style_url { get; private set; }
        public LatLngBounds bounds { get; private set; }
        public Geometry? geometry { get; private set; }
        public double min_zoom { get; private set; }
        public double max_zoom { get; private set; }
        public float pixel_ratio { get; private set; }
        public bool include_ideographs { get; private set; }

        private OfflineRegionDefinition (OfflineRegionDefinitionType definition_type, string style_url, LatLngBounds bounds, Geometry? geometry, double min_zoom, double max_zoom, float pixel_ratio, bool include_ideographs) {
            this.definition_type = definition_type;
            this.style_url = style_url;
            this.bounds = bounds;
            this.geometry = geometry;
            this.min_zoom = min_zoom;
            this.max_zoom = max_zoom;
            this.pixel_ratio = pixel_ratio;
            this.include_ideographs = include_ideographs;
        }

        public static OfflineRegionDefinition tile_pyramid (string style_url, LatLngBounds bounds, double min_zoom, double max_zoom, float pixel_ratio = 1.0f, bool include_ideographs = true) {
            return new OfflineRegionDefinition (OfflineRegionDefinitionType.TILE_PYRAMID, style_url, bounds, null, min_zoom, max_zoom, pixel_ratio, include_ideographs);
        }

        public static OfflineRegionDefinition geometry_region (string style_url, Geometry geometry, double min_zoom, double max_zoom, float pixel_ratio = 1.0f, bool include_ideographs = true) {
            return new OfflineRegionDefinition (OfflineRegionDefinitionType.GEOMETRY, style_url, LatLngBounds (LatLng (0.0, 0.0), LatLng (0.0, 0.0)), geometry, min_zoom, max_zoom, pixel_ratio, include_ideographs);
        }

        public OfflineRegionDefinition copy () throws Error {
            switch (definition_type) {
            case OfflineRegionDefinitionType.TILE_PYRAMID:
                return OfflineRegionDefinition.tile_pyramid (style_url, bounds, min_zoom, max_zoom, pixel_ratio, include_ideographs);
            case OfflineRegionDefinitionType.GEOMETRY:
                if (geometry == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_STATE ("offline geometry region has no geometry");
                }
                return OfflineRegionDefinition.geometry_region (style_url, geometry.copy (), min_zoom, max_zoom, pixel_ratio, include_ideographs);
            default:
                clear_unknown_status ();
                throw new Error.UNSUPPORTED ("unknown offline region definition type %u", definition_type);
            }
        }

        public bool equal (OfflineRegionDefinition other) {
            if (definition_type != other.definition_type
                || style_url != other.style_url
                || min_zoom != other.min_zoom
                || max_zoom != other.max_zoom
                || pixel_ratio != other.pixel_ratio
                || include_ideographs != other.include_ideographs) {
                return false;
            }
            switch (definition_type) {
            case OfflineRegionDefinitionType.TILE_PYRAMID:
                return bounds.southwest.latitude == other.bounds.southwest.latitude
                    && bounds.southwest.longitude == other.bounds.southwest.longitude
                    && bounds.northeast.latitude == other.bounds.northeast.latitude
                    && bounds.northeast.longitude == other.bounds.northeast.longitude;
            case OfflineRegionDefinitionType.GEOMETRY:
                return geometry == null
                    ? other.geometry == null
                    : other.geometry != null && geometry.equal (other.geometry);
            default:
                return false;
            }
        }

        internal Raw.OfflineRegionDefinition to_native (ref Raw.Geometry geometry_storage, out Geometry? geometry_owner) throws Error {
            geometry_owner = null;
            Raw.OfflineRegionDefinition definition = {};
            definition.size = (uint32) sizeof (Raw.OfflineRegionDefinition);
            definition.type = (uint32) definition_type;
            if (definition_type == OfflineRegionDefinitionType.TILE_PYRAMID) {
                definition.tile_pyramid = Raw.OfflineTilePyramidRegionDefinition () {
                    size = (uint32) sizeof (Raw.OfflineTilePyramidRegionDefinition),
                    style_url = c_string (style_url),
                    bounds = bounds.to_native (),
                    min_zoom = min_zoom,
                    max_zoom = max_zoom,
                    pixel_ratio = pixel_ratio,
                    include_ideographs = include_ideographs
                };
            } else if (definition_type == OfflineRegionDefinitionType.GEOMETRY) {
                if (geometry == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("offline geometry region has no geometry");
                }
                geometry_owner = geometry.copy ();
                geometry_storage = geometry_owner.to_native ();
                definition.geometry = Raw.OfflineGeometryRegionDefinition () {
                    size = (uint32) sizeof (Raw.OfflineGeometryRegionDefinition),
                    style_url = c_string (style_url),
                    geometry = &geometry_storage,
                    min_zoom = min_zoom,
                    max_zoom = max_zoom,
                    pixel_ratio = pixel_ratio,
                    include_ideographs = include_ideographs
                };
            } else {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("unknown offline region definition type");
            }
            return definition;
        }

        internal static OfflineRegionDefinition from_native (Raw.OfflineRegionDefinition native) throws Error {
            switch ((OfflineRegionDefinitionType) native.type) {
                case OfflineRegionDefinitionType.TILE_PYRAMID:
                    return OfflineRegionDefinition.tile_pyramid (
                        copy_c_string (native.tile_pyramid.style_url),
                        LatLngBounds.from_native (native.tile_pyramid.bounds),
                        native.tile_pyramid.min_zoom,
                        native.tile_pyramid.max_zoom,
                        native.tile_pyramid.pixel_ratio,
                        native.tile_pyramid.include_ideographs);
                case OfflineRegionDefinitionType.GEOMETRY:
                    if (native.geometry.geometry == null) {
                        clear_unknown_status ();
                        throw new Error.INVALID_ARGUMENT ("offline geometry region native geometry is null");
                    }
                    return OfflineRegionDefinition.geometry_region (
                        copy_c_string (native.geometry.style_url),
                        Geometry.from_native (native.geometry.geometry[0]),
                        native.geometry.min_zoom,
                        native.geometry.max_zoom,
                        native.geometry.pixel_ratio,
                        native.geometry.include_ideographs);
                default:
                    clear_unknown_status ();
                    throw new Error.UNSUPPORTED ("unknown offline region definition type %u", native.type);
            }
        }
    }

    public class OfflineRegionInfo {
        private uint8[] metadata_storage;

        public OfflineRegionId id { get; private set; }
        public OfflineRegionDefinition definition { get; private set; }
        public uint8[] metadata {
            owned get {
                return copy_byte_array (metadata_storage);
            }
        }

        internal OfflineRegionInfo.from_native (Raw.OfflineRegionInfo native) throws Error {
            id = OfflineRegionId (native.id);
            definition = OfflineRegionDefinition.from_native (native.definition);
            metadata_storage = copy_bytes (native.metadata, native.metadata_size) ?? new uint8[0];
        }

        private OfflineRegionInfo.from_values (OfflineRegionId id, OfflineRegionDefinition definition, uint8[] metadata) {
            this.id = id;
            this.definition = definition;
            metadata_storage = copy_byte_array (metadata);
        }

        public OfflineRegionInfo copy () throws Error {
            return new OfflineRegionInfo.from_values (id, definition.copy (), metadata_storage);
        }

        public bool equal (OfflineRegionInfo other) {
            if (id.value != other.id.value
                || !definition.equal (other.definition)
                || metadata_storage.length != other.metadata_storage.length) {
                return false;
            }
            for (var index = 0; index < metadata_storage.length; index++) {
                if (metadata_storage[index] != other.metadata_storage[index]) {
                    return false;
                }
            }
            return true;
        }
    }

    internal OfflineRegionInfo copy_offline_region_snapshot (owned Raw.OfflineRegionSnapshot native) throws Error {
        try {
            Raw.OfflineRegionInfo info = {};
            info.size = (uint32) sizeof (Raw.OfflineRegionInfo);
            check_status (Raw.offline_region_snapshot_get (native, &info));
            return new OfflineRegionInfo.from_native (info);
        } finally {
            Raw.offline_region_snapshot_destroy (native);
        }
    }

    internal OfflineRegionInfo[] copy_offline_region_list (owned Raw.OfflineRegionList native) throws Error {
        try {
            size_t item_count;
            check_status (Raw.offline_region_list_count (native, out item_count));
            OfflineRegionInfo[] values = new OfflineRegionInfo[item_count];
            for (size_t index = 0; index < item_count; index++) {
                Raw.OfflineRegionInfo info = {};
                info.size = (uint32) sizeof (Raw.OfflineRegionInfo);
                check_status (Raw.offline_region_list_get (native, index, &info));
                values[index] = new OfflineRegionInfo.from_native (info);
            }
            return values;
        } finally {
            Raw.offline_region_list_destroy (native);
        }
    }

    public struct RenderingStats {
        public double encoding_time;
        public double rendering_time;
        public int64 frame_count;
        public int64 draw_call_count;
        public int64 total_draw_call_count;

        internal static RenderingStats from_native (Raw.RenderingStats native) {
            return RenderingStats () {
                encoding_time = native.encoding_time,
                rendering_time = native.rendering_time,
                frame_count = native.frame_count,
                draw_call_count = native.draw_call_count,
                total_draw_call_count = native.total_draw_call_count
            };
        }
    }

    public struct TileId {
        public uint32 overscaled_z;
        public int32 wrap;
        public uint32 canonical_z;
        public uint32 canonical_x;
        public uint32 canonical_y;

        internal static TileId from_native (Raw.TileId native) {
            return TileId () {
                overscaled_z = native.overscaled_z,
                wrap = native.wrap,
                canonical_z = native.canonical_z,
                canonical_x = native.canonical_x,
                canonical_y = native.canonical_y
            };
        }
    }

    public struct OfflineRegionStatus {
        public OfflineRegionDownloadState download_state;
        public uint64 completed_resource_count;
        public uint64 completed_resource_size;
        public uint64 completed_tile_count;
        public uint64 required_tile_count;
        public uint64 completed_tile_size;
        public uint64 required_resource_count;
        public bool required_resource_count_is_precise;
        public bool complete;

        internal static OfflineRegionStatus from_native (Raw.OfflineRegionStatus native) {
            return OfflineRegionStatus () {
                download_state = offline_region_download_state_from_raw (native.download_state),
                completed_resource_count = native.completed_resource_count,
                completed_resource_size = native.completed_resource_size,
                completed_tile_count = native.completed_tile_count,
                required_tile_count = native.required_tile_count,
                completed_tile_size = native.completed_tile_size,
                required_resource_count = native.required_resource_count,
                required_resource_count_is_precise = native.required_resource_count_is_precise,
                complete = native.complete
            };
        }
    }

    public class RuntimeEventRenderFrame {
        public RenderMode mode { get; private set; }
        public bool needs_repaint { get; private set; }
        public bool placement_changed { get; private set; }
        public RenderingStats stats { get; private set; }

        internal RuntimeEventRenderFrame.from_native (Raw.RuntimeEventRenderFrame native) {
            mode = render_mode_from_raw (native.mode);
            needs_repaint = native.needs_repaint;
            placement_changed = native.placement_changed;
            stats = RenderingStats.from_native (native.stats);
        }
    }

    public class RuntimeEventRenderMap {
        public RenderMode mode { get; private set; }

        internal RuntimeEventRenderMap.from_native (Raw.RuntimeEventRenderMap native) {
            mode = render_mode_from_raw (native.mode);
        }
    }

    public class RuntimeEventStyleImageMissing {
        private Utf8String image_id_storage;

        public string? image_id {
            owned get { return image_id_storage.to_string_or_null (); }
        }

        internal RuntimeEventStyleImageMissing.from_native (Raw.RuntimeEventStyleImageMissing native) throws Error {
            image_id_storage = copy_sized_utf8 (native.image_id, native.image_id_size, "style image ID");
        }

        public Utf8String get_image_id_utf8 () {
            return image_id_storage.copy ();
        }
    }

    public class RuntimeEventTileAction {
        private Utf8String source_id_storage;

        public TileOperation operation { get; private set; }
        public TileId tile_id { get; private set; }
        public string? source_id {
            owned get { return source_id_storage.to_string_or_null (); }
        }

        internal RuntimeEventTileAction.from_native (Raw.RuntimeEventTileAction native) throws Error {
            operation = tile_operation_from_raw (native.operation);
            tile_id = TileId.from_native (native.tile_id);
            source_id_storage = copy_sized_utf8 (native.source_id, native.source_id_size, "tile source ID");
        }

        public Utf8String get_source_id_utf8 () {
            return source_id_storage.copy ();
        }
    }

    public class RuntimeEventOfflineRegionStatus {
        public OfflineRegionId region_id { get; private set; }
        public OfflineRegionStatus status { get; private set; }

        internal RuntimeEventOfflineRegionStatus.from_native (Raw.RuntimeEventOfflineRegionStatus native) {
            region_id = OfflineRegionId (native.region_id);
            status = OfflineRegionStatus.from_native (native.status);
        }
    }

    public class RuntimeEventOfflineRegionResponseError {
        public OfflineRegionId region_id { get; private set; }
        public ResourceErrorReason reason { get; private set; }

        internal RuntimeEventOfflineRegionResponseError.from_native (Raw.RuntimeEventOfflineRegionResponseError native) {
            region_id = OfflineRegionId (native.region_id);
            reason = resource_error_reason_from_raw (native.reason);
        }
    }

    public class RuntimeEventOfflineRegionTileCountLimit {
        public OfflineRegionId region_id { get; private set; }
        public uint64 limit { get; private set; }

        internal RuntimeEventOfflineRegionTileCountLimit.from_native (Raw.RuntimeEventOfflineRegionTileCountLimit native) {
            region_id = OfflineRegionId (native.region_id);
            limit = native.limit;
        }
    }

    public class RuntimeEventOfflineOperationCompleted {
        public OfflineOperationHandle? operation { get; private set; }
        public OfflineOperationKind operation_kind { get; private set; }
        public OfflineOperationResultKind result_kind { get; private set; }
        public int32 result_status { get; private set; }
        public bool found { get; private set; }

        internal RuntimeEventOfflineOperationCompleted.from_native (RuntimeHandle runtime, Raw.RuntimeEventOfflineOperationCompleted native) {
            operation = runtime.resolve_offline_operation (native.operation_id);
            operation_kind = offline_operation_kind_from_raw (native.operation_kind);
            result_kind = offline_operation_result_kind_from_raw (native.result_kind);
            result_status = native.result_status;
            found = native.found;
        }
    }

    public class RuntimeEventCameraTransitionFinished {
        public uint64 transition_id { get; private set; }

        internal RuntimeEventCameraTransitionFinished.from_native (Raw.RuntimeEventCameraTransitionFinished native) {
            transition_id = native.transition_id;
        }
    }

    /**
     * Handles a copied resource request on the native worker or network thread
     * that issued it. Implementations stay thread-safe and return promptly,
     * call only methods on the provided request handle, and avoid map or runtime
     * APIs. A handled request may retain the handle for later completion from
     * any thread and releases it when finished; a pass-through request retains
     * neither argument. The runtime retains this callback until replacement or
     * close and waits for in-flight calls before releasing it.
     */
    public delegate ResourceProviderDecision ResourceProviderCallback (ResourceRequest request, ResourceRequestHandle handle);

    public class ResourceRequest {
        private uint8[]? prior_data_storage;

        public string url { get; private set; }
        public ResourceKind kind { get; private set; }
        public ResourceLoadingMethod loading_method { get; private set; }
        public ResourcePriority priority { get; private set; }
        public ResourceUsage usage { get; private set; }
        public ResourceStoragePolicy storage_policy { get; private set; }
        public bool has_range { get; private set; }
        public uint64 range_start { get; private set; }
        public uint64 range_end { get; private set; }
        public int64? prior_modified_unix_ms { get; private set; }
        public int64? prior_expires_unix_ms { get; private set; }
        public string? prior_etag { get; private set; }
        public uint8[]? prior_data {
            owned get {
                return prior_data_storage == null ? null : copy_byte_array (prior_data_storage);
            }
        }

        internal ResourceRequest.from_native (Raw.ResourceRequest* native) {
            url = copy_c_string (native->url);
            kind = resource_kind_from_raw (native->kind);
            loading_method = resource_loading_method_from_raw (native->loading_method);
            priority = resource_priority_from_raw (native->priority);
            usage = resource_usage_from_raw (native->usage);
            storage_policy = resource_storage_policy_from_raw (native->storage_policy);
            has_range = native->has_range;
            range_start = native->range_start;
            range_end = native->range_end;
            if (native->has_prior_modified) {
                prior_modified_unix_ms = native->prior_modified_unix_ms;
            } else {
                prior_modified_unix_ms = null;
            }
            if (native->has_prior_expires) {
                prior_expires_unix_ms = native->prior_expires_unix_ms;
            } else {
                prior_expires_unix_ms = null;
            }
            prior_etag = native->prior_etag != null ? copy_c_string (native->prior_etag) : null;
            prior_data_storage = copy_bytes (native->prior_data, native->prior_data_size);
        }
    }

    public class ResourceResponse {
        private uint8[] bytes_storage = new uint8[0];

        public ResourceResponseStatus status { get; set; default = ResourceResponseStatus.OK; }
        public ResourceErrorReason error_reason { get; set; default = ResourceErrorReason.NONE; }
        public uint8[] bytes {
            owned get {
                return copy_byte_array (bytes_storage);
            }
            set {
                bytes_storage = copy_byte_array (value);
            }
        }
        public string? error_message { get; set; }
        public bool must_revalidate { get; set; }
        public int64? modified_unix_ms { get; set; }
        public int64? expires_unix_ms { get; set; }
        public string? etag { get; set; }
        public int64? retry_after_unix_ms { get; set; }

        public ResourceResponse () {}

        public static ResourceResponse data (uint8[] bytes) {
            var response = new ResourceResponse ();
            response.status = ResourceResponseStatus.OK;
            response.bytes = bytes;
            return response;
        }

        public static ResourceResponse error (ResourceErrorReason reason, string message) {
            var response = new ResourceResponse ();
            response.status = ResourceResponseStatus.ERROR;
            response.error_reason = reason;
            response.error_message = message;
            return response;
        }

        public ResourceResponse copy () {
            var copied = new ResourceResponse ();
            copied.status = status;
            copied.error_reason = error_reason;
            copied.bytes_storage = copy_byte_array (bytes_storage);
            copied.error_message = error_message;
            copied.must_revalidate = must_revalidate;
            copied.modified_unix_ms = modified_unix_ms;
            copied.expires_unix_ms = expires_unix_ms;
            copied.etag = etag;
            copied.retry_after_unix_ms = retry_after_unix_ms;
            return copied;
        }

        public bool equal (ResourceResponse other) {
            if (status != other.status
                || error_reason != other.error_reason
                || error_message != other.error_message
                || must_revalidate != other.must_revalidate
                || modified_unix_ms != other.modified_unix_ms
                || expires_unix_ms != other.expires_unix_ms
                || etag != other.etag
                || retry_after_unix_ms != other.retry_after_unix_ms
                || bytes_storage.length != other.bytes_storage.length) {
                return false;
            }
            for (var index = 0; index < bytes_storage.length; index++) {
                if (bytes_storage[index] != other.bytes_storage[index]) {
                    return false;
                }
            }
            return true;
        }

        internal Raw.ResourceResponse to_native () throws Error {
            Raw.ResourceResponse response = {};
            response.size = (uint32) sizeof (Raw.ResourceResponse);
            response.status = (uint32) status;
            response.error_reason = (uint32) error_reason;
            response.bytes = bytes_storage.length > 0 ? bytes_storage : null;
            response.byte_count = bytes_storage.length;
            response.error_message = optional_c_string (error_message);
            response.must_revalidate = must_revalidate;
            response.has_modified = modified_unix_ms != null;
            response.modified_unix_ms = modified_unix_ms ?? 0;
            response.has_expires = expires_unix_ms != null;
            response.expires_unix_ms = expires_unix_ms ?? 0;
            response.etag = optional_c_string (etag);
            response.has_retry_after = retry_after_unix_ms != null;
            response.retry_after_unix_ms = retry_after_unix_ms ?? 0;
            return response;
        }
    }

    public class ResourceRequestHandle {
        private unowned Raw.ResourceRequestHandle? native;
        private Mutex mutex;
        private bool completed;
        private bool provider_decision_pending = true;

        public bool released {
            get {
                mutex.lock ();
                var value = native == null;
                mutex.unlock ();
                return value;
            }
        }

        public bool is_completed {
            get {
                mutex.lock ();
                var value = completed;
                mutex.unlock ();
                return value;
            }
        }

        internal ResourceRequestHandle (Raw.ResourceRequestHandle native) {
            this.native = native;
        }

        ~ResourceRequestHandle () {
            mutex.lock ();
            unowned Raw.ResourceRequestHandle? live = native;
            native = null;
            mutex.unlock ();

            if (live != null) {
                warning ("ResourceRequestHandle finalized while live; call release() after completing or abandoning the request");
                Raw.resource_request_release (live);
            }
        }

        internal unowned Raw.ResourceRequestHandle require_live () throws Error {
            if (native == null) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("resource request handle is released");
            }
            return native;
        }

        public bool cancelled () throws Error {
            bool is_cancelled = false;
            mutex.lock ();
            try {
                check_status (Raw.resource_request_cancelled (require_live (), out is_cancelled));
            } finally {
                mutex.unlock ();
            }
            return is_cancelled;
        }

        public void complete (ResourceResponse response) throws Error {
            var response_storage = response.copy ();
            var native_response = response_storage.to_native ();
            mutex.lock ();
            try {
                if (completed) {
                    clear_unknown_status ();
                    throw new Error.INVALID_STATE ("resource request is already completed");
                }
                var status = Raw.resource_request_complete (require_live (), &native_response);
                completed = true;
                check_status (status);
            } finally {
                mutex.unlock ();
            }
        }

        public void complete_and_release (ResourceResponse response) throws Error {
            try {
                complete (response);
            } catch (Error error) {
                if (is_completed && !provider_decision_is_pending ()) {
                    release ();
                }
                throw error;
            }
            if (!provider_decision_is_pending ()) {
                release ();
            }
        }

        private bool provider_decision_is_pending () {
            mutex.lock ();
            var pending = provider_decision_pending;
            mutex.unlock ();
            return pending;
        }

        public void release () throws Error {
            mutex.lock ();
            if (provider_decision_pending) {
                mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("resource request handle ownership transfers only after the provider callback returns HANDLE");
            }
            unowned Raw.ResourceRequestHandle? live = native;
            native = null;
            mutex.unlock ();

            if (live != null) {
                Raw.resource_request_release (live);
            }
        }

        internal uint32 finish_provider_decision (ResourceProviderDecision decision) {
            unowned Raw.ResourceRequestHandle? release_after_unlock = null;
            uint32 native_decision;

            mutex.lock ();
            provider_decision_pending = false;
            if (native == null) {
                native_decision = (uint32) Raw.ResourceProviderDecision.HANDLE;
            } else if (completed) {
                release_after_unlock = native;
                native = null;
                native_decision = (uint32) Raw.ResourceProviderDecision.HANDLE;
            } else if (decision == ResourceProviderDecision.HANDLE) {
                native_decision = (uint32) Raw.ResourceProviderDecision.HANDLE;
            } else {
                native = null;
                native_decision = resource_provider_decision_to_raw (decision);
            }
            mutex.unlock ();

            if (release_after_unlock != null) {
                Raw.resource_request_release (release_after_unlock);
            }
            return native_decision;
        }

    }

    internal uint32 resource_provider_decision_to_raw (ResourceProviderDecision decision) {
        return (uint32) decision;
    }

    public class RuntimeOptions {
        public string? asset_path { get; set; }
        public string? cache_path { get; set; }
        public uint64? maximum_cache_size { get; set; }

        public RuntimeOptions copy () {
            var copied = new RuntimeOptions ();
            copied.asset_path = asset_path;
            copied.cache_path = cache_path;
            copied.maximum_cache_size = maximum_cache_size;
            return copied;
        }

        public bool equal (RuntimeOptions other) {
            return asset_path == other.asset_path
                && cache_path == other.cache_path
                && maximum_cache_size == other.maximum_cache_size;
        }

        internal Raw.RuntimeOptions to_native () throws Error {
            Raw.RuntimeOptions options = Raw.runtime_options_default ();
            options.asset_path = optional_c_string (asset_path);
            options.cache_path = optional_c_string (cache_path);
            if (maximum_cache_size != null) {
                options.maximum_cache_size = maximum_cache_size;
                options.flags |= (uint32) Raw.RuntimeOptionFlag.MAXIMUM_CACHE_SIZE;
            }
            return options;
        }
    }

    public class RuntimeEvent {
        private uint8[] payload_storage;
        private Utf8String message_storage;

        public RuntimeEventType event_type { get; private set; }
        public RuntimeEventSourceType source_type { get; private set; }
        public int32 code { get; private set; }
        public CameraChangeMode? camera_change_mode { get; private set; }
        public RuntimeEventPayloadType payload_type { get; private set; }
        public string? message {
            owned get { return message_storage.to_string_or_null (); }
        }
        public uint8[] payload_bytes {
            owned get {
                return copy_byte_array (payload_storage);
            }
        }
        public MapHandle? source_map { get; private set; }
        public RuntimeEventRenderFrame? render_frame { get; private set; }
        public RuntimeEventRenderMap? render_map { get; private set; }
        public RuntimeEventStyleImageMissing? style_image_missing { get; private set; }
        public RuntimeEventTileAction? tile_action { get; private set; }
        public RuntimeEventOfflineRegionStatus? offline_region_status { get; private set; }
        public RuntimeEventOfflineRegionResponseError? offline_region_response_error { get; private set; }
        public RuntimeEventOfflineRegionTileCountLimit? offline_region_tile_count_limit { get; private set; }
        public RuntimeEventOfflineOperationCompleted? offline_operation_completed { get; private set; }
        public RuntimeEventCameraTransitionFinished? camera_transition_finished { get; private set; }

        internal RuntimeEvent (RuntimeHandle runtime, Raw.RuntimeEvent native) throws Error {
            event_type = runtime_event_type_from_raw (native.type);
            source_type = runtime_event_source_type_from_raw (native.source_type);
            if (source_type == RuntimeEventSourceType.MAP) {
                source_map = runtime.resolve_map (native.source);
                if (event_type == RuntimeEventType.MAP_STYLE_LOADED && source_map != null) {
                    source_map.release_replaced_custom_geometry_sources ();
                }
            }
            code = native.code;
            if (event_type == RuntimeEventType.MAP_CAMERA_WILL_CHANGE
                || event_type == RuntimeEventType.MAP_CAMERA_DID_CHANGE) {
                camera_change_mode = (CameraChangeMode) ((uint32) native.code);
            }
            payload_type = runtime_event_payload_type_from_raw (native.payload_type);
            message_storage = copy_sized_utf8 (native.message, native.message_size, "runtime event message");
            payload_storage = copy_sized_bytes ((uint8*) native.payload, native.payload_size, "runtime event payload");
            if (native.payload == null) {
                return;
            }
            switch (payload_type) {
                case RuntimeEventPayloadType.RENDER_FRAME:
                    validate_payload (native, sizeof (Raw.RuntimeEventRenderFrame));
                    render_frame = new RuntimeEventRenderFrame.from_native (((Raw.RuntimeEventRenderFrame*) native.payload)[0]);
                    break;
                case RuntimeEventPayloadType.RENDER_MAP:
                    validate_payload (native, sizeof (Raw.RuntimeEventRenderMap));
                    render_map = new RuntimeEventRenderMap.from_native (((Raw.RuntimeEventRenderMap*) native.payload)[0]);
                    break;
                case RuntimeEventPayloadType.STYLE_IMAGE_MISSING:
                    validate_payload (native, sizeof (Raw.RuntimeEventStyleImageMissing));
                    style_image_missing = new RuntimeEventStyleImageMissing.from_native (((Raw.RuntimeEventStyleImageMissing*) native.payload)[0]);
                    break;
                case RuntimeEventPayloadType.TILE_ACTION:
                    validate_payload (native, sizeof (Raw.RuntimeEventTileAction));
                    tile_action = new RuntimeEventTileAction.from_native (((Raw.RuntimeEventTileAction*) native.payload)[0]);
                    break;
                case RuntimeEventPayloadType.OFFLINE_REGION_STATUS:
                    validate_payload (native, sizeof (Raw.RuntimeEventOfflineRegionStatus));
                    offline_region_status = new RuntimeEventOfflineRegionStatus.from_native (((Raw.RuntimeEventOfflineRegionStatus*) native.payload)[0]);
                    break;
                case RuntimeEventPayloadType.OFFLINE_REGION_RESPONSE_ERROR:
                    validate_payload (native, sizeof (Raw.RuntimeEventOfflineRegionResponseError));
                    offline_region_response_error = new RuntimeEventOfflineRegionResponseError.from_native (((Raw.RuntimeEventOfflineRegionResponseError*) native.payload)[0]);
                    break;
                case RuntimeEventPayloadType.OFFLINE_REGION_TILE_COUNT_LIMIT:
                    validate_payload (native, sizeof (Raw.RuntimeEventOfflineRegionTileCountLimit));
                    offline_region_tile_count_limit = new RuntimeEventOfflineRegionTileCountLimit.from_native (((Raw.RuntimeEventOfflineRegionTileCountLimit*) native.payload)[0]);
                    break;
                case RuntimeEventPayloadType.OFFLINE_OPERATION_COMPLETED:
                    validate_payload (native, sizeof (Raw.RuntimeEventOfflineOperationCompleted));
                    offline_operation_completed = new RuntimeEventOfflineOperationCompleted.from_native (runtime, ((Raw.RuntimeEventOfflineOperationCompleted*) native.payload)[0]);
                    break;
                case RuntimeEventPayloadType.CAMERA_TRANSITION_FINISHED:
                    validate_payload (native, sizeof (Raw.RuntimeEventCameraTransitionFinished));
                    camera_transition_finished = new RuntimeEventCameraTransitionFinished.from_native (((Raw.RuntimeEventCameraTransitionFinished*) native.payload)[0]);
                    break;
                default:
                    break;
            }
        }

        public Utf8String get_message_utf8 () {
            return message_storage.copy ();
        }

        private static void validate_payload (Raw.RuntimeEvent native, size_t expected_size) throws Error {
            if (native.payload_size < expected_size) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("runtime event payload is smaller than its declared type");
            }
            uint32 declared_size = ((uint32*) native.payload)[0];
            if (declared_size < expected_size) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("runtime event payload struct size is too small");
            }
        }
    }

    /**
     * Receives a copied log record on a logging or worker thread, potentially
     * while MapLibre holds internal logging locks. Implementations stay
     * thread-safe, return promptly, and call no MapLibre APIs, including log
     * callback replacement or clearing. The process retains this callback until
     * replacement or clearing and waits for in-flight calls before releasing it.
     */
    public delegate bool LogCallback (LogSeverity severity, LogEvent event, int64 code, string? message);

    /**
     * Rewrites a copied resource URL on an arbitrary worker or network thread.
     * Implementations stay thread-safe, return promptly, and call no MapLibre
     * APIs; the returned string is copied before the callback returns. The
     * runtime retains this callback until replacement, clearing, or close and
     * waits for in-flight calls before releasing it.
     */
    public delegate string? ResourceTransformCallback (ResourceKind kind, string url);

    private class LogRegistration {
        private LogCallback callback;
        private Mutex mutex;
        private Cond idle;
        private bool closing;
        private uint active_callbacks;

        public LogRegistration (owned LogCallback callback) {
            this.callback = (owned) callback;
        }

        public uint32 invoke (uint32 severity, uint32 event, int64 code, string? message) {
            mutex.lock ();
            if (closing) {
                mutex.unlock ();
                return 0;
            }
            active_callbacks++;
            mutex.unlock ();
            try {
                return callback (log_severity_from_raw (severity), log_event_from_raw (event), code, message) ? 1U : 0U;
            } finally {
                mutex.lock ();
                active_callbacks--;
                if (closing && active_callbacks == 0) {
                    idle.broadcast ();
                }
                mutex.unlock ();
            }
        }

        public void close () {
            mutex.lock ();
            closing = true;
            while (active_callbacks > 0) {
                idle.wait (mutex);
            }
            mutex.unlock ();
        }

    }

    private class ResourceTransformRegistration {
        private ResourceTransformCallback callback;
        private Mutex mutex;
        private Cond idle;
        private bool closing;
        private uint active_callbacks;

        public ResourceTransformRegistration (owned ResourceTransformCallback callback) {
            this.callback = (owned) callback;
        }

        public Raw.Status invoke (uint32 raw_kind, string url, Raw.ResourceTransformResponse* out_response) {
            if (out_response == null) {
                return Raw.Status.INVALID_ARGUMENT;
            }
            out_response->size = (uint32) sizeof (Raw.ResourceTransformResponse);
            out_response->url = null;
            mutex.lock ();
            if (closing) {
                mutex.unlock ();
                return Raw.Status.OK;
            }
            active_callbacks++;
            mutex.unlock ();
            try {
                var replacement = callback (resource_kind_from_raw (raw_kind), url);
                if (replacement != null && replacement.length > 0) {
                    return Raw.resource_transform_response_set_url (out_response, replacement, replacement.length);
                }
                return Raw.Status.OK;
            } finally {
                mutex.lock ();
                active_callbacks--;
                if (closing && active_callbacks == 0) {
                    idle.broadcast ();
                }
                mutex.unlock ();
            }
        }

        public void close () {
            mutex.lock ();
            closing = true;
            while (active_callbacks > 0) {
                idle.wait (mutex);
            }
            mutex.unlock ();
        }

        public void reopen () {
            mutex.lock ();
            closing = false;
            mutex.unlock ();
        }
    }

    private class ResourceProviderRegistration {
        private ResourceProviderCallback callback;
        private Mutex mutex;
        private Cond idle;
        private bool closing;
        private uint active_callbacks;

        public ResourceProviderRegistration (owned ResourceProviderCallback callback) {
            this.callback = (owned) callback;
        }

        public uint32 invoke (Raw.ResourceRequest* request, Raw.ResourceRequestHandle handle) {
            mutex.lock ();
            if (closing) {
                mutex.unlock ();
                return (uint32) Raw.ResourceProviderDecision.PASS_THROUGH;
            }
            active_callbacks++;
            mutex.unlock ();
            try {
                var copied_request = new ResourceRequest.from_native (request);
                var request_handle = new ResourceRequestHandle (handle);
                var decision = callback (copied_request, request_handle);
                return request_handle.finish_provider_decision (decision);
            } finally {
                mutex.lock ();
                active_callbacks--;
                if (closing && active_callbacks == 0) {
                    idle.broadcast ();
                }
                mutex.unlock ();
            }
        }

        public void close () {
            mutex.lock ();
            closing = true;
            while (active_callbacks > 0) {
                idle.wait (mutex);
            }
            mutex.unlock ();
        }

        public void reopen () {
            mutex.lock ();
            closing = false;
            mutex.unlock ();
        }
    }

    private LogRegistration? current_log_registration;
    private Mutex log_registration_mutex;

    private uint32 log_trampoline (void* user_data, uint32 severity, uint32 event, int64 code, string? message) {
        if (user_data == null) {
            return 0;
        }
        unowned LogRegistration registration = (LogRegistration) user_data;
        return registration.invoke (severity, event, code, message);
    }

    private Raw.Status resource_transform_trampoline (void* user_data, uint32 kind, string url, Raw.ResourceTransformResponse* out_response) {
        if (user_data == null) {
            return Raw.Status.INVALID_ARGUMENT;
        }
        unowned ResourceTransformRegistration registration = (ResourceTransformRegistration) user_data;
        return registration.invoke (kind, url, out_response);
    }

    private uint32 resource_provider_trampoline (void* user_data, Raw.ResourceRequest* request, Raw.ResourceRequestHandle handle) {
        if (user_data == null || request == null || handle == null) {
            return (uint32) Raw.ResourceProviderDecision.PASS_THROUGH;
        }
        unowned ResourceProviderRegistration registration = (ResourceProviderRegistration) user_data;
        return registration.invoke (request, handle);
    }

    public uint32 c_version () {
        return Raw.c_version ();
    }

    public RenderBackendFlags supported_render_backends () {
        return (RenderBackendFlags) Raw.supported_render_backend_mask ();
    }

    public OpenGLContextProviderFlags opengl_supported_context_providers () {
        return (OpenGLContextProviderFlags) Raw.opengl_supported_context_provider_mask ();
    }

    public NetworkStatus network_status () throws Error {
        uint32 raw_status;
        check_status (Raw.network_status_get (out raw_status));
        return network_status_from_raw (raw_status);
    }

    public void set_network_status (NetworkStatus status) throws Error {
        check_status (Raw.network_status_set ((uint32) status));
    }

    public void set_log_callback (owned LogCallback callback) throws Error {
        var registration = new LogRegistration ((owned) callback);
        LogRegistration? previous = null;
        log_registration_mutex.lock ();
        try {
            check_status (Raw.log_set_callback (log_trampoline, registration));
            previous = current_log_registration;
            current_log_registration = registration;
        } finally {
            log_registration_mutex.unlock ();
        }
        if (previous != null) {
            previous.close ();
        }
    }

    public void clear_log_callback () throws Error {
        LogRegistration? previous = null;
        log_registration_mutex.lock ();
        try {
            check_status (Raw.log_clear_callback ());
            previous = current_log_registration;
            current_log_registration = null;
        } finally {
            log_registration_mutex.unlock ();
        }
        if (previous != null) {
            previous.close ();
        }
    }

    public void set_log_async_severity_mask (LogSeverityMask mask) throws Error {
        check_status (Raw.log_set_async_severity_mask ((uint32) mask));
    }

    private class MapRegistration {
        public weak MapHandle? map;

        public MapRegistration (MapHandle map) {
            this.map = map;
        }
    }

    private class OfflineOperationRegistration {
        public uint64 native_id;
        public weak OfflineOperationHandle? handle;

        public OfflineOperationRegistration (uint64 native_id, OfflineOperationHandle handle) {
            this.native_id = native_id;
            this.handle = handle;
        }
    }

    private ResourceTransformRegistration[] leaked_resource_transforms;
    private ResourceProviderRegistration[] leaked_resource_providers;
    private Mutex leaked_callback_roots_mutex;

    private void retain_leaked_runtime_callbacks (
        ResourceTransformRegistration? transform,
        ResourceProviderRegistration? provider) {
        leaked_callback_roots_mutex.lock ();
        if (transform != null) {
            leaked_resource_transforms += transform;
        }
        if (provider != null) {
            leaked_resource_providers += provider;
        }
        leaked_callback_roots_mutex.unlock ();
    }

    internal class RuntimeNativeLease {
        private RuntimeHandle owner;
        public unowned Raw.Runtime native { get; private set; }

        internal RuntimeNativeLease (RuntimeHandle owner, Raw.Runtime native) {
            this.owner = owner;
            this.native = native;
        }

        ~RuntimeNativeLease () {
            owner.release_native_lease ();
        }
    }

    public class WakeSource {
        private Raw.WakeSource? native;
        private Mutex mutex;

        internal WakeSource (owned Raw.WakeSource native) {
            this.native = (owned) native;
        }

        public bool closed {
            get {
                mutex.lock ();
                var value = native == null;
                mutex.unlock ();
                return value;
            }
        }

        public void signal () throws Error {
            mutex.lock ();
            if (native == null) {
                mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("wake source is closed");
            }
            try {
                check_status (Raw.wake_source_signal (native));
            } finally {
                mutex.unlock ();
            }
        }

        public void close () {
            mutex.lock ();
            unowned Raw.WakeSource? closing = native;
            native = null;
            mutex.unlock ();
            if (closing != null) {
                Raw.wake_source_destroy (closing);
            }
        }

        ~WakeSource () {
            close ();
        }
    }

    public class RuntimeHandle {
        private Raw.Runtime? native;
        private ResourceTransformRegistration? resource_transform;
        private ResourceProviderRegistration? resource_provider;
        private MapRegistration[] maps = new MapRegistration[0];
        private OfflineOperationRegistration[] offline_operations = new OfflineOperationRegistration[0];
        private Mutex state_mutex;
        private Cond idle;
        private bool releasing;
        private uint active_native_leases;
        private Mutex registry_mutex;

        public bool closed {
            get {
                state_mutex.lock ();
                var value = native == null;
                state_mutex.unlock ();
                return value;
            }
        }

        public RuntimeHandle (RuntimeOptions? options = null) throws Error {
            var actual_version = Raw.c_version ();
            if (actual_version != EXPECTED_C_ABI_VERSION) {
                clear_unknown_status ();
                throw new Error.ABI_MISMATCH ("MapLibre Native C ABI version mismatch: expected %u, loaded %u", EXPECTED_C_ABI_VERSION, actual_version);
            }
            var options_snapshot = (options ?? new RuntimeOptions ()).copy ();
            var native_options = options_snapshot.to_native ();
            Raw.Runtime created;
            check_status (Raw.runtime_create (&native_options, out created));
            native = (owned) created;
        }

        ~RuntimeHandle () {
            state_mutex.lock ();
            var leaked = native != null;
            state_mutex.unlock ();
            if (leaked) {
                registry_mutex.lock ();
                var transform = resource_transform;
                var provider = resource_provider;
                registry_mutex.unlock ();
                warning ("RuntimeHandle finalized while live; call close() on the owner thread");
                retain_leaked_runtime_callbacks (transform, provider);
            }
        }

        internal RuntimeNativeLease require_live () throws Error {
            state_mutex.lock ();
            if (native == null || releasing) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("runtime handle is closed");
            }
            active_native_leases++;
            var lease = new RuntimeNativeLease (this, native);
            state_mutex.unlock ();
            return lease;
        }

        internal void release_native_lease () {
            state_mutex.lock ();
            active_native_leases--;
            if (releasing && active_native_leases == 0) {
                idle.broadcast ();
            }
            state_mutex.unlock ();
        }

        private void cancel_release () {
            state_mutex.lock ();
            releasing = false;
            idle.broadcast ();
            state_mutex.unlock ();
        }

        public void close () throws Error {
            state_mutex.lock ();
            if (native == null) {
                state_mutex.unlock ();
                return;
            }
            if (releasing) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("runtime handle release is already in progress");
            }
            releasing = true;
            while (active_native_leases > 0) {
                idle.wait (state_mutex);
            }
            unowned Raw.Runtime closing = native;
            state_mutex.unlock ();

            registry_mutex.lock ();
            prune_maps ();
            if (maps.length > 0) {
                registry_mutex.unlock ();
                cancel_release ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("runtime has live map handles");
            }
            prune_offline_operations ();
            if (offline_operations.length > 0) {
                registry_mutex.unlock ();
                cancel_release ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("runtime has live offline operation handles");
            }
            var provider = resource_provider;
            var transform = resource_transform;
            registry_mutex.unlock ();

            if (provider != null) {
                provider.close ();
            }
            if (transform != null) {
                transform.close ();
            }
            try {
                check_status (Raw.runtime_destroy (closing));
            } catch (Error error) {
                if (transform != null) {
                    transform.reopen ();
                }
                if (provider != null) {
                    provider.reopen ();
                }
                cancel_release ();
                throw error;
            }

            state_mutex.lock ();
            native = null;
            releasing = false;
            idle.broadcast ();
            state_mutex.unlock ();

            registry_mutex.lock ();
            resource_transform = null;
            resource_provider = null;
            registry_mutex.unlock ();
        }

        public void pump (int64 timeout_ms = 0) throws Error {
            if (timeout_ms < -1) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("runtime pump timeout must be -1 or non-negative");
            }
            var lease = require_live ();
            check_status (Raw.runtime_pump (lease.native, timeout_ms));
        }

        public WakeSource acquire_wake_source () throws Error {
            var lease = require_live ();
            Raw.WakeSource source;
            check_status (Raw.runtime_wake_source_acquire (lease.native, out source));
            return new WakeSource ((owned) source);
        }

        public RuntimeEvent? poll_event () throws Error {
            var lease = require_live ();
            Raw.RuntimeEvent raw_event = {};
            raw_event.size = (uint32) sizeof (Raw.RuntimeEvent);
            bool has_event;
            check_status (Raw.runtime_poll_event (lease.native, &raw_event, out has_event));
            if (!has_event) {
                return null;
            }
            return new RuntimeEvent (this, raw_event);
        }

        internal void register_map (MapHandle map) {
            registry_mutex.lock ();
            prune_maps ();
            var retained = new MapRegistration[maps.length + 1];
            for (var index = 0; index < maps.length; index++) {
                retained[index] = maps[index];
            }
            retained[maps.length] = new MapRegistration (map);
            maps = retained;
            registry_mutex.unlock ();
        }

        internal void unregister_map (MapHandle map) {
            registry_mutex.lock ();
            uint retained_count = 0;
            for (var index = 0; index < maps.length; index++) {
                if (maps[index].map != null && maps[index].map != map) {
                    retained_count++;
                }
            }
            var retained = new MapRegistration[retained_count];
            uint output_index = 0;
            for (var index = 0; index < maps.length; index++) {
                if (maps[index].map != null && maps[index].map != map) {
                    retained[output_index++] = maps[index];
                }
            }
            maps = retained;
            registry_mutex.unlock ();
        }

        internal MapHandle? resolve_map (void* source) {
            registry_mutex.lock ();
            prune_maps ();
            for (var index = 0; index < maps.length; index++) {
                var map = maps[index].map;
                if (map != null && map.matches_native_source (source)) {
                    registry_mutex.unlock ();
                    return map;
                }
            }
            registry_mutex.unlock ();
            return null;
        }

        private void prune_maps () {
            uint retained_count = 0;
            for (var index = 0; index < maps.length; index++) {
                if (maps[index].map != null) {
                    retained_count++;
                }
            }
            if (retained_count == maps.length) {
                return;
            }
            var retained = new MapRegistration[retained_count];
            uint output_index = 0;
            for (var index = 0; index < maps.length; index++) {
                if (maps[index].map != null) {
                    retained[output_index++] = maps[index];
                }
            }
            maps = retained;
        }

        private OfflineOperationHandle register_offline_operation (uint64 native_id, OfflineOperationKind operation_kind, OfflineOperationResultKind result_kind) {
            registry_mutex.lock ();
            prune_offline_operations ();
            var handle = new OfflineOperationHandle (this, native_id, operation_kind, result_kind);
            var retained = new OfflineOperationRegistration[offline_operations.length + 1];
            for (var index = 0; index < offline_operations.length; index++) {
                retained[index] = offline_operations[index];
            }
            retained[offline_operations.length] = new OfflineOperationRegistration (native_id, handle);
            offline_operations = retained;
            registry_mutex.unlock ();
            return handle;
        }

        private void unregister_offline_operation (uint64 native_id) {
            registry_mutex.lock ();
            uint retained_count = 0;
            for (var index = 0; index < offline_operations.length; index++) {
                if (offline_operations[index].native_id != native_id) {
                    retained_count++;
                }
            }
            var retained = new OfflineOperationRegistration[retained_count];
            uint output_index = 0;
            for (var index = 0; index < offline_operations.length; index++) {
                if (offline_operations[index].native_id != native_id) {
                    retained[output_index++] = offline_operations[index];
                }
            }
            offline_operations = retained;
            registry_mutex.unlock ();
        }

        internal OfflineOperationHandle? resolve_offline_operation (uint64 native_id) {
            registry_mutex.lock ();
            prune_offline_operations ();
            for (var index = 0; index < offline_operations.length; index++) {
                if (offline_operations[index].native_id == native_id) {
                    var handle = offline_operations[index].handle;
                    registry_mutex.unlock ();
                    return handle;
                }
            }
            registry_mutex.unlock ();
            return null;
        }

        private void prune_offline_operations () {
            uint retained_count = 0;
            for (var index = 0; index < offline_operations.length; index++) {
                if (offline_operations[index].handle != null) {
                    retained_count++;
                }
            }
            if (retained_count == offline_operations.length) {
                return;
            }
            var retained = new OfflineOperationRegistration[retained_count];
            uint output_index = 0;
            for (var index = 0; index < offline_operations.length; index++) {
                if (offline_operations[index].handle != null) {
                    retained[output_index++] = offline_operations[index];
                }
            }
            offline_operations = retained;
        }

        public void set_resource_provider (owned ResourceProviderCallback callback) throws Error {
            var lease = require_live ();
            ResourceProviderRegistration? previous = null;
            registry_mutex.lock ();
            try {
                var registration = new ResourceProviderRegistration ((owned) callback);
                Raw.ResourceProvider provider = {};
                provider.size = (uint32) sizeof (Raw.ResourceProvider);
                provider.callback = resource_provider_trampoline;
                provider.user_data = registration;
                check_status (Raw.runtime_set_resource_provider (lease.native, &provider));
                previous = resource_provider;
                resource_provider = registration;
            } finally {
                registry_mutex.unlock ();
            }
            if (previous != null) {
                previous.close ();
            }
        }

        public void clear_resource_provider () throws Error {
            var lease = require_live ();
            ResourceProviderRegistration? previous = null;
            registry_mutex.lock ();
            try {
                check_status (Raw.runtime_clear_resource_provider (lease.native));
                previous = resource_provider;
                resource_provider = null;
            } finally {
                registry_mutex.unlock ();
            }
            if (previous != null) {
                previous.close ();
            }
        }

        public void set_resource_transform (owned ResourceTransformCallback callback) throws Error {
            var lease = require_live ();
            ResourceTransformRegistration? previous = null;
            registry_mutex.lock ();
            try {
                var registration = new ResourceTransformRegistration ((owned) callback);
                Raw.ResourceTransform transform = {};
                transform.size = (uint32) sizeof (Raw.ResourceTransform);
                transform.callback = resource_transform_trampoline;
                transform.user_data = registration;
                check_status (Raw.runtime_set_resource_transform (lease.native, &transform));
                previous = resource_transform;
                resource_transform = registration;
            } finally {
                registry_mutex.unlock ();
            }
            if (previous != null) {
                previous.close ();
            }
        }

        public void clear_resource_transform () throws Error {
            var lease = require_live ();
            ResourceTransformRegistration? previous = null;
            registry_mutex.lock ();
            try {
                check_status (Raw.runtime_clear_resource_transform (lease.native));
                previous = resource_transform;
                resource_transform = null;
            } finally {
                registry_mutex.unlock ();
            }
            if (previous != null) {
                previous.close ();
            }
        }

        public OfflineOperationHandle run_ambient_cache_operation_start (AmbientCacheOperation operation) throws Error {
            var lease = require_live ();
            uint64 operation_id;
            check_status (Raw.runtime_run_ambient_cache_operation_start (lease.native, (uint32) operation, out operation_id));
            return register_offline_operation (operation_id, OfflineOperationKind.AMBIENT_CACHE, OfflineOperationResultKind.NONE);
        }

        public void discard_offline_operation (OfflineOperationHandle operation) throws Error {
            var lease = require_live ();
            var operation_id = operation.begin_consume (this);
            try {
                check_status (Raw.runtime_offline_operation_discard (lease.native, operation_id));
            } catch (Error error) {
                operation.cancel_consume ();
                throw error;
            }
            operation.finish_consume ();
            unregister_offline_operation (operation_id);
        }

        public OfflineOperationHandle offline_region_create_start (OfflineRegionDefinition definition, uint8[]? metadata = null) throws Error {
            var lease = require_live ();
            Raw.Geometry geometry_storage = {};
            Geometry? geometry_owner;
            var native_definition = definition.to_native (ref geometry_storage, out geometry_owner);
            var metadata_snapshot = metadata == null ? new uint8[0] : copy_byte_array (metadata);
            uint8* metadata_data = metadata_snapshot.length == 0 ? null : metadata_snapshot;
            size_t metadata_size = metadata_snapshot.length;
            uint64 operation_id;
            check_status (Raw.runtime_offline_region_create_start (lease.native, &native_definition, metadata_data, metadata_size, out operation_id));
            return register_offline_operation (operation_id, OfflineOperationKind.REGION_CREATE, OfflineOperationResultKind.REGION);
        }

        public OfflineOperationHandle offline_region_get_start (OfflineRegionId region_id) throws Error {
            var lease = require_live ();
            uint64 operation_id;
            check_status (Raw.runtime_offline_region_get_start (lease.native, region_id.to_native (), out operation_id));
            return register_offline_operation (operation_id, OfflineOperationKind.REGION_GET, OfflineOperationResultKind.OPTIONAL_REGION);
        }

        public OfflineOperationHandle offline_regions_list_start () throws Error {
            var lease = require_live ();
            uint64 operation_id;
            check_status (Raw.runtime_offline_regions_list_start (lease.native, out operation_id));
            return register_offline_operation (operation_id, OfflineOperationKind.REGIONS_LIST, OfflineOperationResultKind.REGION_LIST);
        }

        public OfflineOperationHandle offline_regions_merge_database_start (string side_database_path) throws Error {
            var lease = require_live ();
            uint64 operation_id;
            check_status (Raw.runtime_offline_regions_merge_database_start (lease.native, c_string (side_database_path), out operation_id));
            return register_offline_operation (operation_id, OfflineOperationKind.REGIONS_MERGE_DATABASE, OfflineOperationResultKind.REGION_LIST);
        }

        public OfflineOperationHandle offline_region_update_metadata_start (OfflineRegionId region_id, uint8[]? metadata = null) throws Error {
            var lease = require_live ();
            var metadata_snapshot = metadata == null ? new uint8[0] : copy_byte_array (metadata);
            uint8* metadata_data = metadata_snapshot.length == 0 ? null : metadata_snapshot;
            size_t metadata_size = metadata_snapshot.length;
            uint64 operation_id;
            check_status (Raw.runtime_offline_region_update_metadata_start (lease.native, region_id.to_native (), metadata_data, metadata_size, out operation_id));
            return register_offline_operation (operation_id, OfflineOperationKind.REGION_UPDATE_METADATA, OfflineOperationResultKind.REGION);
        }

        public OfflineOperationHandle offline_region_get_status_start (OfflineRegionId region_id) throws Error {
            var lease = require_live ();
            uint64 operation_id;
            check_status (Raw.runtime_offline_region_get_status_start (lease.native, region_id.to_native (), out operation_id));
            return register_offline_operation (operation_id, OfflineOperationKind.REGION_GET_STATUS, OfflineOperationResultKind.REGION_STATUS);
        }

        public OfflineOperationHandle offline_region_set_observed_start (OfflineRegionId region_id, bool observed) throws Error {
            var lease = require_live ();
            uint64 operation_id;
            check_status (Raw.runtime_offline_region_set_observed_start (lease.native, region_id.to_native (), observed, out operation_id));
            return register_offline_operation (operation_id, OfflineOperationKind.REGION_SET_OBSERVED, OfflineOperationResultKind.NONE);
        }

        public OfflineOperationHandle offline_region_set_download_state_start (OfflineRegionId region_id, OfflineRegionDownloadState state) throws Error {
            var lease = require_live ();
            uint64 operation_id;
            check_status (Raw.runtime_offline_region_set_download_state_start (lease.native, region_id.to_native (), (uint32) state, out operation_id));
            return register_offline_operation (operation_id, OfflineOperationKind.REGION_SET_DOWNLOAD_STATE, OfflineOperationResultKind.NONE);
        }

        public OfflineOperationHandle offline_region_invalidate_start (OfflineRegionId region_id) throws Error {
            var lease = require_live ();
            uint64 operation_id;
            check_status (Raw.runtime_offline_region_invalidate_start (lease.native, region_id.to_native (), out operation_id));
            return register_offline_operation (operation_id, OfflineOperationKind.REGION_INVALIDATE, OfflineOperationResultKind.NONE);
        }

        public OfflineOperationHandle offline_region_delete_start (OfflineRegionId region_id) throws Error {
            var lease = require_live ();
            uint64 operation_id;
            check_status (Raw.runtime_offline_region_delete_start (lease.native, region_id.to_native (), out operation_id));
            return register_offline_operation (operation_id, OfflineOperationKind.REGION_DELETE, OfflineOperationResultKind.NONE);
        }

        public OfflineRegionInfo offline_region_create_take_result (OfflineOperationHandle operation) throws Error {
            var lease = require_live ();
            var operation_id = operation.begin_result (this, OfflineOperationKind.REGION_CREATE, OfflineOperationResultKind.REGION);
            Raw.OfflineRegionSnapshot? snapshot;
            try {
                check_status (Raw.runtime_offline_region_create_take_result (lease.native, operation_id, out snapshot));
            } catch (Error error) {
                operation.cancel_consume ();
                throw error;
            }
            operation.finish_consume ();
            unregister_offline_operation (operation_id);
            if (snapshot == null) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("offline region create returned no snapshot");
            }
            return copy_offline_region_snapshot ((owned) snapshot);
        }

        public OfflineRegionInfo? offline_region_get_take_result (OfflineOperationHandle operation) throws Error {
            var lease = require_live ();
            var operation_id = operation.begin_result (this, OfflineOperationKind.REGION_GET, OfflineOperationResultKind.OPTIONAL_REGION);
            Raw.OfflineRegionSnapshot? snapshot;
            bool found;
            try {
                check_status (Raw.runtime_offline_region_get_take_result (lease.native, operation_id, out snapshot, out found));
            } catch (Error error) {
                operation.cancel_consume ();
                throw error;
            }
            operation.finish_consume ();
            unregister_offline_operation (operation_id);
            if (!found || snapshot == null) {
                return null;
            }
            return copy_offline_region_snapshot ((owned) snapshot);
        }

        public OfflineRegionInfo[] offline_regions_list_take_result (OfflineOperationHandle operation) throws Error {
            var lease = require_live ();
            var operation_id = operation.begin_result (this, OfflineOperationKind.REGIONS_LIST, OfflineOperationResultKind.REGION_LIST);
            Raw.OfflineRegionList? list;
            try {
                check_status (Raw.runtime_offline_regions_list_take_result (lease.native, operation_id, out list));
            } catch (Error error) {
                operation.cancel_consume ();
                throw error;
            }
            operation.finish_consume ();
            unregister_offline_operation (operation_id);
            if (list == null) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("offline regions list returned no list");
            }
            return copy_offline_region_list ((owned) list);
        }

        public OfflineRegionInfo[] offline_regions_merge_database_take_result (OfflineOperationHandle operation) throws Error {
            var lease = require_live ();
            var operation_id = operation.begin_result (this, OfflineOperationKind.REGIONS_MERGE_DATABASE, OfflineOperationResultKind.REGION_LIST);
            Raw.OfflineRegionList? list;
            try {
                check_status (Raw.runtime_offline_regions_merge_database_take_result (lease.native, operation_id, out list));
            } catch (Error error) {
                operation.cancel_consume ();
                throw error;
            }
            operation.finish_consume ();
            unregister_offline_operation (operation_id);
            if (list == null) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("offline regions merge returned no list");
            }
            return copy_offline_region_list ((owned) list);
        }

        public OfflineRegionInfo offline_region_update_metadata_take_result (OfflineOperationHandle operation) throws Error {
            var lease = require_live ();
            var operation_id = operation.begin_result (this, OfflineOperationKind.REGION_UPDATE_METADATA, OfflineOperationResultKind.REGION);
            Raw.OfflineRegionSnapshot? snapshot;
            try {
                check_status (Raw.runtime_offline_region_update_metadata_take_result (lease.native, operation_id, out snapshot));
            } catch (Error error) {
                operation.cancel_consume ();
                throw error;
            }
            operation.finish_consume ();
            unregister_offline_operation (operation_id);
            if (snapshot == null) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("offline region metadata update returned no snapshot");
            }
            return copy_offline_region_snapshot ((owned) snapshot);
        }

        public OfflineRegionStatus offline_region_get_status_take_result (OfflineOperationHandle operation) throws Error {
            var lease = require_live ();
            var operation_id = operation.begin_result (this, OfflineOperationKind.REGION_GET_STATUS, OfflineOperationResultKind.REGION_STATUS);
            Raw.OfflineRegionStatus status = {};
            status.size = (uint32) sizeof (Raw.OfflineRegionStatus);
            try {
                check_status (Raw.runtime_offline_region_get_status_take_result (lease.native, operation_id, &status));
            } catch (Error error) {
                operation.cancel_consume ();
                throw error;
            }
            operation.finish_consume ();
            unregister_offline_operation (operation_id);
            return OfflineRegionStatus.from_native (status);
        }
    }

}
