namespace MaplibreNative {
    public errordomain Error {
        INVALID_ARGUMENT,
        INVALID_STATE,
        WRONG_THREAD,
        UNSUPPORTED,
        NATIVE_ERROR,
        ABI_MISMATCH,
        UNKNOWN_STATUS
    }

    [Flags]
    public enum RenderBackendFlags {
        METAL = 1 << 0,
        VULKAN = 1 << 1,
        OPENGL = 1 << 2
    }

    [Flags]
    public enum OpenGLContextProviderFlags {
        WGL = 1 << 0,
        EGL = 1 << 1
    }

    public enum OpenGLContextPlatform {
        UNSPECIFIED = 0,
        WGL = 1,
        EGL = 2
    }

    public enum NetworkStatus {
        ONLINE = 1,
        OFFLINE = 2,
        UNKNOWN = 0
    }

    public enum MapMode {
        CONTINUOUS = 0,
        STATIC = 1,
        TILE = 2
    }

    [Flags]
    public enum MapDebugOptions {
        NONE = 0,
        TILE_BORDERS = 1 << 1,
        PARSE_STATUS = 1 << 2,
        TIMESTAMPS = 1 << 3,
        COLLISION = 1 << 4,
        OVERDRAW = 1 << 5,
        STENCIL_CLIP = 1 << 6,
        DEPTH_BUFFER = 1 << 7
    }

    public enum NorthOrientation {
        UP = 0,
        RIGHT = 1,
        DOWN = 2,
        LEFT = 3
    }

    public enum ConstrainMode {
        NONE = 0,
        HEIGHT_ONLY = 1,
        WIDTH_AND_HEIGHT = 2,
        SCREEN = 3
    }

    public enum ViewportMode {
        DEFAULT = 0,
        FLIPPED_Y = 1
    }

    public enum TileLodMode {
        DEFAULT = 0,
        DISTANCE = 1
    }

    public struct UnitBezier {
        public double x1;
        public double y1;
        public double x2;
        public double y2;

        public UnitBezier (double x1, double y1, double x2, double y2) {
            this.x1 = x1;
            this.y1 = y1;
            this.x2 = x2;
            this.y2 = y2;
        }

        internal Raw.UnitBezier to_native () {
            return Raw.UnitBezier () { x1 = x1, y1 = y1, x2 = x2, y2 = y2 };
        }
    }

    public enum AmbientCacheOperation {
        RESET_DATABASE = 1,
        PACK_DATABASE = 2,
        INVALIDATE = 3,
        CLEAR = 4
    }

    public enum RuntimeEventType {
        MAP_CAMERA_WILL_CHANGE = 1,
        MAP_CAMERA_IS_CHANGING = 2,
        MAP_CAMERA_DID_CHANGE = 3,
        MAP_STYLE_LOADED = 4,
        MAP_LOADING_STARTED = 5,
        MAP_LOADING_FINISHED = 6,
        MAP_LOADING_FAILED = 7,
        MAP_IDLE = 8,
        MAP_RENDER_UPDATE_AVAILABLE = 9,
        MAP_RENDER_ERROR = 10,
        MAP_STILL_IMAGE_FINISHED = 11,
        MAP_STILL_IMAGE_FAILED = 12,
        MAP_RENDER_FRAME_STARTED = 13,
        MAP_RENDER_FRAME_FINISHED = 14,
        MAP_RENDER_MAP_STARTED = 15,
        MAP_RENDER_MAP_FINISHED = 16,
        MAP_STYLE_IMAGE_MISSING = 17,
        MAP_TILE_ACTION = 18,
        OFFLINE_REGION_STATUS_CHANGED = 19,
        OFFLINE_REGION_RESPONSE_ERROR = 20,
        OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 21,
        OFFLINE_OPERATION_COMPLETED = 22,
        UNKNOWN = 0
    }

    public enum RuntimeEventSourceType {
        RUNTIME = 0,
        MAP = 1,
        UNKNOWN = 2
    }

    public enum LogSeverity {
        INFO = 1,
        WARNING = 2,
        ERROR = 3,
        UNKNOWN = 0
    }

    [Flags]
    public enum LogSeverityMask {
        INFO = 1 << LogSeverity.INFO,
        WARNING = 1 << LogSeverity.WARNING,
        ERROR = 1 << LogSeverity.ERROR,
        DEFAULT = INFO | WARNING,
        ALL = INFO | WARNING | ERROR
    }

    public enum ResourceKind {
        UNKNOWN = 0,
        STYLE = 1,
        SOURCE = 2,
        TILE = 3,
        GLYPHS = 4,
        SPRITE_IMAGE = 5,
        SPRITE_JSON = 6,
        IMAGE = 7
    }

    public enum ResourceProviderDecision {
        PASS_THROUGH = 0,
        HANDLE = 1
    }

    public enum RenderedQueryGeometryType {
        POINT = 1,
        BOX = 2,
        LINE_STRING = 3
    }

    public enum LogEvent {
        GENERAL = 0,
        SETUP = 1,
        SHADER = 2,
        PARSE_STYLE = 3,
        PARSE_TILE = 4,
        RENDER = 5,
        STYLE = 6,
        DATABASE = 7,
        HTTP_REQUEST = 8,
        SPRITE = 9,
        IMAGE = 10,
        OPENGL = 11,
        JNI = 12,
        ANDROID = 13,
        CRASH = 14,
        GLYPH = 15,
        TIMING = 16,
        UNKNOWN = 255
    }

    public struct LatLng {
        public double latitude;
        public double longitude;

        public LatLng (double latitude, double longitude) {
            this.latitude = latitude;
            this.longitude = longitude;
        }

        internal Raw.LatLng to_native () {
            return Raw.LatLng () { latitude = latitude, longitude = longitude };
        }

        internal static LatLng from_native (Raw.LatLng native) {
            return LatLng (native.latitude, native.longitude);
        }
    }

    public struct LatLngBounds {
        public LatLng southwest;
        public LatLng northeast;

        public LatLngBounds (LatLng southwest, LatLng northeast) {
            this.southwest = southwest;
            this.northeast = northeast;
        }

        internal Raw.LatLngBounds to_native () {
            return Raw.LatLngBounds () { southwest = southwest.to_native (), northeast = northeast.to_native () };
        }

        internal static LatLngBounds from_native (Raw.LatLngBounds native) {
            return LatLngBounds (LatLng.from_native (native.southwest), LatLng.from_native (native.northeast));
        }
    }

    public struct ScreenPoint {
        public double x;
        public double y;

        public ScreenPoint (double x, double y) {
            this.x = x;
            this.y = y;
        }

        internal Raw.ScreenPoint to_native () {
            return Raw.ScreenPoint () { x = x, y = y };
        }

        internal static ScreenPoint from_native (Raw.ScreenPoint native) {
            return ScreenPoint (native.x, native.y);
        }
    }

    public struct ScreenBox {
        public ScreenPoint min;
        public ScreenPoint max;

        public ScreenBox (ScreenPoint min, ScreenPoint max) {
            this.min = min;
            this.max = max;
        }

        internal Raw.ScreenBox to_native () {
            return Raw.ScreenBox () { min = min.to_native (), max = max.to_native () };
        }
    }

    public struct Vec3 {
        public double x;
        public double y;
        public double z;

        public Vec3 (double x, double y, double z) {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        internal Raw.Vec3 to_native () {
            return Raw.Vec3 () { x = x, y = y, z = z };
        }

        internal static Vec3 from_native (Raw.Vec3 native) {
            return Vec3 (native.x, native.y, native.z);
        }
    }

    public struct Quaternion {
        public double x;
        public double y;
        public double z;
        public double w;

        public Quaternion (double x, double y, double z, double w) {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        internal Raw.Quaternion to_native () {
            return Raw.Quaternion () { x = x, y = y, z = z, w = w };
        }

        internal static Quaternion from_native (Raw.Quaternion native) {
            return Quaternion (native.x, native.y, native.z, native.w);
        }
    }

    public struct EdgeInsets {
        public double top;
        public double left;
        public double bottom;
        public double right;

        public EdgeInsets (double top, double left, double bottom, double right) {
            this.top = top;
            this.left = left;
            this.bottom = bottom;
            this.right = right;
        }

        internal Raw.EdgeInsets to_native () {
            return Raw.EdgeInsets () { top = top, left = left, bottom = bottom, right = right };
        }

        internal static EdgeInsets from_native (Raw.EdgeInsets native) {
            return EdgeInsets (native.top, native.left, native.bottom, native.right);
        }
    }

    public struct ProjectedMeters {
        public double northing;
        public double easting;

        public ProjectedMeters (double northing, double easting) {
            this.northing = northing;
            this.easting = easting;
        }

        internal Raw.ProjectedMeters to_native () {
            return Raw.ProjectedMeters () { northing = northing, easting = easting };
        }

        internal static ProjectedMeters from_native (Raw.ProjectedMeters native) {
            return ProjectedMeters (native.northing, native.easting);
        }
    }

    public class NativePointer {
        private size_t bits;

        private NativePointer (size_t bits) {
            this.bits = bits;
        }

        /**
         * Creates a non-owning backend pointer.
         *
         * The caller keeps the pointed-to native resource valid and synchronized
         * for every C API borrow window that uses this value.
         */
        public static NativePointer borrowed (size_t bits) {
            return new NativePointer (bits);
        }

        public bool is_null () {
            return bits == 0;
        }

        public NativePointer copy () {
            return new NativePointer (bits);
        }

        public bool equal (NativePointer other) {
            return bits == other.bits;
        }

        internal void* to_native () throws Error {
            if (bits == 0) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("native pointer is null");
            }
            return (void*) bits;
        }
    }

    public delegate void FrameResourceValidator () throws Error;

    internal class FrameAccessState {
        private Mutex mutex;
        private bool closed;
        private bool releasing;
        private uint active_accesses;
        private string resource_name;

        internal FrameAccessState (string resource_name) {
            this.resource_name = resource_name;
        }

        internal bool is_closed {
            get {
                mutex.lock ();
                var value = closed;
                mutex.unlock ();
                return value;
            }
        }

        internal FrameAccessLease acquire () throws Error {
            mutex.lock ();
            if (closed || releasing) {
                mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("%s is closed", resource_name);
            }
            active_accesses++;
            var lease = new FrameAccessLease (this);
            mutex.unlock ();
            return lease;
        }

        internal bool begin_close () throws Error {
            mutex.lock ();
            if (closed) {
                mutex.unlock ();
                return false;
            }
            if (releasing) {
                mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("%s release is already in progress", resource_name);
            }
            if (active_accesses > 0) {
                mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("%s has an active native-value borrow", resource_name);
            }
            releasing = true;
            mutex.unlock ();
            return true;
        }

        internal void finish_close (bool succeeded) {
            mutex.lock ();
            if (succeeded) {
                closed = true;
            }
            releasing = false;
            mutex.unlock ();
        }

        internal void release_access () {
            mutex.lock ();
            active_accesses--;
            mutex.unlock ();
        }
    }

    internal class FrameAccessLease {
        private FrameAccessState owner;

        internal FrameAccessLease (FrameAccessState owner) {
            this.owner = owner;
        }

        internal void keep_alive () {}

        ~FrameAccessLease () {
            owner.release_access ();
        }
    }

    public class FrameNativePointer {
        private size_t bits;
        private FrameResourceValidator validator;

        internal FrameNativePointer (size_t bits, owned FrameResourceValidator validator) {
            this.bits = bits;
            this.validator = (owned) validator;
        }

        public size_t get_bits () throws Error {
            validator ();
            return bits;
        }

        public bool is_null () throws Error {
            return get_bits () == 0;
        }

    }

    public class FrameUInt32 {
        private uint32 value;
        private FrameResourceValidator validator;

        internal FrameUInt32 (uint32 value, owned FrameResourceValidator validator) {
            this.value = value;
            this.validator = (owned) validator;
        }

        public uint32 get () throws Error {
            validator ();
            return value;
        }
    }

    public class Utf8String {
        private uint8[] bytes;

        public Utf8String (string value) {
            bytes = copy_string_bytes (value);
        }

        public Utf8String.from_bytes (uint8[] value) throws Error {
            validate_bytes (value);
            bytes = copy_byte_array (value);
        }

        private Utf8String.from_owned_bytes (owned uint8[] value) {
            bytes = (owned) value;
        }

        internal static void validate_bytes (uint8[] value) throws Error {
            var index = 0;
            while (index < value.length) {
                uint8 first = value[index];
                var count = 0;
                if (first <= 0x7f) {
                    count = 1;
                } else if (first >= 0xc2 && first <= 0xdf) {
                    count = 2;
                } else if (first >= 0xe0 && first <= 0xef) {
                    count = 3;
                } else if (first >= 0xf0 && first <= 0xf4) {
                    count = 4;
                } else {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("UTF-8 input contains an invalid leading byte");
                }
                if (index + count > value.length) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("UTF-8 input ends inside a code point");
                }
                for (var continuation = 1; continuation < count; continuation++) {
                    if ((value[index + continuation] & 0xc0) != 0x80) {
                        clear_unknown_status ();
                        throw new Error.INVALID_ARGUMENT ("UTF-8 input contains an invalid continuation byte");
                    }
                }
                if ((first == 0xe0 && value[index + 1] < 0xa0)
                    || (first == 0xed && value[index + 1] >= 0xa0)
                    || (first == 0xf0 && value[index + 1] < 0x90)
                    || (first == 0xf4 && value[index + 1] >= 0x90)) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("UTF-8 input contains an invalid code point");
                }
                index += count;
            }
        }

        public uint length {
            get { return bytes.length; }
        }

        public uint8[] to_bytes () {
            return copy_byte_array (bytes);
        }

        public string to_string () throws Error {
            return string_from_bytes (bytes);
        }

        public string? to_string_or_null () {
            try {
                return string_from_bytes (bytes);
            } catch (Error error) {
                return null;
            }
        }

        public Utf8String copy () {
            return new Utf8String.from_owned_bytes (copy_byte_array (bytes));
        }

        public bool equal (Utf8String other) {
            if (bytes.length != other.bytes.length) {
                return false;
            }
            for (var index = 0; index < bytes.length; index++) {
                if (bytes[index] != other.bytes[index]) {
                    return false;
                }
            }
            return true;
        }

        internal Raw.StringView to_native () {
            return byte_string_view (bytes);
        }
    }

    public class StringList {
        private Utf8String[] values;

        internal StringList (owned Utf8String[] values) {
            this.values = (owned) values;
        }

        public uint length {
            get { return values.length; }
        }

        public string get (uint index) throws Error {
            if (index >= values.length) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("string list index is out of range");
            }
            return values[index].to_string ();
        }

        public Utf8String get_utf8 (uint index) throws Error {
            if (index >= values.length) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("string list index is out of range");
            }
            return values[index].copy ();
        }

        public string[] to_array () throws Error {
            string[] copied = new string[values.length];
            for (var index = 0; index < values.length; index++) {
                copied[index] = values[index].to_string ();
            }
            return copied;
        }

        public Utf8String[] to_utf8_array () {
            Utf8String[] copied = new Utf8String[values.length];
            for (var index = 0; index < values.length; index++) {
                copied[index] = values[index].copy ();
            }
            return copied;
        }

        public bool contains (string value) {
            return contains_utf8 (new Utf8String (value));
        }

        public bool contains_utf8 (Utf8String value) {
            foreach (var item in values) {
                if (item.equal (value)) {
                    return true;
                }
            }
            return false;
        }
    }
}
