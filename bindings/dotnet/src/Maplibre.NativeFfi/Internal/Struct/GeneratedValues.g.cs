// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Internal.Struct;

internal static unsafe class GeneratedValues
{
    internal static mln_animation_options NativeAnimationOptions(
        AnimationOptions value,
        NativeCallScope scope
    )
    {
        var native = new mln_animation_options();
        native.fields = 0;
        native.fields |= Put(
            value.DurationMs,
            ref native.duration_ms,
            MLN_ANIMATION_OPTION_DURATION
        );
        native.fields |= Put(value.Velocity, ref native.velocity, MLN_ANIMATION_OPTION_VELOCITY);
        native.fields |= Put(value.MinZoom, ref native.min_zoom, MLN_ANIMATION_OPTION_MIN_ZOOM);
        native.fields |= Put(
            value.Easing,
            ref native.easing,
            MLN_ANIMATION_OPTION_EASING,
            NativeUnitBezier
        );
        native.fields |= Put(
            value.TransitionId,
            ref native.transition_id,
            MLN_ANIMATION_OPTION_TRANSITION_ID
        );
        native.end_handler = NativeCameraTransitionHandler(value.EndHandler, scope);
        return native;
    }

    internal static BoundOptions CopyBoundOptions(mln_bound_options value) =>
        new()
        {
            Bounds = value.fields.HasFlag(MLN_BOUND_OPTION_BOUNDS)
                ? CopyLatLngBounds(value.bounds)
                : null,
            MinZoom = value.fields.HasFlag(MLN_BOUND_OPTION_MIN_ZOOM) ? value.min_zoom : null,
            MaxZoom = value.fields.HasFlag(MLN_BOUND_OPTION_MAX_ZOOM) ? value.max_zoom : null,
            MinPitch = value.fields.HasFlag(MLN_BOUND_OPTION_MIN_PITCH) ? value.min_pitch : null,
            MaxPitch = value.fields.HasFlag(MLN_BOUND_OPTION_MAX_PITCH) ? value.max_pitch : null,
            Unbounded = value.fields.HasFlag(MLN_BOUND_OPTION_UNBOUNDED),
        };

    internal static mln_bound_options NativeBoundOptions(BoundOptions value)
    {
        var native = new mln_bound_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_bound_options);
        native.fields |= Put(
            value.Bounds,
            ref native.bounds,
            MLN_BOUND_OPTION_BOUNDS,
            NativeLatLngBounds
        );
        native.fields |= Put(value.MinZoom, ref native.min_zoom, MLN_BOUND_OPTION_MIN_ZOOM);
        native.fields |= Put(value.MaxZoom, ref native.max_zoom, MLN_BOUND_OPTION_MAX_ZOOM);
        native.fields |= Put(value.MinPitch, ref native.min_pitch, MLN_BOUND_OPTION_MIN_PITCH);
        native.fields |= Put(value.MaxPitch, ref native.max_pitch, MLN_BOUND_OPTION_MAX_PITCH);
        if (value.Unbounded)
            native.fields |= MLN_BOUND_OPTION_UNBOUNDED;
        return native;
    }

    internal static Camera CopyCamera(mln_camera value) =>
        new(
            CopyLatLng(value.center),
            value.center_altitude,
            CopyEdgeInsets(value.padding),
            value.zoom,
            value.bearing,
            value.pitch,
            value.roll,
            value.field_of_view
        );

    internal static mln_camera NativeCamera(Camera value)
    {
        var native = new mln_camera();
        native.center = NativeLatLng(value.Center);
        native.center_altitude = value.CenterAltitude;
        native.padding = NativeEdgeInsets(value.Padding);
        native.zoom = value.Zoom;
        native.bearing = value.Bearing;
        native.pitch = value.Pitch;
        native.roll = value.Roll;
        native.field_of_view = value.FieldOfView;
        return native;
    }

    internal static mln_camera_delta NativeCameraDelta(CameraDelta value, NativeCallScope scope)
    {
        Required(value.Animation, "CameraDelta.Animation must not be null.");
        var native = new mln_camera_delta();
        native.fields = 0;
        native.size = (uint)sizeof(mln_camera_delta);
        native.fields |= Put(
            value.Offset,
            ref native.offset,
            MLN_CAMERA_DELTA_OFFSET,
            NativeScreenPoint
        );
        native.fields |= Put(value.Scale, ref native.scale, MLN_CAMERA_DELTA_SCALE);
        native.fields |= Put(value.Bearing, ref native.bearing, MLN_CAMERA_DELTA_BEARING);
        native.fields |= Put(value.Pitch, ref native.pitch, MLN_CAMERA_DELTA_PITCH);
        native.fields |= Put(
            value.Anchor,
            ref native.anchor,
            MLN_CAMERA_DELTA_ANCHOR,
            NativeScreenPoint
        );
        native.animation = NativeAnimationOptions(value.Animation, scope);
        native.gesture_phase = (mln_gesture_phase)value.GesturePhase;
        return native;
    }

    internal static mln_camera_fit_options NativeCameraFitOptions(CameraFitOptions value)
    {
        var native = new mln_camera_fit_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_camera_fit_options);
        native.fields |= Put(
            value.Padding,
            ref native.padding,
            MLN_CAMERA_FIT_OPTION_PADDING,
            NativeEdgeInsets
        );
        native.fields |= Put(value.Bearing, ref native.bearing, MLN_CAMERA_FIT_OPTION_BEARING);
        native.fields |= Put(value.Pitch, ref native.pitch, MLN_CAMERA_FIT_OPTION_PITCH);
        return native;
    }

    internal static CameraOptions CopyCameraOptions(mln_camera_options value) =>
        new()
        {
            Center = value.fields.HasFlag(MLN_CAMERA_OPTION_CENTER)
                ? CopyLatLng(value.center)
                : null,
            CenterAltitude = value.fields.HasFlag(MLN_CAMERA_OPTION_CENTER_ALTITUDE)
                ? value.center_altitude
                : null,
            Padding = value.fields.HasFlag(MLN_CAMERA_OPTION_PADDING)
                ? CopyEdgeInsets(value.padding)
                : null,
            Anchor = value.fields.HasFlag(MLN_CAMERA_OPTION_ANCHOR)
                ? CopyScreenPoint(value.anchor)
                : null,
            Zoom = value.fields.HasFlag(MLN_CAMERA_OPTION_ZOOM) ? value.zoom : null,
            Bearing = value.fields.HasFlag(MLN_CAMERA_OPTION_BEARING) ? value.bearing : null,
            Pitch = value.fields.HasFlag(MLN_CAMERA_OPTION_PITCH) ? value.pitch : null,
            Roll = value.fields.HasFlag(MLN_CAMERA_OPTION_ROLL) ? value.roll : null,
            FieldOfView = value.fields.HasFlag(MLN_CAMERA_OPTION_FOV) ? value.field_of_view : null,
        };

    internal static mln_camera_options NativeCameraOptions(CameraOptions value)
    {
        var native = new mln_camera_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_camera_options);
        native.fields |= Put(
            value.Center,
            ref native.center,
            MLN_CAMERA_OPTION_CENTER,
            NativeLatLng
        );
        native.fields |= Put(
            value.CenterAltitude,
            ref native.center_altitude,
            MLN_CAMERA_OPTION_CENTER_ALTITUDE
        );
        native.fields |= Put(
            value.Padding,
            ref native.padding,
            MLN_CAMERA_OPTION_PADDING,
            NativeEdgeInsets
        );
        native.fields |= Put(
            value.Anchor,
            ref native.anchor,
            MLN_CAMERA_OPTION_ANCHOR,
            NativeScreenPoint
        );
        native.fields |= Put(value.Zoom, ref native.zoom, MLN_CAMERA_OPTION_ZOOM);
        native.fields |= Put(value.Bearing, ref native.bearing, MLN_CAMERA_OPTION_BEARING);
        native.fields |= Put(value.Pitch, ref native.pitch, MLN_CAMERA_OPTION_PITCH);
        native.fields |= Put(value.Roll, ref native.roll, MLN_CAMERA_OPTION_ROLL);
        native.fields |= Put(value.FieldOfView, ref native.field_of_view, MLN_CAMERA_OPTION_FOV);
        return native;
    }

    internal static CameraQueryResult CopyCameraQueryResult(mln_camera_query_result value) =>
        new(value.generation, CopyCamera(value.camera));

    internal static CameraTransitionEnd CopyCameraTransitionEnd(mln_camera_transition_end value) =>
        new((CameraTransitionOutcome)value.outcome, value.generation);

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeCameraTransitionHandlerCallback(
        void* user_data,
        mln_camera_transition_end* end
    )
    {
        try
        {
            ((CameraTransitionHandler)NativeCallbackRoot.Value(user_data)).Callback?.Invoke(
                CopyCameraTransitionEnd(NativeCallScope.Read(end))
            );
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_camera_transition_end_callback", error);
        }
    }

    internal static mln_camera_transition_handler NativeCameraTransitionHandler(
        CameraTransitionHandler value,
        NativeCallScope scope
    )
    {
        var native = new mln_camera_transition_handler();
        if (value.Callback is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.callback = value.Callback is null
                ? null
                : &InvokeCameraTransitionHandlerCallback;
        }
        return native;
    }

    internal static mln_camera_update NativeCameraUpdate(CameraUpdate value, NativeCallScope scope)
    {
        Required(value.Camera, "CameraUpdate.Camera must not be null.");
        Required(value.Animation, "CameraUpdate.Animation must not be null.");
        var native = new mln_camera_update();
        native.size = (uint)sizeof(mln_camera_update);
        native.mode = (mln_camera_update_mode)value.Mode;
        native.camera = NativeCameraOptions(value.Camera);
        native.animation = NativeAnimationOptions(value.Animation, scope);
        native.gesture_phase = (mln_gesture_phase)value.GesturePhase;
        return native;
    }

    internal static CanonicalTileId CopyCanonicalTileId(mln_canonical_tile_id value) =>
        new(value.z, value.x, value.y);

    internal static mln_canonical_tile_id NativeCanonicalTileId(CanonicalTileId value)
    {
        var native = new mln_canonical_tile_id();
        native.z = value.Z;
        native.x = value.X;
        native.y = value.Y;
        return native;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeCustomGeometrySourceOptionsFetchTile(
        void* user_data,
        mln_canonical_tile_id tile_id
    )
    {
        try
        {
            ((CustomGeometrySourceOptions)NativeCallbackRoot.Value(user_data)).FetchTile?.Invoke(
                CopyCanonicalTileId(tile_id)
            );
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_custom_source_fetch_tile_callback", error);
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeCustomGeometrySourceOptionsCancelTile(
        void* user_data,
        mln_canonical_tile_id tile_id
    )
    {
        try
        {
            ((CustomGeometrySourceOptions)NativeCallbackRoot.Value(user_data)).CancelTile?.Invoke(
                CopyCanonicalTileId(tile_id)
            );
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_custom_source_cancel_tile_callback", error);
        }
    }

    internal static mln_custom_geometry_source_options NativeCustomGeometrySourceOptions(
        CustomGeometrySourceOptions value,
        NativeCallScope scope
    )
    {
        var native = new mln_custom_geometry_source_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_custom_geometry_source_options);
        if (value.FetchTile is not null || value.CancelTile is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.fetch_tile = value.FetchTile is null
                ? null
                : &InvokeCustomGeometrySourceOptionsFetchTile;
            native.cancel_tile = value.CancelTile is null
                ? null
                : &InvokeCustomGeometrySourceOptionsCancelTile;
        }
        native.fields |= Put(
            value.MinZoom,
            ref native.min_zoom,
            MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
        );
        native.fields |= Put(
            value.MaxZoom,
            ref native.max_zoom,
            MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
        );
        native.fields |= Put(
            value.Tolerance,
            ref native.tolerance,
            MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
        );
        native.fields |= Put(
            value.TileSize,
            ref native.tile_size,
            MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
        );
        native.fields |= Put(
            value.Buffer,
            ref native.buffer,
            MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
        );
        native.fields |= Put(
            value.Clip,
            ref native.clip,
            MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP,
            static present => (byte)(present ? 1 : 0)
        );
        native.fields |= Put(
            value.Wrap,
            ref native.wrap,
            MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP,
            static present => (byte)(present ? 1 : 0)
        );
        return native;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeCustomVectorSourceOptionsFetchTile(
        void* user_data,
        mln_canonical_tile_id tile_id
    )
    {
        try
        {
            ((CustomVectorSourceOptions)NativeCallbackRoot.Value(user_data)).FetchTile?.Invoke(
                CopyCanonicalTileId(tile_id)
            );
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_custom_source_fetch_tile_callback", error);
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeCustomVectorSourceOptionsCancelTile(
        void* user_data,
        mln_canonical_tile_id tile_id
    )
    {
        try
        {
            ((CustomVectorSourceOptions)NativeCallbackRoot.Value(user_data)).CancelTile?.Invoke(
                CopyCanonicalTileId(tile_id)
            );
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_custom_source_cancel_tile_callback", error);
        }
    }

    internal static mln_custom_vector_source_options NativeCustomVectorSourceOptions(
        CustomVectorSourceOptions value,
        NativeCallScope scope
    )
    {
        var native = new mln_custom_vector_source_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_custom_vector_source_options);
        if (value.FetchTile is not null || value.CancelTile is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.fetch_tile = value.FetchTile is null
                ? null
                : &InvokeCustomVectorSourceOptionsFetchTile;
            native.cancel_tile = value.CancelTile is null
                ? null
                : &InvokeCustomVectorSourceOptionsCancelTile;
        }
        native.fields |= Put(
            value.MinZoom,
            ref native.min_zoom,
            MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM
        );
        native.fields |= Put(
            value.MaxZoom,
            ref native.max_zoom,
            MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM
        );
        return native;
    }

    internal static EdgeInsets CopyEdgeInsets(mln_edge_insets value) =>
        new(value.top, value.left, value.bottom, value.right);

    internal static mln_edge_insets NativeEdgeInsets(EdgeInsets value)
    {
        var native = new mln_edge_insets();
        native.top = value.Top;
        native.left = value.Left;
        native.bottom = value.Bottom;
        native.right = value.Right;
        return native;
    }

    internal static mln_egl_context_descriptor NativeEglContextDescriptor(
        EglContextDescriptor value
    )
    {
        var native = new mln_egl_context_descriptor();
        native.display = (void*)value.Display.Address;
        native.config = (void*)value.Config.Address;
        native.share_context = (void*)value.ShareContext.Address;
        native.client_api = (mln_open_gl_client_api)value.ClientApi;
        native.get_proc_address = (void*)value.GetProcAddress.Address;
        return native;
    }

    internal static RuntimeEvent[] CopyEventBatchViewEvents(mln_event_batch_view value)
    {
        var count = checked((int)value.event_count);
        if (value.event_size < sizeof(mln_runtime_event) || (count != 0 && value.events == null))
            throw new InvalidOperationException("Invalid native array storage.");
        var copied = new RuntimeEvent[count];
        for (var index = 0; index < count; index++)
        {
            var record = (byte*)value.events + checked((nuint)index * value.event_size);
            var item = *(mln_runtime_event*)record;
            copied[index] = CopyRuntimeEvent(item, record, value.event_size);
        }
        return copied;
    }

    internal static mln_feature_state_selector NativeFeatureStateSelector(
        FeatureStateSelector value,
        NativeCallScope scope
    )
    {
        Required(value.SourceId, "FeatureStateSelector.SourceId must not be null.");
        Required(value.SourceLayerId, "FeatureStateSelector.SourceLayerId must not be null.");
        Required(value.StateKey, "FeatureStateSelector.StateKey must not be null.");
        var native = new mln_feature_state_selector();
        native.fields = 0;
        native.size = (uint)sizeof(mln_feature_state_selector);
        native.source_id = scope.Utf8(value.SourceId);
        native.source_layer_id = scope.Utf8(value.SourceLayerId);
        native.fields |= Put(
            value.FeatureId,
            ref native.feature_id,
            MLN_FEATURE_STATE_SELECTOR_FEATURE_ID,
            scope,
            static (present, scope) => scope.Utf8(present)
        );
        native.state_key = scope.Utf8(value.StateKey);
        return native;
    }

    internal static mln_frame_demand NativeFrameDemand(FrameDemand value)
    {
        var native = new mln_frame_demand();
        native.size = (uint)sizeof(mln_frame_demand);
        native.flags = (uint)value.Flags;
        native.token = value.Token;
        return native;
    }

    internal static FreeCameraOptions CopyFreeCameraOptions(mln_free_camera_options value) =>
        new()
        {
            Position = value.fields.HasFlag(MLN_FREE_CAMERA_OPTION_POSITION)
                ? CopyVec3(value.position)
                : null,
            Orientation = value.fields.HasFlag(MLN_FREE_CAMERA_OPTION_ORIENTATION)
                ? CopyQuaternion(value.orientation)
                : null,
        };

    internal static mln_free_camera_options NativeFreeCameraOptions(FreeCameraOptions value)
    {
        var native = new mln_free_camera_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_free_camera_options);
        native.fields |= Put(
            value.Position,
            ref native.position,
            MLN_FREE_CAMERA_OPTION_POSITION,
            NativeVec3
        );
        native.fields |= Put(
            value.Orientation,
            ref native.orientation,
            MLN_FREE_CAMERA_OPTION_ORIENTATION,
            NativeQuaternion
        );
        return native;
    }

    internal static mln_geo_json_source_options NativeGeoJsonSourceOptions(
        GeoJsonSourceOptions value,
        NativeCallScope scope
    )
    {
        Required(
            value.ClusterProperties,
            "GeoJsonSourceOptions.ClusterProperties must not be null."
        );
        var native = new mln_geo_json_source_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_geo_json_source_options);
        native.fields |= Put(
            value.MinZoom,
            ref native.min_zoom,
            MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM
        );
        native.fields |= Put(
            value.MaxZoom,
            ref native.max_zoom,
            MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM
        );
        native.fields |= Put(
            value.Tolerance,
            ref native.tolerance,
            MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE
        );
        native.fields |= Put(
            value.ClusterMaxZoom,
            ref native.cluster_max_zoom,
            MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
        );
        native.cluster_properties = scope.Utf8(value.ClusterProperties);
        native.fields |= Put(value.Buffer, ref native.buffer, MLN_GEO_JSON_SOURCE_OPTION_BUFFER);
        native.fields |= Put(
            value.ClusterRadius,
            ref native.cluster_radius,
            MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS
        );
        native.fields |= Put(
            value.ClusterMinPoints,
            ref native.cluster_min_points,
            MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
        );
        native.fields |= Put(
            value.LineMetrics,
            ref native.line_metrics,
            MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS,
            static present => (byte)(present ? 1 : 0)
        );
        native.fields |= Put(
            value.Cluster,
            ref native.cluster,
            MLN_GEO_JSON_SOURCE_OPTION_CLUSTER,
            static present => (byte)(present ? 1 : 0)
        );
        native.fields |= Put(
            value.SynchronousTiling,
            ref native.synchronous_tiling,
            MLN_GEO_JSON_SOURCE_OPTION_SYNCHRONOUS_TILING,
            static present => (byte)(present ? 1 : 0)
        );
        return native;
    }

    internal static GpuSync CopyGpuSync(mln_gpu_sync value) =>
        new((GpuSyncKind)value.kind, value.handle);

    internal static mln_gpu_sync NativeGpuSync(GpuSync value)
    {
        var native = new mln_gpu_sync();
        native.size = (uint)sizeof(mln_gpu_sync);
        native.kind = (mln_gpu_sync_kind)value.Kind;
        native.handle = value.Handle;
        return native;
    }

    internal static mln_graphics_device_options NativeGraphicsDeviceOptions(
        GraphicsDeviceOptions value
    )
    {
        var native = new mln_graphics_device_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_graphics_device_options);
        native.fields |= Put(
            value.AdapterIndex,
            ref native.adapter_index,
            MLN_GRAPHICS_DEVICE_OPTION_ADAPTER_INDEX
        );
        native.fields |= Put(
            value.SingleContext,
            ref native.single_context,
            MLN_GRAPHICS_DEVICE_OPTION_SINGLE_CONTEXT,
            static present => (byte)(present ? 1 : 0)
        );
        return native;
    }

    internal static mln_graphics_device_texture_descriptor NativeGraphicsDeviceTextureDescriptor(
        GraphicsDeviceTextureDescriptor value
    )
    {
        var native = new mln_graphics_device_texture_descriptor();
        native.size = (uint)sizeof(mln_graphics_device_texture_descriptor);
        native.extent = NativeLogicalExtent(value.Extent);
        return native;
    }

    internal static mln_http_header NativeHttpHeader(HttpHeader value, NativeCallScope scope)
    {
        Required(value.Name, "HttpHeader.Name must not be null.");
        Required(value.Value, "HttpHeader.Value must not be null.");
        var native = new mln_http_header();
        native.name = scope.Utf8(value.Name);
        native.value = scope.Utf8(value.Value);
        return native;
    }

    internal static mln_http_header_rule NativeHttpHeaderRule(
        HttpHeaderRule value,
        NativeCallScope scope
    )
    {
        Required(value.Url, "HttpHeaderRule.Url must not be null.");
        var native = new mln_http_header_rule();
        native.size = (uint)sizeof(mln_http_header_rule);
        native.kinds = (uint)value.Kinds;
        native.flags = (uint)value.Flags;
        native.url = scope.Utf8(value.Url);
        native.headers = scope.Array<mln_http_header, HttpHeader>(
            value.HeadersStorage.Items,
            static (item, scope) => NativeHttpHeader(item, scope)
        );
        native.header_count = checked((nuint)value.HeadersStorage.Items.Length);
        return native;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static mln_status InvokeHttpHeaderTransformCallback(
        void* user_data,
        mln_resource_kind kind,
        mln_buffer_view url,
        mln_http_header_transform_response* out_response
    )
    {
        try
        {
            var responseOutResponse = new HttpHeaderTransformResponse(out_response);
            try
            {
                ((HttpHeaderTransform)NativeCallbackRoot.Value(user_data)).Callback?.Invoke(
                    (ResourceKind)kind,
                    RuntimeStructs.CopyUtf8((sbyte*)url.data, url.size),
                    responseOutResponse
                );
            }
            finally
            {
                responseOutResponse.Expire();
            }
            return mln_status.MLN_STATUS_OK;
        }
        catch (MaplibreException error) when ((mln_status)error.Status != mln_status.MLN_STATUS_OK)
        {
            return (mln_status)error.Status;
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_http_header_transform_callback", error);
            return mln_status.MLN_STATUS_NATIVE_ERROR;
        }
    }

    internal static mln_http_header_transform NativeHttpHeaderTransform(
        HttpHeaderTransform value,
        NativeCallScope scope
    )
    {
        var native = new mln_http_header_transform();
        native.size = (uint)sizeof(mln_http_header_transform);
        if (value.Callback is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.callback = value.Callback is null ? null : &InvokeHttpHeaderTransformCallback;
        }
        native.rules = scope.Array<mln_http_header_rule, HttpHeaderRule>(
            value.RulesStorage.Items,
            static (item, scope) => NativeHttpHeaderRule(item, scope)
        );
        native.rule_count = checked((nuint)value.RulesStorage.Items.Length);
        return native;
    }

    internal static ImageContent CopyImageContent(mln_image_content value) =>
        new(value.left, value.top, value.right, value.bottom);

    internal static mln_image_content NativeImageContent(ImageContent value)
    {
        var native = new mln_image_content();
        native.left = value.Left;
        native.top = value.Top;
        native.right = value.Right;
        native.bottom = value.Bottom;
        return native;
    }

    internal static ImageStretch CopyImageStretch(mln_image_stretch value) =>
        new(value.from, value.to);

    internal static mln_image_stretch NativeImageStretch(ImageStretch value)
    {
        var native = new mln_image_stretch();
        native.from = value.From;
        native.to = value.To;
        return native;
    }

    internal static LatLng CopyLatLng(mln_lat_lng value) => new(value.latitude, value.longitude);

    internal static mln_lat_lng NativeLatLng(LatLng value)
    {
        var native = new mln_lat_lng();
        native.latitude = value.Latitude;
        native.longitude = value.Longitude;
        return native;
    }

    internal static LatLngBounds CopyLatLngBounds(mln_lat_lng_bounds value) =>
        new(CopyLatLng(value.southwest), CopyLatLng(value.northeast));

    internal static mln_lat_lng_bounds NativeLatLngBounds(LatLngBounds value)
    {
        var native = new mln_lat_lng_bounds();
        native.southwest = NativeLatLng(value.Southwest);
        native.northeast = NativeLatLng(value.Northeast);
        return native;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static byte InvokeLogHandlerCallback(
        void* user_data,
        mln_log_severity severity,
        mln_log_event @event,
        long code,
        mln_buffer_view message
    )
    {
        try
        {
            var answer = ((LogHandler)NativeCallbackRoot.Value(user_data)).Callback;
            return answer is null
                ? (byte)0
                : (byte)(
                    answer(
                        (LogSeverity)severity,
                        (LogEvent)@event,
                        code,
                        RuntimeStructs.CopyUtf8((sbyte*)message.data, message.size)
                    )
                        ? 1
                        : 0
                );
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_log_callback", error);
            return (byte)0;
        }
    }

    internal static mln_log_handler NativeLogHandler(LogHandler value, NativeCallScope scope)
    {
        var native = new mln_log_handler();
        native.size = (uint)sizeof(mln_log_handler);
        if (value.Callback is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.callback = value.Callback is null ? null : &InvokeLogHandlerCallback;
        }
        return native;
    }

    internal static LogicalExtent CopyLogicalExtent(mln_logical_extent value) =>
        new(value.width, value.height, value.scale_factor);

    internal static mln_logical_extent NativeLogicalExtent(LogicalExtent value)
    {
        var native = new mln_logical_extent();
        native.width = value.Width;
        native.height = value.Height;
        native.scale_factor = value.ScaleFactor;
        return native;
    }

    internal static mln_map_options NativeMapOptions(MapOptions value)
    {
        var native = new mln_map_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_map_options);
        native.initial_extent = NativeLogicalExtent(value.InitialExtent);
        native.map_mode = (mln_map_mode)value.MapMode;
        native.fast_pfor_enabled = (byte)(value.FastPforEnabled ? 1 : 0);
        native.fields |= Put(
            value.EventMask,
            ref native.event_mask,
            MLN_MAP_OPTION_EVENT_MASK,
            static present => (ulong)present
        );
        return native;
    }

    internal static MapSnapshot CopyMapSnapshot(mln_map_snapshot value) =>
        new(
            (MapDebugFlag)value.debug_options,
            value.generation,
            value.fully_loaded != 0,
            value.rendering_stats_view_enabled != 0,
            value.gesture_in_progress != 0,
            value.transition_in_progress != 0,
            (RuntimeEventFlag)value.event_mask,
            value.latest_render_update_generation,
            CopyMapTileOptions(value.tile),
            CopyBoundOptions(value.bounds),
            CopyFreeCameraOptions(value.free_camera),
            CopyMapView(value.view)
        );

    internal static MapTileOptions CopyMapTileOptions(mln_map_tile_options value) =>
        new()
        {
            PrefetchZoomDelta = value.fields.HasFlag(MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA)
                ? value.prefetch_zoom_delta
                : null,
            LodMinRadius = value.fields.HasFlag(MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS)
                ? value.lod_min_radius
                : null,
            LodScale = value.fields.HasFlag(MLN_MAP_TILE_OPTION_LOD_SCALE) ? value.lod_scale : null,
            LodPitchThreshold = value.fields.HasFlag(MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD)
                ? value.lod_pitch_threshold
                : null,
            LodZoomShift = value.fields.HasFlag(MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT)
                ? value.lod_zoom_shift
                : null,
            LodMode = value.fields.HasFlag(MLN_MAP_TILE_OPTION_LOD_MODE)
                ? (TileLodMode)value.lod_mode
                : null,
        };

    internal static mln_map_tile_options NativeMapTileOptions(MapTileOptions value)
    {
        var native = new mln_map_tile_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_map_tile_options);
        native.fields |= Put(
            value.PrefetchZoomDelta,
            ref native.prefetch_zoom_delta,
            MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
        );
        native.fields |= Put(
            value.LodMinRadius,
            ref native.lod_min_radius,
            MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS
        );
        native.fields |= Put(value.LodScale, ref native.lod_scale, MLN_MAP_TILE_OPTION_LOD_SCALE);
        native.fields |= Put(
            value.LodPitchThreshold,
            ref native.lod_pitch_threshold,
            MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
        );
        native.fields |= Put(
            value.LodZoomShift,
            ref native.lod_zoom_shift,
            MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT
        );
        native.fields |= Put(
            value.LodMode,
            ref native.lod_mode,
            MLN_MAP_TILE_OPTION_LOD_MODE,
            static present => (mln_tile_lod_mode)present
        );
        return native;
    }

    internal static MapView CopyMapView(mln_map_view value) =>
        new(
            CopyCamera(value.camera),
            CopyLogicalExtent(value.extent),
            CopyProjectionMode(value.projection_mode),
            CopyMapViewportOptions(value.viewport)
        );

    internal static mln_map_view NativeMapView(MapView value)
    {
        Required(value.ProjectionMode, "MapView.ProjectionMode must not be null.");
        Required(value.Viewport, "MapView.Viewport must not be null.");
        var native = new mln_map_view();
        native.size = (uint)sizeof(mln_map_view);
        native.camera = NativeCamera(value.Camera);
        native.extent = NativeLogicalExtent(value.Extent);
        native.projection_mode = NativeProjectionMode(value.ProjectionMode);
        native.viewport = NativeMapViewportOptions(value.Viewport);
        return native;
    }

    internal static MapViewportOptions CopyMapViewportOptions(mln_map_viewport_options value) =>
        new()
        {
            NorthOrientation = value.fields.HasFlag(MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION)
                ? (NorthOrientation)value.north_orientation
                : null,
            ConstrainMode = value.fields.HasFlag(MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE)
                ? (ConstrainMode)value.constrain_mode
                : null,
            ViewportMode = value.fields.HasFlag(MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE)
                ? (ViewportMode)value.viewport_mode
                : null,
            FrustumOffset = value.fields.HasFlag(MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET)
                ? CopyEdgeInsets(value.frustum_offset)
                : null,
        };

    internal static mln_map_viewport_options NativeMapViewportOptions(MapViewportOptions value)
    {
        var native = new mln_map_viewport_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_map_viewport_options);
        native.fields |= Put(
            value.NorthOrientation,
            ref native.north_orientation,
            MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION,
            static present => (mln_north_orientation)present
        );
        native.fields |= Put(
            value.ConstrainMode,
            ref native.constrain_mode,
            MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE,
            static present => (mln_constrain_mode)present
        );
        native.fields |= Put(
            value.ViewportMode,
            ref native.viewport_mode,
            MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE,
            static present => (mln_viewport_mode)present
        );
        native.fields |= Put(
            value.FrustumOffset,
            ref native.frustum_offset,
            MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET,
            NativeEdgeInsets
        );
        return native;
    }

    internal static mln_metal_borrowed_texture NativeMetalBorrowedTexture(
        MetalBorrowedTexture value
    )
    {
        var native = new mln_metal_borrowed_texture();
        native.texture = (void*)value.Texture.Address;
        return native;
    }

    internal static mln_metal_borrowed_texture_descriptor NativeMetalBorrowedTextureDescriptor(
        MetalBorrowedTextureDescriptor value,
        NativeCallScope scope
    )
    {
        var native = new mln_metal_borrowed_texture_descriptor();
        native.size = (uint)sizeof(mln_metal_borrowed_texture_descriptor);
        native.extent = NativeLogicalExtent(value.Extent);
        native.physical_width = value.PhysicalWidth;
        native.physical_height = value.PhysicalHeight;
        native.textures = scope.Array<mln_metal_borrowed_texture, MetalBorrowedTexture>(
            value.TexturesStorage.Items,
            static (item, scope) => NativeMetalBorrowedTexture(item)
        );
        native.texture_count = checked((nuint)value.TexturesStorage.Items.Length);
        return native;
    }

    internal static mln_metal_context_descriptor NativeMetalContextDescriptor(
        MetalContextDescriptor value
    )
    {
        var native = new mln_metal_context_descriptor();
        native.device = (void*)value.Device.Address;
        return native;
    }

    internal static mln_metal_owned_texture_descriptor NativeMetalOwnedTextureDescriptor(
        MetalOwnedTextureDescriptor value
    )
    {
        var native = new mln_metal_owned_texture_descriptor();
        native.size = (uint)sizeof(mln_metal_owned_texture_descriptor);
        native.extent = NativeLogicalExtent(value.Extent);
        native.context = NativeMetalContextDescriptor(value.Context);
        native.pixel_format = value.PixelFormat;
        return native;
    }

    internal static mln_metal_surface_descriptor NativeMetalSurfaceDescriptor(
        MetalSurfaceDescriptor value
    )
    {
        var native = new mln_metal_surface_descriptor();
        native.size = (uint)sizeof(mln_metal_surface_descriptor);
        native.extent = NativeLogicalExtent(value.Extent);
        native.context = NativeMetalContextDescriptor(value.Context);
        native.layer = (void*)value.Layer.Address;
        return native;
    }

    internal static MetalTextureFrame CopyMetalTextureFrame(mln_metal_texture_frame value) =>
        new(
            value.generation,
            value.width,
            value.height,
            value.scale_factor,
            value.frame_id,
            value.slot,
            NativePointer.FromNativeAddress((nint)value.texture),
            NativePointer.FromNativeAddress((nint)value.device),
            value.pixel_format
        );

    internal static OfflineGeometryRegionDefinition CopyOfflineGeometryRegionDefinition(
        mln_offline_geometry_region_definition value
    ) =>
        default(OfflineGeometryRegionDefinition) with
        {
            StyleUrl = RuntimeStructs.CopyUtf8((sbyte*)value.style_url.data, value.style_url.size),
            Geometry = RuntimeStructs.CopyUtf8((sbyte*)value.geometry.data, value.geometry.size),
            MinZoom = value.min_zoom,
            MaxZoom = value.max_zoom,
            PixelRatio = value.pixel_ratio,
            IncludeIdeographs = value.include_ideographs != 0,
        };

    internal static mln_offline_geometry_region_definition NativeOfflineGeometryRegionDefinition(
        OfflineGeometryRegionDefinition value,
        NativeCallScope scope
    )
    {
        Required(value.StyleUrl, "OfflineGeometryRegionDefinition.StyleUrl must not be null.");
        Required(value.Geometry, "OfflineGeometryRegionDefinition.Geometry must not be null.");
        var native = new mln_offline_geometry_region_definition();
        native.style_url = scope.Utf8(value.StyleUrl);
        native.geometry = scope.Utf8(value.Geometry);
        native.min_zoom = value.MinZoom;
        native.max_zoom = value.MaxZoom;
        native.pixel_ratio = value.PixelRatio;
        native.include_ideographs = (byte)(value.IncludeIdeographs ? 1 : 0);
        return native;
    }

    internal static OfflineRegionDefinition CopyOfflineRegionDefinition(
        mln_offline_region_definition value
    ) =>
        value.type switch
        {
            mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID =>
                new OfflineRegionDefinition.TilePyramid(
                    CopyOfflineTilePyramidRegionDefinition(value.data.tile_pyramid)
                ),
            mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY =>
                new OfflineRegionDefinition.Geometry(
                    CopyOfflineGeometryRegionDefinition(value.data.geometry)
                ),
            _ => new OfflineRegionDefinition.Unknown(
                (uint)value.type,
                NativeCallScope.CopyValueBytes(value.data)
            ),
        };

    internal static mln_offline_region_definition NativeOfflineRegionDefinition(
        OfflineRegionDefinition value,
        NativeCallScope scope
    )
    {
        var native = new mln_offline_region_definition();
        native.size = (uint)sizeof(mln_offline_region_definition);
        switch (value)
        {
            case OfflineRegionDefinition.TilePyramid selected:
                native.type =
                    mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
                native.data.tile_pyramid = NativeOfflineTilePyramidRegionDefinition(
                    selected.Value,
                    scope
                );
                break;
            case OfflineRegionDefinition.Geometry selected:
                native.type =
                    mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
                native.data.geometry = NativeOfflineGeometryRegionDefinition(selected.Value, scope);
                break;
            default:
                throw new ArgumentException("A union variant is required.", nameof(value));
        }
        return native;
    }

    internal static OfflineRegionInfo CopyOfflineRegionInfo(mln_offline_region_info value) =>
        default(OfflineRegionInfo) with
        {
            Id = value.id,
            Definition = CopyOfflineRegionDefinition(value.definition),
            MetadataStorage = new(
                NativeCallScope.CopyArray<byte>((byte*)value.metadata, (nuint)(value.metadata_size))
            ),
        };

    internal static OfflineRegionStatus CopyOfflineRegionStatus(mln_offline_region_status value) =>
        new(
            (OfflineRegionDownloadState)value.download_state,
            value.completed_resource_count,
            value.completed_resource_size,
            value.completed_tile_count,
            value.required_tile_count,
            value.completed_tile_size,
            value.required_resource_count,
            value.required_resource_count_is_precise != 0,
            value.complete != 0
        );

    internal static OfflineTilePyramidRegionDefinition CopyOfflineTilePyramidRegionDefinition(
        mln_offline_tile_pyramid_region_definition value
    ) =>
        new(
            RuntimeStructs.CopyUtf8((sbyte*)value.style_url.data, value.style_url.size),
            CopyLatLngBounds(value.bounds),
            value.min_zoom,
            value.max_zoom,
            value.pixel_ratio,
            value.include_ideographs != 0
        );

    internal static mln_offline_tile_pyramid_region_definition NativeOfflineTilePyramidRegionDefinition(
        OfflineTilePyramidRegionDefinition value,
        NativeCallScope scope
    )
    {
        Required(value.StyleUrl, "OfflineTilePyramidRegionDefinition.StyleUrl must not be null.");
        var native = new mln_offline_tile_pyramid_region_definition();
        native.style_url = scope.Utf8(value.StyleUrl);
        native.bounds = NativeLatLngBounds(value.Bounds);
        native.min_zoom = value.MinZoom;
        native.max_zoom = value.MaxZoom;
        native.pixel_ratio = value.PixelRatio;
        native.include_ideographs = (byte)(value.IncludeIdeographs ? 1 : 0);
        return native;
    }

    internal static mln_open_gl_borrowed_texture NativeOpenGlBorrowedTexture(
        OpenGlBorrowedTexture value
    )
    {
        var native = new mln_open_gl_borrowed_texture();
        native.texture = value.Texture;
        return native;
    }

    internal static mln_open_gl_borrowed_texture_descriptor NativeOpenGlBorrowedTextureDescriptor(
        OpenGlBorrowedTextureDescriptor value,
        NativeCallScope scope
    )
    {
        var native = new mln_open_gl_borrowed_texture_descriptor();
        native.size = (uint)sizeof(mln_open_gl_borrowed_texture_descriptor);
        native.extent = NativeLogicalExtent(value.Extent);
        native.physical_width = value.PhysicalWidth;
        native.physical_height = value.PhysicalHeight;
        native.context = NativeOpenGlContextDescriptor(value.Context, scope);
        native.textures = scope.Array<mln_open_gl_borrowed_texture, OpenGlBorrowedTexture>(
            value.TexturesStorage.Items,
            static (item, scope) => NativeOpenGlBorrowedTexture(item)
        );
        native.texture_count = checked((nuint)value.TexturesStorage.Items.Length);
        native.target = value.Target;
        return native;
    }

    internal static mln_open_gl_context_descriptor NativeOpenGlContextDescriptor(
        OpenGlContextDescriptor value,
        NativeCallScope scope
    )
    {
        var native = new mln_open_gl_context_descriptor();
        native.ownership = (mln_open_gl_context_ownership)value.Ownership;
        switch (value.Data)
        {
            case OpenGlContextDescriptor.DataValue.Wgl selected:
                native.platform = mln_open_gl_context_platform.MLN_OPEN_GL_CONTEXT_PLATFORM_WGL;
                native.data.wgl = NativeWglContextDescriptor(selected.Value);
                break;
            case OpenGlContextDescriptor.DataValue.Egl selected:
                native.platform = mln_open_gl_context_platform.MLN_OPEN_GL_CONTEXT_PLATFORM_EGL;
                native.data.egl = NativeEglContextDescriptor(selected.Value);
                break;
            case OpenGlContextDescriptor.DataValue.WebGl selected:
                native.platform = mln_open_gl_context_platform.MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL;
                native.data.web_gl = NativeWebGlContextDescriptor(selected.Value, scope);
                break;
            default:
                throw new ArgumentException("A union variant is required.", nameof(value));
        }
        return native;
    }

    internal static mln_open_gl_owned_texture_descriptor NativeOpenGlOwnedTextureDescriptor(
        OpenGlOwnedTextureDescriptor value,
        NativeCallScope scope
    )
    {
        var native = new mln_open_gl_owned_texture_descriptor();
        native.size = (uint)sizeof(mln_open_gl_owned_texture_descriptor);
        native.extent = NativeLogicalExtent(value.Extent);
        native.context = NativeOpenGlContextDescriptor(value.Context, scope);
        return native;
    }

    internal static mln_open_gl_surface_descriptor NativeOpenGlSurfaceDescriptor(
        OpenGlSurfaceDescriptor value,
        NativeCallScope scope
    )
    {
        var native = new mln_open_gl_surface_descriptor();
        native.size = (uint)sizeof(mln_open_gl_surface_descriptor);
        native.extent = NativeLogicalExtent(value.Extent);
        native.context = NativeOpenGlContextDescriptor(value.Context, scope);
        native.surface = (void*)value.Surface.Address;
        return native;
    }

    internal static OpenGlTextureFrame CopyOpenGlTextureFrame(mln_open_gl_texture_frame value) =>
        new(
            value.generation,
            value.width,
            value.height,
            value.scale_factor,
            value.frame_id,
            value.slot,
            value.texture,
            value.target,
            value.internal_format,
            value.format,
            value.type
        );

    internal static mln_premultiplied_rgba8_image NativePremultipliedRgba8Image(
        PremultipliedRgba8Image value,
        NativeCallScope scope
    )
    {
        var native = new mln_premultiplied_rgba8_image();
        native.size = (uint)sizeof(mln_premultiplied_rgba8_image);
        native.width = value.Width;
        native.height = value.Height;
        native.stride = value.Stride;
        var bufferPixels = scope.Buffer(value.PixelsStorage.Items);
        native.pixels = (byte*)bufferPixels.data;
        native.byte_length = checked((nuint)bufferPixels.size);
        return native;
    }

    internal static ProjectedMeters CopyProjectedMeters(mln_projected_meters value) =>
        new(value.northing, value.easting);

    internal static mln_projected_meters NativeProjectedMeters(ProjectedMeters value)
    {
        var native = new mln_projected_meters();
        native.northing = value.Northing;
        native.easting = value.Easting;
        return native;
    }

    internal static ProjectionMode CopyProjectionMode(mln_projection_mode value) =>
        new()
        {
            Axonometric = value.fields.HasFlag(MLN_PROJECTION_MODE_AXONOMETRIC)
                ? value.axonometric != 0
                : null,
            XSkew = value.fields.HasFlag(MLN_PROJECTION_MODE_X_SKEW) ? value.x_skew : null,
            YSkew = value.fields.HasFlag(MLN_PROJECTION_MODE_Y_SKEW) ? value.y_skew : null,
        };

    internal static mln_projection_mode NativeProjectionMode(ProjectionMode value)
    {
        var native = new mln_projection_mode();
        native.fields = 0;
        native.size = (uint)sizeof(mln_projection_mode);
        native.fields |= Put(
            value.Axonometric,
            ref native.axonometric,
            MLN_PROJECTION_MODE_AXONOMETRIC,
            static present => (byte)(present ? 1 : 0)
        );
        native.fields |= Put(value.XSkew, ref native.x_skew, MLN_PROJECTION_MODE_X_SKEW);
        native.fields |= Put(value.YSkew, ref native.y_skew, MLN_PROJECTION_MODE_Y_SKEW);
        return native;
    }

    internal static Quaternion CopyQuaternion(mln_quaternion value) =>
        new(value.x, value.y, value.z, value.w);

    internal static mln_quaternion NativeQuaternion(Quaternion value)
    {
        var native = new mln_quaternion();
        native.x = value.X;
        native.y = value.Y;
        native.z = value.Z;
        native.w = value.W;
        return native;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeQueueLockLock(void* user_data)
    {
        try
        {
            ((QueueLock)NativeCallbackRoot.Value(user_data)).Lock?.Invoke();
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_queue_lock_callback", error);
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeQueueLockUnlock(void* user_data)
    {
        try
        {
            ((QueueLock)NativeCallbackRoot.Value(user_data)).Unlock?.Invoke();
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_queue_lock_callback", error);
        }
    }

    internal static mln_queue_lock NativeQueueLock(QueueLock value, NativeCallScope scope)
    {
        var native = new mln_queue_lock();
        if (value.NativeRegistration is { } registration)
        {
            scope.Consume(registration);
            if (value.Lock is not null || value.Unlock is not null)
                throw new ArgumentException(
                    "A native registration carries no delegates.",
                    nameof(value)
                );
            native.@lock = (delegate* unmanaged[Cdecl]<void*, void>)registration.Function(0);
            native.unlock = (delegate* unmanaged[Cdecl]<void*, void>)registration.Function(1);
            native.user_data = registration.UserData;
            native.release_user_data = (delegate* unmanaged[Cdecl]<void*, void>)
                registration.ReleaseUserData;
        }
        else if (value.Lock is not null || value.Unlock is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.@lock = value.Lock is null ? null : &InvokeQueueLockLock;
            native.unlock = value.Unlock is null ? null : &InvokeQueueLockUnlock;
        }
        return native;
    }

    internal static RenderAbandonResult CopyRenderAbandonResult(mln_render_abandon_result value) =>
        new((RenderAbandonDisposition)value.disposition, value.quarantined_resource_count);

    internal static RenderFrameResult[] CopyRenderFrameBatchViewResults(
        mln_render_frame_batch_view value
    )
    {
        var count = checked((int)value.result_count);
        if (
            value.result_size < sizeof(mln_render_frame_result)
            || (count != 0 && value.results == null)
        )
            throw new InvalidOperationException("Invalid native array storage.");
        var copied = new RenderFrameResult[count];
        for (var index = 0; index < count; index++)
        {
            var record = (byte*)value.results + checked((nuint)index * value.result_size);
            var item = *(mln_render_frame_result*)record;
            copied[index] = CopyRenderFrameResult(item, record, value.result_size);
        }
        return copied;
    }

    internal static RenderFrameResult CopyRenderFrameResult(
        mln_render_frame_result value,
        byte* record = null,
        nuint recordSize = 0
    ) =>
        new(
            (RenderResult)value.disposition,
            value.token,
            value.map_update_generation,
            value.generation,
            value.frame_generation,
            value.needs_repaint != 0,
            CopyMapView(value.view),
            (RenderMode)value.mode
        );

    internal static mln_render_session_attach_options NativeRenderSessionAttachOptions(
        RenderSessionAttachOptions value,
        NativeCallScope scope
    )
    {
        var native = new mln_render_session_attach_options();
        native.size = (uint)sizeof(mln_render_session_attach_options);
        native.driver = (mln_render_driver_kind)value.Driver;
        native.requested_texture_ring_depth = value.RequestedTextureRingDepth;
        native.producer_sync_kind = (mln_gpu_sync_kind)value.ProducerSyncKind;
        native.frame_wake = NativeWake(value.FrameWake, scope);
        native.driver_work_wake = NativeWake(value.DriverWorkWake, scope);
        native.queue_lock = NativeQueueLock(value.QueueLock, scope);
        return native;
    }

    internal static RenderSessionCapabilities CopyRenderSessionCapabilities(
        mln_render_session_capabilities value
    ) =>
        new(
            (RenderDriverKind)value.driver,
            value.texture_ring_depth,
            (RenderSessionCapabilityFlag)value.flags
        );

    internal static RenderSessionSnapshot CopyRenderSessionSnapshot(
        mln_render_session_snapshot value
    ) =>
        new(
            (RenderSessionState)value.state,
            CopyLogicalExtent(value.extent),
            value.generation,
            value.map_update_generation,
            value.rendered_update_generation,
            value.frame_generation,
            value.acquired_frame_count,
            value.pending_changes != 0
        );

    internal static RenderedFeature CopyRenderedFeature(mln_rendered_feature value) =>
        new(
            RuntimeStructs.CopyUtf8((sbyte*)value.feature.data, value.feature.size),
            RuntimeStructs.CopyUtf8((sbyte*)value.layer_id.data, value.layer_id.size),
            RuntimeStructs.CopyUtf8((sbyte*)value.source_id.data, value.source_id.size),
            RuntimeStructs.CopyUtf8((sbyte*)value.source_layer_id.data, value.source_layer_id.size),
            RuntimeStructs.CopyUtf8((sbyte*)value.state.data, value.state.size)
        );

    internal static mln_rendered_feature_query_options NativeRenderedFeatureQueryOptions(
        RenderedFeatureQueryOptions value,
        NativeCallScope scope
    )
    {
        Required(value.Filter, "RenderedFeatureQueryOptions.Filter must not be null.");
        var native = new mln_rendered_feature_query_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_rendered_feature_query_options);
        if (value.LayerIdsStorage?.Items is { } fieldLayerIds)
        {
            native.fields |= MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
            native.layer_ids = scope.Array<mln_buffer_view, string>(
                fieldLayerIds,
                static (item, scope) => scope.Utf8(item)
            );
            native.layer_id_count = checked((nuint)fieldLayerIds.Length);
        }
        native.filter = scope.Utf8(value.Filter);
        return native;
    }

    internal static mln_rendered_query_geometry NativeRenderedQueryGeometry(
        RenderedQueryGeometry value,
        NativeCallScope scope
    )
    {
        var native = new mln_rendered_query_geometry();
        native.size = (uint)sizeof(mln_rendered_query_geometry);
        switch (value)
        {
            case RenderedQueryGeometry.Point selected:
                native.type =
                    mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT;
                native.data.point = NativeScreenPoint(selected.Value);
                break;
            case RenderedQueryGeometry.Box selected:
                native.type = mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX;
                native.data.box = NativeScreenBox(selected.Value);
                break;
            case RenderedQueryGeometry.LineString selected:
                native.type =
                    mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING;
                native.data.line_string = NativeScreenLineString(selected.Value, scope);
                break;
            default:
                throw new ArgumentException("A union variant is required.", nameof(value));
        }
        return native;
    }

    internal static RenderingStats CopyRenderingStats(mln_rendering_stats value) =>
        new(
            value.encoding_time,
            value.rendering_time,
            value.frame_count,
            value.draw_call_count,
            value.total_draw_call_count
        );

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeResourceProviderCallback(
        void* user_data,
        mln_resource_request_info* request,
        MlnResourceRequest handle
    )
    {
        ResourceRequestHandle? ownedHandle = null;
        try
        {
            ownedHandle = ResourceRequestHandle.Adopt(handle);
            ((ResourceProvider)NativeCallbackRoot.Value(user_data)).Callback?.Invoke(
                CopyResourceRequestInfo(NativeCallScope.Read(request)),
                ownedHandle
            );
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_resource_provider_callback", error);
            ownedHandle?.Dispose();
        }
    }

    internal static mln_resource_provider NativeResourceProvider(
        ResourceProvider value,
        NativeCallScope scope
    )
    {
        var native = new mln_resource_provider();
        native.size = (uint)sizeof(mln_resource_provider);
        if (value.Callback is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.callback = value.Callback is null ? null : &InvokeResourceProviderCallback;
        }
        native.routes = scope.Array<mln_resource_route, ResourceRoute>(
            value.RoutesStorage.Items,
            static (item, scope) => NativeResourceRoute(item, scope)
        );
        native.route_count = checked((nuint)value.RoutesStorage.Items.Length);
        return native;
    }

    internal static ResourceRange CopyResourceRange(mln_resource_range value) =>
        new(value.start, value.end);

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeResourceRequestCancelHandlerCallback(
        void* user_data,
        MlnResourceRequest handle
    )
    {
        try
        {
            ((ResourceRequestCancelHandler)NativeCallbackRoot.Value(user_data)).Callback?.Invoke();
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_resource_request_cancel_callback", error);
        }
    }

    internal static mln_resource_request_cancel_handler NativeResourceRequestCancelHandler(
        ResourceRequestCancelHandler value,
        NativeCallScope scope
    )
    {
        var native = new mln_resource_request_cancel_handler();
        native.size = (uint)sizeof(mln_resource_request_cancel_handler);
        if (value.Callback is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.callback = value.Callback is null
                ? null
                : &InvokeResourceRequestCancelHandlerCallback;
        }
        return native;
    }

    internal static ResourceRequestInfo CopyResourceRequestInfo(mln_resource_request_info value) =>
        new()
        {
            RequestedUrl = RuntimeStructs.CopyUtf8(
                (sbyte*)value.requested_url.data,
                value.requested_url.size
            ),
            ResolvedUrl = RuntimeStructs.CopyUtf8(
                (sbyte*)value.resolved_url.data,
                value.resolved_url.size
            ),
            Kind = (ResourceKind)value.kind,
            LoadingMethod = (ResourceLoadingMethod)value.loading_method,
            Priority = (ResourcePriority)value.priority,
            Usage = (ResourceUsage)value.usage,
            StoragePolicy = (ResourceStoragePolicy)value.storage_policy,
            Range = value.fields.HasFlag(MLN_RESOURCE_REQUEST_RANGE)
                ? CopyResourceRange(value.range)
                : null,
            PriorModifiedUnixMs = value.fields.HasFlag(MLN_RESOURCE_REQUEST_PRIOR_MODIFIED)
                ? value.prior_modified_unix_ms
                : null,
            PriorExpiresUnixMs = value.fields.HasFlag(MLN_RESOURCE_REQUEST_PRIOR_EXPIRES)
                ? value.prior_expires_unix_ms
                : null,
            PriorEtag = RuntimeStructs.CopyUtf8(
                (sbyte*)value.prior_etag.data,
                value.prior_etag.size
            ),
            PriorDataStorage = new(
                NativeCallScope.CopyArray<byte>(
                    (byte*)value.prior_data,
                    (nuint)(value.prior_data_size)
                )
            ),
        };

    internal static mln_resource_response NativeResourceResponse(
        ResourceResponse value,
        NativeCallScope scope
    )
    {
        Required(value.ErrorMessage, "ResourceResponse.ErrorMessage must not be null.");
        Required(value.Etag, "ResourceResponse.Etag must not be null.");
        var native = new mln_resource_response();
        native.fields = 0;
        native.size = (uint)sizeof(mln_resource_response);
        native.status = (mln_resource_response_status)value.Status;
        native.error_reason = (mln_resource_error_reason)value.ErrorReason;
        var bufferBytes = scope.Buffer(value.BytesStorage.Items);
        native.bytes = (byte*)bufferBytes.data;
        native.byte_count = checked((nuint)bufferBytes.size);
        native.error_message = scope.Utf8(value.ErrorMessage);
        native.must_revalidate = (byte)(value.MustRevalidate ? 1 : 0);
        native.fields |= Put(
            value.ModifiedUnixMs,
            ref native.modified_unix_ms,
            MLN_RESOURCE_RESPONSE_MODIFIED
        );
        native.fields |= Put(
            value.ExpiresUnixMs,
            ref native.expires_unix_ms,
            MLN_RESOURCE_RESPONSE_EXPIRES
        );
        native.etag = scope.Utf8(value.Etag);
        native.fields |= Put(
            value.RetryAfterUnixMs,
            ref native.retry_after_unix_ms,
            MLN_RESOURCE_RESPONSE_RETRY_AFTER
        );
        return native;
    }

    internal static mln_resource_route NativeResourceRoute(
        ResourceRoute value,
        NativeCallScope scope
    )
    {
        Required(value.Url, "ResourceRoute.Url must not be null.");
        var native = new mln_resource_route();
        native.size = (uint)sizeof(mln_resource_route);
        native.kinds = (uint)value.Kinds;
        native.flags = (uint)value.Flags;
        native.url = scope.Utf8(value.Url);
        return native;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeResourceTransformCallback(
        void* user_data,
        mln_resource_kind kind,
        mln_buffer_view url,
        MlnResourceTransformRequest request
    )
    {
        ResourceTransformRequestHandle? ownedRequest = null;
        try
        {
            ownedRequest = ResourceTransformRequestHandle.Adopt(request);
            ((ResourceTransform)NativeCallbackRoot.Value(user_data)).Callback?.Invoke(
                (ResourceKind)kind,
                RuntimeStructs.CopyUtf8((sbyte*)url.data, url.size),
                ownedRequest
            );
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_resource_transform_callback", error);
            ownedRequest?.Dispose();
        }
    }

    internal static mln_resource_transform NativeResourceTransform(
        ResourceTransform value,
        NativeCallScope scope
    )
    {
        var native = new mln_resource_transform();
        native.size = (uint)sizeof(mln_resource_transform);
        if (value.Callback is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.callback = value.Callback is null ? null : &InvokeResourceTransformCallback;
        }
        return native;
    }

    internal static RuntimeEvent CopyRuntimeEvent(
        mln_runtime_event value,
        byte* record = null,
        nuint recordSize = 0
    ) =>
        new(
            (RuntimeEventType)value.type,
            (RuntimeEventSourceType)value.source_type,
            value.source,
            value.generation,
            value.code,
            RuntimeStructs.CopyUtf8((sbyte*)value.message.data, value.message.size),
            value.payload_type switch
            {
                mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME =>
                    new RuntimeEvent.PayloadValue.RenderFrame(
                        CopyRuntimeEventRenderFrame(value.payload.render_frame)
                    ),
                mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP =>
                    new RuntimeEvent.PayloadValue.RenderMap(
                        CopyRuntimeEventRenderMap(value.payload.render_map)
                    ),
                mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION =>
                    new RuntimeEvent.PayloadValue.TileAction(
                        CopyRuntimeEventTileAction(value.payload.tile_action)
                    ),
                mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS =>
                    new RuntimeEvent.PayloadValue.OfflineRegionStatus(
                        CopyRuntimeEventOfflineRegionStatus(value.payload.offline_region_status)
                    ),
                mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR =>
                    new RuntimeEvent.PayloadValue.OfflineRegionResponseError(
                        CopyRuntimeEventOfflineRegionResponseError(
                            value.payload.offline_region_response_error
                        )
                    ),
                mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT =>
                    new RuntimeEvent.PayloadValue.OfflineRegionTileCountLimit(
                        CopyRuntimeEventOfflineRegionTileCountLimit(
                            value.payload.offline_region_tile_count_limit
                        )
                    ),
                mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS =>
                    new RuntimeEvent.PayloadValue.Glyphs(
                        CopyRuntimeEventGlyphs(value.payload.glyphs)
                    ),
                mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_SPRITE =>
                    new RuntimeEvent.PayloadValue.Sprite(
                        CopyRuntimeEventSprite(value.payload.sprite)
                    ),
                mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_NONE =>
                    new RuntimeEvent.PayloadValue.None(),
                _ => new RuntimeEvent.PayloadValue.Unknown(
                    (uint)value.payload_type,
                    record == null
                        ? NativeCallScope.CopyValueBytes(value.payload)
                        : NativeCallScope.CopyArray<byte>(
                            record
                                + (nuint)
                                    Marshal.OffsetOf<mln_runtime_event>(
                                        nameof(mln_runtime_event.payload)
                                    ),
                            recordSize
                                - (nuint)
                                    Marshal.OffsetOf<mln_runtime_event>(
                                        nameof(mln_runtime_event.payload)
                                    )
                        )
                ),
            }
        );

    internal static RuntimeEventGlyphs CopyRuntimeEventGlyphs(mln_runtime_event_glyphs value) =>
        default(RuntimeEventGlyphs) with
        {
            FontStackStorage = new(
                NativeCallScope.CopyArray<mln_buffer_view, string>(
                    value.font_stack,
                    (nuint)(value.font_count),
                    static item => RuntimeStructs.CopyUtf8((sbyte*)item.data, item.size)
                )
            ),
            First = value.first,
            Last = value.last,
            FontFace = value.font_face != 0,
        };

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeRuntimeEventHandlerCallback(
        void* user_data,
        mln_event_batch_view* events
    )
    {
        try
        {
            ((RuntimeEventHandler)NativeCallbackRoot.Value(user_data)).Callback?.Invoke(
                CopyEventBatchViewEvents(NativeCallScope.Read(events))
            );
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_runtime_event_callback", error);
        }
    }

    internal static mln_runtime_event_handler NativeRuntimeEventHandler(
        RuntimeEventHandler value,
        NativeCallScope scope
    )
    {
        var native = new mln_runtime_event_handler();
        native.size = (uint)sizeof(mln_runtime_event_handler);
        if (value.Callback is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.callback = value.Callback is null ? null : &InvokeRuntimeEventHandlerCallback;
        }
        return native;
    }

    internal static RuntimeEventOfflineRegionResponseError CopyRuntimeEventOfflineRegionResponseError(
        mln_runtime_event_offline_region_response_error value
    ) => new(value.region_id, (ResourceErrorReason)value.reason);

    internal static RuntimeEventOfflineRegionStatus CopyRuntimeEventOfflineRegionStatus(
        mln_runtime_event_offline_region_status value
    ) => new(value.region_id, CopyOfflineRegionStatus(value.status));

    internal static RuntimeEventOfflineRegionTileCountLimit CopyRuntimeEventOfflineRegionTileCountLimit(
        mln_runtime_event_offline_region_tile_count_limit value
    ) => new(value.region_id, value.limit);

    internal static RuntimeEventRenderFrame CopyRuntimeEventRenderFrame(
        mln_runtime_event_render_frame value
    ) =>
        new(
            (RenderMode)value.mode,
            value.needs_repaint != 0,
            value.placement_changed != 0,
            CopyRenderingStats(value.stats)
        );

    internal static RuntimeEventRenderMap CopyRuntimeEventRenderMap(
        mln_runtime_event_render_map value
    ) => new((RenderMode)value.mode);

    internal static RuntimeEventSprite CopyRuntimeEventSprite(mln_runtime_event_sprite value) =>
        new(
            RuntimeStructs.CopyUtf8((sbyte*)value.id.data, value.id.size),
            RuntimeStructs.CopyUtf8((sbyte*)value.url.data, value.url.size)
        );

    internal static RuntimeEventTileAction CopyRuntimeEventTileAction(
        mln_runtime_event_tile_action value
    ) => new((TileOperation)value.operation, CopyTileId(value.tile_id));

    internal static mln_runtime_options NativeRuntimeOptions(
        RuntimeOptions value,
        NativeCallScope scope
    )
    {
        Required(value.AssetPath, "RuntimeOptions.AssetPath must not be null.");
        Required(value.CachePath, "RuntimeOptions.CachePath must not be null.");
        var native = new mln_runtime_options();
        native.size = (uint)sizeof(mln_runtime_options);
        native.asset_path = scope.Utf8(value.AssetPath);
        native.cache_path = scope.Utf8(value.CachePath);
        return native;
    }

    internal static mln_screen_box NativeScreenBox(ScreenBox value)
    {
        var native = new mln_screen_box();
        native.min = NativeScreenPoint(value.Min);
        native.max = NativeScreenPoint(value.Max);
        return native;
    }

    internal static mln_screen_line_string NativeScreenLineString(
        ScreenLineString value,
        NativeCallScope scope
    )
    {
        var native = new mln_screen_line_string();
        native.points = scope.Array<mln_screen_point, ScreenPoint>(
            value.PointsStorage.Items,
            static (item, scope) => NativeScreenPoint(item)
        );
        native.point_count = checked((nuint)value.PointsStorage.Items.Length);
        return native;
    }

    internal static ScreenPoint CopyScreenPoint(mln_screen_point value) => new(value.x, value.y);

    internal static mln_screen_point NativeScreenPoint(ScreenPoint value)
    {
        var native = new mln_screen_point();
        native.x = value.X;
        native.y = value.Y;
        return native;
    }

    internal static SourceFeature CopySourceFeature(mln_source_feature value) =>
        new(
            RuntimeStructs.CopyUtf8((sbyte*)value.feature.data, value.feature.size),
            RuntimeStructs.CopyUtf8((sbyte*)value.source_layer_id.data, value.source_layer_id.size)
        );

    internal static mln_source_feature_query_options NativeSourceFeatureQueryOptions(
        SourceFeatureQueryOptions value,
        NativeCallScope scope
    )
    {
        Required(value.Filter, "SourceFeatureQueryOptions.Filter must not be null.");
        var native = new mln_source_feature_query_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_source_feature_query_options);
        if (value.SourceLayerIdsStorage?.Items is { } fieldSourceLayerIds)
        {
            native.fields |= MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
            native.source_layer_ids = scope.Array<mln_buffer_view, string>(
                fieldSourceLayerIds,
                static (item, scope) => scope.Utf8(item)
            );
            native.source_layer_id_count = checked((nuint)fieldSourceLayerIds.Length);
        }
        native.filter = scope.Utf8(value.Filter);
        return native;
    }

    internal static StyleExpressionInfo CopyStyleExpressionInfo(mln_style_expression_info value) =>
        new(value.is_feature_constant != 0, value.is_zoom_constant != 0);

    internal static mln_style_expression_inputs NativeStyleExpressionInputs(
        StyleExpressionInputs value,
        NativeCallScope scope
    )
    {
        Required(value.Feature, "StyleExpressionInputs.Feature must not be null.");
        Required(value.FeatureState, "StyleExpressionInputs.FeatureState must not be null.");
        Required(value.GlobalState, "StyleExpressionInputs.GlobalState must not be null.");
        var native = new mln_style_expression_inputs();
        native.fields = 0;
        native.size = (uint)sizeof(mln_style_expression_inputs);
        native.fields |= Put(value.Zoom, ref native.zoom, MLN_STYLE_EXPRESSION_INPUT_ZOOM);
        native.fields |= Put(
            value.ColorRampParameter,
            ref native.color_ramp_parameter,
            MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER
        );
        native.fields |= Put(
            value.Elevation,
            ref native.elevation,
            MLN_STYLE_EXPRESSION_INPUT_ELEVATION
        );
        native.feature = scope.Utf8(value.Feature);
        native.feature_state = scope.Utf8(value.FeatureState);
        native.global_state = scope.Utf8(value.GlobalState);
        native.fields |= Put(
            value.CanonicalTileId,
            ref native.canonical_tile_id,
            MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID,
            NativeCanonicalTileId
        );
        native.available_images = scope.Array<mln_buffer_view, string>(
            value.AvailableImagesStorage.Items,
            static (item, scope) => scope.Utf8(item)
        );
        native.available_image_count = checked((nuint)value.AvailableImagesStorage.Items.Length);
        return native;
    }

    internal static mln_style_expression_options NativeStyleExpressionOptions(
        StyleExpressionOptions value
    )
    {
        var native = new mln_style_expression_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_style_expression_options);
        native.fields |= Put(
            value.ExpectedType,
            ref native.expected_type,
            MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE,
            NativeStyleExpressionType
        );
        return native;
    }

    internal static mln_style_expression_type NativeStyleExpressionType(StyleExpressionType value)
    {
        var native = new mln_style_expression_type();
        native.kind = (mln_style_expression_type_kind)value.Kind;
        native.item_kind = (mln_style_expression_type_kind)value.ItemKind;
        native.item_count = value.ItemCount;
        return native;
    }

    internal static StyleImageInfo CopyStyleImageInfo(mln_style_image_info value) =>
        new()
        {
            Width = value.width,
            Height = value.height,
            PixelsStorage = new(ValueStructs.CopyBufferView(value.pixels)),
            StretchXStorage = new(
                NativeCallScope.CopyArray<mln_image_stretch, ImageStretch>(
                    value.stretch_x,
                    (nuint)(value.stretch_x_count),
                    static item => CopyImageStretch(item)
                )
            ),
            StretchYStorage = new(
                NativeCallScope.CopyArray<mln_image_stretch, ImageStretch>(
                    value.stretch_y,
                    (nuint)(value.stretch_y_count),
                    static item => CopyImageStretch(item)
                )
            ),
            Content = value.fields.HasFlag(MLN_STYLE_IMAGE_INFO_CONTENT)
                ? CopyImageContent(value.content)
                : null,
            TextFitWidth = value.fields.HasFlag(MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH)
                ? (StyleImageTextFit)value.text_fit_width
                : null,
            TextFitHeight = value.fields.HasFlag(MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT)
                ? (StyleImageTextFit)value.text_fit_height
                : null,
            PixelRatio = value.pixel_ratio,
            Sdf = value.sdf != 0,
            KeepUnused = value.keep_unused != 0,
        };

    internal static mln_style_image_options NativeStyleImageOptions(
        StyleImageOptions value,
        NativeCallScope scope
    )
    {
        var native = new mln_style_image_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_style_image_options);
        if (value.StretchXStorage?.Items is { } fieldStretchX)
        {
            native.fields |= MLN_STYLE_IMAGE_OPTION_STRETCH_X;
            native.stretch_x = scope.Array<mln_image_stretch, ImageStretch>(
                fieldStretchX,
                static (item, scope) => NativeImageStretch(item)
            );
            native.stretch_x_count = checked((nuint)fieldStretchX.Length);
        }
        if (value.StretchYStorage?.Items is { } fieldStretchY)
        {
            native.fields |= MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
            native.stretch_y = scope.Array<mln_image_stretch, ImageStretch>(
                fieldStretchY,
                static (item, scope) => NativeImageStretch(item)
            );
            native.stretch_y_count = checked((nuint)fieldStretchY.Length);
        }
        native.fields |= Put(
            value.Content,
            ref native.content,
            MLN_STYLE_IMAGE_OPTION_CONTENT,
            NativeImageContent
        );
        native.fields |= Put(
            value.TextFitWidth,
            ref native.text_fit_width,
            MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH,
            static present => (mln_style_image_text_fit)present
        );
        native.fields |= Put(
            value.TextFitHeight,
            ref native.text_fit_height,
            MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT,
            static present => (mln_style_image_text_fit)present
        );
        native.fields |= Put(
            value.PixelRatio,
            ref native.pixel_ratio,
            MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
        );
        native.fields |= Put(
            value.Sdf,
            ref native.sdf,
            MLN_STYLE_IMAGE_OPTION_SDF,
            static present => (byte)(present ? 1 : 0)
        );
        native.fields |= Put(
            value.KeepUnused,
            ref native.keep_unused,
            MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED,
            static present => (byte)(present ? 1 : 0)
        );
        return native;
    }

    internal static StyleLayerInfo CopyStyleLayerInfo(mln_style_layer_info value) =>
        new(
            RuntimeStructs.CopyUtf8((sbyte*)value.id.data, value.id.size),
            RuntimeStructs.CopyUtf8((sbyte*)value.type.data, value.type.size),
            RuntimeStructs.CopyUtf8((sbyte*)value.source_id.data, value.source_id.size),
            RuntimeStructs.CopyUtf8((sbyte*)value.source_layer.data, value.source_layer.size),
            value.min_zoom,
            value.max_zoom,
            (StyleLayerVisibility)value.visibility
        );

    internal static StyleSourceInfo CopyStyleSourceInfo(mln_style_source_info value) =>
        new()
        {
            Id = RuntimeStructs.CopyUtf8((sbyte*)value.id.data, value.id.size),
            Type = (StyleSourceType)value.type,
            IsVolatile = value.is_volatile != 0,
            Attribution = RuntimeStructs.CopyUtf8(
                (sbyte*)value.attribution.data,
                value.attribution.size
            ),
            Url = RuntimeStructs.CopyUtf8((sbyte*)value.url.data, value.url.size),
            TileJson = value.fields.HasFlag(MLN_STYLE_SOURCE_INFO_TILE_JSON)
                ? CopyStyleSourceTileInfo(value.tile_json)
                : null,
            Bounds = value.fields.HasFlag(MLN_STYLE_SOURCE_INFO_BOUNDS)
                ? CopyLatLngBounds(value.bounds)
                : null,
            TileSize = value.fields.HasFlag(MLN_STYLE_SOURCE_INFO_TILE_SIZE)
                ? value.tile_size
                : null,
            VectorEncoding = value.fields.HasFlag(MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING)
                ? (StyleVectorTileEncoding)value.vector_encoding
                : null,
            RasterEncoding = value.fields.HasFlag(MLN_STYLE_SOURCE_INFO_RASTER_ENCODING)
                ? (StyleRasterDemEncoding)value.raster_encoding
                : null,
        };

    internal static StyleSourceTileInfo CopyStyleSourceTileInfo(mln_style_source_tile_info value) =>
        default(StyleSourceTileInfo) with
        {
            TileUrlsStorage = new(
                NativeCallScope.CopyArray<mln_buffer_view, string>(
                    value.tile_urls,
                    (nuint)(value.tile_url_count),
                    static item => RuntimeStructs.CopyUtf8((sbyte*)item.data, item.size)
                )
            ),
            MinZoom = value.min_zoom,
            MaxZoom = value.max_zoom,
            Scheme = (StyleTileScheme)value.scheme,
        };

    internal static StyleTransitionOptions CopyStyleTransitionOptions(
        mln_style_transition_options value
    ) =>
        new()
        {
            DurationMs = value.fields.HasFlag(MLN_STYLE_TRANSITION_OPTION_DURATION)
                ? value.duration_ms
                : null,
            DelayMs = value.fields.HasFlag(MLN_STYLE_TRANSITION_OPTION_DELAY)
                ? value.delay_ms
                : null,
            EnablePlacementTransitions = value.fields.HasFlag(
                MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
            )
                ? value.enable_placement_transitions != 0
                : null,
        };

    internal static mln_style_transition_options NativeStyleTransitionOptions(
        StyleTransitionOptions value
    )
    {
        var native = new mln_style_transition_options();
        native.fields = 0;
        native.size = (uint)sizeof(mln_style_transition_options);
        native.fields |= Put(
            value.DurationMs,
            ref native.duration_ms,
            MLN_STYLE_TRANSITION_OPTION_DURATION
        );
        native.fields |= Put(value.DelayMs, ref native.delay_ms, MLN_STYLE_TRANSITION_OPTION_DELAY);
        native.fields |= Put(
            value.EnablePlacementTransitions,
            ref native.enable_placement_transitions,
            MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS,
            static present => (byte)(present ? 1 : 0)
        );
        return native;
    }

    internal static TextureImageInfo CopyTextureImageInfo(mln_texture_image_info value) =>
        new(value.width, value.height, value.stride, (ulong)value.byte_length);

    internal static TextureReadbackResult CopyTextureReadbackResult(
        mln_texture_readback_result value
    ) =>
        default(TextureReadbackResult) with
        {
            DataStorage = new(ValueStructs.CopyBufferView(value.data)),
            Info = CopyTextureImageInfo(value.info),
        };

    internal static TileId CopyTileId(mln_tile_id value) =>
        new(
            value.overscaled_z,
            value.wrap,
            value.canonical_z,
            value.canonical_x,
            value.canonical_y
        );

    internal static mln_unit_bezier NativeUnitBezier(UnitBezier value)
    {
        var native = new mln_unit_bezier();
        native.x1 = value.X1;
        native.y1 = value.Y1;
        native.x2 = value.X2;
        native.y2 = value.Y2;
        return native;
    }

    internal static Vec3 CopyVec3(mln_vec3 value) => new(value.x, value.y, value.z);

    internal static mln_vec3 NativeVec3(Vec3 value)
    {
        var native = new mln_vec3();
        native.x = value.X;
        native.y = value.Y;
        native.z = value.Z;
        return native;
    }

    internal static mln_vulkan_borrowed_texture NativeVulkanBorrowedTexture(
        VulkanBorrowedTexture value
    )
    {
        var native = new mln_vulkan_borrowed_texture();
        native.image = value.Image;
        native.image_view = value.ImageView;
        return native;
    }

    internal static mln_vulkan_borrowed_texture_descriptor NativeVulkanBorrowedTextureDescriptor(
        VulkanBorrowedTextureDescriptor value,
        NativeCallScope scope
    )
    {
        var native = new mln_vulkan_borrowed_texture_descriptor();
        native.size = (uint)sizeof(mln_vulkan_borrowed_texture_descriptor);
        native.extent = NativeLogicalExtent(value.Extent);
        native.physical_width = value.PhysicalWidth;
        native.physical_height = value.PhysicalHeight;
        native.context = NativeVulkanContextDescriptor(value.Context);
        native.textures = scope.Array<mln_vulkan_borrowed_texture, VulkanBorrowedTexture>(
            value.TexturesStorage.Items,
            static (item, scope) => NativeVulkanBorrowedTexture(item)
        );
        native.texture_count = checked((nuint)value.TexturesStorage.Items.Length);
        native.format = value.Format;
        native.initial_layout = value.InitialLayout;
        native.final_layout = value.FinalLayout;
        return native;
    }

    internal static mln_vulkan_context_descriptor NativeVulkanContextDescriptor(
        VulkanContextDescriptor value
    )
    {
        var native = new mln_vulkan_context_descriptor();
        native.instance = (void*)value.Instance.Address;
        native.physical_device = (void*)value.PhysicalDevice.Address;
        native.device = (void*)value.Device.Address;
        native.graphics_queue = (void*)value.GraphicsQueue.Address;
        native.graphics_queue_family_index = value.GraphicsQueueFamilyIndex;
        native.get_instance_proc_addr = (void*)value.GetInstanceProcAddr.Address;
        native.get_device_proc_addr = (void*)value.GetDeviceProcAddr.Address;
        return native;
    }

    internal static mln_vulkan_owned_texture_descriptor NativeVulkanOwnedTextureDescriptor(
        VulkanOwnedTextureDescriptor value
    )
    {
        var native = new mln_vulkan_owned_texture_descriptor();
        native.size = (uint)sizeof(mln_vulkan_owned_texture_descriptor);
        native.extent = NativeLogicalExtent(value.Extent);
        native.context = NativeVulkanContextDescriptor(value.Context);
        return native;
    }

    internal static mln_vulkan_surface_descriptor NativeVulkanSurfaceDescriptor(
        VulkanSurfaceDescriptor value
    )
    {
        var native = new mln_vulkan_surface_descriptor();
        native.size = (uint)sizeof(mln_vulkan_surface_descriptor);
        native.extent = NativeLogicalExtent(value.Extent);
        native.context = NativeVulkanContextDescriptor(value.Context);
        native.surface = value.Surface;
        return native;
    }

    internal static VulkanTextureFrame CopyVulkanTextureFrame(mln_vulkan_texture_frame value) =>
        new(
            value.generation,
            value.width,
            value.height,
            value.scale_factor,
            value.frame_id,
            value.slot,
            value.image,
            value.image_view,
            NativePointer.FromNativeAddress((nint)value.device),
            value.format,
            value.layout
        );

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeWakeCallback(void* user_data)
    {
        try
        {
            ((Wake)NativeCallbackRoot.Value(user_data)).Callback?.Invoke();
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_wake_callback", error);
        }
    }

    internal static mln_wake NativeWake(Wake value, NativeCallScope scope)
    {
        var native = new mln_wake();
        if (value.NativeRegistration is { } registration)
        {
            scope.Consume(registration);
            if (value.Callback is not null)
                throw new ArgumentException(
                    "A native registration carries no delegates.",
                    nameof(value)
                );
            native.callback = (delegate* unmanaged[Cdecl]<void*, void>)registration.Function(0);
            native.user_data = registration.UserData;
            native.release_user_data = (delegate* unmanaged[Cdecl]<void*, void>)
                registration.ReleaseUserData;
        }
        else if (value.Callback is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.callback = value.Callback is null ? null : &InvokeWakeCallback;
        }
        return native;
    }

    internal static mln_web_gl_context_descriptor NativeWebGlContextDescriptor(
        WebGlContextDescriptor value,
        NativeCallScope scope
    )
    {
        Required(value.CanvasSelector, "WebGlContextDescriptor.CanvasSelector must not be null.");
        var native = new mln_web_gl_context_descriptor();
        native.kind = (mln_web_gl_context_kind)value.Kind;
        native.context = value.Context;
        native.canvas_selector = scope.Utf8(value.CanvasSelector);
        return native;
    }

    internal static mln_wgl_context_descriptor NativeWglContextDescriptor(
        WglContextDescriptor value
    )
    {
        var native = new mln_wgl_context_descriptor();
        native.device_context = (void*)value.DeviceContext.Address;
        native.share_context = (void*)value.ShareContext.Address;
        native.get_proc_address = (void*)value.GetProcAddress.Address;
        return native;
    }
}
