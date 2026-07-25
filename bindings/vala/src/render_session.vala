namespace MaplibreNative {
    public class RenderSessionHandle {
        private MapHandle map;
        private Raw.RenderSession? native;
        private bool frame_acquired;
        private bool detached;

        public bool closed { get { return native == null; } }
        public bool is_detached { get { return detached; } }

        internal RenderSessionHandle (MapHandle map, owned Raw.RenderSession native) {
            this.map = map;
            this.native = (owned) native;
        }

        ~RenderSessionHandle () {
            if (native != null) {
                warning ("RenderSessionHandle finalized while live; call close() on the owner thread");
            }
        }

        internal unowned Raw.RenderSession require_live () throws Error {
            if (native == null) {
                throw new Error.INVALID_STATE ("render session handle is closed");
            }
            return native;
        }

        internal void begin_frame_borrow () throws Error {
            if (frame_acquired) {
                throw new Error.INVALID_STATE ("render session already has an acquired frame");
            }
            frame_acquired = true;
        }

        internal void finish_frame_borrow () {
            frame_acquired = false;
        }

        public void close () throws Error {
            if (native == null) {
                return;
            }
            if (frame_acquired) {
                throw new Error.INVALID_STATE ("render session has an acquired frame");
            }
            unowned Raw.RenderSession closing = native;
            check_status (Raw.render_session_destroy (closing));
            native = null;
        }

        public void resize (uint32 width, uint32 height, double scale_factor) throws Error {
            if (frame_acquired) {
                throw new Error.INVALID_STATE ("render session has an acquired frame");
            }
            check_status (Raw.render_session_resize (require_live (), width, height, scale_factor));
        }

        public void render_update () throws Error {
            if (frame_acquired) {
                throw new Error.INVALID_STATE ("render session has an acquired frame");
            }
            check_status (Raw.render_session_render_update (require_live ()));
        }

        public void detach () throws Error {
            if (frame_acquired) {
                throw new Error.INVALID_STATE ("render session has an acquired frame");
            }
            check_status (Raw.render_session_detach (require_live ()));
            detached = true;
        }

        public void reduce_memory_use () throws Error {
            check_status (Raw.render_session_reduce_memory_use (require_live ()));
        }

        public void clear_data () throws Error {
            check_status (Raw.render_session_clear_data (require_live ()));
        }

        public void dump_debug_logs () throws Error {
            check_status (Raw.render_session_dump_debug_logs (require_live ()));
        }

        public TextureImageInfo read_premultiplied_rgba8 (uint8[] out_data) throws Error {
            if (out_data.length == 0) {
                throw new Error.INVALID_ARGUMENT ("readback buffer is empty");
            }
            Raw.TextureImageInfo info = Raw.texture_image_info_default ();
            check_status (Raw.texture_read_premultiplied_rgba8 (require_live (), out_data, out_data.length, &info));
            return new TextureImageInfo (info);
        }

        public QueriedFeature[] query_rendered_features (RenderedQueryGeometry geometry, RenderedFeatureQueryOptions? options = null) throws Error {
            Raw.RenderedQueryGeometry native_geometry = geometry.to_native ();
            Raw.RenderedFeatureQueryOptions native_options = {};
            Raw.RenderedFeatureQueryOptions* options_ptr = null;
            if (options != null) {
                native_options = options.to_native ();
                options_ptr = &native_options;
            }
            Raw.FeatureQueryResult result;
            check_status (Raw.render_session_query_rendered_features (require_live (), &native_geometry, options_ptr, out result));
            return FeatureQueryResultHandle.copy_from_native ((owned) result);
        }

        public QueriedFeature[] query_source_features (string source_id, SourceFeatureQueryOptions? options = null) throws Error {
            Raw.SourceFeatureQueryOptions native_options = {};
            Raw.SourceFeatureQueryOptions* options_ptr = null;
            if (options != null) {
                native_options = options.to_native ();
                options_ptr = &native_options;
            }
            Raw.FeatureQueryResult result;
            check_status (Raw.render_session_query_source_features (require_live (), string_view (source_id), options_ptr, out result));
            return FeatureQueryResultHandle.copy_from_native ((owned) result);
        }

        public FeatureExtensionResult query_feature_extensions (string source_id, Feature feature, string extension, string extension_field, JsonValue? arguments = null) throws Error {
            Raw.Feature native_feature = feature.to_native ();
            Raw.JsonValue native_arguments = {};
            Raw.JsonValue* arguments_ptr = null;
            if (arguments != null) {
                native_arguments = arguments.to_native ();
                arguments_ptr = &native_arguments;
            }
            Raw.FeatureExtensionResult result;
            check_status (Raw.render_session_query_feature_extensions (require_live (), string_view (source_id), &native_feature, string_view (extension), string_view (extension_field), arguments_ptr, out result));
            return FeatureExtensionResultHandle.copy_from_native ((owned) result);
        }

        public void set_feature_state (FeatureStateSelector selector, JsonValue state) throws Error {
            Raw.FeatureStateSelector native_selector = selector.to_native ();
            Raw.JsonValue native_state = state.to_native ();
            check_status (Raw.render_session_set_feature_state (require_live (), &native_selector, &native_state));
        }

        public JsonValue get_feature_state (FeatureStateSelector selector) throws Error {
            Raw.FeatureStateSelector native_selector = selector.to_native ();
            Raw.JsonSnapshot snapshot;
            check_status (Raw.render_session_get_feature_state (require_live (), &native_selector, out snapshot));
            try {
                Raw.JsonValue* value;
                check_status (Raw.json_snapshot_get (snapshot, out value));
                return JsonValue.from_native (value[0]);
            } finally {
                Raw.json_snapshot_destroy (snapshot);
            }
        }

        public void remove_feature_state (FeatureStateSelector selector) throws Error {
            Raw.FeatureStateSelector native_selector = selector.to_native ();
            check_status (Raw.render_session_remove_feature_state (require_live (), &native_selector));
        }

        public MetalOwnedTextureFrameHandle acquire_metal_owned_texture_frame () throws Error {
            begin_frame_borrow ();
            Raw.MetalOwnedTextureFrame frame = {};
            frame.size = (uint32) sizeof (Raw.MetalOwnedTextureFrame);
            try {
                check_status (Raw.metal_owned_texture_acquire_frame (require_live (), &frame));
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
                check_status (Raw.vulkan_owned_texture_acquire_frame (require_live (), &frame));
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
                check_status (Raw.opengl_owned_texture_acquire_frame (require_live (), &frame));
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
        private bool closed;

        internal MetalOwnedTextureFrameHandle (RenderSessionHandle session, Raw.MetalOwnedTextureFrame frame) {
            this.session = session;
            this.frame = frame;
        }

        ~MetalOwnedTextureFrameHandle () {
            if (!closed) {
                warning ("MetalOwnedTextureFrameHandle finalized while live; call close() on the owner thread");
            }
        }

        private void require_live () throws Error {
            if (closed) {
                throw new Error.INVALID_STATE ("metal texture frame is closed");
            }
        }

        public void close () throws Error {
            if (closed) {
                return;
            }
            check_status (Raw.metal_owned_texture_release_frame (session.require_live (), &frame));
            closed = true;
            session.finish_frame_borrow ();
        }

        public uint32 get_width () throws Error {
            require_live ();
            return frame.width;
        }

        public uint32 get_height () throws Error {
            require_live ();
            return frame.height;
        }

        public double get_scale_factor () throws Error {
            require_live ();
            return frame.scale_factor;
        }

        public uint64 get_generation () throws Error {
            require_live ();
            return frame.generation;
        }

        public uint64 get_frame_id () throws Error {
            require_live ();
            return frame.frame_id;
        }

        public FrameNativePointer get_texture () throws Error {
            require_live ();
            return new FrameNativePointer ((size_t) frame.texture, () => require_live ());
        }

        public FrameNativePointer get_device () throws Error {
            require_live ();
            return new FrameNativePointer ((size_t) frame.device, () => require_live ());
        }

        public uint64 get_pixel_format () throws Error {
            require_live ();
            return frame.pixel_format;
        }
    }

}
