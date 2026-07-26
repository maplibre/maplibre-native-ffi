namespace MaplibreNative {
    public enum FeatureExtensionResultType {
        VALUE = 1,
        FEATURE_COLLECTION = 2
    }

    public class FeatureExtensionResult {
        public FeatureExtensionResultType result_type { get; private set; }
        public JsonValue? value { get; private set; }
        public FeatureCollection? feature_collection { get; private set; }

        private FeatureExtensionResult (FeatureExtensionResultType result_type, JsonValue? value, FeatureCollection? feature_collection) {
            this.result_type = result_type;
            this.value = value;
            this.feature_collection = feature_collection;
        }

        internal static FeatureExtensionResult from_native (Raw.FeatureExtensionResultInfo native) throws Error {
            switch ((FeatureExtensionResultType) native.type) {
            case FeatureExtensionResultType.VALUE:
                if (native.value == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("feature extension value is null");
                }
                return new FeatureExtensionResult (FeatureExtensionResultType.VALUE, JsonValue.from_native (native.value[0]), null);
            case FeatureExtensionResultType.FEATURE_COLLECTION:
                return new FeatureExtensionResult (FeatureExtensionResultType.FEATURE_COLLECTION, null, FeatureCollection.from_native (native.feature_collection));
            default:
                clear_unknown_status ();
                throw new Error.UNSUPPORTED ("unknown feature extension result type %u", native.type);
            }
        }

        public FeatureExtensionResult copy () throws Error {
            return new FeatureExtensionResult (
                result_type,
                value == null ? null : value.copy (),
                feature_collection == null ? null : feature_collection.copy ()
            );
        }

        public bool equal (FeatureExtensionResult other) {
            return result_type == other.result_type
                && (value == null
                    ? other.value == null
                    : other.value != null && value.equal (other.value))
                && (feature_collection == null
                    ? other.feature_collection == null
                    : other.feature_collection != null && feature_collection.equal (other.feature_collection));
        }
    }

    internal class FeatureExtensionResultHandle {
        private Raw.FeatureExtensionResult? native;

        public bool closed { get { return native == null; } }

        internal FeatureExtensionResultHandle (owned Raw.FeatureExtensionResult native) {
            this.native = (owned) native;
        }

        ~FeatureExtensionResultHandle () {
            if (native != null) {
                Raw.feature_extension_result_destroy (native);
                native = null;
            }
        }

        private unowned Raw.FeatureExtensionResult require_live () throws Error {
            if (native == null) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("feature extension result handle is closed");
            }
            return native;
        }

        public FeatureExtensionResult get () throws Error {
            Raw.FeatureExtensionResultInfo info = {};
            info.size = (uint32) sizeof (Raw.FeatureExtensionResultInfo);
            check_status (Raw.feature_extension_result_get (require_live (), &info));
            return FeatureExtensionResult.from_native (info);
        }

        internal static FeatureExtensionResult copy_from_native (owned Raw.FeatureExtensionResult native) throws Error {
            var handle = new FeatureExtensionResultHandle ((owned) native);
            try {
                return handle.get ();
            } finally {
                handle.close ();
            }
        }

        public void close () {
            if (native == null) {
                return;
            }
            unowned Raw.FeatureExtensionResult closing = native;
            Raw.feature_extension_result_destroy (closing);
            native = null;
        }
    }

    public class RenderedQueryGeometry {
        internal Raw.RenderedQueryGeometry native;
        private Raw.ScreenPoint[] points;

        private RenderedQueryGeometry (Raw.RenderedQueryGeometry native, owned Raw.ScreenPoint[]? points = null) {
            this.native = native;
            this.points = (owned) points;
        }

        internal Raw.RenderedQueryGeometry to_native () {
            if (points != null && native.type == (uint32) RenderedQueryGeometryType.LINE_STRING) {
                return Raw.rendered_query_geometry_line_string (points, points.length);
            }
            return native;
        }

        public static RenderedQueryGeometry point (ScreenPoint point) {
            return new RenderedQueryGeometry (Raw.rendered_query_geometry_point (point.to_native ()));
        }

        public static RenderedQueryGeometry box (ScreenBox box) {
            return new RenderedQueryGeometry (Raw.rendered_query_geometry_box (box.to_native ()));
        }

        public static RenderedQueryGeometry line_string (ScreenPoint[] points) throws Error {
            if (points.length == 0) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("rendered query line string is empty");
            }
            Raw.ScreenPoint[] native_points = new Raw.ScreenPoint[points.length];
            for (var i = 0; i < points.length; i++) {
                native_points[i] = points[i].to_native ();
            }
            return new RenderedQueryGeometry (Raw.rendered_query_geometry_line_string (native_points, native_points.length), (owned) native_points);
        }

        public RenderedQueryGeometry copy () {
            Raw.ScreenPoint[]? copied_points = null;
            if (points != null) {
                copied_points = new Raw.ScreenPoint[points.length];
                for (var index = 0; index < points.length; index++) {
                    copied_points[index] = points[index];
                }
            }
            return new RenderedQueryGeometry (native, (owned) copied_points);
        }

        public bool equal (RenderedQueryGeometry other) {
            if (native.type != other.native.type) {
                return false;
            }
            switch ((RenderedQueryGeometryType) native.type) {
            case RenderedQueryGeometryType.POINT:
                return native.point.x == other.native.point.x
                    && native.point.y == other.native.point.y;
            case RenderedQueryGeometryType.BOX:
                return native.box.min.x == other.native.box.min.x
                    && native.box.min.y == other.native.box.min.y
                    && native.box.max.x == other.native.box.max.x
                    && native.box.max.y == other.native.box.max.y;
            case RenderedQueryGeometryType.LINE_STRING:
                if (points.length != other.points.length) {
                    return false;
                }
                for (var index = 0; index < points.length; index++) {
                    if (points[index].x != other.points[index].x
                        || points[index].y != other.points[index].y) {
                        return false;
                    }
                }
                return true;
            default:
                return false;
            }
        }
    }

    public class RenderedFeatureQueryOptions {
        private Utf8String[] layer_ids;
        private bool has_layer_ids;
        private JsonValue? filter;
        private Raw.StringView[] layer_id_views;
        private Raw.JsonValue filter_native;

        public RenderedFeatureQueryOptions () {
            layer_ids = new Utf8String[0];
        }

        public void set_layer_ids (string[] layer_ids) throws Error {
            var copied = new Utf8String[layer_ids.length];
            for (var index = 0; index < layer_ids.length; index++) {
                if (layer_ids[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("layer ID is null");
                }
                copied[index] = new Utf8String (layer_ids[index]);
            }
            this.layer_ids = copied;
            has_layer_ids = true;
        }

        public void set_layer_ids_utf8 (Utf8String[] layer_ids) throws Error {
            var copied = new Utf8String[layer_ids.length];
            for (var index = 0; index < layer_ids.length; index++) {
                if (layer_ids[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("layer ID is null");
                }
                copied[index] = layer_ids[index].copy ();
            }
            this.layer_ids = copied;
            has_layer_ids = true;
        }

        public void set_filter (JsonValue filter) {
            this.filter = filter;
        }

        public RenderedFeatureQueryOptions copy () {
            var copied = new RenderedFeatureQueryOptions ();
            copied.has_layer_ids = has_layer_ids;
            copied.layer_ids = new Utf8String[layer_ids.length];
            for (var index = 0; index < layer_ids.length; index++) {
                copied.layer_ids[index] = layer_ids[index].copy ();
            }
            copied.filter = filter == null ? null : filter.copy ();
            return copied;
        }

        public bool equal (RenderedFeatureQueryOptions other) {
            if (has_layer_ids != other.has_layer_ids || layer_ids.length != other.layer_ids.length) {
                return false;
            }
            for (var index = 0; index < layer_ids.length; index++) {
                if (!layer_ids[index].equal (other.layer_ids[index])) {
                    return false;
                }
            }
            return filter == null
                ? other.filter == null
                : other.filter != null && filter.equal (other.filter);
        }

        internal Raw.RenderedFeatureQueryOptions to_native () throws Error {
            Raw.RenderedFeatureQueryOptions native = Raw.rendered_feature_query_options_default ();
            if (has_layer_ids) {
                layer_id_views = new Raw.StringView[layer_ids.length];
                for (var i = 0; i < layer_ids.length; i++) {
                    layer_id_views[i] = layer_ids[i].to_native ();
                }
                native.fields |= (uint32) Raw.RenderedFeatureQueryOptionField.LAYER_IDS;
                native.layer_ids = layer_id_views;
                native.layer_id_count = layer_id_views.length;
            }
            if (filter != null) {
                filter_native = filter.to_native ();
                native.filter = &filter_native;
            }
            return native;
        }
    }

    public class SourceFeatureQueryOptions {
        private Utf8String[] source_layer_ids;
        private bool has_source_layer_ids;
        private JsonValue? filter;
        private Raw.StringView[] source_layer_id_views;
        private Raw.JsonValue filter_native;

        public SourceFeatureQueryOptions () {
            source_layer_ids = new Utf8String[0];
        }

        public void set_source_layer_ids (string[] source_layer_ids) throws Error {
            var copied = new Utf8String[source_layer_ids.length];
            for (var index = 0; index < source_layer_ids.length; index++) {
                if (source_layer_ids[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("source layer ID is null");
                }
                copied[index] = new Utf8String (source_layer_ids[index]);
            }
            this.source_layer_ids = copied;
            has_source_layer_ids = true;
        }

        public void set_source_layer_ids_utf8 (Utf8String[] source_layer_ids) throws Error {
            var copied = new Utf8String[source_layer_ids.length];
            for (var index = 0; index < source_layer_ids.length; index++) {
                if (source_layer_ids[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("source layer ID is null");
                }
                copied[index] = source_layer_ids[index].copy ();
            }
            this.source_layer_ids = copied;
            has_source_layer_ids = true;
        }

        public void set_filter (JsonValue filter) {
            this.filter = filter;
        }

        public SourceFeatureQueryOptions copy () {
            var copied = new SourceFeatureQueryOptions ();
            copied.has_source_layer_ids = has_source_layer_ids;
            copied.source_layer_ids = new Utf8String[source_layer_ids.length];
            for (var index = 0; index < source_layer_ids.length; index++) {
                copied.source_layer_ids[index] = source_layer_ids[index].copy ();
            }
            copied.filter = filter == null ? null : filter.copy ();
            return copied;
        }

        public bool equal (SourceFeatureQueryOptions other) {
            if (has_source_layer_ids != other.has_source_layer_ids
                || source_layer_ids.length != other.source_layer_ids.length) {
                return false;
            }
            for (var index = 0; index < source_layer_ids.length; index++) {
                if (!source_layer_ids[index].equal (other.source_layer_ids[index])) {
                    return false;
                }
            }
            return filter == null
                ? other.filter == null
                : other.filter != null && filter.equal (other.filter);
        }

        internal Raw.SourceFeatureQueryOptions to_native () throws Error {
            Raw.SourceFeatureQueryOptions native = Raw.source_feature_query_options_default ();
            if (has_source_layer_ids) {
                source_layer_id_views = new Raw.StringView[source_layer_ids.length];
                for (var i = 0; i < source_layer_ids.length; i++) {
                    source_layer_id_views[i] = source_layer_ids[i].to_native ();
                }
                native.fields |= (uint32) Raw.SourceFeatureQueryOptionField.SOURCE_LAYER_IDS;
                native.source_layer_ids = source_layer_id_views;
                native.source_layer_id_count = source_layer_id_views.length;
            }
            if (filter != null) {
                filter_native = filter.to_native ();
                native.filter = &filter_native;
            }
            return native;
        }
    }

    public class QueriedFeature {
        private Utf8String? source_id_storage;
        private Utf8String? source_layer_id_storage;

        public Feature feature { get; private set; }
        public string? source_id {
            owned get {
                return source_id_storage == null ? null : source_id_storage.to_string_or_null ();
            }
        }
        public string? source_layer_id {
            owned get {
                return source_layer_id_storage == null ? null : source_layer_id_storage.to_string_or_null ();
            }
        }
        public JsonValue? state { get; private set; }

        private QueriedFeature (Feature feature, Utf8String? source_id, Utf8String? source_layer_id, JsonValue? state) {
            this.feature = feature;
            source_id_storage = source_id;
            source_layer_id_storage = source_layer_id;
            this.state = state;
        }

        internal static QueriedFeature from_native (Raw.QueriedFeature native) throws Error {
            Utf8String? source_id = null;
            Utf8String? source_layer_id = null;
            JsonValue? state = null;
            if ((native.fields & (uint32) Raw.QueriedFeatureField.SOURCE_ID) != 0) {
                source_id = new Utf8String.from_bytes (copy_string_view_bytes (native.source_id));
            }
            if ((native.fields & (uint32) Raw.QueriedFeatureField.SOURCE_LAYER_ID) != 0) {
                source_layer_id = new Utf8String.from_bytes (copy_string_view_bytes (native.source_layer_id));
            }
            if ((native.fields & (uint32) Raw.QueriedFeatureField.STATE) != 0 && native.state != null) {
                state = JsonValue.from_native (native.state[0]);
            }
            return new QueriedFeature (Feature.from_native (native.feature), source_id, source_layer_id, state);
        }

        public Utf8String? get_source_id_utf8 () {
            return source_id_storage == null ? null : source_id_storage.copy ();
        }

        public Utf8String? get_source_layer_id_utf8 () {
            return source_layer_id_storage == null ? null : source_layer_id_storage.copy ();
        }

        public QueriedFeature copy () throws Error {
            return new QueriedFeature (
                feature.copy (),
                source_id_storage == null ? null : source_id_storage.copy (),
                source_layer_id_storage == null ? null : source_layer_id_storage.copy (),
                state == null ? null : state.copy ()
            );
        }

        public bool equal (QueriedFeature other) {
            return feature.equal (other.feature)
                && (source_id_storage == null
                    ? other.source_id_storage == null
                    : other.source_id_storage != null && source_id_storage.equal (other.source_id_storage))
                && (source_layer_id_storage == null
                    ? other.source_layer_id_storage == null
                    : other.source_layer_id_storage != null && source_layer_id_storage.equal (other.source_layer_id_storage))
                && (state == null
                    ? other.state == null
                    : other.state != null && state.equal (other.state));
        }
    }

    internal class FeatureQueryResultHandle {
        private Raw.FeatureQueryResult? native;

        public bool closed { get { return native == null; } }

        internal FeatureQueryResultHandle (owned Raw.FeatureQueryResult native) {
            this.native = (owned) native;
        }

        ~FeatureQueryResultHandle () {
            if (native != null) {
                Raw.feature_query_result_destroy (native);
                native = null;
            }
        }

        internal unowned Raw.FeatureQueryResult require_live () throws Error {
            if (native == null) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("feature query result handle is closed");
            }
            return native;
        }

        public void close () {
            if (native == null) {
                return;
            }
            unowned Raw.FeatureQueryResult closing = native;
            Raw.feature_query_result_destroy (closing);
            native = null;
        }

        public size_t count () throws Error {
            size_t result_count;
            check_status (Raw.feature_query_result_count (require_live (), out result_count));
            return result_count;
        }

        public QueriedFeature get (size_t index) throws Error {
            Raw.QueriedFeature feature = {};
            feature.size = (uint32) sizeof (Raw.QueriedFeature);
            check_status (Raw.feature_query_result_get (require_live (), index, &feature));
            return QueriedFeature.from_native (feature);
        }

        public QueriedFeature[] to_array () throws Error {
            var result_count = count ();
            QueriedFeature[] features = new QueriedFeature[result_count];
            for (size_t index = 0; index < result_count; index++) {
                features[index] = get (index);
            }
            return features;
        }

        internal static QueriedFeature[] copy_from_native (owned Raw.FeatureQueryResult native) throws Error {
            var handle = new FeatureQueryResultHandle ((owned) native);
            try {
                return handle.to_array ();
            } finally {
                handle.close ();
            }
        }
    }

}
