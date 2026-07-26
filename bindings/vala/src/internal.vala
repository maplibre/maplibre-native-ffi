namespace MaplibreNative {
    private static Private? unknown_status_data;
    private static Mutex unknown_status_init_mutex;

    private static unowned Private unknown_status_store () {
        unknown_status_init_mutex.lock ();
        if (unknown_status_data == null) {
            unknown_status_data = new Private (GLib.free);
        }
        unowned Private store = unknown_status_data;
        unknown_status_init_mutex.unlock ();
        return store;
    }

    public int32? thread_last_unknown_status () {
        int32* value = (int32*) unknown_status_store ().get ();
        if (value == null) {
            return null;
        }
        return value[0];
    }

    internal void check_status (Raw.Status status) throws Error {
        unknown_status_store ().replace (null);
        if (status == Raw.Status.OK) {
            return;
        }

        var message = Raw.thread_last_error_message ();
        if (message == null || message.length == 0) {
            message = "MapLibre Native operation failed";
        }

        switch (status) {
            case Raw.Status.INVALID_ARGUMENT:
                throw new Error.INVALID_ARGUMENT ("%s", message);
            case Raw.Status.INVALID_STATE:
                throw new Error.INVALID_STATE ("%s", message);
            case Raw.Status.WRONG_THREAD:
                throw new Error.WRONG_THREAD ("%s", message);
            case Raw.Status.UNSUPPORTED:
                throw new Error.UNSUPPORTED ("%s", message);
            case Raw.Status.NATIVE_ERROR:
                throw new Error.NATIVE_ERROR ("%s", message);
            default:
                int32* raw_status = (int32*) GLib.malloc (sizeof (int32));
                raw_status[0] = (int32) status;
                unknown_status_store ().replace ((owned) raw_status);
                throw new Error.UNKNOWN_STATUS ("unknown native status %d: %s", (int32) status, message);
        }
    }

    internal string copy_c_string (char* value) {
        if (value == null) {
            return "";
        }
        return (string) value;
    }

    internal string copy_c_string_bytes (char* value, size_t size) throws Error {
        if (value == null || size == 0) {
            return "";
        }
        return copy_utf8_bytes ((uint8*) value, size);
    }

    internal uint8[] copy_string_bytes (string value) {
        uint8[] copied = new uint8[value.length];
        for (var index = 0; index < value.length; index++) {
            copied[index] = value[index];
        }
        return copied;
    }

    internal uint8[] copy_byte_array (uint8[] value) {
        uint8[] copied = new uint8[value.length];
        for (var index = 0; index < value.length; index++) {
            copied[index] = value[index];
        }
        return copied;
    }

    internal Raw.StringView byte_string_view (uint8[] value) {
        return Raw.StringView () {
            data = value.length == 0 ? null : (char*) value,
            size = value.length
        };
    }

    internal uint8[] copy_string_view_bytes (Raw.StringView view) throws Error {
        if (view.data == null && view.size == 0) {
            return new uint8[0];
        }
        if (view.data == null) {
            throw new Error.INVALID_ARGUMENT ("string view data is null");
        }
        return copy_bytes ((uint8*) view.data, view.size) ?? new uint8[0];
    }

    internal string string_from_bytes (uint8[] value) throws Error {
        for (var index = 0; index < value.length; index++) {
            if (value[index] == 0) {
                throw new Error.INVALID_STATE ("string contains embedded NUL; use the byte accessor");
            }
        }
        return copy_utf8_bytes (value, value.length);
    }

    internal void reject_embedded_nul (string value) throws Error {
        for (var index = 0; index < value.length; index++) {
            if (value[index] == '\0') {
                throw new Error.INVALID_ARGUMENT ("string inputs must not contain embedded NUL bytes");
            }
        }
    }

    internal void reject_optional_embedded_nul (string? value) throws Error {
        if (value != null) {
            reject_embedded_nul (value);
        }
    }

    internal unowned string c_string (string value) throws Error {
        reject_embedded_nul (value);
        return value;
    }

    internal unowned string? optional_c_string (string? value) throws Error {
        reject_optional_embedded_nul (value);
        return value;
    }

    internal string copy_utf8_bytes (uint8* data, size_t size) throws Error {
        if (data == null || size == 0) {
            return "";
        }
        uint8[] copied = new uint8[size + 1];
        for (size_t index = 0; index < size; index++) {
            copied[index] = data[index];
        }
        copied[size] = 0;
        return ((string) copied).dup ();
    }

    internal uint8[]? copy_bytes (uint8* data, size_t size) {
        if (data == null) {
            return null;
        }
        uint8[] copied = new uint8[size];
        for (size_t index = 0; index < size; index++) {
            copied[index] = data[index];
        }
        return copied;
    }

    internal StringList copy_style_id_list (owned Raw.StyleIdList list) throws Error {
        try {
            size_t count;
            check_status (Raw.style_id_list_count (list, out count));
            Utf8String[] values = new Utf8String[count];
            for (size_t index = 0; index < count; index++) {
                Raw.StringView item;
                check_status (Raw.style_id_list_get (list, index, out item));
                values[index] = new Utf8String.from_bytes (copy_string_view_bytes (item));
            }
            return new StringList ((owned) values);
        } finally {
            Raw.style_id_list_destroy (list);
        }
    }

    internal JsonValue? copy_json_snapshot (owned Raw.JsonSnapshot? snapshot) throws Error {
        if (snapshot == null) {
            return null;
        }
        try {
            Raw.JsonValue* value;
            check_status (Raw.json_snapshot_get (snapshot, out value));
            if (value == null) {
                return null;
            }
            return JsonValue.from_native (value[0]);
        } finally {
            Raw.json_snapshot_destroy (snapshot);
        }
    }

    internal NetworkStatus network_status_from_raw (uint32 raw_status) {
        return (NetworkStatus) raw_status;
    }

    internal RuntimeEventType runtime_event_type_from_raw (uint32 raw_type) {
        return (RuntimeEventType) raw_type;
    }

    internal RuntimeEventSourceType runtime_event_source_type_from_raw (uint32 raw_type) {
        return (RuntimeEventSourceType) raw_type;
    }

    internal LogSeverity log_severity_from_raw (uint32 raw_severity) {
        return (LogSeverity) raw_severity;
    }

    internal StyleSourceType style_source_type_from_raw (uint32 raw_type) {
        return (StyleSourceType) raw_type;
    }

    internal ResourceKind resource_kind_from_raw (uint32 raw_kind) {
        return (ResourceKind) raw_kind;
    }

    internal ResourceLoadingMethod resource_loading_method_from_raw (uint32 raw_method) {
        return (ResourceLoadingMethod) raw_method;
    }

    internal ResourcePriority resource_priority_from_raw (uint32 raw_priority) {
        return (ResourcePriority) raw_priority;
    }

    internal ResourceUsage resource_usage_from_raw (uint32 raw_usage) {
        return (ResourceUsage) raw_usage;
    }

    internal ResourceStoragePolicy resource_storage_policy_from_raw (uint32 raw_policy) {
        return (ResourceStoragePolicy) raw_policy;
    }

    internal ResourceErrorReason resource_error_reason_from_raw (uint32 raw_reason) {
        return (ResourceErrorReason) raw_reason;
    }

    internal RuntimeEventPayloadType runtime_event_payload_type_from_raw (uint32 raw_type) {
        return (RuntimeEventPayloadType) raw_type;
    }

    internal RenderMode render_mode_from_raw (uint32 raw_mode) {
        return (RenderMode) raw_mode;
    }

    internal TileOperation tile_operation_from_raw (uint32 raw_operation) {
        return (TileOperation) raw_operation;
    }

    internal OfflineRegionDownloadState offline_region_download_state_from_raw (uint32 raw_state) {
        return (OfflineRegionDownloadState) raw_state;
    }

    internal OfflineOperationKind offline_operation_kind_from_raw (uint32 raw_kind) {
        return (OfflineOperationKind) raw_kind;
    }

    internal OfflineOperationResultKind offline_operation_result_kind_from_raw (uint32 raw_kind) {
        return (OfflineOperationResultKind) raw_kind;
    }

    internal LogEvent log_event_from_raw (uint32 raw_event) {
        return (LogEvent) raw_event;
    }
}
