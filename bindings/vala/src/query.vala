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
                    throw new Error.INVALID_ARGUMENT ("feature extension value is null");
                }
                return new FeatureExtensionResult (FeatureExtensionResultType.VALUE, JsonValue.from_native (native.value[0]), null);
            case FeatureExtensionResultType.FEATURE_COLLECTION:
                return new FeatureExtensionResult (FeatureExtensionResultType.FEATURE_COLLECTION, null, FeatureCollection.from_native (native.feature_collection));
            default:
                throw new Error.INVALID_ARGUMENT ("unknown feature extension result type");
            }
        }
    }

    public class FeatureExtensionResultHandle {
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
            refresh_native_views ();
        }

        internal Raw.RenderedQueryGeometry to_native () {
            refresh_native_views ();
            return native;
        }

        private void refresh_native_views () {
            if (points != null && native.type == (uint32) RenderedQueryGeometryType.LINE_STRING) {
                native.line_string = Raw.ScreenLineString () { points = points, point_count = points.length };
            }
        }

        public static RenderedQueryGeometry point (ScreenPoint point) {
            return new RenderedQueryGeometry (Raw.rendered_query_geometry_point (point.to_native ()));
        }

        public static RenderedQueryGeometry box (ScreenBox box) {
            return new RenderedQueryGeometry (Raw.rendered_query_geometry_box (box.to_native ()));
        }

        public static RenderedQueryGeometry line_string (ScreenPoint[] points) throws Error {
            if (points.length == 0) {
                throw new Error.INVALID_ARGUMENT ("rendered query line string is empty");
            }
            Raw.ScreenPoint[] native_points = new Raw.ScreenPoint[points.length];
            for (var i = 0; i < points.length; i++) {
                native_points[i] = points[i].to_native ();
            }
            return new RenderedQueryGeometry (Raw.rendered_query_geometry_line_string (native_points, native_points.length), (owned) native_points);
        }
    }

    public class RenderedFeatureQueryOptions {
        private string[] layer_ids;
        private JsonValue? filter;
        private Raw.StringView[] layer_id_views;
        private Raw.JsonValue filter_native;

        public RenderedFeatureQueryOptions () {
            layer_ids = new string[0];
        }

        public void set_layer_ids (string[] layer_ids) {
            this.layer_ids = layer_ids;
        }

        public void set_filter (JsonValue filter) {
            this.filter = filter;
        }

        internal Raw.RenderedFeatureQueryOptions to_native () throws Error {
            Raw.RenderedFeatureQueryOptions native = Raw.rendered_feature_query_options_default ();
            if (layer_ids.length > 0) {
                layer_id_views = new Raw.StringView[layer_ids.length];
                for (var i = 0; i < layer_ids.length; i++) {
                    layer_id_views[i] = string_view (layer_ids[i]);
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
        private string[] source_layer_ids;
        private JsonValue? filter;
        private Raw.StringView[] source_layer_id_views;
        private Raw.JsonValue filter_native;

        public SourceFeatureQueryOptions () {
            source_layer_ids = new string[0];
        }

        public void set_source_layer_ids (string[] source_layer_ids) {
            this.source_layer_ids = source_layer_ids;
        }

        public void set_filter (JsonValue filter) {
            this.filter = filter;
        }

        internal Raw.SourceFeatureQueryOptions to_native () throws Error {
            Raw.SourceFeatureQueryOptions native = Raw.source_feature_query_options_default ();
            if (source_layer_ids.length > 0) {
                source_layer_id_views = new Raw.StringView[source_layer_ids.length];
                for (var i = 0; i < source_layer_ids.length; i++) {
                    source_layer_id_views[i] = string_view (source_layer_ids[i]);
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
        public Feature feature { get; private set; }
        public string? source_id { get; private set; }
        public string? source_layer_id { get; private set; }
        public JsonValue? state { get; private set; }

        private QueriedFeature (Feature feature, string? source_id, string? source_layer_id, JsonValue? state) {
            this.feature = feature;
            this.source_id = source_id;
            this.source_layer_id = source_layer_id;
            this.state = state;
        }

        internal static QueriedFeature from_native (Raw.QueriedFeature native) throws Error {
            string? source_id = null;
            string? source_layer_id = null;
            JsonValue? state = null;
            if ((native.fields & (uint32) Raw.QueriedFeatureField.SOURCE_ID) != 0) {
                source_id = copy_string_view (native.source_id);
            }
            if ((native.fields & (uint32) Raw.QueriedFeatureField.SOURCE_LAYER_ID) != 0) {
                source_layer_id = copy_string_view (native.source_layer_id);
            }
            if ((native.fields & (uint32) Raw.QueriedFeatureField.STATE) != 0 && native.state != null) {
                state = JsonValue.from_native (native.state[0]);
            }
            return new QueriedFeature (Feature.from_native (native.feature), source_id, source_layer_id, state);
        }
    }

    public class FeatureQueryResultHandle {
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
