namespace MaplibreNative {
    public enum StyleSourceType {
        UNKNOWN = 0,
        VECTOR = 1,
        RASTER = 2,
        RASTER_DEM = 3,
        GEOJSON = 4,
        IMAGE = 5,
        VIDEO = 6,
        ANNOTATIONS = 7,
        CUSTOM_VECTOR = 8
    }

    public enum StyleTileScheme {
        XYZ = 0,
        TMS = 1
    }

    public enum StyleVectorTileEncoding {
        MVT = 0,
        MLT = 1
    }

    public enum StyleRasterDemEncoding {
        MAPBOX = 0,
        TERRARIUM = 1
    }

    public enum LocationIndicatorImageKind {
        TOP = 0,
        BEARING = 1,
        SHADOW = 2
    }

    public struct CanonicalTileId {
        public uint32 z;
        public uint32 x;
        public uint32 y;

        public CanonicalTileId (uint32 z, uint32 x, uint32 y) {
            this.z = z;
            this.x = x;
            this.y = y;
        }

        internal Raw.CanonicalTileId to_native () {
            return Raw.CanonicalTileId () { z = z, x = x, y = y };
        }

        internal static CanonicalTileId from_native (Raw.CanonicalTileId native) {
            return CanonicalTileId (native.z, native.x, native.y);
        }
    }

    /**
     * Receives tile fetch or cancellation work on arbitrary native worker
     * threads, potentially concurrently with map-owner calls and other tile
     * callbacks. Implementations return promptly, never throw across the
     * callback boundary, and queue thread-affine map work to the owner thread
     * instead of calling map APIs directly. The map retains callbacks through
     * source removal, completed style replacement, or map close and waits for
     * in-flight calls before releasing them; cancellation is best-effort and
     * may repeat or race a fetch.
     */
    public delegate void CustomGeometryTileCallback (CanonicalTileId tile_id);

    internal class CustomGeometrySourceRegistration {
        public Utf8String source_id { get; private set; }
        public CustomGeometrySourceOptions options { get; private set; }
        private Mutex mutex;
        private Cond idle;
        private bool closing;
        private uint active_callbacks;

        public CustomGeometrySourceRegistration (Utf8String source_id, CustomGeometrySourceOptions options) {
            this.source_id = source_id.copy ();
            this.options = options;
        }

        internal bool begin_callback () {
            mutex.lock ();
            if (closing) {
                mutex.unlock ();
                return false;
            }
            active_callbacks++;
            mutex.unlock ();
            return true;
        }

        internal void end_callback () {
            mutex.lock ();
            active_callbacks--;
            if (closing && active_callbacks == 0) {
                idle.broadcast ();
            }
            mutex.unlock ();
        }

        internal void close () {
            mutex.lock ();
            closing = true;
            while (active_callbacks > 0) {
                idle.wait (mutex);
            }
            mutex.unlock ();
        }

        internal void invoke_fetch_tile (Raw.CanonicalTileId tile_id) {
            if (!begin_callback ()) {
                return;
            }
            try {
                options.invoke_fetch_tile (tile_id);
            } finally {
                end_callback ();
            }
        }

        internal void invoke_cancel_tile (Raw.CanonicalTileId tile_id) {
            if (!begin_callback ()) {
                return;
            }
            try {
                options.invoke_cancel_tile (tile_id);
            } finally {
                end_callback ();
            }
        }
    }

    public class CustomGeometrySourceOptions {
        private CustomGeometryTileCallback fetch_tile;
        private CustomGeometryTileCallback? cancel_tile;
        public double? min_zoom { get; set; }
        public double? max_zoom { get; set; }
        public double? tolerance { get; set; }
        public uint32? tile_size { get; set; }
        public uint32? buffer { get; set; }
        public bool? clip { get; set; }
        public bool? wrap { get; set; }

        public CustomGeometrySourceOptions (owned CustomGeometryTileCallback fetch_tile, owned CustomGeometryTileCallback? cancel_tile = null) {
            this.fetch_tile = (owned) fetch_tile;
            this.cancel_tile = (owned) cancel_tile;
        }

        internal Raw.CustomGeometrySourceOptions to_native (CustomGeometrySourceRegistration registration) {
            Raw.CustomGeometrySourceOptions options = Raw.custom_geometry_source_options_default ();
            options.fetch_tile = custom_geometry_fetch_tile_trampoline;
            if (cancel_tile != null) {
                options.cancel_tile = custom_geometry_cancel_tile_trampoline;
            }
            options.user_data = (void*) registration;
            if (min_zoom != null) {
                options.min_zoom = min_zoom;
                options.fields |= (uint32) Raw.CustomGeometrySourceOptionField.MIN_ZOOM;
            }
            if (max_zoom != null) {
                options.max_zoom = max_zoom;
                options.fields |= (uint32) Raw.CustomGeometrySourceOptionField.MAX_ZOOM;
            }
            if (tolerance != null) {
                options.tolerance = tolerance;
                options.fields |= (uint32) Raw.CustomGeometrySourceOptionField.TOLERANCE;
            }
            if (tile_size != null) {
                options.tile_size = tile_size;
                options.fields |= (uint32) Raw.CustomGeometrySourceOptionField.TILE_SIZE;
            }
            if (buffer != null) {
                options.buffer = buffer;
                options.fields |= (uint32) Raw.CustomGeometrySourceOptionField.BUFFER;
            }
            if (clip != null) {
                options.clip = clip;
                options.fields |= (uint32) Raw.CustomGeometrySourceOptionField.CLIP;
            }
            if (wrap != null) {
                options.wrap = wrap;
                options.fields |= (uint32) Raw.CustomGeometrySourceOptionField.WRAP;
            }
            return options;
        }

        internal void invoke_fetch_tile (Raw.CanonicalTileId tile_id) {
            fetch_tile (CanonicalTileId.from_native (tile_id));
        }

        internal void invoke_cancel_tile (Raw.CanonicalTileId tile_id) {
            if (cancel_tile != null) {
                cancel_tile (CanonicalTileId.from_native (tile_id));
            }
        }
    }

    private static void custom_geometry_fetch_tile_trampoline (void* user_data, Raw.CanonicalTileId tile_id) {
        if (user_data == null) {
            return;
        }
        ((CustomGeometrySourceRegistration) user_data).invoke_fetch_tile (tile_id);
    }

    private static void custom_geometry_cancel_tile_trampoline (void* user_data, Raw.CanonicalTileId tile_id) {
        if (user_data == null) {
            return;
        }
        ((CustomGeometrySourceRegistration) user_data).invoke_cancel_tile (tile_id);
    }

    public class StyleTileSourceOptions {
        private Utf8String? attribution_storage;

        public double? min_zoom { get; set; }
        public double? max_zoom { get; set; }
        public string? attribution {
            owned get {
                return attribution_storage == null ? null : attribution_storage.to_string_or_null ();
            }
            set {
                attribution_storage = value == null ? null : new Utf8String (value);
            }
        }
        public StyleTileScheme? scheme { get; set; }
        public LatLngBounds? bounds { get; set; }
        public uint32? tile_size { get; set; }
        public StyleVectorTileEncoding? vector_encoding { get; set; }
        public StyleRasterDemEncoding? raster_encoding { get; set; }

        public void set_attribution_utf8 (Utf8String? value) {
            attribution_storage = value == null ? null : value.copy ();
        }

        public Utf8String? get_attribution_utf8 () {
            return attribution_storage == null ? null : attribution_storage.copy ();
        }

        public StyleTileSourceOptions copy () {
            var copied = new StyleTileSourceOptions ();
            copied.min_zoom = min_zoom;
            copied.max_zoom = max_zoom;
            copied.attribution_storage = attribution_storage == null ? null : attribution_storage.copy ();
            copied.scheme = scheme;
            copied.bounds = bounds;
            copied.tile_size = tile_size;
            copied.vector_encoding = vector_encoding;
            copied.raster_encoding = raster_encoding;
            return copied;
        }

        public bool equal (StyleTileSourceOptions other) {
            if (min_zoom != other.min_zoom
                || max_zoom != other.max_zoom
                || (attribution_storage == null) != (other.attribution_storage == null)
                || scheme != other.scheme
                || tile_size != other.tile_size
                || vector_encoding != other.vector_encoding
                || raster_encoding != other.raster_encoding
                || (bounds == null) != (other.bounds == null)) {
                return false;
            }
            if (attribution_storage != null
                && other.attribution_storage != null
                && !attribution_storage.equal (other.attribution_storage)) {
                return false;
            }
            if (bounds != null && other.bounds != null) {
                return bounds.southwest.latitude == other.bounds.southwest.latitude
                    && bounds.southwest.longitude == other.bounds.southwest.longitude
                    && bounds.northeast.latitude == other.bounds.northeast.latitude
                    && bounds.northeast.longitude == other.bounds.northeast.longitude;
            }
            return true;
        }

        internal Raw.StyleTileSourceOptions to_native () throws Error {
            Raw.StyleTileSourceOptions options = Raw.style_tile_source_options_default ();
            if (min_zoom != null) {
                options.min_zoom = min_zoom;
                options.fields |= (uint32) Raw.StyleTileSourceOptionField.MIN_ZOOM;
            }
            if (max_zoom != null) {
                options.max_zoom = max_zoom;
                options.fields |= (uint32) Raw.StyleTileSourceOptionField.MAX_ZOOM;
            }
            if (attribution_storage != null) {
                options.attribution = attribution_storage.to_native ();
                options.fields |= (uint32) Raw.StyleTileSourceOptionField.ATTRIBUTION;
            }
            if (scheme != null) {
                options.scheme = (uint32) scheme;
                options.fields |= (uint32) Raw.StyleTileSourceOptionField.SCHEME;
            }
            if (bounds != null) {
                options.bounds = bounds.to_native ();
                options.fields |= (uint32) Raw.StyleTileSourceOptionField.BOUNDS;
            }
            if (tile_size != null) {
                options.tile_size = tile_size;
                options.fields |= (uint32) Raw.StyleTileSourceOptionField.TILE_SIZE;
            }
            if (vector_encoding != null) {
                options.vector_encoding = (uint32) vector_encoding;
                options.fields |= (uint32) Raw.StyleTileSourceOptionField.VECTOR_ENCODING;
            }
            if (raster_encoding != null) {
                options.raster_encoding = (uint32) raster_encoding;
                options.fields |= (uint32) Raw.StyleTileSourceOptionField.RASTER_ENCODING;
            }
            return options;
        }
    }

    public class GeoJsonSourceOptions {
        public double? min_zoom { get; set; }
        public double? max_zoom { get; set; }
        public double? tolerance { get; set; }
        public double? cluster_max_zoom { get; set; }
        public JsonValue? cluster_properties { get; set; }
        public uint32? tile_size { get; set; }
        public uint32? buffer { get; set; }
        public uint32? cluster_radius { get; set; }
        public uint32? cluster_min_points { get; set; }
        public bool? line_metrics { get; set; }
        public bool? cluster { get; set; }

        public GeoJsonSourceOptions copy () {
            var copied = new GeoJsonSourceOptions ();
            copied.min_zoom = min_zoom;
            copied.max_zoom = max_zoom;
            copied.tolerance = tolerance;
            copied.cluster_max_zoom = cluster_max_zoom;
            copied.cluster_properties = cluster_properties == null ? null : cluster_properties.copy ();
            copied.tile_size = tile_size;
            copied.buffer = buffer;
            copied.cluster_radius = cluster_radius;
            copied.cluster_min_points = cluster_min_points;
            copied.line_metrics = line_metrics;
            copied.cluster = cluster;
            return copied;
        }

        public bool equal (GeoJsonSourceOptions other) {
            return min_zoom == other.min_zoom
                && max_zoom == other.max_zoom
                && tolerance == other.tolerance
                && cluster_max_zoom == other.cluster_max_zoom
                && ((cluster_properties == null && other.cluster_properties == null)
                    || (cluster_properties != null
                        && other.cluster_properties != null
                        && cluster_properties.equal (other.cluster_properties)))
                && tile_size == other.tile_size
                && buffer == other.buffer
                && cluster_radius == other.cluster_radius
                && cluster_min_points == other.cluster_min_points
                && line_metrics == other.line_metrics
                && cluster == other.cluster;
        }

        internal Raw.GeoJsonSourceOptions to_native (ref Raw.JsonValue cluster_properties_storage, out JsonValue? cluster_properties_owner) throws Error {
            Raw.GeoJsonSourceOptions options = Raw.geojson_source_options_default ();
            cluster_properties_owner = null;
            if (min_zoom != null) {
                options.min_zoom = min_zoom;
                options.fields |= (uint32) Raw.GeoJsonSourceOptionField.MIN_ZOOM;
            }
            if (max_zoom != null) {
                options.max_zoom = max_zoom;
                options.fields |= (uint32) Raw.GeoJsonSourceOptionField.MAX_ZOOM;
            }
            if (tolerance != null) {
                options.tolerance = tolerance;
                options.fields |= (uint32) Raw.GeoJsonSourceOptionField.TOLERANCE;
            }
            if (cluster_max_zoom != null) {
                options.cluster_max_zoom = cluster_max_zoom;
                options.fields |= (uint32) Raw.GeoJsonSourceOptionField.CLUSTER_MAX_ZOOM;
            }
            if (cluster_properties != null) {
                cluster_properties_owner = cluster_properties.copy ();
                cluster_properties_storage = cluster_properties_owner.to_native ();
                options.cluster_properties = &cluster_properties_storage;
                options.fields |= (uint32) Raw.GeoJsonSourceOptionField.CLUSTER_PROPERTIES;
            }
            if (tile_size != null) {
                options.tile_size = tile_size;
                options.fields |= (uint32) Raw.GeoJsonSourceOptionField.TILE_SIZE;
            }
            if (buffer != null) {
                options.buffer = buffer;
                options.fields |= (uint32) Raw.GeoJsonSourceOptionField.BUFFER;
            }
            if (cluster_radius != null) {
                options.cluster_radius = cluster_radius;
                options.fields |= (uint32) Raw.GeoJsonSourceOptionField.CLUSTER_RADIUS;
            }
            if (cluster_min_points != null) {
                options.cluster_min_points = cluster_min_points;
                options.fields |= (uint32) Raw.GeoJsonSourceOptionField.CLUSTER_MIN_POINTS;
            }
            if (line_metrics != null) {
                options.line_metrics = line_metrics;
                options.fields |= (uint32) Raw.GeoJsonSourceOptionField.LINE_METRICS;
            }
            if (cluster != null) {
                options.cluster = cluster;
                options.fields |= (uint32) Raw.GeoJsonSourceOptionField.CLUSTER;
            }
            return options;
        }
    }

    public class PremultipliedRgba8Image {
        public uint32 width { get; private set; }
        public uint32 height { get; private set; }
        public uint32 stride { get; private set; }
        private uint8[] pixels;

        public PremultipliedRgba8Image (uint32 width, uint32 height, uint32 stride, uint8[] pixels) {
            this.width = width;
            this.height = height;
            this.stride = stride;
            this.pixels = copy_byte_array (pixels);
        }

        internal Raw.PremultipliedRgba8Image to_native () {
            Raw.PremultipliedRgba8Image image = Raw.premultiplied_rgba8_image_default ();
            image.width = width;
            image.height = height;
            image.stride = stride;
            image.pixels = pixels;
            image.byte_length = pixels.length;
            return image;
        }

        public uint8[] copy_pixels () {
            uint8[] copied = new uint8[pixels.length];
            for (int index = 0; index < pixels.length; index++) {
                copied[index] = pixels[index];
            }
            return copied;
        }

        public bool equal (PremultipliedRgba8Image other) {
            if (width != other.width
                || height != other.height
                || stride != other.stride
                || pixels.length != other.pixels.length) {
                return false;
            }
            for (int index = 0; index < pixels.length; index++) {
                if (pixels[index] != other.pixels[index]) {
                    return false;
                }
            }
            return true;
        }
    }

    public class StyleImageOptions {
        public float? pixel_ratio { get; set; }
        public bool? sdf { get; set; }

        public StyleImageOptions copy () {
            var copied = new StyleImageOptions ();
            copied.pixel_ratio = pixel_ratio;
            copied.sdf = sdf;
            return copied;
        }

        public bool equal (StyleImageOptions other) {
            return pixel_ratio == other.pixel_ratio && sdf == other.sdf;
        }

        internal Raw.StyleImageOptions to_native () {
            Raw.StyleImageOptions options = Raw.style_image_options_default ();
            if (pixel_ratio != null) {
                options.pixel_ratio = pixel_ratio;
                options.fields |= (uint32) Raw.StyleImageOptionField.PIXEL_RATIO;
            }
            if (sdf != null) {
                options.sdf = sdf;
                options.fields |= (uint32) Raw.StyleImageOptionField.SDF;
            }
            return options;
        }
    }

    public class StyleSourceInfo {
        public StyleSourceType source_type { get; private set; }
        public size_t id_byte_length { get; private set; }
        public bool is_volatile { get; private set; }
        public bool has_attribution { get; private set; }
        public size_t attribution_byte_length { get; private set; }

        internal StyleSourceInfo (Raw.StyleSourceInfo native) {
            source_type = style_source_type_from_raw (native.type);
            id_byte_length = native.id_size;
            is_volatile = native.is_volatile;
            has_attribution = native.has_attribution;
            attribution_byte_length = native.attribution_size;
        }

        public StyleSourceInfo copy () {
            Raw.StyleSourceInfo native = {};
            native.type = (uint32) source_type;
            native.id_size = id_byte_length;
            native.is_volatile = is_volatile;
            native.has_attribution = has_attribution;
            native.attribution_size = attribution_byte_length;
            return new StyleSourceInfo (native);
        }

        public bool equal (StyleSourceInfo other) {
            return source_type == other.source_type
                && id_byte_length == other.id_byte_length
                && is_volatile == other.is_volatile
                && has_attribution == other.has_attribution
                && attribution_byte_length == other.attribution_byte_length;
        }
    }

    public class StyleImageInfo {
        public uint32 width { get; private set; }
        public uint32 height { get; private set; }
        public uint32 stride { get; private set; }
        public size_t byte_length { get; private set; }
        public float pixel_ratio { get; private set; }
        public bool sdf { get; private set; }

        internal StyleImageInfo (Raw.StyleImageInfo native) {
            width = native.width;
            height = native.height;
            stride = native.stride;
            byte_length = native.byte_length;
            pixel_ratio = native.pixel_ratio;
            sdf = native.sdf;
        }

        public StyleImageInfo copy () {
            Raw.StyleImageInfo native = Raw.style_image_info_default ();
            native.width = width;
            native.height = height;
            native.stride = stride;
            native.byte_length = byte_length;
            native.pixel_ratio = pixel_ratio;
            native.sdf = sdf;
            return new StyleImageInfo (native);
        }

        public bool equal (StyleImageInfo other) {
            return width == other.width
                && height == other.height
                && stride == other.stride
                && byte_length == other.byte_length
                && pixel_ratio == other.pixel_ratio
                && sdf == other.sdf;
        }
    }

    internal Raw.StringView[] string_views_for_tiles_utf8 (Utf8String[] tiles) throws Error {
        if (tiles.length == 0) {
            clear_unknown_status ();
            throw new Error.INVALID_ARGUMENT ("tile URL list is empty");
        }
        Raw.StringView[] views = new Raw.StringView[tiles.length];
        for (var index = 0; index < tiles.length; index++) {
            if (tiles[index] == null) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("tile URL is null");
            }
            views[index] = tiles[index].to_native ();
        }
        return views;
    }

    internal Raw.LatLng[] image_source_coordinates_to_native (LatLng[] coordinates) throws Error {
        if (coordinates.length != 4) {
            clear_unknown_status ();
            throw new Error.INVALID_ARGUMENT ("image source coordinates must contain exactly four coordinates");
        }
        Raw.LatLng[] native_coordinates = new Raw.LatLng[coordinates.length];
        for (var index = 0; index < coordinates.length; index++) {
            native_coordinates[index] = coordinates[index].to_native ();
        }
        return native_coordinates;
    }

}
