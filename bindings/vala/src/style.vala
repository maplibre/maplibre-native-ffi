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

    public delegate void CustomGeometryTileCallback (CanonicalTileId tile_id);

    internal class CustomGeometrySourceRegistration {
        public string source_id { get; private set; }
        public CustomGeometrySourceOptions options { get; private set; }

        public CustomGeometrySourceRegistration (string source_id, CustomGeometrySourceOptions options) {
            this.source_id = source_id;
            this.options = options;
        }
    }

    public class CustomGeometrySourceOptions {
        public CustomGeometryTileCallback fetch_tile;
        public CustomGeometryTileCallback? cancel_tile;
        public double? min_zoom { get; set; }
        public double? max_zoom { get; set; }
        public double? tolerance { get; set; }
        public uint32? tile_size { get; set; }
        public uint32? buffer { get; set; }
        public bool? clip { get; set; }
        public bool? wrap { get; set; }

        public CustomGeometrySourceOptions (owned CustomGeometryTileCallback fetch_tile) {
            this.fetch_tile = (owned) fetch_tile;
        }

        internal Raw.CustomGeometrySourceOptions to_native () {
            Raw.CustomGeometrySourceOptions options = Raw.custom_geometry_source_options_default ();
            options.fetch_tile = custom_geometry_fetch_tile_trampoline;
            if (cancel_tile != null) {
                options.cancel_tile = custom_geometry_cancel_tile_trampoline;
            }
            options.user_data = (void*) this;
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
        ((CustomGeometrySourceOptions) user_data).invoke_fetch_tile (tile_id);
    }

    private static void custom_geometry_cancel_tile_trampoline (void* user_data, Raw.CanonicalTileId tile_id) {
        if (user_data == null) {
            return;
        }
        ((CustomGeometrySourceOptions) user_data).invoke_cancel_tile (tile_id);
    }

    public class StyleTileSourceOptions {
        public double? min_zoom { get; set; }
        public double? max_zoom { get; set; }
        public string? attribution { get; set; }
        public StyleTileScheme? scheme { get; set; }
        public LatLngBounds? bounds { get; set; }
        public uint32? tile_size { get; set; }
        public StyleVectorTileEncoding? vector_encoding { get; set; }
        public StyleRasterDemEncoding? raster_encoding { get; set; }

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
            if (attribution != null) {
                options.attribution = string_view (attribution);
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

    public class PremultipliedRgba8Image {
        public uint32 width { get; private set; }
        public uint32 height { get; private set; }
        public uint32 stride { get; private set; }
        private uint8[] pixels;

        public PremultipliedRgba8Image (uint32 width, uint32 height, uint32 stride, owned uint8[] pixels) {
            this.width = width;
            this.height = height;
            this.stride = stride;
            this.pixels = (owned) pixels;
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
    }

    public class StyleImageOptions {
        public float? pixel_ratio { get; set; }
        public bool? sdf { get; set; }

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
    }

    internal Raw.StringView[] string_views_for_tiles (string[] tiles) throws Error {
        if (tiles.length == 0) {
            throw new Error.INVALID_ARGUMENT ("tile URL list is empty");
        }
        Raw.StringView[] views = new Raw.StringView[tiles.length];
        for (var index = 0; index < tiles.length; index++) {
            views[index] = string_view (tiles[index]);
        }
        return views;
    }

    internal Raw.LatLng[] image_source_coordinates_to_native (LatLng[] coordinates) throws Error {
        if (coordinates.length != 4) {
            throw new Error.INVALID_ARGUMENT ("image source coordinates must contain exactly four coordinates");
        }
        Raw.LatLng[] native_coordinates = new Raw.LatLng[coordinates.length];
        for (var index = 0; index < coordinates.length; index++) {
            native_coordinates[index] = coordinates[index].to_native ();
        }
        return native_coordinates;
    }

}
