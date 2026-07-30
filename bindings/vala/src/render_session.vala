namespace MaplibreNative {
    internal class RenderSessionNativeLease {
        private RenderSessionHandle owner;
        public Raw.RenderSession native { get; private set; }

        internal RenderSessionNativeLease (RenderSessionHandle owner, Raw.RenderSession native) {
            this.owner = owner;
            this.native = native;
        }

        ~RenderSessionNativeLease () {
            owner.release_native_lease ();
        }
    }

    public class RenderSessionHandle {
        private MapHandle map;
        private Raw.RenderSession native = (Raw.RenderSession) 0;
        private bool frame_acquired;
        private bool detached;
        private Mutex state_mutex;
        private Cond idle;
        private bool releasing;
        private uint active_native_leases;
        private unowned Thread<void*> owner_thread;

        public bool closed {
            get {
                state_mutex.lock ();
                var value = (uint64) native == 0;
                state_mutex.unlock ();
                return value;
            }
        }
        public bool is_detached {
            get {
                state_mutex.lock ();
                var value = detached;
                state_mutex.unlock ();
                return value;
            }
        }

        internal RenderSessionHandle (MapHandle map, Raw.RenderSession native) {
            this.map = map;
            this.native = native;
            owner_thread = Thread.self<void*> ();
        }

        ~RenderSessionHandle () {
            state_mutex.lock ();
            var leaked = (uint64) native != 0;
            state_mutex.unlock ();
            if (leaked) {
                warning ("RenderSessionHandle finalized while live; call close() on the owner thread");
            }
        }

        internal RenderSessionNativeLease require_live () throws Error {
            ensure_owner_thread ();
            state_mutex.lock ();
            if ((uint64) native == 0 || releasing) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("render session handle is closed");
            }
            active_native_leases++;
            var lease = new RenderSessionNativeLease (this, native);
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

        internal void begin_frame_borrow () throws Error {
            ensure_owner_thread ();
            state_mutex.lock ();
            if ((uint64) native == 0 || releasing) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("render session handle is closed");
            }
            if (frame_acquired) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("render session already has an acquired frame");
            }
            frame_acquired = true;
            state_mutex.unlock ();
        }

        internal void finish_frame_borrow () {
            state_mutex.lock ();
            frame_acquired = false;
            state_mutex.unlock ();
        }

        private RenderSessionNativeLease require_available () throws Error {
            ensure_owner_thread ();
            state_mutex.lock ();
            if ((uint64) native == 0 || releasing) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("render session handle is closed");
            }
            if (frame_acquired) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("render session has an acquired frame");
            }
            active_native_leases++;
            var lease = new RenderSessionNativeLease (this, native);
            state_mutex.unlock ();
            return lease;
        }

        public void close () throws Error {
            ensure_owner_thread ();
            state_mutex.lock ();
            if ((uint64) native == 0) {
                state_mutex.unlock ();
                return;
            }
            if (releasing) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("render session release is already in progress");
            }
            if (frame_acquired) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("render session has an acquired frame");
            }
            releasing = true;
            while (active_native_leases > 0) {
                idle.wait (state_mutex);
            }
            Raw.RenderSession closing = native;
            state_mutex.unlock ();

            var status = Raw.render_session_destroy (closing);

            state_mutex.lock ();
            if (status == Raw.Status.OK) {
                native = (Raw.RenderSession) 0;
            }
            releasing = false;
            idle.broadcast ();
            state_mutex.unlock ();
            check_status (status);
        }

        private void ensure_owner_thread () throws Error {
            if (Thread.self<void*> () != owner_thread) {
                clear_unknown_status ();
                throw new Error.WRONG_THREAD ("render session called from a thread other than its owner thread");
            }
        }

        public void resize (uint32 width, uint32 height, double scale_factor) throws Error {
            var lease = require_available ();
            check_status (Raw.render_session_resize (lease.native, width, height, scale_factor));
        }

        public bool render_update () throws Error {
            Raw.CBool rendered;
            var lease = require_available ();
            check_status (Raw.render_session_render_update (lease.native, out rendered));
            return bool_from_raw (rendered);
        }

        public void detach () throws Error {
            var lease = require_available ();
            check_status (Raw.render_session_detach (lease.native));
            state_mutex.lock ();
            detached = true;
            state_mutex.unlock ();
        }

        public void reduce_memory_use () throws Error {
            var lease = require_available ();
            check_status (Raw.render_session_reduce_memory_use (lease.native));
        }

        public void clear_data () throws Error {
            var lease = require_available ();
            check_status (Raw.render_session_clear_data (lease.native));
        }

        public void dump_debug_logs () throws Error {
            var lease = require_available ();
            check_status (Raw.render_session_dump_debug_logs (lease.native));
        }

        public TextureImageInfo read_premultiplied_rgba8 (uint8[] out_data) throws Error {
            if (out_data.length == 0) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("readback buffer is empty");
            }
            Raw.TextureImageInfo info = Raw.texture_image_info_default ();
            var lease = require_available ();
            check_status (Raw.texture_read_premultiplied_rgba8 (lease.native, out_data, out_data.length, &info));
            return new TextureImageInfo (info);
        }

        public QueriedFeature[] query_rendered_features (RenderedQueryGeometry geometry, RenderedFeatureQueryOptions? options = null) throws Error {
            Raw.RenderedQueryGeometry native_geometry = geometry.to_native ();
            Raw.RenderedFeatureQueryOptions native_options = {};
            Raw.RenderedFeatureQueryOptions* options_ptr = null;
            RenderedFeatureQueryOptions? options_storage = null;
            if (options != null) {
                options_storage = options.copy ();
                native_options = options_storage.to_native ();
                options_ptr = &native_options;
            }
            Raw.FeatureQueryResult result = (Raw.FeatureQueryResult) 0;
            var lease = require_available ();
            check_status (Raw.render_session_query_rendered_features (lease.native, &native_geometry, options_ptr, out result));
            return FeatureQueryResultHandle.copy_from_native (result);
        }

        public QueriedFeature[] query_source_features (string source_id, SourceFeatureQueryOptions? options = null) throws Error {
            return query_source_features_utf8 (new Utf8String (source_id), options);
        }

        public QueriedFeature[] query_source_features_utf8 (Utf8String source_id, SourceFeatureQueryOptions? options = null) throws Error {
            Raw.SourceFeatureQueryOptions native_options = {};
            Raw.SourceFeatureQueryOptions* options_ptr = null;
            SourceFeatureQueryOptions? options_storage = null;
            if (options != null) {
                options_storage = options.copy ();
                native_options = options_storage.to_native ();
                options_ptr = &native_options;
            }
            Raw.FeatureQueryResult result = (Raw.FeatureQueryResult) 0;
            var lease = require_available ();
            check_status (Raw.render_session_query_source_features (lease.native, source_id.to_native (), options_ptr, out result));
            return FeatureQueryResultHandle.copy_from_native (result);
        }

        public FeatureExtensionResult query_feature_extensions (string source_id, Feature feature, string extension, string extension_field, JsonValue? arguments = null) throws Error {
            return query_feature_extensions_utf8 (
                new Utf8String (source_id),
                feature,
                new Utf8String (extension),
                new Utf8String (extension_field),
                arguments
            );
        }

        public FeatureExtensionResult query_feature_extensions_utf8 (Utf8String source_id, Feature feature, Utf8String extension, Utf8String extension_field, JsonValue? arguments = null) throws Error {
            var feature_storage = feature.copy ();
            Raw.Feature native_feature = feature_storage.to_native ();
            Raw.JsonValue native_arguments = {};
            Raw.JsonValue* arguments_ptr = null;
            JsonValue? arguments_storage = null;
            if (arguments != null) {
                arguments_storage = arguments.copy ();
                native_arguments = arguments_storage.to_native ();
                arguments_ptr = &native_arguments;
            }
            Raw.FeatureExtensionResult result = (Raw.FeatureExtensionResult) 0;
            var lease = require_available ();
            check_status (Raw.render_session_query_feature_extensions (lease.native, source_id.to_native (), &native_feature, extension.to_native (), extension_field.to_native (), arguments_ptr, out result));
            return FeatureExtensionResultHandle.copy_from_native (result);
        }

        public void set_feature_state (FeatureStateSelector selector, JsonValue state) throws Error {
            var selector_storage = selector.copy ();
            Raw.FeatureStateSelector native_selector = selector_storage.to_native ();
            var state_storage = state.copy ();
            Raw.JsonValue native_state = state_storage.to_native ();
            var lease = require_available ();
            check_status (Raw.render_session_set_feature_state (lease.native, &native_selector, &native_state));
        }

        public JsonValue get_feature_state (FeatureStateSelector selector) throws Error {
            var selector_storage = selector.copy ();
            Raw.FeatureStateSelector native_selector = selector_storage.to_native ();
            Raw.JsonSnapshot snapshot = (Raw.JsonSnapshot) 0;
            var lease = require_available ();
            check_status (Raw.render_session_get_feature_state (lease.native, &native_selector, out snapshot));
            try {
                Raw.JsonValue* value;
                check_status (Raw.json_snapshot_get (snapshot, out value));
                return JsonValue.from_native (value[0]);
            } finally {
                Raw.json_snapshot_destroy (snapshot);
            }
        }

        public void remove_feature_state (FeatureStateSelector selector) throws Error {
            var selector_storage = selector.copy ();
            Raw.FeatureStateSelector native_selector = selector_storage.to_native ();
            var lease = require_available ();
            check_status (Raw.render_session_remove_feature_state (lease.native, &native_selector));
        }

        public MetalOwnedTextureFrameHandle acquire_metal_owned_texture_frame () throws Error {
            begin_frame_borrow ();
            Raw.MetalOwnedTextureFrame frame = {};
            frame.size = (uint32) sizeof (Raw.MetalOwnedTextureFrame);
            try {
                var lease = require_live ();
                check_status (Raw.metal_owned_texture_acquire_frame (lease.native, &frame));
                return new MetalOwnedTextureFrameHandle (this, frame);
            } catch (Error error) {
                finish_frame_borrow ();
                throw error;
            }
        }

        public VulkanOwnedTextureFrameHandle acquire_vulkan_owned_texture_frame () throws Error {
            begin_frame_borrow ();
            Raw.VulkanOwnedTextureFrame frame = {};
            frame.size = (uint32) sizeof (Raw.VulkanOwnedTextureFrame);
            try {
                var lease = require_live ();
                check_status (Raw.vulkan_owned_texture_acquire_frame (lease.native, &frame));
                return new VulkanOwnedTextureFrameHandle (this, frame);
            } catch (Error error) {
                finish_frame_borrow ();
                throw error;
            }
        }

        public OpenGLOwnedTextureFrameHandle acquire_opengl_owned_texture_frame () throws Error {
            begin_frame_borrow ();
            Raw.OpenGLOwnedTextureFrame frame = {};
            frame.size = (uint32) sizeof (Raw.OpenGLOwnedTextureFrame);
            try {
                var lease = require_live ();
                check_status (Raw.opengl_owned_texture_acquire_frame (lease.native, &frame));
                return new OpenGLOwnedTextureFrameHandle (this, frame);
            } catch (Error error) {
                finish_frame_borrow ();
                throw error;
            }
        }
    }

    public class MetalOwnedTextureFrameHandle {
        private RenderSessionHandle session;
        private Raw.MetalOwnedTextureFrame frame;
        private FrameAccessState state = new FrameAccessState ("metal texture frame");

        internal MetalOwnedTextureFrameHandle (RenderSessionHandle session, Raw.MetalOwnedTextureFrame frame) {
            this.session = session;
            this.frame = frame;
        }

        ~MetalOwnedTextureFrameHandle () {
            if (!state.is_closed) {
                warning ("MetalOwnedTextureFrameHandle finalized while live; call close() on the owner thread");
            }
        }

        private FrameAccessLease require_live () throws Error {
            return state.acquire ();
        }

        public void close () throws Error {
            if (!state.begin_close ()) {
                return;
            }
            bool released = false;
            try {
                var lease = session.require_live ();
                check_status (Raw.metal_owned_texture_release_frame (lease.native, &frame));
                released = true;
            } finally {
                state.finish_close (released);
                if (released) {
                    session.finish_frame_borrow ();
                }
            }
        }

        public uint32 get_width () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.width;
        }

        public uint32 get_height () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.height;
        }

        public double get_scale_factor () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.scale_factor;
        }

        public uint64 get_generation () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.generation;
        }

        public uint64 get_frame_id () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.frame_id;
        }

        public FrameNativePointer get_texture () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return new FrameNativePointer ((size_t) frame.texture, () => {
                var checked_access = require_live ();
                checked_access.keep_alive ();
            });
        }

        public FrameNativePointer get_device () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return new FrameNativePointer ((size_t) frame.device, () => {
                var checked_access = require_live ();
                checked_access.keep_alive ();
            });
        }

        public uint64 get_pixel_format () throws Error {
            var access = require_live ();
            access.keep_alive ();
            return frame.pixel_format;
        }
    }

}
