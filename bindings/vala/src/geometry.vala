namespace MaplibreNative {
    public enum GeometryType {
        EMPTY = 0,
        POINT = 1,
        LINE_STRING = 2,
        POLYGON = 3,
        MULTI_POINT = 4,
        MULTI_LINE_STRING = 5,
        MULTI_POLYGON = 6,
        GEOMETRY_COLLECTION = 7
    }

    public enum JsonValueType {
        NULL = 0,
        BOOL = 1,
        UINT = 2,
        INT = 3,
        DOUBLE = 4,
        STRING = 5,
        ARRAY = 6,
        OBJECT = 7
    }

    public enum FeatureIdentifierType {
        NULL = 0,
        UINT = 1,
        INT = 2,
        DOUBLE = 3,
        STRING = 4
    }

    public enum GeoJsonType {
        GEOMETRY = 1,
        FEATURE = 2,
        FEATURE_COLLECTION = 3
    }

    public class CoordinateList {
        private Raw.LatLng[] coordinates;

        public CoordinateList (LatLng[] coordinates) throws Error {
            if (coordinates.length == 0) {
                throw new Error.INVALID_ARGUMENT ("coordinates are empty");
            }
            this.coordinates = new Raw.LatLng[coordinates.length];
            for (var i = 0; i < coordinates.length; i++) {
                this.coordinates[i] = coordinates[i].to_native ();
            }
        }

        internal Raw.CoordinateSpan to_native () {
            return Raw.CoordinateSpan () { coordinates = coordinates, coordinate_count = coordinates.length };
        }

        internal static CoordinateList from_native (Raw.CoordinateSpan native) throws Error {
            if (native.coordinates == null || native.coordinate_count == 0) {
                throw new Error.INVALID_ARGUMENT ("coordinate span is empty");
            }
            LatLng[] values = new LatLng[native.coordinate_count];
            for (size_t i = 0; i < native.coordinate_count; i++) {
                values[i] = LatLng.from_native (native.coordinates[i]);
            }
            return new CoordinateList (values);
        }

        internal LatLng[] to_values () {
            LatLng[] values = new LatLng[coordinates.length];
            for (var i = 0; i < coordinates.length; i++) {
                values[i] = LatLng.from_native (coordinates[i]);
            }
            return values;
        }
    }

    public class Polygon {
        private CoordinateList[] rings;
        private Raw.CoordinateSpan[] ring_natives;

        public Polygon (CoordinateList[] rings) throws Error {
            if (rings.length == 0) {
                throw new Error.INVALID_ARGUMENT ("polygon rings are empty");
            }
            this.rings = rings;
        }

        internal Raw.PolygonGeometry to_native () {
            ring_natives = new Raw.CoordinateSpan[rings.length];
            for (var i = 0; i < rings.length; i++) {
                ring_natives[i] = rings[i].to_native ();
            }
            return Raw.PolygonGeometry () { rings = ring_natives, ring_count = ring_natives.length };
        }

        internal static Polygon from_native (Raw.PolygonGeometry native) throws Error {
            if (native.rings == null || native.ring_count == 0) {
                throw new Error.INVALID_ARGUMENT ("polygon rings are empty");
            }
            CoordinateList[] rings = new CoordinateList[native.ring_count];
            for (size_t i = 0; i < native.ring_count; i++) {
                rings[i] = CoordinateList.from_native (native.rings[i]);
            }
            return new Polygon (rings);
        }
    }

    public class Geometry {
        internal Raw.Geometry native;
        private Raw.LatLng[] coordinates;
        private CoordinateList[] coordinate_lists;
        private Polygon[] polygon_list;
        private Geometry[] geometry_list;
        private Raw.CoordinateSpan[] coordinate_span_natives;
        private Raw.PolygonGeometry[] polygon_natives;
        private Raw.Geometry[] geometry_natives;

        private Geometry (Raw.Geometry native, owned Raw.LatLng[]? coordinates = null, CoordinateList[]? coordinate_lists = null, Polygon[]? polygon_list = null, Geometry[]? geometry_list = null) {
            this.native = native;
            this.coordinates = (owned) coordinates;
            this.coordinate_lists = coordinate_lists ?? new CoordinateList[0];
            this.polygon_list = polygon_list ?? new Polygon[0];
            this.geometry_list = geometry_list ?? new Geometry[0];
            refresh_native_views ();
        }

        internal Raw.Geometry to_native () {
            refresh_native_views ();
            return native;
        }

        private void refresh_native_views () {
            if (coordinates != null) {
                if (native.type == (uint32) GeometryType.LINE_STRING) {
                    native.line_string = Raw.CoordinateSpan () { coordinates = coordinates, coordinate_count = coordinates.length };
                } else if (native.type == (uint32) GeometryType.MULTI_POINT) {
                    native.multi_point = Raw.CoordinateSpan () { coordinates = coordinates, coordinate_count = coordinates.length };
                }
            }
            if (native.type == (uint32) GeometryType.POLYGON) {
                if (polygon_list.length > 0) {
                    native.polygon = polygon_list[0].to_native ();
                }
            } else if (native.type == (uint32) GeometryType.MULTI_LINE_STRING) {
                coordinate_span_natives = new Raw.CoordinateSpan[coordinate_lists.length];
                for (var i = 0; i < coordinate_lists.length; i++) {
                    coordinate_span_natives[i] = coordinate_lists[i].to_native ();
                }
                native.multi_line_string = Raw.MultiLineGeometry () { lines = coordinate_span_natives, line_count = coordinate_span_natives.length };
            } else if (native.type == (uint32) GeometryType.MULTI_POLYGON) {
                polygon_natives = new Raw.PolygonGeometry[polygon_list.length];
                for (var i = 0; i < polygon_list.length; i++) {
                    polygon_natives[i] = polygon_list[i].to_native ();
                }
                native.multi_polygon = Raw.MultiPolygonGeometry () { polygons = polygon_natives, polygon_count = polygon_natives.length };
            } else if (native.type == (uint32) GeometryType.GEOMETRY_COLLECTION) {
                geometry_natives = new Raw.Geometry[geometry_list.length];
                for (var i = 0; i < geometry_list.length; i++) {
                    geometry_natives[i] = geometry_list[i].to_native ();
                }
                native.geometry_collection = Raw.GeometryCollection () { geometries = (void*) geometry_natives, geometry_count = geometry_natives.length };
            }
        }

        private static Raw.LatLng[] copy_coordinates (LatLng[] coordinates) throws Error {
            if (coordinates.length == 0) {
                throw new Error.INVALID_ARGUMENT ("geometry coordinates are empty");
            }
            Raw.LatLng[] native_coordinates = new Raw.LatLng[coordinates.length];
            for (var i = 0; i < coordinates.length; i++) {
                native_coordinates[i] = coordinates[i].to_native ();
            }
            return native_coordinates;
        }

        public static Geometry empty () {
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.EMPTY;
            return new Geometry (geometry);
        }

        public static Geometry point (LatLng coordinate) {
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.POINT;
            geometry.point = coordinate.to_native ();
            return new Geometry (geometry);
        }

        public static Geometry line_string (LatLng[] coordinates) throws Error {
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.LINE_STRING;
            return new Geometry (geometry, copy_coordinates (coordinates));
        }

        public static Geometry multi_point (LatLng[] coordinates) throws Error {
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.MULTI_POINT;
            return new Geometry (geometry, copy_coordinates (coordinates));
        }

        public static Geometry polygon (Polygon polygon) {
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.POLYGON;
            return new Geometry (geometry, null, null, { polygon });
        }

        public static Geometry multi_line_string (CoordinateList[] lines) throws Error {
            if (lines.length == 0) {
                throw new Error.INVALID_ARGUMENT ("multi-line geometry lines are empty");
            }
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.MULTI_LINE_STRING;
            return new Geometry (geometry, null, lines);
        }

        public static Geometry multi_polygon (Polygon[] polygons) throws Error {
            if (polygons.length == 0) {
                throw new Error.INVALID_ARGUMENT ("multi-polygon geometry polygons are empty");
            }
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.MULTI_POLYGON;
            return new Geometry (geometry, null, null, polygons);
        }

        public static Geometry geometry_collection (Geometry[] geometries) throws Error {
            if (geometries.length == 0) {
                throw new Error.INVALID_ARGUMENT ("geometry collection is empty");
            }
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.GEOMETRY_COLLECTION;
            return new Geometry (geometry, null, null, null, geometries);
        }

        internal static Geometry from_native (Raw.Geometry native) throws Error {
            switch ((GeometryType) native.type) {
            case GeometryType.EMPTY:
                return Geometry.empty ();
            case GeometryType.POINT:
                return Geometry.point (LatLng.from_native (native.point));
            case GeometryType.LINE_STRING:
                return Geometry.line_string (CoordinateList.from_native (native.line_string).to_values ());
            case GeometryType.POLYGON:
                return Geometry.polygon (Polygon.from_native (native.polygon));
            case GeometryType.MULTI_POINT:
                return Geometry.multi_point (CoordinateList.from_native (native.multi_point).to_values ());
            case GeometryType.MULTI_LINE_STRING:
                if (native.multi_line_string.lines == null || native.multi_line_string.line_count == 0) {
                    throw new Error.INVALID_ARGUMENT ("multi-line geometry lines are empty");
                }
                CoordinateList[] lines = new CoordinateList[native.multi_line_string.line_count];
                for (size_t i = 0; i < native.multi_line_string.line_count; i++) {
                    lines[i] = CoordinateList.from_native (native.multi_line_string.lines[i]);
                }
                return Geometry.multi_line_string (lines);
            case GeometryType.MULTI_POLYGON:
                if (native.multi_polygon.polygons == null || native.multi_polygon.polygon_count == 0) {
                    throw new Error.INVALID_ARGUMENT ("multi-polygon geometry polygons are empty");
                }
                Polygon[] polygons = new Polygon[native.multi_polygon.polygon_count];
                for (size_t i = 0; i < native.multi_polygon.polygon_count; i++) {
                    polygons[i] = Polygon.from_native (native.multi_polygon.polygons[i]);
                }
                return Geometry.multi_polygon (polygons);
            case GeometryType.GEOMETRY_COLLECTION:
                if (native.geometry_collection.geometries == null || native.geometry_collection.geometry_count == 0) {
                    throw new Error.INVALID_ARGUMENT ("geometry collection is empty");
                }
                Geometry[] geometries = new Geometry[native.geometry_collection.geometry_count];
                Raw.Geometry* raw_geometries = (Raw.Geometry*) native.geometry_collection.geometries;
                for (size_t i = 0; i < native.geometry_collection.geometry_count; i++) {
                    geometries[i] = Geometry.from_native (raw_geometries[i]);
                }
                return Geometry.geometry_collection (geometries);
            default:
                throw new Error.INVALID_ARGUMENT ("unknown geometry type");
            }
        }
    }

    public class JsonValue {
        private JsonValueType type;
        private bool bool_storage;
        private uint64 uint_storage;
        private int64 int_storage;
        private double double_storage;
        private string string_storage;
        private JsonValue[] array_values;
        private JsonMember[] object_members;
        private Raw.JsonValue[] array_natives;
        private Raw.JsonMember[] object_native_members;
        private Raw.JsonValue[] object_native_values;

        private JsonValue (JsonValueType type) {
            this.type = type;
            string_storage = "";
            array_values = new JsonValue[0];
            object_members = new JsonMember[0];
        }

        public static JsonValue null_value () {
            return new JsonValue (JsonValueType.NULL);
        }

        public static JsonValue bool_value (bool value) {
            var json = new JsonValue (JsonValueType.BOOL);
            json.bool_storage = value;
            return json;
        }

        public static JsonValue uint_value (uint64 value) {
            var json = new JsonValue (JsonValueType.UINT);
            json.uint_storage = value;
            return json;
        }

        public static JsonValue int_value (int64 value) {
            var json = new JsonValue (JsonValueType.INT);
            json.int_storage = value;
            return json;
        }

        public static JsonValue double_value (double value) {
            var json = new JsonValue (JsonValueType.DOUBLE);
            json.double_storage = value;
            return json;
        }

        public static JsonValue string_value (string value) {
            var json = new JsonValue (JsonValueType.STRING);
            json.string_storage = value;
            return json;
        }

        public static JsonValue array_value (JsonValue[] values) {
            var json = new JsonValue (JsonValueType.ARRAY);
            json.array_values = values;
            return json;
        }

        public static JsonValue object_value (JsonMember[] members) {
            var json = new JsonValue (JsonValueType.OBJECT);
            json.object_members = members;
            return json;
        }

        public JsonValueType value_type { get { return type; } }

        public bool get_bool () throws Error {
            if (type != JsonValueType.BOOL) {
                throw new Error.INVALID_STATE ("JSON value is not a bool");
            }
            return bool_storage;
        }

        public int64 get_int () throws Error {
            if (type != JsonValueType.INT) {
                throw new Error.INVALID_STATE ("JSON value is not an int");
            }
            return int_storage;
        }

        public uint64 get_uint () throws Error {
            if (type != JsonValueType.UINT) {
                throw new Error.INVALID_STATE ("JSON value is not a uint");
            }
            return uint_storage;
        }

        public double get_double () throws Error {
            if (type != JsonValueType.DOUBLE) {
                throw new Error.INVALID_STATE ("JSON value is not a double");
            }
            return double_storage;
        }

        public string get_string () throws Error {
            if (type != JsonValueType.STRING) {
                throw new Error.INVALID_STATE ("JSON value is not a string");
            }
            return string_storage;
        }

        public JsonValue[] get_array_values () throws Error {
            if (type != JsonValueType.ARRAY) {
                throw new Error.INVALID_STATE ("JSON value is not an array");
            }
            return array_values;
        }

        public JsonMember[] get_object_members () throws Error {
            if (type != JsonValueType.OBJECT) {
                throw new Error.INVALID_STATE ("JSON value is not an object");
            }
            return object_members;
        }

        internal static JsonValue from_native (Raw.JsonValue native) throws Error {
            switch ((JsonValueType) native.type) {
            case JsonValueType.BOOL:
                return JsonValue.bool_value (native.bool_value);
            case JsonValueType.UINT:
                return JsonValue.uint_value (native.uint_value);
            case JsonValueType.INT:
                return JsonValue.int_value (native.int_value);
            case JsonValueType.DOUBLE:
                return JsonValue.double_value (native.double_value);
            case JsonValueType.STRING:
                return JsonValue.string_value (copy_string_view (native.string_value));
            case JsonValueType.ARRAY:
                JsonValue[] values = new JsonValue[native.array_value.value_count];
                Raw.JsonValue* raw_values = (Raw.JsonValue*) native.array_value.values;
                for (size_t i = 0; i < native.array_value.value_count; i++) {
                    values[i] = JsonValue.from_native (raw_values[i]);
                }
                return JsonValue.array_value (values);
            case JsonValueType.OBJECT:
                JsonMember[] members = new JsonMember[native.object_value.member_count];
                for (size_t i = 0; i < native.object_value.member_count; i++) {
                    Raw.JsonMember member = native.object_value.members[i];
                    Raw.JsonValue* value = (Raw.JsonValue*) member.value;
                    members[i] = new JsonMember (copy_string_view (member.key), JsonValue.from_native (value[0]));
                }
                return JsonValue.object_value (members);
            case JsonValueType.NULL:
            default:
                return JsonValue.null_value ();
            }
        }

        internal Raw.JsonValue to_native () throws Error {
            Raw.JsonValue native = {};
            native.size = (uint32) sizeof (Raw.JsonValue);
            native.type = (uint32) type;
            switch (type) {
            case JsonValueType.BOOL:
                native.bool_value = bool_storage;
                break;
            case JsonValueType.UINT:
                native.uint_value = uint_storage;
                break;
            case JsonValueType.INT:
                native.int_value = int_storage;
                break;
            case JsonValueType.DOUBLE:
                native.double_value = double_storage;
                break;
            case JsonValueType.STRING:
                native.string_value = string_view (string_storage);
                break;
            case JsonValueType.ARRAY:
                array_natives = new Raw.JsonValue[array_values.length];
                for (var i = 0; i < array_values.length; i++) {
                    array_natives[i] = array_values[i].to_native ();
                }
                native.array_value = Raw.JsonArray () { values = (void*) array_natives, value_count = array_natives.length };
                break;
            case JsonValueType.OBJECT:
                object_native_members = new Raw.JsonMember[object_members.length];
                object_native_values = new Raw.JsonValue[object_members.length];
                for (var i = 0; i < object_members.length; i++) {
                    object_native_values[i] = object_members[i].value.to_native ();
                    object_native_members[i] = Raw.JsonMember () { key = string_view (object_members[i].key), value = (void*) &object_native_values[i] };
                }
                native.object_value = Raw.JsonObject () { members = object_native_members, member_count = object_native_members.length };
                break;
            default:
                break;
            }
            return native;
        }
    }

    public class JsonMember {
        public string key { get; private set; }
        public JsonValue value { get; private set; }

        public JsonMember (string key, JsonValue value) {
            this.key = key;
            this.value = value;
        }
    }

    public class FeatureIdentifier {
        private FeatureIdentifierType type;
        private uint64 uint_storage;
        private int64 int_storage;
        private double double_storage;
        private string string_storage;

        private FeatureIdentifier (FeatureIdentifierType type) {
            this.type = type;
            string_storage = "";
        }

        public static FeatureIdentifier none () {
            return new FeatureIdentifier (FeatureIdentifierType.NULL);
        }

        public static FeatureIdentifier uint_value (uint64 value) {
            var identifier = new FeatureIdentifier (FeatureIdentifierType.UINT);
            identifier.uint_storage = value;
            return identifier;
        }

        public static FeatureIdentifier int_value (int64 value) {
            var identifier = new FeatureIdentifier (FeatureIdentifierType.INT);
            identifier.int_storage = value;
            return identifier;
        }

        public static FeatureIdentifier double_value (double value) {
            var identifier = new FeatureIdentifier (FeatureIdentifierType.DOUBLE);
            identifier.double_storage = value;
            return identifier;
        }

        public static FeatureIdentifier string_value (string value) {
            var identifier = new FeatureIdentifier (FeatureIdentifierType.STRING);
            identifier.string_storage = value;
            return identifier;
        }

        public FeatureIdentifierType value_type { get { return type; } }

        internal static FeatureIdentifier from_native (Raw.Feature native) throws Error {
            switch ((FeatureIdentifierType) native.identifier_type) {
            case FeatureIdentifierType.UINT:
                return FeatureIdentifier.uint_value (native.identifier_uint_value);
            case FeatureIdentifierType.INT:
                return FeatureIdentifier.int_value (native.identifier_int_value);
            case FeatureIdentifierType.DOUBLE:
                return FeatureIdentifier.double_value (native.identifier_double_value);
            case FeatureIdentifierType.STRING:
                return FeatureIdentifier.string_value (copy_string_view (native.identifier_string_value));
            case FeatureIdentifierType.NULL:
            default:
                return FeatureIdentifier.none ();
            }
        }

        internal void apply_to_native (ref Raw.Feature native) throws Error {
            native.identifier_type = (uint32) type;
            switch (type) {
            case FeatureIdentifierType.UINT:
                native.identifier_uint_value = uint_storage;
                break;
            case FeatureIdentifierType.INT:
                native.identifier_int_value = int_storage;
                break;
            case FeatureIdentifierType.DOUBLE:
                native.identifier_double_value = double_storage;
                break;
            case FeatureIdentifierType.STRING:
                native.identifier_string_value = string_view (string_storage);
                break;
            default:
                break;
            }
        }
    }

    public class FeatureStateSelector {
        private string source_id;
        private string? source_layer_id;
        private string? feature_id;
        private string? state_key;

        public FeatureStateSelector (string source_id) {
            this.source_id = source_id;
        }

        public void set_source_layer_id (string value) {
            source_layer_id = value;
        }

        public void set_feature_id (string value) {
            feature_id = value;
        }

        public void set_state_key (string value) {
            state_key = value;
        }

        internal Raw.FeatureStateSelector to_native () throws Error {
            Raw.FeatureStateSelector native = {};
            native.size = (uint32) sizeof (Raw.FeatureStateSelector);
            native.source_id = string_view (source_id);
            if (source_layer_id != null) {
                native.fields |= (uint32) Raw.FeatureStateSelectorField.SOURCE_LAYER_ID;
                native.source_layer_id = string_view (source_layer_id);
            }
            if (feature_id != null) {
                native.fields |= (uint32) Raw.FeatureStateSelectorField.FEATURE_ID;
                native.feature_id = string_view (feature_id);
            }
            if (state_key != null) {
                native.fields |= (uint32) Raw.FeatureStateSelectorField.STATE_KEY;
                native.state_key = string_view (state_key);
            }
            return native;
        }
    }

    public class Feature {
        private Geometry geometry_ref;
        private JsonMember[] properties;
        private FeatureIdentifier identifier;
        private Raw.Geometry geometry_native;
        private Raw.JsonMember[] property_natives;
        private Raw.JsonValue[] property_value_natives;
        private Raw.Feature native;

        public Feature (Geometry geometry, JsonMember[] properties, FeatureIdentifier? identifier = null) {
            geometry_ref = geometry;
            this.properties = properties;
            this.identifier = identifier ?? FeatureIdentifier.none ();
        }

        public Geometry geometry { get { return geometry_ref; } }
        public JsonMember[] property_members { get { return properties; } }
        public FeatureIdentifier feature_identifier { get { return identifier; } }

        internal static Feature from_native (Raw.Feature native) throws Error {
            if (native.geometry == null) {
                throw new Error.INVALID_ARGUMENT ("queried feature geometry is null");
            }
            JsonMember[] members = new JsonMember[native.property_count];
            if (native.property_count > 0 && native.properties == null) {
                throw new Error.INVALID_ARGUMENT ("queried feature properties are null");
            }
            for (size_t i = 0; i < native.property_count; i++) {
                Raw.JsonMember member = native.properties[i];
                Raw.JsonValue* value = (Raw.JsonValue*) member.value;
                if (value == null) {
                    throw new Error.INVALID_ARGUMENT ("queried feature property value is null");
                }
                members[i] = new JsonMember (copy_string_view (member.key), JsonValue.from_native (value[0]));
            }
            return new Feature (Geometry.from_native (native.geometry[0]), members, FeatureIdentifier.from_native (native));
        }

        internal Raw.Feature to_native () throws Error {
            geometry_native = geometry_ref.to_native ();
            property_natives = new Raw.JsonMember[properties.length];
            property_value_natives = new Raw.JsonValue[properties.length];
            for (var i = 0; i < properties.length; i++) {
                property_value_natives[i] = properties[i].value.to_native ();
                property_natives[i] = Raw.JsonMember () { key = string_view (properties[i].key), value = (void*) &property_value_natives[i] };
            }
            native = {};
            native.size = (uint32) sizeof (Raw.Feature);
            native.geometry = &geometry_native;
            native.properties = property_natives;
            native.property_count = property_natives.length;
            identifier.apply_to_native (ref native);
            return native;
        }
    }

    public class FeatureCollection {
        private Feature[] features;
        private Raw.Feature[] feature_natives;
        private Raw.FeatureCollection native;

        public FeatureCollection (Feature[] features) {
            this.features = features;
        }

        public Feature[] feature_items { get { return features; } }

        internal Raw.FeatureCollection to_native () throws Error {
            feature_natives = new Raw.Feature[features.length];
            for (var i = 0; i < features.length; i++) {
                feature_natives[i] = features[i].to_native ();
            }
            native = Raw.FeatureCollection () { features = feature_natives, feature_count = feature_natives.length };
            return native;
        }

        internal static FeatureCollection from_native (Raw.FeatureCollection native) throws Error {
            Feature[] features = new Feature[native.feature_count];
            if (native.feature_count > 0 && native.features == null) {
                throw new Error.INVALID_ARGUMENT ("feature collection data is null");
            }
            for (size_t i = 0; i < native.feature_count; i++) {
                features[i] = Feature.from_native (native.features[i]);
            }
            return new FeatureCollection (features);
        }
    }

    public class GeoJson {
        private Geometry? geometry_ref;
        private Feature? feature_ref;
        private FeatureCollection? feature_collection_ref;
        private Raw.Geometry geometry_native;
        private Raw.Feature feature_native;
        private Raw.FeatureCollection feature_collection_native;
        private Raw.GeoJson native;

        private GeoJson.for_geometry (Geometry geometry) {
            geometry_ref = geometry;
            native = {};
            native.size = (uint32) sizeof (Raw.GeoJson);
            native.type = (uint32) GeoJsonType.GEOMETRY;
        }

        private GeoJson.for_feature (Feature feature) {
            feature_ref = feature;
            native = {};
            native.size = (uint32) sizeof (Raw.GeoJson);
            native.type = (uint32) GeoJsonType.FEATURE;
        }

        private GeoJson.for_feature_collection (FeatureCollection feature_collection) {
            feature_collection_ref = feature_collection;
            native = {};
            native.size = (uint32) sizeof (Raw.GeoJson);
            native.type = (uint32) GeoJsonType.FEATURE_COLLECTION;
        }

        public static GeoJson geometry (Geometry geometry) {
            return new GeoJson.for_geometry (geometry);
        }

        public static GeoJson feature (Feature feature) {
            return new GeoJson.for_feature (feature);
        }

        public static GeoJson feature_collection (FeatureCollection feature_collection) {
            return new GeoJson.for_feature_collection (feature_collection);
        }

        internal Raw.GeoJson to_native () throws Error {
            if (native.type == (uint32) GeoJsonType.FEATURE) {
                feature_native = feature_ref.to_native ();
                native.feature = &feature_native;
            } else if (native.type == (uint32) GeoJsonType.FEATURE_COLLECTION) {
                feature_collection_native = feature_collection_ref.to_native ();
                native.feature_collection = feature_collection_native;
            } else {
                geometry_native = geometry_ref.to_native ();
                native.geometry = &geometry_native;
            }
            return native;
        }
    }

}
