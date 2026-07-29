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
            this.coordinates = new Raw.LatLng[coordinates.length];
            for (var i = 0; i < coordinates.length; i++) {
                this.coordinates[i] = coordinates[i].to_native ();
            }
        }

        internal Raw.CoordinateSpan to_native () {
            return Raw.CoordinateSpan () { coordinates = coordinates, coordinate_count = coordinates.length };
        }

        internal static CoordinateList from_native (Raw.CoordinateSpan native) throws Error {
            if (native.coordinate_count > 0 && native.coordinates == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("coordinate span data is null");
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

        public LatLng[] to_array () {
            return to_values ();
        }

        public CoordinateList copy () throws Error {
            return new CoordinateList (to_values ());
        }

        public bool equal (CoordinateList other) {
            if (coordinates.length != other.coordinates.length) {
                return false;
            }
            for (var index = 0; index < coordinates.length; index++) {
                if (coordinates[index].latitude != other.coordinates[index].latitude
                    || coordinates[index].longitude != other.coordinates[index].longitude) {
                    return false;
                }
            }
            return true;
        }
    }

    public class Polygon {
        private CoordinateList[] rings;
        private Raw.CoordinateSpan[] ring_natives;

        public Polygon (CoordinateList[] rings) throws Error {
            this.rings = new CoordinateList[rings.length];
            for (var index = 0; index < rings.length; index++) {
                if (rings[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("polygon ring at index %d is null", index);
                }
                this.rings[index] = rings[index];
            }
        }

        internal Polygon.copy_for_native (Polygon other) {
            rings = new CoordinateList[other.rings.length];
            for (var index = 0; index < other.rings.length; index++) {
                rings[index] = other.rings[index];
            }
        }

        internal Raw.PolygonGeometry to_native () {
            ring_natives = new Raw.CoordinateSpan[rings.length];
            for (var i = 0; i < rings.length; i++) {
                ring_natives[i] = rings[i].to_native ();
            }
            return Raw.PolygonGeometry () { rings = ring_natives, ring_count = ring_natives.length };
        }

        internal static Polygon from_native (Raw.PolygonGeometry native) throws Error {
            if (native.ring_count > 0 && native.rings == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("polygon ring data is null");
            }
            CoordinateList[] rings = new CoordinateList[native.ring_count];
            for (size_t i = 0; i < native.ring_count; i++) {
                rings[i] = CoordinateList.from_native (native.rings[i]);
            }
            return new Polygon (rings);
        }

        public CoordinateList[] get_rings () {
            var copied = new CoordinateList[rings.length];
            for (var index = 0; index < rings.length; index++) {
                copied[index] = rings[index];
            }
            return copied;
        }

        public Polygon copy () throws Error {
            var copied = new CoordinateList[rings.length];
            for (var index = 0; index < rings.length; index++) {
                copied[index] = rings[index].copy ();
            }
            return new Polygon (copied);
        }

        public bool equal (Polygon other) {
            if (rings.length != other.rings.length) {
                return false;
            }
            for (var index = 0; index < rings.length; index++) {
                if (!rings[index].equal (other.rings[index])) {
                    return false;
                }
            }
            return true;
        }
    }

    public class Geometry {
        internal Raw.Geometry native;
        private Raw.LatLng[] coordinates;
        private CoordinateList[] coordinate_lists;
        private Polygon[] polygon_list;
        private Geometry[] geometry_list;
        private Raw.CoordinateSpan[] coordinate_span_natives;
        private Polygon[] polygon_native_storage;
        private Raw.PolygonGeometry[] polygon_natives;
        private Geometry[] geometry_native_storage;
        private Raw.Geometry[] geometry_natives;

        private Geometry (Raw.Geometry native, owned Raw.LatLng[]? coordinates = null, CoordinateList[]? coordinate_lists = null, Polygon[]? polygon_list = null, Geometry[]? geometry_list = null) {
            this.native = native;
            this.coordinates = (owned) coordinates;
            this.coordinate_lists = new CoordinateList[coordinate_lists == null ? 0 : coordinate_lists.length];
            for (var index = 0; coordinate_lists != null && index < coordinate_lists.length; index++) {
                this.coordinate_lists[index] = coordinate_lists[index];
            }
            this.polygon_list = new Polygon[polygon_list == null ? 0 : polygon_list.length];
            for (var index = 0; polygon_list != null && index < polygon_list.length; index++) {
                this.polygon_list[index] = polygon_list[index];
            }
            this.geometry_list = new Geometry[geometry_list == null ? 0 : geometry_list.length];
            for (var index = 0; geometry_list != null && index < geometry_list.length; index++) {
                this.geometry_list[index] = geometry_list[index];
            }
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
                    polygon_native_storage = new Polygon[1];
                    polygon_native_storage[0] = new Polygon.copy_for_native (polygon_list[0]);
                    native.polygon = polygon_native_storage[0].to_native ();
                }
            } else if (native.type == (uint32) GeometryType.MULTI_LINE_STRING) {
                coordinate_span_natives = new Raw.CoordinateSpan[coordinate_lists.length];
                for (var i = 0; i < coordinate_lists.length; i++) {
                    coordinate_span_natives[i] = coordinate_lists[i].to_native ();
                }
                native.multi_line_string = Raw.MultiLineGeometry () { lines = coordinate_span_natives, line_count = coordinate_span_natives.length };
            } else if (native.type == (uint32) GeometryType.MULTI_POLYGON) {
                polygon_native_storage = new Polygon[polygon_list.length];
                polygon_natives = new Raw.PolygonGeometry[polygon_list.length];
                for (var i = 0; i < polygon_list.length; i++) {
                    polygon_native_storage[i] = new Polygon.copy_for_native (polygon_list[i]);
                    polygon_natives[i] = polygon_native_storage[i].to_native ();
                }
                native.multi_polygon = Raw.MultiPolygonGeometry () { polygons = polygon_natives, polygon_count = polygon_natives.length };
            } else if (native.type == (uint32) GeometryType.GEOMETRY_COLLECTION) {
                geometry_native_storage = new Geometry[geometry_list.length];
                geometry_natives = new Raw.Geometry[geometry_list.length];
                for (var i = 0; i < geometry_list.length; i++) {
                    geometry_native_storage[i] = geometry_list[i].copy_for_native ();
                    geometry_natives[i] = geometry_native_storage[i].to_native ();
                }
                native.geometry_collection = Raw.GeometryCollection () { geometries = (void*) geometry_natives, geometry_count = geometry_natives.length };
            }
        }

        private Geometry copy_for_native () {
            Raw.LatLng[]? copied_coordinates = null;
            if (coordinates != null) {
                copied_coordinates = new Raw.LatLng[coordinates.length];
                for (var index = 0; index < coordinates.length; index++) {
                    copied_coordinates[index] = coordinates[index];
                }
            }
            return new Geometry (
                native,
                (owned) copied_coordinates,
                coordinate_lists,
                polygon_list,
                geometry_list
            );
        }

        private static Raw.LatLng[] copy_coordinates (LatLng[] coordinates) throws Error {
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

        public static Geometry polygon (Polygon polygon) throws Error {
            if (polygon == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("polygon geometry is null");
            }
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.POLYGON;
            return new Geometry (geometry, null, null, { polygon });
        }

        public static Geometry multi_line_string (CoordinateList[] lines) throws Error {
            for (var index = 0; index < lines.length; index++) {
                if (lines[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("multi-line string item at index %d is null", index);
                }
            }
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.MULTI_LINE_STRING;
            return new Geometry (geometry, null, lines);
        }

        public static Geometry multi_polygon (Polygon[] polygons) throws Error {
            for (var index = 0; index < polygons.length; index++) {
                if (polygons[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("multi-polygon item at index %d is null", index);
                }
            }
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.MULTI_POLYGON;
            return new Geometry (geometry, null, null, polygons);
        }

        public static Geometry geometry_collection (Geometry[] geometries) throws Error {
            for (var index = 0; index < geometries.length; index++) {
                if (geometries[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("geometry collection item at index %d is null", index);
                }
            }
            Raw.Geometry geometry = {};
            geometry.size = (uint32) sizeof (Raw.Geometry);
            geometry.type = (uint32) GeometryType.GEOMETRY_COLLECTION;
            return new Geometry (geometry, null, null, null, geometries);
        }

        public GeometryType geometry_type { get { return (GeometryType) native.type; } }

        public LatLng get_point () throws Error {
            if (geometry_type != GeometryType.POINT) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("geometry is not a point");
            }
            return LatLng.from_native (native.point);
        }

        public LatLng[] get_coordinates () throws Error {
            if (geometry_type != GeometryType.LINE_STRING && geometry_type != GeometryType.MULTI_POINT) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("geometry does not contain a coordinate list");
            }
            var copied = new LatLng[coordinates.length];
            for (var index = 0; index < coordinates.length; index++) {
                copied[index] = LatLng.from_native (coordinates[index]);
            }
            return copied;
        }

        public Polygon get_polygon () throws Error {
            if (geometry_type != GeometryType.POLYGON) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("geometry is not a polygon");
            }
            return polygon_list[0];
        }

        public CoordinateList[] get_lines () throws Error {
            if (geometry_type != GeometryType.MULTI_LINE_STRING) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("geometry is not a multi-line string");
            }
            var copied = new CoordinateList[coordinate_lists.length];
            for (var index = 0; index < coordinate_lists.length; index++) {
                copied[index] = coordinate_lists[index];
            }
            return copied;
        }

        public Polygon[] get_polygons () throws Error {
            if (geometry_type != GeometryType.MULTI_POLYGON) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("geometry is not a multi-polygon");
            }
            var copied = new Polygon[polygon_list.length];
            for (var index = 0; index < polygon_list.length; index++) {
                copied[index] = polygon_list[index];
            }
            return copied;
        }

        public Geometry[] get_geometries () throws Error {
            if (geometry_type != GeometryType.GEOMETRY_COLLECTION) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("geometry is not a geometry collection");
            }
            var copied = new Geometry[geometry_list.length];
            for (var index = 0; index < geometry_list.length; index++) {
                copied[index] = geometry_list[index];
            }
            return copied;
        }

        public Geometry copy () throws Error {
            return copy_for_native ();
        }

        public bool equal (Geometry other) {
            if (native.type != other.native.type) {
                return false;
            }
            switch ((GeometryType) native.type) {
            case GeometryType.POINT:
                return native.point.latitude == other.native.point.latitude
                    && native.point.longitude == other.native.point.longitude;
            case GeometryType.LINE_STRING:
            case GeometryType.MULTI_POINT:
                if (coordinates.length != other.coordinates.length) {
                    return false;
                }
                for (var index = 0; index < coordinates.length; index++) {
                    if (coordinates[index].latitude != other.coordinates[index].latitude
                        || coordinates[index].longitude != other.coordinates[index].longitude) {
                        return false;
                    }
                }
                return true;
            case GeometryType.POLYGON:
            case GeometryType.MULTI_POLYGON:
                if (polygon_list.length != other.polygon_list.length) {
                    return false;
                }
                for (var index = 0; index < polygon_list.length; index++) {
                    if (!polygon_list[index].equal (other.polygon_list[index])) {
                        return false;
                    }
                }
                return true;
            case GeometryType.MULTI_LINE_STRING:
                if (coordinate_lists.length != other.coordinate_lists.length) {
                    return false;
                }
                for (var index = 0; index < coordinate_lists.length; index++) {
                    if (!coordinate_lists[index].equal (other.coordinate_lists[index])) {
                        return false;
                    }
                }
                return true;
            case GeometryType.GEOMETRY_COLLECTION:
                if (geometry_list.length != other.geometry_list.length) {
                    return false;
                }
                for (var index = 0; index < geometry_list.length; index++) {
                    if (!geometry_list[index].equal (other.geometry_list[index])) {
                        return false;
                    }
                }
                return true;
            case GeometryType.EMPTY:
            default:
                return true;
            }
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
                if (native.multi_line_string.line_count > 0 && native.multi_line_string.lines == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("multi-line geometry data is null");
                }
                CoordinateList[] lines = new CoordinateList[native.multi_line_string.line_count];
                for (size_t i = 0; i < native.multi_line_string.line_count; i++) {
                    lines[i] = CoordinateList.from_native (native.multi_line_string.lines[i]);
                }
                return Geometry.multi_line_string (lines);
            case GeometryType.MULTI_POLYGON:
                if (native.multi_polygon.polygon_count > 0 && native.multi_polygon.polygons == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("multi-polygon geometry data is null");
                }
                Polygon[] polygons = new Polygon[native.multi_polygon.polygon_count];
                for (size_t i = 0; i < native.multi_polygon.polygon_count; i++) {
                    polygons[i] = Polygon.from_native (native.multi_polygon.polygons[i]);
                }
                return Geometry.multi_polygon (polygons);
            case GeometryType.GEOMETRY_COLLECTION:
                if (native.geometry_collection.geometry_count > 0 && native.geometry_collection.geometries == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("geometry collection data is null");
                }
                Geometry[] geometries = new Geometry[native.geometry_collection.geometry_count];
                Raw.Geometry* raw_geometries = (Raw.Geometry*) native.geometry_collection.geometries;
                for (size_t i = 0; i < native.geometry_collection.geometry_count; i++) {
                    geometries[i] = Geometry.from_native (raw_geometries[i]);
                }
                return Geometry.geometry_collection (geometries);
            default:
                clear_unknown_status ();
                throw new Error.UNSUPPORTED ("unknown geometry type %u", native.type);
            }
        }
    }

    public class JsonValue {
        private JsonValueType type;
        private bool bool_storage;
        private uint64 uint_storage;
        private int64 int_storage;
        private double double_storage;
        private uint8[] string_storage;
        private JsonValue[] array_values;
        private JsonMember[] object_members;
        private Raw.JsonValue[] array_natives;
        private Raw.JsonMember[] object_native_members;
        private Raw.JsonValue[] object_native_values;
        private JsonValue[] native_children;

        private JsonValue (JsonValueType type) {
            this.type = type;
            string_storage = new uint8[0];
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
            json.string_storage = copy_string_bytes (value);
            return json;
        }

        public static JsonValue string_utf8_bytes (uint8[] value) throws Error {
            Utf8String.validate_bytes (value);
            return JsonValue.string_utf8_bytes_unchecked (value);
        }

        private static JsonValue string_utf8_bytes_unchecked (uint8[] value) {
            var json = new JsonValue (JsonValueType.STRING);
            json.string_storage = copy_byte_array (value);
            return json;
        }

        public static JsonValue array_value (JsonValue[] values) throws Error {
            for (var index = 0; index < values.length; index++) {
                if (values[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("JSON array value at index %d is null", index);
                }
            }
            return JsonValue.array_value_unchecked (values);
        }

        private static JsonValue array_value_unchecked (JsonValue[] values) {
            var json = new JsonValue (JsonValueType.ARRAY);
            json.array_values = new JsonValue[values.length];
            for (var index = 0; index < values.length; index++) {
                json.array_values[index] = values[index];
            }
            return json;
        }

        public static JsonValue object_value (JsonMember[] members) throws Error {
            for (var index = 0; index < members.length; index++) {
                if (members[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("JSON object member at index %d is null", index);
                }
            }
            return JsonValue.object_value_unchecked (members);
        }

        private static JsonValue object_value_unchecked (JsonMember[] members) {
            var json = new JsonValue (JsonValueType.OBJECT);
            json.object_members = new JsonMember[members.length];
            for (var index = 0; index < members.length; index++) {
                json.object_members[index] = members[index];
            }
            return json;
        }

        public JsonValueType value_type { get { return type; } }

        public bool get_bool () throws Error {
            if (type != JsonValueType.BOOL) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("JSON value is not a bool");
            }
            return bool_storage;
        }

        public int64 get_int () throws Error {
            if (type != JsonValueType.INT) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("JSON value is not an int");
            }
            return int_storage;
        }

        public uint64 get_uint () throws Error {
            if (type != JsonValueType.UINT) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("JSON value is not a uint");
            }
            return uint_storage;
        }

        public double get_double () throws Error {
            if (type != JsonValueType.DOUBLE) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("JSON value is not a double");
            }
            return double_storage;
        }

        public string get_string () throws Error {
            if (type != JsonValueType.STRING) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("JSON value is not a string");
            }
            return string_from_bytes (string_storage);
        }

        public uint8[] get_string_utf8_bytes () throws Error {
            if (type != JsonValueType.STRING) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("JSON value is not a string");
            }
            var copied = new uint8[string_storage.length];
            for (var index = 0; index < string_storage.length; index++) {
                copied[index] = string_storage[index];
            }
            return copied;
        }

        public JsonValue[] get_array_values () throws Error {
            if (type != JsonValueType.ARRAY) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("JSON value is not an array");
            }
            var copied = new JsonValue[array_values.length];
            for (var index = 0; index < array_values.length; index++) {
                copied[index] = array_values[index];
            }
            return copied;
        }

        public JsonMember[] get_object_members () throws Error {
            if (type != JsonValueType.OBJECT) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("JSON value is not an object");
            }
            var copied = new JsonMember[object_members.length];
            for (var index = 0; index < object_members.length; index++) {
                copied[index] = object_members[index];
            }
            return copied;
        }

        public JsonValue copy () {
            switch (type) {
            case JsonValueType.BOOL:
                return JsonValue.bool_value (bool_storage);
            case JsonValueType.UINT:
                return JsonValue.uint_value (uint_storage);
            case JsonValueType.INT:
                return JsonValue.int_value (int_storage);
            case JsonValueType.DOUBLE:
                return JsonValue.double_value (double_storage);
            case JsonValueType.STRING:
                return JsonValue.string_utf8_bytes_unchecked (string_storage);
            case JsonValueType.ARRAY:
                JsonValue[] values = new JsonValue[array_values.length];
                for (var index = 0; index < array_values.length; index++) {
                    values[index] = array_values[index].copy ();
                }
                return JsonValue.array_value_unchecked (values);
            case JsonValueType.OBJECT:
                JsonMember[] members = new JsonMember[object_members.length];
                for (var index = 0; index < object_members.length; index++) {
                    members[index] = object_members[index].copy ();
                }
                return JsonValue.object_value_unchecked (members);
            case JsonValueType.NULL:
            default:
                return JsonValue.null_value ();
            }
        }

        public bool equal (JsonValue other) {
            if (type != other.type) {
                return false;
            }
            switch (type) {
            case JsonValueType.BOOL:
                return bool_storage == other.bool_storage;
            case JsonValueType.UINT:
                return uint_storage == other.uint_storage;
            case JsonValueType.INT:
                return int_storage == other.int_storage;
            case JsonValueType.DOUBLE:
                return double_storage == other.double_storage;
            case JsonValueType.STRING:
                if (string_storage.length != other.string_storage.length) {
                    return false;
                }
                for (var index = 0; index < string_storage.length; index++) {
                    if (string_storage[index] != other.string_storage[index]) {
                        return false;
                    }
                }
                return true;
            case JsonValueType.ARRAY:
                if (array_values.length != other.array_values.length) {
                    return false;
                }
                for (var index = 0; index < array_values.length; index++) {
                    if (!array_values[index].equal (other.array_values[index])) {
                        return false;
                    }
                }
                return true;
            case JsonValueType.OBJECT:
                if (object_members.length != other.object_members.length) {
                    return false;
                }
                for (var index = 0; index < object_members.length; index++) {
                    if (!object_members[index].equal (other.object_members[index])) {
                        return false;
                    }
                }
                return true;
            case JsonValueType.NULL:
            default:
                return true;
            }
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
                return JsonValue.string_utf8_bytes (copy_string_view_bytes (native.string_value));
            case JsonValueType.ARRAY:
                if (native.array_value.value_count > 0 && native.array_value.values == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("native JSON array values are null");
                }
                JsonValue[] values = new JsonValue[native.array_value.value_count];
                Raw.JsonValue* raw_values = (Raw.JsonValue*) native.array_value.values;
                for (size_t i = 0; i < native.array_value.value_count; i++) {
                    values[i] = JsonValue.from_native (raw_values[i]);
                }
                return JsonValue.array_value (values);
            case JsonValueType.OBJECT:
                if (native.object_value.member_count > 0 && native.object_value.members == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("native JSON object members are null");
                }
                JsonMember[] members = new JsonMember[native.object_value.member_count];
                for (size_t i = 0; i < native.object_value.member_count; i++) {
                    Raw.JsonMember member = native.object_value.members[i];
                    Raw.JsonValue* value = (Raw.JsonValue*) member.value;
                    if (value == null) {
                        clear_unknown_status ();
                        throw new Error.INVALID_ARGUMENT ("native JSON object member value is null");
                    }
                    members[i] = new JsonMember.from_utf8_bytes (copy_string_view_bytes (member.key), JsonValue.from_native (value[0]));
                }
                return JsonValue.object_value (members);
            case JsonValueType.NULL:
                return JsonValue.null_value ();
            default:
                clear_unknown_status ();
                throw new Error.UNSUPPORTED ("unknown native JSON value type %u", native.type);
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
                native.string_value = byte_string_view (string_storage);
                break;
            case JsonValueType.ARRAY:
                array_natives = new Raw.JsonValue[array_values.length];
                native_children = new JsonValue[array_values.length];
                for (var i = 0; i < array_values.length; i++) {
                    native_children[i] = array_values[i].copy_for_native ();
                    array_natives[i] = native_children[i].to_native ();
                }
                native.array_value = Raw.JsonArray () { values = (void*) array_natives, value_count = array_natives.length };
                break;
            case JsonValueType.OBJECT:
                object_native_members = new Raw.JsonMember[object_members.length];
                object_native_values = new Raw.JsonValue[object_members.length];
                native_children = new JsonValue[object_members.length];
                for (var i = 0; i < object_members.length; i++) {
                    native_children[i] = object_members[i].value.copy_for_native ();
                    object_native_values[i] = native_children[i].to_native ();
                    object_native_members[i] = Raw.JsonMember () { key = object_members[i].to_native_key (), value = (void*) &object_native_values[i] };
                }
                native.object_value = Raw.JsonObject () { members = object_native_members, member_count = object_native_members.length };
                break;
            default:
                break;
            }
            return native;
        }

        private JsonValue copy_for_native () {
            var copied = new JsonValue (type);
            copied.bool_storage = bool_storage;
            copied.uint_storage = uint_storage;
            copied.int_storage = int_storage;
            copied.double_storage = double_storage;
            copied.string_storage = string_storage;
            copied.array_values = array_values;
            copied.object_members = object_members;
            return copied;
        }
    }

    public class JsonMember {
        private uint8[] key_storage;
        public JsonValue value { get; private set; }

        public JsonMember (string key, JsonValue? value) throws Error {
            if (value == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("JSON object member value is null");
            }
            key_storage = copy_string_bytes (key);
            this.value = value;
        }

        public JsonMember.from_utf8_bytes (uint8[] key, JsonValue? value) throws Error {
            Utf8String.validate_bytes (key);
            if (value == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("JSON object member value is null");
            }
            key_storage = copy_byte_array (key);
            this.value = value;
        }

        private JsonMember.from_utf8_bytes_unchecked (uint8[] key, JsonValue value) {
            key_storage = copy_byte_array (key);
            this.value = value;
        }

        public string get_key () throws Error {
            return string_from_bytes (key_storage);
        }

        public uint8[] get_key_utf8_bytes () {
            var copied = new uint8[key_storage.length];
            for (var index = 0; index < key_storage.length; index++) {
                copied[index] = key_storage[index];
            }
            return copied;
        }

        public JsonMember copy () {
            return new JsonMember.from_utf8_bytes_unchecked (key_storage, value.copy ());
        }

        public bool equal (JsonMember other) {
            if (key_storage.length != other.key_storage.length || !value.equal (other.value)) {
                return false;
            }
            for (var index = 0; index < key_storage.length; index++) {
                if (key_storage[index] != other.key_storage[index]) {
                    return false;
                }
            }
            return true;
        }

        internal Raw.StringView to_native_key () {
            return byte_string_view (key_storage);
        }
    }

    public class FeatureIdentifier {
        private uint32 raw_type;
        private uint64 uint_storage;
        private int64 int_storage;
        private double double_storage;
        private uint8[] string_storage;

        private FeatureIdentifier (FeatureIdentifierType type) {
            raw_type = (uint32) type;
            string_storage = new uint8[0];
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
            identifier.string_storage = copy_string_bytes (value);
            return identifier;
        }

        public static FeatureIdentifier string_utf8_bytes (uint8[] value) throws Error {
            Utf8String.validate_bytes (value);
            return FeatureIdentifier.string_utf8_bytes_unchecked (value);
        }

        private static FeatureIdentifier string_utf8_bytes_unchecked (uint8[] value) {
            var identifier = new FeatureIdentifier (FeatureIdentifierType.STRING);
            identifier.string_storage = copy_byte_array (value);
            return identifier;
        }

        public FeatureIdentifierType value_type { get { return (FeatureIdentifierType) raw_type; } }
        public uint32 raw_value_type { get { return raw_type; } }

        public uint64 get_uint () throws Error {
            if (raw_type != (uint32) FeatureIdentifierType.UINT) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("feature identifier is not a uint");
            }
            return uint_storage;
        }

        public int64 get_int () throws Error {
            if (raw_type != (uint32) FeatureIdentifierType.INT) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("feature identifier is not an int");
            }
            return int_storage;
        }

        public double get_double () throws Error {
            if (raw_type != (uint32) FeatureIdentifierType.DOUBLE) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("feature identifier is not a double");
            }
            return double_storage;
        }

        public string get_string () throws Error {
            if (raw_type != (uint32) FeatureIdentifierType.STRING) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("feature identifier is not a string");
            }
            return string_from_bytes (string_storage);
        }

        public uint8[] get_string_utf8_bytes () throws Error {
            if (raw_type != (uint32) FeatureIdentifierType.STRING) {
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("feature identifier is not a string");
            }
            var copied = new uint8[string_storage.length];
            for (var index = 0; index < string_storage.length; index++) {
                copied[index] = string_storage[index];
            }
            return copied;
        }

        public FeatureIdentifier copy () {
            switch ((FeatureIdentifierType) raw_type) {
            case FeatureIdentifierType.UINT:
                return FeatureIdentifier.uint_value (uint_storage);
            case FeatureIdentifierType.INT:
                return FeatureIdentifier.int_value (int_storage);
            case FeatureIdentifierType.DOUBLE:
                return FeatureIdentifier.double_value (double_storage);
            case FeatureIdentifierType.STRING:
                return FeatureIdentifier.string_utf8_bytes_unchecked (string_storage);
            case FeatureIdentifierType.NULL:
                return FeatureIdentifier.none ();
            default:
                assert_not_reached ();
            }
        }

        public bool equal (FeatureIdentifier other) {
            if (raw_type != other.raw_type) {
                return false;
            }
            switch ((FeatureIdentifierType) raw_type) {
            case FeatureIdentifierType.UINT:
                return uint_storage == other.uint_storage;
            case FeatureIdentifierType.INT:
                return int_storage == other.int_storage;
            case FeatureIdentifierType.DOUBLE:
                return double_storage == other.double_storage;
            case FeatureIdentifierType.STRING:
                if (string_storage.length != other.string_storage.length) {
                    return false;
                }
                for (var index = 0; index < string_storage.length; index++) {
                    if (string_storage[index] != other.string_storage[index]) {
                        return false;
                    }
                }
                return true;
            case FeatureIdentifierType.NULL:
                return true;
            default:
                return false;
            }
        }

        internal static FeatureIdentifier from_native (Raw.Feature native) throws Error {
            switch ((FeatureIdentifierType) native.identifier_type) {
            case FeatureIdentifierType.UINT:
                return FeatureIdentifier.uint_value (native.identifier_uint_value);
            case FeatureIdentifierType.INT:
                return FeatureIdentifier.int_value (native.identifier_int_value);
            case FeatureIdentifierType.DOUBLE:
                return FeatureIdentifier.double_value (native.identifier_double_value);
            case FeatureIdentifierType.STRING:
                return FeatureIdentifier.string_utf8_bytes (copy_string_view_bytes (native.identifier_string_value));
            case FeatureIdentifierType.NULL:
                return FeatureIdentifier.none ();
            default:
                clear_unknown_status ();
                throw new Error.UNSUPPORTED ("unknown native feature identifier type %u", native.identifier_type);
            }
        }

        internal void apply_to_native (ref Raw.Feature native) throws Error {
            native.identifier_type = raw_type;
            switch ((FeatureIdentifierType) raw_type) {
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
                native.identifier_string_value = byte_string_view (string_storage);
                break;
            case FeatureIdentifierType.NULL:
                break;
            default:
                clear_unknown_status ();
                throw new Error.UNSUPPORTED ("unknown feature identifier type %u", raw_type);
            }
        }
    }

    public class FeatureStateSelector {
        private Utf8String source_id;
        private Utf8String? source_layer_id;
        private Utf8String? feature_id;
        private Utf8String? state_key;

        public FeatureStateSelector (string source_id) {
            this.source_id = new Utf8String (source_id);
        }

        public FeatureStateSelector.from_utf8 (Utf8String source_id) {
            this.source_id = source_id.copy ();
        }

        public void set_source_layer_id (string value) {
            source_layer_id = new Utf8String (value);
        }

        public void set_source_layer_id_utf8 (Utf8String value) {
            source_layer_id = value.copy ();
        }

        public void set_feature_id (string value) {
            feature_id = new Utf8String (value);
        }

        public void set_feature_id_utf8 (Utf8String value) {
            feature_id = value.copy ();
        }

        public void set_state_key (string value) {
            state_key = new Utf8String (value);
        }

        public void set_state_key_utf8 (Utf8String value) {
            state_key = value.copy ();
        }

        public FeatureStateSelector copy () {
            var copied = new FeatureStateSelector.from_utf8 (source_id);
            copied.source_layer_id = source_layer_id == null ? null : source_layer_id.copy ();
            copied.feature_id = feature_id == null ? null : feature_id.copy ();
            copied.state_key = state_key == null ? null : state_key.copy ();
            return copied;
        }

        public bool equal (FeatureStateSelector other) {
            return source_id.equal (other.source_id)
                && (source_layer_id == null
                    ? other.source_layer_id == null
                    : other.source_layer_id != null && source_layer_id.equal (other.source_layer_id))
                && (feature_id == null
                    ? other.feature_id == null
                    : other.feature_id != null && feature_id.equal (other.feature_id))
                && (state_key == null
                    ? other.state_key == null
                    : other.state_key != null && state_key.equal (other.state_key));
        }

        internal Raw.FeatureStateSelector to_native () throws Error {
            Raw.FeatureStateSelector native = {};
            native.size = (uint32) sizeof (Raw.FeatureStateSelector);
            native.source_id = source_id.to_native ();
            if (source_layer_id != null) {
                native.fields |= (uint32) Raw.FeatureStateSelectorField.SOURCE_LAYER_ID;
                native.source_layer_id = source_layer_id.to_native ();
            }
            if (feature_id != null) {
                native.fields |= (uint32) Raw.FeatureStateSelectorField.FEATURE_ID;
                native.feature_id = feature_id.to_native ();
            }
            if (state_key != null) {
                native.fields |= (uint32) Raw.FeatureStateSelectorField.STATE_KEY;
                native.state_key = state_key.to_native ();
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
        private JsonValue[] property_value_storage;
        private Raw.JsonValue[] property_value_natives;
        private Raw.Feature native;

        public Feature (Geometry geometry, JsonMember[] properties, FeatureIdentifier? identifier = null) throws Error {
            if (geometry == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("feature geometry is null");
            }
            geometry_ref = geometry;
            this.properties = new JsonMember[properties.length];
            for (var index = 0; index < properties.length; index++) {
                if (properties[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("feature property at index %d is null", index);
                }
                this.properties[index] = properties[index];
            }
            this.identifier = identifier ?? FeatureIdentifier.none ();
        }

        public Geometry geometry { get { return geometry_ref; } }
        public JsonMember[] property_members {
            owned get {
                var copied = new JsonMember[properties.length];
                for (var index = 0; index < properties.length; index++) {
                    copied[index] = properties[index];
                }
                return copied;
            }
        }
        public FeatureIdentifier feature_identifier { get { return identifier; } }

        internal static Feature from_native (Raw.Feature native) throws Error {
            if (native.geometry == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("queried feature geometry is null");
            }
            JsonMember[] members = new JsonMember[native.property_count];
            if (native.property_count > 0 && native.properties == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("queried feature properties are null");
            }
            for (size_t i = 0; i < native.property_count; i++) {
                Raw.JsonMember member = native.properties[i];
                Raw.JsonValue* value = (Raw.JsonValue*) member.value;
                if (value == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("queried feature property value is null");
                }
                members[i] = new JsonMember.from_utf8_bytes (copy_string_view_bytes (member.key), JsonValue.from_native (value[0]));
            }
            return new Feature (Geometry.from_native (native.geometry[0]), members, FeatureIdentifier.from_native (native));
        }

        internal Raw.Feature to_native () throws Error {
            geometry_native = geometry_ref.to_native ();
            property_natives = new Raw.JsonMember[properties.length];
            property_value_storage = new JsonValue[properties.length];
            property_value_natives = new Raw.JsonValue[properties.length];
            for (var i = 0; i < properties.length; i++) {
                property_value_storage[i] = properties[i].value.copy ();
                property_value_natives[i] = property_value_storage[i].to_native ();
                property_natives[i] = Raw.JsonMember () { key = properties[i].to_native_key (), value = (void*) &property_value_natives[i] };
            }
            native = {};
            native.size = (uint32) sizeof (Raw.Feature);
            native.geometry = &geometry_native;
            native.properties = property_natives;
            native.property_count = property_natives.length;
            identifier.apply_to_native (ref native);
            return native;
        }

        public Feature copy () throws Error {
            JsonMember[] copied_properties = new JsonMember[properties.length];
            for (var index = 0; index < properties.length; index++) {
                copied_properties[index] = properties[index].copy ();
            }
            return new Feature (geometry_ref.copy (), copied_properties, identifier.copy ());
        }

        public bool equal (Feature other) {
            if (!geometry_ref.equal (other.geometry_ref)
                || !identifier.equal (other.identifier)
                || properties.length != other.properties.length) {
                return false;
            }
            for (var index = 0; index < properties.length; index++) {
                if (!properties[index].equal (other.properties[index])) {
                    return false;
                }
            }
            return true;
        }
    }

    public class FeatureCollection {
        private Feature[] features;
        private Feature[] native_feature_storage;
        private Raw.Feature[] feature_natives;
        private Raw.FeatureCollection native;

        public FeatureCollection (Feature[] features) throws Error {
            this.features = new Feature[features.length];
            for (var index = 0; index < features.length; index++) {
                if (features[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("feature collection item at index %d is null", index);
                }
                this.features[index] = features[index];
            }
        }

        public Feature[] feature_items {
            owned get {
                var copied = new Feature[features.length];
                for (var index = 0; index < features.length; index++) {
                    copied[index] = features[index];
                }
                return copied;
            }
        }

        internal Raw.FeatureCollection to_native () throws Error {
            native_feature_storage = new Feature[features.length];
            feature_natives = new Raw.Feature[features.length];
            for (var i = 0; i < features.length; i++) {
                native_feature_storage[i] = features[i].copy ();
                feature_natives[i] = native_feature_storage[i].to_native ();
            }
            native = Raw.FeatureCollection () { features = feature_natives, feature_count = feature_natives.length };
            return native;
        }

        internal static FeatureCollection from_native (Raw.FeatureCollection native) throws Error {
            Feature[] features = new Feature[native.feature_count];
            if (native.feature_count > 0 && native.features == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("feature collection data is null");
            }
            for (size_t i = 0; i < native.feature_count; i++) {
                features[i] = Feature.from_native (native.features[i]);
            }
            return new FeatureCollection (features);
        }

        public FeatureCollection copy () throws Error {
            Feature[] copied = new Feature[features.length];
            for (var index = 0; index < features.length; index++) {
                copied[index] = features[index].copy ();
            }
            return new FeatureCollection (copied);
        }

        public bool equal (FeatureCollection other) {
            if (features.length != other.features.length) {
                return false;
            }
            for (var index = 0; index < features.length; index++) {
                if (!features[index].equal (other.features[index])) {
                    return false;
                }
            }
            return true;
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

        public static GeoJson geometry (Geometry geometry) throws Error {
            if (geometry == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("GeoJSON geometry is null");
            }
            return new GeoJson.for_geometry (geometry);
        }

        public static GeoJson feature (Feature feature) throws Error {
            if (feature == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("GeoJSON feature is null");
            }
            return new GeoJson.for_feature (feature);
        }

        public static GeoJson feature_collection (FeatureCollection feature_collection) throws Error {
            if (feature_collection == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("GeoJSON feature collection is null");
            }
            return new GeoJson.for_feature_collection (feature_collection);
        }

        public GeoJson copy () throws Error {
            if (native.type == (uint32) GeoJsonType.FEATURE) {
                return GeoJson.feature (feature_ref.copy ());
            }
            if (native.type == (uint32) GeoJsonType.FEATURE_COLLECTION) {
                return GeoJson.feature_collection (feature_collection_ref.copy ());
            }
            return GeoJson.geometry (geometry_ref.copy ());
        }

        public bool equal (GeoJson other) {
            if (native.type != other.native.type) {
                return false;
            }
            if (native.type == (uint32) GeoJsonType.FEATURE) {
                return feature_ref.equal (other.feature_ref);
            }
            if (native.type == (uint32) GeoJsonType.FEATURE_COLLECTION) {
                return feature_collection_ref.equal (other.feature_collection_ref);
            }
            return geometry_ref.equal (other.geometry_ref);
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
