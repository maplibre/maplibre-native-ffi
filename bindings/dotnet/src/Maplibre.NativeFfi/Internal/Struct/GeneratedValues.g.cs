// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Internal.Struct;

internal static unsafe class GeneratedValues
{
    internal static AnimationOptions CopyAnimationOptions(mln_animation_options value) =>
        new()
        {
            DurationMs = value.fields.HasFlag(MLN_ANIMATION_OPTION_DURATION)
                ? value.duration_ms
                : null,
            Velocity = value.fields.HasFlag(MLN_ANIMATION_OPTION_VELOCITY) ? value.velocity : null,
            MinZoom = value.fields.HasFlag(MLN_ANIMATION_OPTION_MIN_ZOOM) ? value.min_zoom : null,
            Easing = value.fields.HasFlag(MLN_ANIMATION_OPTION_EASING)
                ? CopyUnitBezier(value.easing)
                : null,
            TransitionId = value.fields.HasFlag(MLN_ANIMATION_OPTION_TRANSITION_ID)
                ? value.transition_id
                : null,
        };

    internal static mln_animation_options NativeAnimationOptions(AnimationOptions value)
    {
        var native = NativeMethods.mln_animation_options_default();
        native.fields = 0;
        native.size = (uint)sizeof(mln_animation_options);
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
        var native = NativeMethods.mln_bound_options_default();
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

    internal static CameraDelta CopyCameraDelta(mln_camera_delta value) =>
        new()
        {
            Kind = (CameraDeltaKind)value.kind,
            Offset = CopyScreenPoint(value.offset),
            Amount = value.amount,
            Anchor = value.has_anchor != 0 ? CopyScreenPoint(value.anchor) : null,
            Animation = CopyAnimationOptions(value.animation),
        };

    internal static mln_camera_delta NativeCameraDelta(CameraDelta value)
    {
        Required(value.Animation, "CameraDelta.Animation must not be null.");
        var native = NativeMethods.mln_camera_delta_default();
        native.has_anchor = 0;
        native.size = (uint)sizeof(mln_camera_delta);
        native.kind = (uint)value.Kind;
        native.offset = NativeScreenPoint(value.Offset);
        native.amount = value.Amount;
        if (value.Anchor is { } fieldAnchor)
        {
            native.has_anchor = 1;
            native.anchor = NativeScreenPoint(fieldAnchor);
        }
        native.animation = NativeAnimationOptions(value.Animation);
        return native;
    }

    internal static CameraFitOptions CopyCameraFitOptions(mln_camera_fit_options value) =>
        new()
        {
            Padding = value.fields.HasFlag(MLN_CAMERA_FIT_OPTION_PADDING)
                ? CopyEdgeInsets(value.padding)
                : null,
            Bearing = value.fields.HasFlag(MLN_CAMERA_FIT_OPTION_BEARING) ? value.bearing : null,
            Pitch = value.fields.HasFlag(MLN_CAMERA_FIT_OPTION_PITCH) ? value.pitch : null,
        };

    internal static mln_camera_fit_options NativeCameraFitOptions(CameraFitOptions value)
    {
        var native = NativeMethods.mln_camera_fit_options_default();
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
                ? new LatLng(value.latitude, value.longitude)
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
        var native = NativeMethods.mln_camera_options_default();
        native.fields = 0;
        native.size = (uint)sizeof(mln_camera_options);
        if (value.Center is { } fieldCenter)
        {
            native.fields |= MLN_CAMERA_OPTION_CENTER;
            native.latitude = fieldCenter.Latitude;
            native.longitude = fieldCenter.Longitude;
        }
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
        new(value.generation, CopyCameraOptions(value.camera));

    internal static mln_camera_query_result NativeCameraQueryResult(CameraQueryResult value)
    {
        Required(value.Camera, "CameraQueryResult.Camera must not be null.");
        var native = new mln_camera_query_result();
        native.size = (uint)sizeof(mln_camera_query_result);
        native.generation = value.Generation;
        native.camera = NativeCameraOptions(value.Camera);
        return native;
    }

    internal static CameraUpdate CopyCameraUpdate(mln_camera_update value) =>
        new(
            (CameraUpdateMode)value.mode,
            CopyCameraOptions(value.camera),
            CopyAnimationOptions(value.animation),
            (GesturePhase)value.gesture_phase
        );

    internal static mln_camera_update NativeCameraUpdate(CameraUpdate value)
    {
        Required(value.Camera, "CameraUpdate.Camera must not be null.");
        Required(value.Animation, "CameraUpdate.Animation must not be null.");
        var native = NativeMethods.mln_camera_update_default();
        native.size = (uint)sizeof(mln_camera_update);
        native.mode = (uint)value.Mode;
        native.camera = NativeCameraOptions(value.Camera);
        native.animation = NativeAnimationOptions(value.Animation);
        native.gesture_phase = (uint)value.GesturePhase;
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

    internal static CustomGeometrySourceOptions CopyCustomGeometrySourceOptions(
        mln_custom_geometry_source_options value
    ) =>
        new()
        {
            FetchTile = default,
            CancelTile = default,
            MinZoom = value.fields.HasFlag(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM)
                ? value.min_zoom
                : null,
            MaxZoom = value.fields.HasFlag(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM)
                ? value.max_zoom
                : null,
            Tolerance = value.fields.HasFlag(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE)
                ? value.tolerance
                : null,
            TileSize = value.fields.HasFlag(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE)
                ? value.tile_size
                : null,
            Buffer = value.fields.HasFlag(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER)
                ? value.buffer
                : null,
            Clip = value.fields.HasFlag(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP)
                ? value.clip != 0
                : null,
            Wrap = value.fields.HasFlag(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP)
                ? value.wrap != 0
                : null,
        };

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
            NativeCallbackFailure.Report("mln_custom_geometry_source_tile_callback", error);
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
            NativeCallbackFailure.Report("mln_custom_geometry_source_tile_callback", error);
        }
    }

    internal static mln_custom_geometry_source_options NativeCustomGeometrySourceOptions(
        CustomGeometrySourceOptions value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_custom_geometry_source_options_default();
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

    internal static CustomMvtVectorSourceOptions CopyCustomMvtVectorSourceOptions(
        mln_custom_mvt_vector_source_options value
    ) =>
        new()
        {
            FetchTile = default,
            CancelTile = default,
            MinZoom = value.fields.HasFlag(MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM)
                ? value.min_zoom
                : null,
            MaxZoom = value.fields.HasFlag(MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM)
                ? value.max_zoom
                : null,
        };

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeCustomMvtVectorSourceOptionsFetchTile(
        void* user_data,
        mln_canonical_tile_id tile_id
    )
    {
        try
        {
            ((CustomMvtVectorSourceOptions)NativeCallbackRoot.Value(user_data)).FetchTile?.Invoke(
                CopyCanonicalTileId(tile_id)
            );
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_custom_mvt_vector_source_tile_callback", error);
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeCustomMvtVectorSourceOptionsCancelTile(
        void* user_data,
        mln_canonical_tile_id tile_id
    )
    {
        try
        {
            ((CustomMvtVectorSourceOptions)NativeCallbackRoot.Value(user_data)).CancelTile?.Invoke(
                CopyCanonicalTileId(tile_id)
            );
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_custom_mvt_vector_source_tile_callback", error);
        }
    }

    internal static mln_custom_mvt_vector_source_options NativeCustomMvtVectorSourceOptions(
        CustomMvtVectorSourceOptions value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_custom_mvt_vector_source_options_default();
        native.fields = 0;
        native.size = (uint)sizeof(mln_custom_mvt_vector_source_options);
        if (value.FetchTile is not null || value.CancelTile is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.fetch_tile = value.FetchTile is null
                ? null
                : &InvokeCustomMvtVectorSourceOptionsFetchTile;
            native.cancel_tile = value.CancelTile is null
                ? null
                : &InvokeCustomMvtVectorSourceOptionsCancelTile;
        }
        native.fields |= Put(
            value.MinZoom,
            ref native.min_zoom,
            MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM
        );
        native.fields |= Put(
            value.MaxZoom,
            ref native.max_zoom,
            MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM
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

    internal static EglContextDescriptor CopyEglContextDescriptor(
        mln_egl_context_descriptor value
    ) =>
        new(
            NativePointer.FromNativeAddress((nint)value.display),
            NativePointer.FromNativeAddress((nint)value.config),
            NativePointer.FromNativeAddress((nint)value.share_context),
            (OpenglClientApi)value.client_api,
            NativePointer.FromNativeAddress((nint)value.get_proc_address)
        );

    internal static mln_egl_context_descriptor NativeEglContextDescriptor(
        EglContextDescriptor value
    )
    {
        var native = new mln_egl_context_descriptor();
        native.size = (uint)sizeof(mln_egl_context_descriptor);
        native.display = (void*)value.Display.Address;
        native.config = (void*)value.Config.Address;
        native.share_context = (void*)value.ShareContext.Address;
        native.client_api = (uint)value.ClientApi;
        native.get_proc_address = (void*)value.GetProcAddress.Address;
        return native;
    }

    internal static mln_feature_state_selector NativeFeatureStateSelector(
        FeatureStateSelector value,
        NativeCallScope scope
    )
    {
        Required(value.SourceId, "FeatureStateSelector.SourceId must not be null.");
        var native = new mln_feature_state_selector();
        native.fields = 0;
        native.size = (uint)sizeof(mln_feature_state_selector);
        native.source_id = scope.Utf8(value.SourceId);
        native.fields |= Put(
            value.SourceLayerId,
            ref native.source_layer_id,
            MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID,
            present => scope.Utf8(present)
        );
        native.fields |= Put(
            value.FeatureId,
            ref native.feature_id,
            MLN_FEATURE_STATE_SELECTOR_FEATURE_ID,
            present => scope.Utf8(present)
        );
        native.fields |= Put(
            value.StateKey,
            ref native.state_key,
            MLN_FEATURE_STATE_SELECTOR_STATE_KEY,
            present => scope.Utf8(present)
        );
        return native;
    }

    internal static FrameDemand CopyFrameDemand(mln_frame_demand value) =>
        new((FrameDemandFlag)value.flags, value.token, value.coalescing_boundary, value.timeout_ns);

    internal static mln_frame_demand NativeFrameDemand(FrameDemand value)
    {
        var native = NativeMethods.mln_frame_demand_default();
        native.size = (uint)sizeof(mln_frame_demand);
        native.flags = (uint)value.Flags;
        native.token = value.Token;
        native.coalescing_boundary = value.CoalescingBoundary;
        native.timeout_ns = value.TimeoutNs;
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
        var native = NativeMethods.mln_free_camera_options_default();
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

    internal static GeojsonSourceOptions CopyGeojsonSourceOptions(
        mln_geojson_source_options value
    ) =>
        new()
        {
            MinZoom = value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM)
                ? value.min_zoom
                : null,
            MaxZoom = value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM)
                ? value.max_zoom
                : null,
            Tolerance = value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_TOLERANCE)
                ? value.tolerance
                : null,
            ClusterMaxZoom = value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM)
                ? value.cluster_max_zoom
                : null,
            ClusterPropertiesStorage = ValueArray.Optional(
                value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES)
                    ? ValueStructs.CopyBufferView(value.cluster_properties)
                    : null
            ),
            TileSize = value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE)
                ? value.tile_size
                : null,
            Buffer = value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_BUFFER) ? value.buffer : null,
            ClusterRadius = value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS)
                ? value.cluster_radius
                : null,
            ClusterMinPoints = value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS)
                ? value.cluster_min_points
                : null,
            LineMetrics = value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS)
                ? value.line_metrics != 0
                : null,
            Cluster = value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_CLUSTER)
                ? value.cluster != 0
                : null,
            SynchronousTiling = value.fields.HasFlag(MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING)
                ? value.synchronous_tiling != 0
                : null,
        };

    internal static mln_geojson_source_options NativeGeojsonSourceOptions(
        GeojsonSourceOptions value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_geojson_source_options_default();
        native.fields = 0;
        native.size = (uint)sizeof(mln_geojson_source_options);
        native.fields |= Put(
            value.MinZoom,
            ref native.min_zoom,
            MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM
        );
        native.fields |= Put(
            value.MaxZoom,
            ref native.max_zoom,
            MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM
        );
        native.fields |= Put(
            value.Tolerance,
            ref native.tolerance,
            MLN_GEOJSON_SOURCE_OPTION_TOLERANCE
        );
        native.fields |= Put(
            value.ClusterMaxZoom,
            ref native.cluster_max_zoom,
            MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
        );
        native.fields |= Put(
            value.ClusterPropertiesStorage?.Items,
            ref native.cluster_properties,
            MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES,
            present => scope.Buffer(present)
        );
        native.fields |= Put(
            value.TileSize,
            ref native.tile_size,
            MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE
        );
        native.fields |= Put(value.Buffer, ref native.buffer, MLN_GEOJSON_SOURCE_OPTION_BUFFER);
        native.fields |= Put(
            value.ClusterRadius,
            ref native.cluster_radius,
            MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
        );
        native.fields |= Put(
            value.ClusterMinPoints,
            ref native.cluster_min_points,
            MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
        );
        native.fields |= Put(
            value.LineMetrics,
            ref native.line_metrics,
            MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS,
            static present => (byte)(present ? 1 : 0)
        );
        native.fields |= Put(
            value.Cluster,
            ref native.cluster,
            MLN_GEOJSON_SOURCE_OPTION_CLUSTER,
            static present => (byte)(present ? 1 : 0)
        );
        native.fields |= Put(
            value.SynchronousTiling,
            ref native.synchronous_tiling,
            MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING,
            static present => (byte)(present ? 1 : 0)
        );
        return native;
    }

    internal static GpuSync CopyGpuSync(mln_gpu_sync value) =>
        new((GpuSyncKind)value.kind, value.@object, value.value);

    internal static mln_gpu_sync NativeGpuSync(GpuSync value)
    {
        var native = NativeMethods.mln_gpu_sync_default();
        native.size = (uint)sizeof(mln_gpu_sync);
        native.kind = (uint)value.Kind;
        native.@object = value.Object;
        native.value = value.Value;
        return native;
    }

    private static readonly string[] AllowedHttpHeaderTransformCallback =
    [
        "mln_http_header_transform_response_set",
    ];

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static mln_status InvokeHttpHeaderTransformCallback(
        void* user_data,
        uint kind,
        sbyte* url,
        mln_http_header_transform_response* out_response
    )
    {
        try
        {
            var responseOutResponse = new HttpHeaderTransformResponse(out_response);
            using var restriction = NativeCallbackGuard.Restrict(
                responseOutResponse,
                AllowedHttpHeaderTransformCallback
            );
            try
            {
                ((HttpHeaderTransform)NativeCallbackRoot.Value(user_data)).Callback?.Invoke(
                    (ResourceKind)kind,
                    NativeCallScope.CopyCString(url),
                    responseOutResponse
                );
            }
            finally
            {
                responseOutResponse.Expire();
            }
            return mln_status.MLN_STATUS_OK;
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

    internal static MapOptions CopyMapOptions(mln_map_options value) =>
        new(
            CopyLogicalExtent(value.initial_extent),
            (MapMode)value.map_mode,
            value.fast_pfor_enabled != 0,
            (RuntimeEventMask)value.event_mask
        );

    internal static mln_map_options NativeMapOptions(MapOptions value)
    {
        var native = NativeMethods.mln_map_options_default();
        native.size = (uint)sizeof(mln_map_options);
        native.initial_extent = NativeLogicalExtent(value.InitialExtent);
        native.map_mode = (uint)value.MapMode;
        native.fast_pfor_enabled = (byte)(value.FastPforEnabled ? 1 : 0);
        native.event_mask = (ulong)value.EventMask;
        return native;
    }

    internal static MapSnapshot CopyMapSnapshot(mln_map_snapshot value) =>
        new(
            (MapDebugOption)value.debug_options,
            value.generation,
            CopyCameraOptions(value.camera),
            CopyLogicalExtent(value.logical_extent),
            CopyProjectionMode(value.projection_mode),
            CopyMapViewportOptions(value.viewport),
            value.fully_loaded != 0,
            value.rendering_stats_view_enabled != 0,
            value.repaint_demand != 0,
            value.gesture_in_progress != 0,
            (RuntimeEventMask)value.event_mask,
            value.latest_render_update_generation,
            CopyMapTileOptions(value.tile),
            CopyBoundOptions(value.bounds),
            CopyFreeCameraOptions(value.free_camera)
        );

    internal static mln_map_snapshot NativeMapSnapshot(MapSnapshot value)
    {
        Required(value.Camera, "MapSnapshot.Camera must not be null.");
        Required(value.ProjectionMode, "MapSnapshot.ProjectionMode must not be null.");
        Required(value.Viewport, "MapSnapshot.Viewport must not be null.");
        Required(value.Tile, "MapSnapshot.Tile must not be null.");
        Required(value.Bounds, "MapSnapshot.Bounds must not be null.");
        Required(value.FreeCamera, "MapSnapshot.FreeCamera must not be null.");
        var native = new mln_map_snapshot();
        native.size = (uint)sizeof(mln_map_snapshot);
        native.debug_options = (uint)value.DebugOptions;
        native.generation = value.Generation;
        native.camera = NativeCameraOptions(value.Camera);
        native.logical_extent = NativeLogicalExtent(value.LogicalExtent);
        native.projection_mode = NativeProjectionMode(value.ProjectionMode);
        native.viewport = NativeMapViewportOptions(value.Viewport);
        native.fully_loaded = (byte)(value.FullyLoaded ? 1 : 0);
        native.rendering_stats_view_enabled = (byte)(value.RenderingStatsViewEnabled ? 1 : 0);
        native.repaint_demand = (byte)(value.RepaintDemand ? 1 : 0);
        native.gesture_in_progress = (byte)(value.GestureInProgress ? 1 : 0);
        native.event_mask = (ulong)value.EventMask;
        native.latest_render_update_generation = value.LatestRenderUpdateGeneration;
        native.tile = NativeMapTileOptions(value.Tile);
        native.bounds = NativeBoundOptions(value.Bounds);
        native.free_camera = NativeFreeCameraOptions(value.FreeCamera);
        return native;
    }

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
        var native = NativeMethods.mln_map_tile_options_default();
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
            static present => (uint)present
        );
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
        var native = NativeMethods.mln_map_viewport_options_default();
        native.fields = 0;
        native.size = (uint)sizeof(mln_map_viewport_options);
        native.fields |= Put(
            value.NorthOrientation,
            ref native.north_orientation,
            MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION,
            static present => (uint)present
        );
        native.fields |= Put(
            value.ConstrainMode,
            ref native.constrain_mode,
            MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE,
            static present => (uint)present
        );
        native.fields |= Put(
            value.ViewportMode,
            ref native.viewport_mode,
            MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE,
            static present => (uint)present
        );
        native.fields |= Put(
            value.FrustumOffset,
            ref native.frustum_offset,
            MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET,
            NativeEdgeInsets
        );
        return native;
    }

    internal static MetalBorrowedTextureDescriptor CopyMetalBorrowedTextureDescriptor(
        mln_metal_borrowed_texture_descriptor value
    ) =>
        new(
            CopyRenderTargetExtent(value.extent),
            value.physical_width,
            value.physical_height,
            NativePointer.FromNativeAddress((nint)value.texture)
        );

    internal static mln_metal_borrowed_texture_descriptor NativeMetalBorrowedTextureDescriptor(
        MetalBorrowedTextureDescriptor value
    )
    {
        var native = NativeMethods.mln_metal_borrowed_texture_descriptor_default();
        native.size = (uint)sizeof(mln_metal_borrowed_texture_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.physical_width = value.PhysicalWidth;
        native.physical_height = value.PhysicalHeight;
        native.texture = (void*)value.Texture.Address;
        return native;
    }

    internal static MetalContextDescriptor CopyMetalContextDescriptor(
        mln_metal_context_descriptor value
    ) => new(NativePointer.FromNativeAddress((nint)value.device));

    internal static mln_metal_context_descriptor NativeMetalContextDescriptor(
        MetalContextDescriptor value
    )
    {
        var native = new mln_metal_context_descriptor();
        native.size = (uint)sizeof(mln_metal_context_descriptor);
        native.device = (void*)value.Device.Address;
        return native;
    }

    internal static MetalOwnedTextureDescriptor CopyMetalOwnedTextureDescriptor(
        mln_metal_owned_texture_descriptor value
    ) => new(CopyRenderTargetExtent(value.extent), CopyMetalContextDescriptor(value.context));

    internal static mln_metal_owned_texture_descriptor NativeMetalOwnedTextureDescriptor(
        MetalOwnedTextureDescriptor value
    )
    {
        var native = NativeMethods.mln_metal_owned_texture_descriptor_default();
        native.size = (uint)sizeof(mln_metal_owned_texture_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.context = NativeMetalContextDescriptor(value.Context);
        return native;
    }

    internal static MetalOwnedTextureFrame CopyMetalOwnedTextureFrame(
        mln_metal_owned_texture_frame value
    ) =>
        new(
            value.generation,
            value.width,
            value.height,
            value.scale_factor,
            value.frame_id,
            NativePointer.FromNativeAddress((nint)value.texture),
            NativePointer.FromNativeAddress((nint)value.device),
            value.pixel_format
        );

    internal static mln_metal_owned_texture_frame NativeMetalOwnedTextureFrame(
        MetalOwnedTextureFrame value
    )
    {
        var native = new mln_metal_owned_texture_frame();
        native.size = (uint)sizeof(mln_metal_owned_texture_frame);
        native.generation = value.Generation;
        native.width = value.Width;
        native.height = value.Height;
        native.scale_factor = value.ScaleFactor;
        native.frame_id = value.FrameId;
        native.texture = (void*)value.Texture.Address;
        native.device = (void*)value.Device.Address;
        native.pixel_format = value.PixelFormat;
        return native;
    }

    internal static MetalSurfaceDescriptor CopyMetalSurfaceDescriptor(
        mln_metal_surface_descriptor value
    ) =>
        new(
            CopyRenderTargetExtent(value.extent),
            CopyMetalContextDescriptor(value.context),
            NativePointer.FromNativeAddress((nint)value.layer)
        );

    internal static mln_metal_surface_descriptor NativeMetalSurfaceDescriptor(
        MetalSurfaceDescriptor value
    )
    {
        var native = NativeMethods.mln_metal_surface_descriptor_default();
        native.size = (uint)sizeof(mln_metal_surface_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.context = NativeMetalContextDescriptor(value.Context);
        native.layer = (void*)value.Layer.Address;
        return native;
    }

    internal static OfflineGeometryRegionDefinition CopyOfflineGeometryRegionDefinition(
        mln_offline_geometry_region_definition value
    ) =>
        new()
        {
            StyleUrl = NativeCallScope.CopyCString(value.style_url),
            GeometryStorage = new(ValueStructs.CopyBufferView(value.geometry)),
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
        var native = new mln_offline_geometry_region_definition();
        native.size = (uint)sizeof(mln_offline_geometry_region_definition);
        native.style_url = scope.CString(value.StyleUrl);
        native.geometry = scope.Buffer(value.GeometryStorage.Items);
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
            (uint)mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID =>
                new OfflineRegionDefinition.TilePyramid(
                    CopyOfflineTilePyramidRegionDefinition(value.data.tile_pyramid)
                ),
            (uint)mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY =>
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
                native.type = (uint)
                    mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
                native.data.tile_pyramid = NativeOfflineTilePyramidRegionDefinition(
                    selected.Value,
                    scope
                );
                break;
            case OfflineRegionDefinition.Geometry selected:
                native.type = (uint)
                    mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
                native.data.geometry = NativeOfflineGeometryRegionDefinition(selected.Value, scope);
                break;
            default:
                throw new ArgumentException("A union variant is required.", nameof(value));
        }
        return native;
    }

    internal static OfflineRegionInfo CopyOfflineRegionInfo(mln_offline_region_info value) =>
        new()
        {
            Id = value.id,
            Definition = CopyOfflineRegionDefinition(value.definition),
            MetadataStorage = new(
                NativeCallScope.CopyArray<byte>((byte*)value.metadata, (nuint)(value.metadata_size))
            ),
        };

    internal static mln_offline_region_info NativeOfflineRegionInfo(
        OfflineRegionInfo value,
        NativeCallScope scope
    )
    {
        Required(value.Definition, "OfflineRegionInfo.Definition must not be null.");
        var native = new mln_offline_region_info();
        native.size = (uint)sizeof(mln_offline_region_info);
        native.id = value.Id;
        native.definition = NativeOfflineRegionDefinition(value.Definition, scope);
        var bufferMetadata = scope.Buffer(value.MetadataStorage.Items);
        native.metadata = (byte*)bufferMetadata.data;
        native.metadata_size = checked((nuint)bufferMetadata.size);
        return native;
    }

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

    internal static mln_offline_region_status NativeOfflineRegionStatus(OfflineRegionStatus value)
    {
        var native = new mln_offline_region_status();
        native.size = (uint)sizeof(mln_offline_region_status);
        native.download_state = (uint)value.DownloadState;
        native.completed_resource_count = value.CompletedResourceCount;
        native.completed_resource_size = value.CompletedResourceSize;
        native.completed_tile_count = value.CompletedTileCount;
        native.required_tile_count = value.RequiredTileCount;
        native.completed_tile_size = value.CompletedTileSize;
        native.required_resource_count = value.RequiredResourceCount;
        native.required_resource_count_is_precise = (byte)(
            value.RequiredResourceCountIsPrecise ? 1 : 0
        );
        native.complete = (byte)(value.Complete ? 1 : 0);
        return native;
    }

    internal static OfflineTilePyramidRegionDefinition CopyOfflineTilePyramidRegionDefinition(
        mln_offline_tile_pyramid_region_definition value
    ) =>
        new(
            NativeCallScope.CopyCString(value.style_url),
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
        native.size = (uint)sizeof(mln_offline_tile_pyramid_region_definition);
        native.style_url = scope.CString(value.StyleUrl);
        native.bounds = NativeLatLngBounds(value.Bounds);
        native.min_zoom = value.MinZoom;
        native.max_zoom = value.MaxZoom;
        native.pixel_ratio = value.PixelRatio;
        native.include_ideographs = (byte)(value.IncludeIdeographs ? 1 : 0);
        return native;
    }

    internal static OpenglBorrowedTextureDescriptor CopyOpenglBorrowedTextureDescriptor(
        mln_opengl_borrowed_texture_descriptor value
    ) =>
        new(
            CopyRenderTargetExtent(value.extent),
            value.physical_width,
            value.physical_height,
            CopyOpenglContextDescriptor(value.context),
            value.texture,
            value.target
        );

    internal static mln_opengl_borrowed_texture_descriptor NativeOpenglBorrowedTextureDescriptor(
        OpenglBorrowedTextureDescriptor value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_opengl_borrowed_texture_descriptor_default();
        native.size = (uint)sizeof(mln_opengl_borrowed_texture_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.physical_width = value.PhysicalWidth;
        native.physical_height = value.PhysicalHeight;
        native.context = NativeOpenglContextDescriptor(value.Context, scope);
        native.texture = value.Texture;
        native.target = value.Target;
        return native;
    }

    internal static OpenglContextDescriptor CopyOpenglContextDescriptor(
        mln_opengl_context_descriptor value
    ) =>
        new(
            (OpenglContextOwnership)value.ownership,
            value.platform switch
            {
                (uint)mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_WGL =>
                    new OpenglContextDescriptor.DataValue.Wgl(
                        CopyWglContextDescriptor(value.data.wgl)
                    ),
                (uint)mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_EGL =>
                    new OpenglContextDescriptor.DataValue.Egl(
                        CopyEglContextDescriptor(value.data.egl)
                    ),
                (uint)mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_WEBGL =>
                    new OpenglContextDescriptor.DataValue.Webgl(
                        CopyWebglContextDescriptor(value.data.webgl)
                    ),
                _ => new OpenglContextDescriptor.DataValue.Unknown(
                    (uint)value.platform,
                    NativeCallScope.CopyValueBytes(value.data)
                ),
            }
        );

    internal static mln_opengl_context_descriptor NativeOpenglContextDescriptor(
        OpenglContextDescriptor value,
        NativeCallScope scope
    )
    {
        var native = new mln_opengl_context_descriptor();
        native.size = (uint)sizeof(mln_opengl_context_descriptor);
        native.ownership = (uint)value.Ownership;
        switch (value.Data)
        {
            case OpenglContextDescriptor.DataValue.Wgl selected:
                native.platform = (uint)mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_WGL;
                native.data.wgl = NativeWglContextDescriptor(selected.Value);
                break;
            case OpenglContextDescriptor.DataValue.Egl selected:
                native.platform = (uint)mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_EGL;
                native.data.egl = NativeEglContextDescriptor(selected.Value);
                break;
            case OpenglContextDescriptor.DataValue.Webgl selected:
                native.platform = (uint)
                    mln_opengl_context_platform.MLN_OPENGL_CONTEXT_PLATFORM_WEBGL;
                native.data.webgl = NativeWebglContextDescriptor(selected.Value, scope);
                break;
            default:
                throw new ArgumentException("A union variant is required.", nameof(value));
        }
        return native;
    }

    internal static OpenglOwnedTextureDescriptor CopyOpenglOwnedTextureDescriptor(
        mln_opengl_owned_texture_descriptor value
    ) => new(CopyRenderTargetExtent(value.extent), CopyOpenglContextDescriptor(value.context));

    internal static mln_opengl_owned_texture_descriptor NativeOpenglOwnedTextureDescriptor(
        OpenglOwnedTextureDescriptor value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_opengl_owned_texture_descriptor_default();
        native.size = (uint)sizeof(mln_opengl_owned_texture_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.context = NativeOpenglContextDescriptor(value.Context, scope);
        return native;
    }

    internal static OpenglOwnedTextureFrame CopyOpenglOwnedTextureFrame(
        mln_opengl_owned_texture_frame value
    ) =>
        new(
            value.generation,
            value.width,
            value.height,
            value.scale_factor,
            value.frame_id,
            value.texture,
            value.target,
            value.internal_format,
            value.format,
            value.type
        );

    internal static mln_opengl_owned_texture_frame NativeOpenglOwnedTextureFrame(
        OpenglOwnedTextureFrame value
    )
    {
        var native = new mln_opengl_owned_texture_frame();
        native.size = (uint)sizeof(mln_opengl_owned_texture_frame);
        native.generation = value.Generation;
        native.width = value.Width;
        native.height = value.Height;
        native.scale_factor = value.ScaleFactor;
        native.frame_id = value.FrameId;
        native.texture = value.Texture;
        native.target = value.Target;
        native.internal_format = value.InternalFormat;
        native.format = value.Format;
        native.type = value.Type;
        return native;
    }

    internal static OpenglSurfaceDescriptor CopyOpenglSurfaceDescriptor(
        mln_opengl_surface_descriptor value
    ) =>
        new(
            CopyRenderTargetExtent(value.extent),
            CopyOpenglContextDescriptor(value.context),
            NativePointer.FromNativeAddress((nint)value.surface)
        );

    internal static mln_opengl_surface_descriptor NativeOpenglSurfaceDescriptor(
        OpenglSurfaceDescriptor value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_opengl_surface_descriptor_default();
        native.size = (uint)sizeof(mln_opengl_surface_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.context = NativeOpenglContextDescriptor(value.Context, scope);
        native.surface = (void*)value.Surface.Address;
        return native;
    }

    internal static PremultipliedRgba8Image CopyPremultipliedRgba8Image(
        mln_premultiplied_rgba8_image value
    ) =>
        new()
        {
            Width = value.width,
            Height = value.height,
            Stride = value.stride,
            PixelsStorage = new(
                NativeCallScope.CopyArray<byte>((byte*)value.pixels, (nuint)(value.byte_length))
            ),
        };

    internal static mln_premultiplied_rgba8_image NativePremultipliedRgba8Image(
        PremultipliedRgba8Image value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_premultiplied_rgba8_image_default();
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
        var native = NativeMethods.mln_projection_mode_default();
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

    internal static QueriedFeature CopyQueriedFeature(mln_queried_feature value) =>
        new()
        {
            FeatureStorage = new(ValueStructs.CopyBufferView(value.feature)),
            SourceId = value.fields.HasFlag(MLN_QUERIED_FEATURE_SOURCE_ID)
                ? RuntimeStructs.CopyUtf8((sbyte*)value.source_id.data, value.source_id.size)
                : null,
            SourceLayerId = value.fields.HasFlag(MLN_QUERIED_FEATURE_SOURCE_LAYER_ID)
                ? RuntimeStructs.CopyUtf8(
                    (sbyte*)value.source_layer_id.data,
                    value.source_layer_id.size
                )
                : null,
            StateStorage = ValueArray.Optional(
                value.fields.HasFlag(MLN_QUERIED_FEATURE_STATE)
                    ? ValueStructs.CopyBufferView(value.state)
                    : null
            ),
        };

    internal static mln_queried_feature NativeQueriedFeature(
        QueriedFeature value,
        NativeCallScope scope
    )
    {
        var native = new mln_queried_feature();
        native.fields = 0;
        native.size = (uint)sizeof(mln_queried_feature);
        native.feature = scope.Buffer(value.FeatureStorage.Items);
        native.fields |= Put(
            value.SourceId,
            ref native.source_id,
            MLN_QUERIED_FEATURE_SOURCE_ID,
            present => scope.Utf8(present)
        );
        native.fields |= Put(
            value.SourceLayerId,
            ref native.source_layer_id,
            MLN_QUERIED_FEATURE_SOURCE_LAYER_ID,
            present => scope.Utf8(present)
        );
        native.fields |= Put(
            value.StateStorage?.Items,
            ref native.state,
            MLN_QUERIED_FEATURE_STATE,
            present => scope.Buffer(present)
        );
        return native;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void InvokeQueueLockLock(void* user_data)
    {
        try
        {
            using var restriction = NativeCallbackGuard.ForbidReentry();
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
            using var restriction = NativeCallbackGuard.ForbidReentry();
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
        native.size = (uint)sizeof(mln_queue_lock);
        if (value.Lock is not null || value.Unlock is not null)
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

    internal static mln_render_abandon_result NativeRenderAbandonResult(RenderAbandonResult value)
    {
        var native = new mln_render_abandon_result();
        native.size = (uint)sizeof(mln_render_abandon_result);
        native.disposition = (uint)value.Disposition;
        native.quarantined_resource_count = value.QuarantinedResourceCount;
        return native;
    }

    internal static RenderFrameResult CopyRenderFrameResult(mln_render_frame_result value) =>
        new(
            (RenderResult)value.disposition,
            value.token,
            value.map_update_generation,
            value.extent_generation,
            value.frame_generation,
            value.needs_repaint != 0
        );

    internal static mln_render_frame_result NativeRenderFrameResult(RenderFrameResult value)
    {
        var native = new mln_render_frame_result();
        native.size = (uint)sizeof(mln_render_frame_result);
        native.disposition = (uint)value.Disposition;
        native.token = value.Token;
        native.map_update_generation = value.MapUpdateGeneration;
        native.extent_generation = value.ExtentGeneration;
        native.frame_generation = value.FrameGeneration;
        native.needs_repaint = (byte)(value.NeedsRepaint ? 1 : 0);
        return native;
    }

    internal static RenderSessionAttachOptions CopyRenderSessionAttachOptions(
        mln_render_session_attach_options value
    ) =>
        new(
            (RenderDriverKind)value.driver,
            value.requested_texture_ring_depth,
            default,
            default,
            default
        );

    internal static mln_render_session_attach_options NativeRenderSessionAttachOptions(
        RenderSessionAttachOptions value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_render_session_attach_options_default();
        native.size = (uint)sizeof(mln_render_session_attach_options);
        native.driver = (uint)value.Driver;
        native.requested_texture_ring_depth = value.RequestedTextureRingDepth;
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

    internal static mln_render_session_capabilities NativeRenderSessionCapabilities(
        RenderSessionCapabilities value
    )
    {
        var native = new mln_render_session_capabilities();
        native.size = (uint)sizeof(mln_render_session_capabilities);
        native.driver = (uint)value.Driver;
        native.texture_ring_depth = value.TextureRingDepth;
        native.flags = (uint)value.Flags;
        return native;
    }

    internal static RenderSessionSnapshot CopyRenderSessionSnapshot(
        mln_render_session_snapshot value
    ) =>
        new(
            (RenderSessionState)value.state,
            (RenderDriverKind)value.driver,
            (RenderResult)value.latest_result,
            CopyRenderTargetExtent(value.extent),
            value.generation,
            value.map_update_generation,
            value.rendered_update_generation,
            value.extent_generation,
            value.frame_generation,
            value.latest_demand_token,
            value.pending_demand_count,
            value.acquired_frame_count,
            value.target_ready != 0,
            value.pending_changes != 0
        );

    internal static mln_render_session_snapshot NativeRenderSessionSnapshot(
        RenderSessionSnapshot value
    )
    {
        var native = new mln_render_session_snapshot();
        native.size = (uint)sizeof(mln_render_session_snapshot);
        native.state = (uint)value.State;
        native.driver = (uint)value.Driver;
        native.latest_result = (uint)value.LatestResult;
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.generation = value.Generation;
        native.map_update_generation = value.MapUpdateGeneration;
        native.rendered_update_generation = value.RenderedUpdateGeneration;
        native.extent_generation = value.ExtentGeneration;
        native.frame_generation = value.FrameGeneration;
        native.latest_demand_token = value.LatestDemandToken;
        native.pending_demand_count = value.PendingDemandCount;
        native.acquired_frame_count = value.AcquiredFrameCount;
        native.target_ready = (byte)(value.TargetReady ? 1 : 0);
        native.pending_changes = (byte)(value.PendingChanges ? 1 : 0);
        return native;
    }

    internal static RenderTargetExtent CopyRenderTargetExtent(mln_render_target_extent value) =>
        new(value.width, value.height, value.scale_factor);

    internal static mln_render_target_extent NativeRenderTargetExtent(RenderTargetExtent value)
    {
        var native = new mln_render_target_extent();
        native.size = (uint)sizeof(mln_render_target_extent);
        native.width = value.Width;
        native.height = value.Height;
        native.scale_factor = value.ScaleFactor;
        return native;
    }

    internal static RenderedFeatureQueryOptions CopyRenderedFeatureQueryOptions(
        mln_rendered_feature_query_options value
    ) =>
        new()
        {
            LayerIdsStorage = ValueArray.Optional(
                value.fields.HasFlag(MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS)
                    ? NativeCallScope.CopyArray<mln_buffer_view, string>(
                        value.layer_ids,
                        (nuint)(value.layer_id_count),
                        static item => RuntimeStructs.CopyUtf8((sbyte*)item.data, item.size)
                    )
                    : null
            ),
            FilterStorage = ValueArray.Optional(
                value.filter == null
                    ? null
                    : ValueStructs.CopyBufferView(NativeCallScope.Read(value.filter))
            ),
        };

    internal static mln_rendered_feature_query_options NativeRenderedFeatureQueryOptions(
        RenderedFeatureQueryOptions value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_rendered_feature_query_options_default();
        native.fields = 0;
        native.size = (uint)sizeof(mln_rendered_feature_query_options);
        if (value.LayerIdsStorage?.Items is { } fieldLayerIds)
        {
            native.fields |= MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
            native.layer_ids = scope.Array<mln_buffer_view, string>(
                fieldLayerIds,
                item => scope.Utf8(item)
            );
            native.layer_id_count = checked((nuint)fieldLayerIds.Length);
        }
        var fieldFilter = value.FilterStorage?.Items;
        native.filter = fieldFilter is null ? null : scope.Value(scope.Buffer(fieldFilter));
        return native;
    }

    internal static RenderedQueryGeometry CopyRenderedQueryGeometry(
        mln_rendered_query_geometry value
    ) =>
        value.type switch
        {
            (uint)mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT =>
                new RenderedQueryGeometry.Point(CopyScreenPoint(value.data.point)),
            (uint)mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX =>
                new RenderedQueryGeometry.Box(CopyScreenBox(value.data.box)),
            (uint)mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING =>
                new RenderedQueryGeometry.LineString(CopyScreenLineString(value.data.line_string)),
            _ => new RenderedQueryGeometry.Unknown(
                (uint)value.type,
                NativeCallScope.CopyValueBytes(value.data)
            ),
        };

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
                native.type = (uint)
                    mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT;
                native.data.point = NativeScreenPoint(selected.Value);
                break;
            case RenderedQueryGeometry.Box selected:
                native.type = (uint)
                    mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX;
                native.data.box = NativeScreenBox(selected.Value);
                break;
            case RenderedQueryGeometry.LineString selected:
                native.type = (uint)
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

    internal static mln_rendering_stats NativeRenderingStats(RenderingStats value)
    {
        var native = new mln_rendering_stats();
        native.encoding_time = value.EncodingTime;
        native.rendering_time = value.RenderingTime;
        native.frame_count = value.FrameCount;
        native.draw_call_count = value.DrawCallCount;
        native.total_draw_call_count = value.TotalDrawCallCount;
        return native;
    }

    private static readonly string[] AllowedResourceProviderCallback =
    [
        "mln_resource_request_complete",
        "mln_resource_request_cancelled",
        "mln_resource_request_set_cancel_callback",
        "mln_resource_request_release",
    ];

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static uint InvokeResourceProviderCallback(
        void* user_data,
        mln_resource_request* request,
        MlnResourceRequest handle
    )
    {
        ResourceRequestHandle? owned = null;
        try
        {
            owned = ResourceRequestHandle.BorrowDecision(handle);
            using var restriction = NativeCallbackGuard.Restrict(
                owned,
                AllowedResourceProviderCallback
            );
            var decision =
                ((ResourceProvider)NativeCallbackRoot.Value(user_data)).Callback?.Invoke(
                    CopyResourceRequest(NativeCallScope.Read(request)),
                    owned
                )
                ?? (ResourceProviderDecision)
                    mln_resource_provider_decision.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
            return owned.FinishDecision(
                decision
                    == (ResourceProviderDecision)
                        mln_resource_provider_decision.MLN_RESOURCE_PROVIDER_DECISION_HANDLE
            )
                ? (uint)mln_resource_provider_decision.MLN_RESOURCE_PROVIDER_DECISION_HANDLE
                : (uint)decision;
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_resource_provider_callback", error);
            try
            {
                return owned is not null && owned.FinishDecision(false)
                    ? (uint)mln_resource_provider_decision.MLN_RESOURCE_PROVIDER_DECISION_HANDLE
                    : (uint)
                        mln_resource_provider_decision.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
            }
            catch
            {
                return (uint)mln_resource_provider_decision.MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
            }
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
        return native;
    }

    internal static ResourceRequest CopyResourceRequest(mln_resource_request value) =>
        new()
        {
            RequestedUrl =
                value.requested_url == null
                    ? null
                    : NativeCallScope.CopyCString(value.requested_url),
            ResolvedUrl =
                value.resolved_url == null ? null : NativeCallScope.CopyCString(value.resolved_url),
            Kind = (ResourceKind)value.kind,
            LoadingMethod = (ResourceLoadingMethod)value.loading_method,
            Priority = (ResourcePriority)value.priority,
            Usage = (ResourceUsage)value.usage,
            StoragePolicy = (ResourceStoragePolicy)value.storage_policy,
            Range =
                value.has_range != 0
                    ? new ResourceRequest.RangeValue(value.range_start, value.range_end)
                    : null,
            PriorModifiedUnixMs =
                value.has_prior_modified != 0 ? value.prior_modified_unix_ms : null,
            PriorExpiresUnixMs = value.has_prior_expires != 0 ? value.prior_expires_unix_ms : null,
            PriorEtag =
                value.prior_etag == null ? null : NativeCallScope.CopyCString(value.prior_etag),
            PriorDataStorage = new(
                NativeCallScope.CopyArray<byte>(
                    (byte*)value.prior_data,
                    (nuint)(value.prior_data_size)
                )
            ),
        };

    internal static mln_resource_request NativeResourceRequest(
        ResourceRequest value,
        NativeCallScope scope
    )
    {
        var native = new mln_resource_request();
        native.has_range = 0;
        native.has_prior_modified = 0;
        native.has_prior_expires = 0;
        native.size = (uint)sizeof(mln_resource_request);
        native.requested_url = value.RequestedUrl is null
            ? null
            : scope.CString(value.RequestedUrl);
        native.resolved_url = value.ResolvedUrl is null ? null : scope.CString(value.ResolvedUrl);
        native.kind = (uint)value.Kind;
        native.loading_method = (uint)value.LoadingMethod;
        native.priority = (uint)value.Priority;
        native.usage = (uint)value.Usage;
        native.storage_policy = (uint)value.StoragePolicy;
        if (value.Range is { } fieldRange)
        {
            native.has_range = 1;
            native.range_start = fieldRange.RangeStart;
            native.range_end = fieldRange.RangeEnd;
        }
        if (value.PriorModifiedUnixMs is { } fieldPriorModifiedUnixMs)
        {
            native.has_prior_modified = 1;
            native.prior_modified_unix_ms = fieldPriorModifiedUnixMs;
        }
        if (value.PriorExpiresUnixMs is { } fieldPriorExpiresUnixMs)
        {
            native.has_prior_expires = 1;
            native.prior_expires_unix_ms = fieldPriorExpiresUnixMs;
        }
        native.prior_etag = value.PriorEtag is null ? null : scope.CString(value.PriorEtag);
        var bufferPriorData = scope.Buffer(value.PriorDataStorage.Items);
        native.prior_data = (byte*)bufferPriorData.data;
        native.prior_data_size = checked((nuint)bufferPriorData.size);
        return native;
    }

    internal static mln_resource_response NativeResourceResponse(
        ResourceResponse value,
        NativeCallScope scope
    )
    {
        var native = new mln_resource_response();
        native.has_modified = 0;
        native.has_expires = 0;
        native.has_retry_after = 0;
        native.size = (uint)sizeof(mln_resource_response);
        native.status = (uint)value.Status;
        native.error_reason = (uint)value.ErrorReason;
        var bufferBytes = scope.Buffer(value.BytesStorage.Items);
        native.bytes = (byte*)bufferBytes.data;
        native.byte_count = checked((nuint)bufferBytes.size);
        native.error_message = value.ErrorMessage is null
            ? null
            : scope.CString(value.ErrorMessage);
        native.must_revalidate = (byte)(value.MustRevalidate ? 1 : 0);
        if (value.ModifiedUnixMs is { } fieldModifiedUnixMs)
        {
            native.has_modified = 1;
            native.modified_unix_ms = fieldModifiedUnixMs;
        }
        if (value.ExpiresUnixMs is { } fieldExpiresUnixMs)
        {
            native.has_expires = 1;
            native.expires_unix_ms = fieldExpiresUnixMs;
        }
        native.etag = value.Etag is null ? null : scope.CString(value.Etag);
        if (value.RetryAfterUnixMs is { } fieldRetryAfterUnixMs)
        {
            native.has_retry_after = 1;
            native.retry_after_unix_ms = fieldRetryAfterUnixMs;
        }
        return native;
    }

    private static readonly string[] AllowedResourceTransformCallback =
    [
        "mln_resource_transform_response_set_url",
    ];

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static mln_status InvokeResourceTransformCallback(
        void* user_data,
        uint kind,
        sbyte* url,
        mln_resource_transform_response* out_response
    )
    {
        try
        {
            var responseOutResponse = new ResourceTransformResponse(out_response);
            using var restriction = NativeCallbackGuard.Restrict(
                responseOutResponse,
                AllowedResourceTransformCallback
            );
            try
            {
                ((ResourceTransform)NativeCallbackRoot.Value(user_data)).Callback?.Invoke(
                    (ResourceKind)kind,
                    NativeCallScope.CopyCString(url),
                    responseOutResponse
                );
            }
            finally
            {
                responseOutResponse.Expire();
            }
            return mln_status.MLN_STATUS_OK;
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_resource_transform_callback", error);
            return mln_status.MLN_STATUS_NATIVE_ERROR;
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
        string message = "",
        byte* record = null,
        nuint recordSize = 0
    ) =>
        new(
            (RuntimeEventType)value.type,
            (RuntimeEventSourceType)value.source_type,
            value.source,
            value.code,
            value.payload_type switch
            {
                (uint)mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME =>
                    new RuntimeEvent.PayloadValue.RenderFrame(
                        CopyRuntimeEventRenderFrame(value.payload.render_frame)
                    ),
                (uint)mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP =>
                    new RuntimeEvent.PayloadValue.RenderMap(
                        CopyRuntimeEventRenderMap(value.payload.render_map)
                    ),
                (uint)mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION =>
                    new RuntimeEvent.PayloadValue.TileAction(
                        CopyRuntimeEventTileAction(value.payload.tile_action)
                    ),
                (uint)
                    mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS =>
                    new RuntimeEvent.PayloadValue.OfflineRegionStatus(
                        CopyRuntimeEventOfflineRegionStatus(value.payload.offline_region_status)
                    ),
                (uint)
                    mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR =>
                    new RuntimeEvent.PayloadValue.OfflineRegionResponseError(
                        CopyRuntimeEventOfflineRegionResponseError(
                            value.payload.offline_region_response_error
                        )
                    ),
                (uint)
                    mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT =>
                    new RuntimeEvent.PayloadValue.OfflineRegionTileCountLimit(
                        CopyRuntimeEventOfflineRegionTileCountLimit(
                            value.payload.offline_region_tile_count_limit
                        )
                    ),
                (uint)
                    mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED =>
                    new RuntimeEvent.PayloadValue.CameraTransitionFinished(
                        CopyRuntimeEventCameraTransitionFinished(
                            value.payload.camera_transition_finished
                        )
                    ),
                (uint)mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_NONE =>
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
            },
            message
        );

    internal static RuntimeEventBatchView CopyRuntimeEventBatchView(
        mln_runtime_event_batch_view value
    ) => new() { EventsStorage = new(CopyRuntimeEventBatchViewEvents(value)) };

    internal static RuntimeEvent[] CopyRuntimeEventBatchViewEvents(
        mln_runtime_event_batch_view value
    )
    {
        var count = checked((int)value.event_count);
        if (value.event_size < sizeof(mln_runtime_event) || (count != 0 && value.events == null))
            throw new InvalidOperationException("Invalid native array storage.");
        var copied = new RuntimeEvent[count];
        for (var index = 0; index < count; index++)
        {
            var record = (byte*)value.events + checked((nuint)index * value.event_size);
            var item = *(mln_runtime_event*)record;
            if (
                item.message_offset > value.messages_size
                || item.message_size > value.messages_size - item.message_offset
                || (item.message_size != 0 && value.messages == null)
            )
                throw new InvalidOperationException("Invalid native item buffer.");
            var message = RuntimeStructs.CopyUtf8(
                (byte*)value.messages + item.message_offset,
                item.message_size
            );
            copied[index] = CopyRuntimeEvent(item, message, record, value.event_size);
        }
        return copied;
    }

    internal static RuntimeEventCameraTransitionFinished CopyRuntimeEventCameraTransitionFinished(
        mln_runtime_event_camera_transition_finished value
    ) => new(value.transition_id);

    internal static mln_runtime_event_camera_transition_finished NativeRuntimeEventCameraTransitionFinished(
        RuntimeEventCameraTransitionFinished value
    )
    {
        var native = new mln_runtime_event_camera_transition_finished();
        native.transition_id = value.TransitionId;
        return native;
    }

    internal static RuntimeEventOfflineRegionResponseError CopyRuntimeEventOfflineRegionResponseError(
        mln_runtime_event_offline_region_response_error value
    ) => new(value.region_id, (ResourceErrorReason)value.reason);

    internal static mln_runtime_event_offline_region_response_error NativeRuntimeEventOfflineRegionResponseError(
        RuntimeEventOfflineRegionResponseError value
    )
    {
        var native = new mln_runtime_event_offline_region_response_error();
        native.region_id = value.RegionId;
        native.reason = (uint)value.Reason;
        return native;
    }

    internal static RuntimeEventOfflineRegionStatus CopyRuntimeEventOfflineRegionStatus(
        mln_runtime_event_offline_region_status value
    ) => new(value.region_id, CopyOfflineRegionStatus(value.status));

    internal static mln_runtime_event_offline_region_status NativeRuntimeEventOfflineRegionStatus(
        RuntimeEventOfflineRegionStatus value
    )
    {
        var native = new mln_runtime_event_offline_region_status();
        native.region_id = value.RegionId;
        native.status = NativeOfflineRegionStatus(value.Status);
        return native;
    }

    internal static RuntimeEventOfflineRegionTileCountLimit CopyRuntimeEventOfflineRegionTileCountLimit(
        mln_runtime_event_offline_region_tile_count_limit value
    ) => new(value.region_id, value.limit);

    internal static mln_runtime_event_offline_region_tile_count_limit NativeRuntimeEventOfflineRegionTileCountLimit(
        RuntimeEventOfflineRegionTileCountLimit value
    )
    {
        var native = new mln_runtime_event_offline_region_tile_count_limit();
        native.region_id = value.RegionId;
        native.limit = value.Limit;
        return native;
    }

    internal static RuntimeEventRenderFrame CopyRuntimeEventRenderFrame(
        mln_runtime_event_render_frame value
    ) =>
        new(
            (RenderMode)value.mode,
            value.needs_repaint != 0,
            value.placement_changed != 0,
            CopyRenderingStats(value.stats)
        );

    internal static mln_runtime_event_render_frame NativeRuntimeEventRenderFrame(
        RuntimeEventRenderFrame value
    )
    {
        var native = new mln_runtime_event_render_frame();
        native.mode = (uint)value.Mode;
        native.needs_repaint = (byte)(value.NeedsRepaint ? 1 : 0);
        native.placement_changed = (byte)(value.PlacementChanged ? 1 : 0);
        native.stats = NativeRenderingStats(value.Stats);
        return native;
    }

    internal static RuntimeEventRenderMap CopyRuntimeEventRenderMap(
        mln_runtime_event_render_map value
    ) => new((RenderMode)value.mode);

    internal static mln_runtime_event_render_map NativeRuntimeEventRenderMap(
        RuntimeEventRenderMap value
    )
    {
        var native = new mln_runtime_event_render_map();
        native.mode = (uint)value.Mode;
        return native;
    }

    internal static RuntimeEventTileAction CopyRuntimeEventTileAction(
        mln_runtime_event_tile_action value
    ) => new((TileOperation)value.operation, CopyTileId(value.tile_id));

    internal static mln_runtime_event_tile_action NativeRuntimeEventTileAction(
        RuntimeEventTileAction value
    )
    {
        var native = new mln_runtime_event_tile_action();
        native.operation = (uint)value.Operation;
        native.tile_id = NativeTileId(value.TileId);
        return native;
    }

    internal static RuntimeOptions CopyRuntimeOptions(mln_runtime_options value) =>
        new(
            value.flags,
            value.asset_path == null ? null : NativeCallScope.CopyCString(value.asset_path),
            value.cache_path == null ? null : NativeCallScope.CopyCString(value.cache_path),
            (RuntimeEventMask)value.event_mask,
            default
        );

    internal static mln_runtime_options NativeRuntimeOptions(
        RuntimeOptions value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_runtime_options_default();
        native.size = (uint)sizeof(mln_runtime_options);
        native.flags = value.Flags;
        native.asset_path = value.AssetPath is null ? null : scope.CString(value.AssetPath);
        native.cache_path = value.CachePath is null ? null : scope.CString(value.CachePath);
        native.event_mask = (ulong)value.EventMask;
        native.event_wake = NativeWake(value.EventWake, scope);
        return native;
    }

    internal static ScreenBox CopyScreenBox(mln_screen_box value) =>
        new(CopyScreenPoint(value.min), CopyScreenPoint(value.max));

    internal static mln_screen_box NativeScreenBox(ScreenBox value)
    {
        var native = new mln_screen_box();
        native.min = NativeScreenPoint(value.Min);
        native.max = NativeScreenPoint(value.Max);
        return native;
    }

    internal static ScreenLineString CopyScreenLineString(mln_screen_line_string value) =>
        new()
        {
            PointsStorage = new(
                NativeCallScope.CopyArray<mln_screen_point, ScreenPoint>(
                    value.points,
                    (nuint)(value.point_count),
                    static item => CopyScreenPoint(item)
                )
            ),
        };

    internal static mln_screen_line_string NativeScreenLineString(
        ScreenLineString value,
        NativeCallScope scope
    )
    {
        var native = new mln_screen_line_string();
        native.points = scope.Array<mln_screen_point, ScreenPoint>(
            value.PointsStorage.Items,
            item => NativeScreenPoint(item)
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

    internal static SourceFeatureQueryOptions CopySourceFeatureQueryOptions(
        mln_source_feature_query_options value
    ) =>
        new()
        {
            SourceLayerIdsStorage = ValueArray.Optional(
                value.fields.HasFlag(MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS)
                    ? NativeCallScope.CopyArray<mln_buffer_view, string>(
                        value.source_layer_ids,
                        (nuint)(value.source_layer_id_count),
                        static item => RuntimeStructs.CopyUtf8((sbyte*)item.data, item.size)
                    )
                    : null
            ),
            FilterStorage = ValueArray.Optional(
                value.filter == null
                    ? null
                    : ValueStructs.CopyBufferView(NativeCallScope.Read(value.filter))
            ),
        };

    internal static mln_source_feature_query_options NativeSourceFeatureQueryOptions(
        SourceFeatureQueryOptions value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_source_feature_query_options_default();
        native.fields = 0;
        native.size = (uint)sizeof(mln_source_feature_query_options);
        if (value.SourceLayerIdsStorage?.Items is { } fieldSourceLayerIds)
        {
            native.fields |= MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
            native.source_layer_ids = scope.Array<mln_buffer_view, string>(
                fieldSourceLayerIds,
                item => scope.Utf8(item)
            );
            native.source_layer_id_count = checked((nuint)fieldSourceLayerIds.Length);
        }
        var fieldFilter = value.FilterStorage?.Items;
        native.filter = fieldFilter is null ? null : scope.Value(scope.Buffer(fieldFilter));
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
        };

    internal static mln_style_image_info NativeStyleImageInfo(
        StyleImageInfo value,
        NativeCallScope scope
    )
    {
        var native = new mln_style_image_info();
        native.fields = 0;
        native.size = (uint)sizeof(mln_style_image_info);
        native.width = value.Width;
        native.height = value.Height;
        native.pixels = scope.Buffer(value.PixelsStorage.Items);
        native.stretch_x = scope.Array<mln_image_stretch, ImageStretch>(
            value.StretchXStorage.Items,
            item => NativeImageStretch(item)
        );
        native.stretch_x_count = checked((nuint)value.StretchXStorage.Items.Length);
        native.stretch_y = scope.Array<mln_image_stretch, ImageStretch>(
            value.StretchYStorage.Items,
            item => NativeImageStretch(item)
        );
        native.stretch_y_count = checked((nuint)value.StretchYStorage.Items.Length);
        native.fields |= Put(
            value.Content,
            ref native.content,
            MLN_STYLE_IMAGE_INFO_CONTENT,
            NativeImageContent
        );
        native.fields |= Put(
            value.TextFitWidth,
            ref native.text_fit_width,
            MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH,
            static present => (uint)present
        );
        native.fields |= Put(
            value.TextFitHeight,
            ref native.text_fit_height,
            MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT,
            static present => (uint)present
        );
        native.pixel_ratio = value.PixelRatio;
        native.sdf = (byte)(value.Sdf ? 1 : 0);
        return native;
    }

    internal static StyleImageOptions CopyStyleImageOptions(mln_style_image_options value) =>
        new()
        {
            StretchXStorage = ValueArray.Optional(
                value.fields.HasFlag(MLN_STYLE_IMAGE_OPTION_STRETCH_X)
                    ? NativeCallScope.CopyArray<mln_image_stretch, ImageStretch>(
                        value.stretch_x,
                        (nuint)(value.stretch_x_count),
                        static item => CopyImageStretch(item)
                    )
                    : null
            ),
            StretchYStorage = ValueArray.Optional(
                value.fields.HasFlag(MLN_STYLE_IMAGE_OPTION_STRETCH_Y)
                    ? NativeCallScope.CopyArray<mln_image_stretch, ImageStretch>(
                        value.stretch_y,
                        (nuint)(value.stretch_y_count),
                        static item => CopyImageStretch(item)
                    )
                    : null
            ),
            Content = value.fields.HasFlag(MLN_STYLE_IMAGE_OPTION_CONTENT)
                ? CopyImageContent(value.content)
                : null,
            TextFitWidth = value.fields.HasFlag(MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH)
                ? (StyleImageTextFit)value.text_fit_width
                : null,
            TextFitHeight = value.fields.HasFlag(MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT)
                ? (StyleImageTextFit)value.text_fit_height
                : null,
            PixelRatio = value.fields.HasFlag(MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO)
                ? value.pixel_ratio
                : null,
            Sdf = value.fields.HasFlag(MLN_STYLE_IMAGE_OPTION_SDF) ? value.sdf != 0 : null,
        };

    internal static mln_style_image_options NativeStyleImageOptions(
        StyleImageOptions value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_style_image_options_default();
        native.fields = 0;
        native.size = (uint)sizeof(mln_style_image_options);
        if (value.StretchXStorage?.Items is { } fieldStretchX)
        {
            native.fields |= MLN_STYLE_IMAGE_OPTION_STRETCH_X;
            native.stretch_x = scope.Array<mln_image_stretch, ImageStretch>(
                fieldStretchX,
                item => NativeImageStretch(item)
            );
            native.stretch_x_count = checked((nuint)fieldStretchX.Length);
        }
        if (value.StretchYStorage?.Items is { } fieldStretchY)
        {
            native.fields |= MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
            native.stretch_y = scope.Array<mln_image_stretch, ImageStretch>(
                fieldStretchY,
                item => NativeImageStretch(item)
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
            static present => (uint)present
        );
        native.fields |= Put(
            value.TextFitHeight,
            ref native.text_fit_height,
            MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT,
            static present => (uint)present
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
        return native;
    }

    internal static StyleLayerEntry CopyStyleLayerEntry(mln_style_layer_entry value) =>
        new(
            RuntimeStructs.CopyUtf8((sbyte*)value.id.data, value.id.size),
            RuntimeStructs.CopyUtf8((sbyte*)value.type.data, value.type.size),
            value.source_id.size == 0
                ? null
                : RuntimeStructs.CopyUtf8((sbyte*)value.source_id.data, value.source_id.size),
            value.source_layer.size == 0
                ? null
                : RuntimeStructs.CopyUtf8((sbyte*)value.source_layer.data, value.source_layer.size)
        );

    internal static mln_style_layer_entry NativeStyleLayerEntry(
        StyleLayerEntry value,
        NativeCallScope scope
    )
    {
        Required(value.Id, "StyleLayerEntry.Id must not be null.");
        Required(value.Type, "StyleLayerEntry.Type must not be null.");
        var native = new mln_style_layer_entry();
        native.size = (uint)sizeof(mln_style_layer_entry);
        native.id = scope.Utf8(value.Id);
        native.type = scope.Utf8(value.Type);
        native.source_id = scope.Utf8(value.SourceId ?? "");
        native.source_layer = scope.Utf8(value.SourceLayer ?? "");
        return native;
    }

    internal static StyleLayerInfo CopyStyleLayerInfo(mln_style_layer_info value) =>
        new(
            (StyleLayerVisibility)value.visibility,
            RuntimeStructs.CopyUtf8((sbyte*)value.type.data, value.type.size),
            value.min_zoom,
            value.max_zoom,
            value.source_id.size == 0
                ? null
                : RuntimeStructs.CopyUtf8((sbyte*)value.source_id.data, value.source_id.size),
            value.source_layer.size == 0
                ? null
                : RuntimeStructs.CopyUtf8((sbyte*)value.source_layer.data, value.source_layer.size)
        );

    internal static mln_style_layer_info NativeStyleLayerInfo(
        StyleLayerInfo value,
        NativeCallScope scope
    )
    {
        Required(value.Type, "StyleLayerInfo.Type must not be null.");
        var native = new mln_style_layer_info();
        native.size = (uint)sizeof(mln_style_layer_info);
        native.visibility = (uint)value.Visibility;
        native.type = scope.Utf8(value.Type);
        native.min_zoom = value.MinZoom;
        native.max_zoom = value.MaxZoom;
        native.source_id = scope.Utf8(value.SourceId ?? "");
        native.source_layer = scope.Utf8(value.SourceLayer ?? "");
        return native;
    }

    internal static StyleSourceInfo CopyStyleSourceInfo(mln_style_source_info value) =>
        new()
        {
            Type = (StyleSourceType)value.type,
            IsVolatile = value.is_volatile != 0,
            Attribution = value.fields.HasFlag(MLN_STYLE_SOURCE_INFO_ATTRIBUTION)
                ? RuntimeStructs.CopyUtf8((sbyte*)value.attribution.data, value.attribution.size)
                : null,
            Url = value.fields.HasFlag(MLN_STYLE_SOURCE_INFO_URL)
                ? RuntimeStructs.CopyUtf8((sbyte*)value.url.data, value.url.size)
                : null,
            Tilejson = value.fields.HasFlag(MLN_STYLE_SOURCE_INFO_TILEJSON)
                ? CopyStyleSourceTileInfo(value.tilejson)
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

    internal static mln_style_source_info NativeStyleSourceInfo(
        StyleSourceInfo value,
        NativeCallScope scope
    )
    {
        var native = new mln_style_source_info();
        native.fields = 0;
        native.size = (uint)sizeof(mln_style_source_info);
        native.type = (uint)value.Type;
        native.is_volatile = (byte)(value.IsVolatile ? 1 : 0);
        native.fields |= Put(
            value.Attribution,
            ref native.attribution,
            MLN_STYLE_SOURCE_INFO_ATTRIBUTION,
            present => scope.Utf8(present)
        );
        native.fields |= Put(
            value.Url,
            ref native.url,
            MLN_STYLE_SOURCE_INFO_URL,
            present => scope.Utf8(present)
        );
        native.fields |= Put(
            value.Tilejson,
            ref native.tilejson,
            MLN_STYLE_SOURCE_INFO_TILEJSON,
            present => NativeStyleSourceTileInfo(present, scope)
        );
        native.fields |= Put(
            value.Bounds,
            ref native.bounds,
            MLN_STYLE_SOURCE_INFO_BOUNDS,
            NativeLatLngBounds
        );
        native.fields |= Put(value.TileSize, ref native.tile_size, MLN_STYLE_SOURCE_INFO_TILE_SIZE);
        native.fields |= Put(
            value.VectorEncoding,
            ref native.vector_encoding,
            MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING,
            static present => (uint)present
        );
        native.fields |= Put(
            value.RasterEncoding,
            ref native.raster_encoding,
            MLN_STYLE_SOURCE_INFO_RASTER_ENCODING,
            static present => (uint)present
        );
        return native;
    }

    internal static StyleSourceTileInfo CopyStyleSourceTileInfo(mln_style_source_tile_info value) =>
        new()
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

    internal static mln_style_source_tile_info NativeStyleSourceTileInfo(
        StyleSourceTileInfo value,
        NativeCallScope scope
    )
    {
        var native = new mln_style_source_tile_info();
        native.tile_urls = scope.Array<mln_buffer_view, string>(
            value.TileUrlsStorage.Items,
            item => scope.Utf8(item)
        );
        native.tile_url_count = checked((nuint)value.TileUrlsStorage.Items.Length);
        native.min_zoom = value.MinZoom;
        native.max_zoom = value.MaxZoom;
        native.scheme = (uint)value.Scheme;
        return native;
    }

    internal static StyleTileSourceOptions CopyStyleTileSourceOptions(
        mln_style_tile_source_options value
    ) =>
        new()
        {
            MinZoom = value.fields.HasFlag(MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM)
                ? value.min_zoom
                : null,
            MaxZoom = value.fields.HasFlag(MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM)
                ? value.max_zoom
                : null,
            Attribution = value.fields.HasFlag(MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION)
                ? RuntimeStructs.CopyUtf8((sbyte*)value.attribution.data, value.attribution.size)
                : null,
            Scheme = value.fields.HasFlag(MLN_STYLE_TILE_SOURCE_OPTION_SCHEME)
                ? (StyleTileScheme)value.scheme
                : null,
            Bounds = value.fields.HasFlag(MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS)
                ? CopyLatLngBounds(value.bounds)
                : null,
            TileSize = value.fields.HasFlag(MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE)
                ? value.tile_size
                : null,
            VectorEncoding = value.fields.HasFlag(MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING)
                ? (StyleVectorTileEncoding)value.vector_encoding
                : null,
            RasterEncoding = value.fields.HasFlag(MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING)
                ? (StyleRasterDemEncoding)value.raster_encoding
                : null,
        };

    internal static mln_style_tile_source_options NativeStyleTileSourceOptions(
        StyleTileSourceOptions value,
        NativeCallScope scope
    )
    {
        var native = NativeMethods.mln_style_tile_source_options_default();
        native.fields = 0;
        native.size = (uint)sizeof(mln_style_tile_source_options);
        native.fields |= Put(
            value.MinZoom,
            ref native.min_zoom,
            MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
        );
        native.fields |= Put(
            value.MaxZoom,
            ref native.max_zoom,
            MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM
        );
        native.fields |= Put(
            value.Attribution,
            ref native.attribution,
            MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION,
            present => scope.Utf8(present)
        );
        native.fields |= Put(
            value.Scheme,
            ref native.scheme,
            MLN_STYLE_TILE_SOURCE_OPTION_SCHEME,
            static present => (uint)present
        );
        native.fields |= Put(
            value.Bounds,
            ref native.bounds,
            MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS,
            NativeLatLngBounds
        );
        native.fields |= Put(
            value.TileSize,
            ref native.tile_size,
            MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
        );
        native.fields |= Put(
            value.VectorEncoding,
            ref native.vector_encoding,
            MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING,
            static present => (uint)present
        );
        native.fields |= Put(
            value.RasterEncoding,
            ref native.raster_encoding,
            MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING,
            static present => (uint)present
        );
        return native;
    }

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
        var native = NativeMethods.mln_style_transition_options_default();
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

    internal static mln_texture_image_info NativeTextureImageInfo(TextureImageInfo value)
    {
        var native = NativeMethods.mln_texture_image_info_default();
        native.size = (uint)sizeof(mln_texture_image_info);
        native.width = value.Width;
        native.height = value.Height;
        native.stride = value.Stride;
        native.byte_length = checked((nuint)value.ByteLength);
        return native;
    }

    internal static TextureReadbackResult CopyTextureReadbackResult(
        mln_texture_readback_result value
    ) =>
        new()
        {
            DataStorage = new(ValueStructs.CopyBufferView(value.data)),
            Info = CopyTextureImageInfo(value.info),
        };

    internal static mln_texture_readback_result NativeTextureReadbackResult(
        TextureReadbackResult value,
        NativeCallScope scope
    )
    {
        var native = new mln_texture_readback_result();
        native.size = (uint)sizeof(mln_texture_readback_result);
        native.data = scope.Buffer(value.DataStorage.Items);
        native.info = NativeTextureImageInfo(value.Info);
        return native;
    }

    internal static TileId CopyTileId(mln_tile_id value) =>
        new(
            value.overscaled_z,
            value.wrap,
            value.canonical_z,
            value.canonical_x,
            value.canonical_y
        );

    internal static mln_tile_id NativeTileId(TileId value)
    {
        var native = new mln_tile_id();
        native.overscaled_z = value.OverscaledZ;
        native.wrap = value.Wrap;
        native.canonical_z = value.CanonicalZ;
        native.canonical_x = value.CanonicalX;
        native.canonical_y = value.CanonicalY;
        return native;
    }

    internal static UnitBezier CopyUnitBezier(mln_unit_bezier value) =>
        new(value.x1, value.y1, value.x2, value.y2);

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

    internal static VulkanBorrowedTextureDescriptor CopyVulkanBorrowedTextureDescriptor(
        mln_vulkan_borrowed_texture_descriptor value
    ) =>
        new(
            CopyRenderTargetExtent(value.extent),
            value.physical_width,
            value.physical_height,
            CopyVulkanContextDescriptor(value.context),
            value.image,
            value.image_view,
            value.format,
            value.initial_layout,
            value.final_layout
        );

    internal static mln_vulkan_borrowed_texture_descriptor NativeVulkanBorrowedTextureDescriptor(
        VulkanBorrowedTextureDescriptor value
    )
    {
        var native = NativeMethods.mln_vulkan_borrowed_texture_descriptor_default();
        native.size = (uint)sizeof(mln_vulkan_borrowed_texture_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.physical_width = value.PhysicalWidth;
        native.physical_height = value.PhysicalHeight;
        native.context = NativeVulkanContextDescriptor(value.Context);
        native.image = value.Image;
        native.image_view = value.ImageView;
        native.format = value.Format;
        native.initial_layout = value.InitialLayout;
        native.final_layout = value.FinalLayout;
        return native;
    }

    internal static VulkanContextDescriptor CopyVulkanContextDescriptor(
        mln_vulkan_context_descriptor value
    ) =>
        new(
            NativePointer.FromNativeAddress((nint)value.instance),
            NativePointer.FromNativeAddress((nint)value.physical_device),
            NativePointer.FromNativeAddress((nint)value.device),
            NativePointer.FromNativeAddress((nint)value.graphics_queue),
            value.graphics_queue_family_index,
            NativePointer.FromNativeAddress((nint)value.get_instance_proc_addr),
            NativePointer.FromNativeAddress((nint)value.get_device_proc_addr)
        );

    internal static mln_vulkan_context_descriptor NativeVulkanContextDescriptor(
        VulkanContextDescriptor value
    )
    {
        var native = new mln_vulkan_context_descriptor();
        native.size = (uint)sizeof(mln_vulkan_context_descriptor);
        native.instance = (void*)value.Instance.Address;
        native.physical_device = (void*)value.PhysicalDevice.Address;
        native.device = (void*)value.Device.Address;
        native.graphics_queue = (void*)value.GraphicsQueue.Address;
        native.graphics_queue_family_index = value.GraphicsQueueFamilyIndex;
        native.get_instance_proc_addr = (void*)value.GetInstanceProcAddr.Address;
        native.get_device_proc_addr = (void*)value.GetDeviceProcAddr.Address;
        return native;
    }

    internal static VulkanOwnedTextureDescriptor CopyVulkanOwnedTextureDescriptor(
        mln_vulkan_owned_texture_descriptor value
    ) => new(CopyRenderTargetExtent(value.extent), CopyVulkanContextDescriptor(value.context));

    internal static mln_vulkan_owned_texture_descriptor NativeVulkanOwnedTextureDescriptor(
        VulkanOwnedTextureDescriptor value
    )
    {
        var native = NativeMethods.mln_vulkan_owned_texture_descriptor_default();
        native.size = (uint)sizeof(mln_vulkan_owned_texture_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.context = NativeVulkanContextDescriptor(value.Context);
        return native;
    }

    internal static VulkanOwnedTextureFrame CopyVulkanOwnedTextureFrame(
        mln_vulkan_owned_texture_frame value
    ) =>
        new(
            value.generation,
            value.width,
            value.height,
            value.scale_factor,
            value.frame_id,
            value.image,
            value.image_view,
            NativePointer.FromNativeAddress((nint)value.device),
            value.format,
            value.layout
        );

    internal static mln_vulkan_owned_texture_frame NativeVulkanOwnedTextureFrame(
        VulkanOwnedTextureFrame value
    )
    {
        var native = new mln_vulkan_owned_texture_frame();
        native.size = (uint)sizeof(mln_vulkan_owned_texture_frame);
        native.generation = value.Generation;
        native.width = value.Width;
        native.height = value.Height;
        native.scale_factor = value.ScaleFactor;
        native.frame_id = value.FrameId;
        native.image = value.Image;
        native.image_view = value.ImageView;
        native.device = (void*)value.Device.Address;
        native.format = value.Format;
        native.layout = value.Layout;
        return native;
    }

    internal static VulkanSurfaceDescriptor CopyVulkanSurfaceDescriptor(
        mln_vulkan_surface_descriptor value
    ) =>
        new(
            CopyRenderTargetExtent(value.extent),
            CopyVulkanContextDescriptor(value.context),
            value.surface
        );

    internal static mln_vulkan_surface_descriptor NativeVulkanSurfaceDescriptor(
        VulkanSurfaceDescriptor value
    )
    {
        var native = NativeMethods.mln_vulkan_surface_descriptor_default();
        native.size = (uint)sizeof(mln_vulkan_surface_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.context = NativeVulkanContextDescriptor(value.Context);
        native.surface = value.Surface;
        return native;
    }

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
        native.size = (uint)sizeof(mln_wake);
        if (value.Callback is not null)
        {
            native.user_data = scope.Register(value with { });
            native.release_user_data = &NativeCallbackRoot.Release;
            native.callback = value.Callback is null ? null : &InvokeWakeCallback;
        }
        return native;
    }

    internal static WebglContextDescriptor CopyWebglContextDescriptor(
        mln_webgl_context_descriptor value
    ) =>
        new(
            (WebglContextKind)value.kind,
            value.context,
            RuntimeStructs.CopyUtf8((sbyte*)value.canvas_selector.data, value.canvas_selector.size)
        );

    internal static mln_webgl_context_descriptor NativeWebglContextDescriptor(
        WebglContextDescriptor value,
        NativeCallScope scope
    )
    {
        Required(value.CanvasSelector, "WebglContextDescriptor.CanvasSelector must not be null.");
        var native = new mln_webgl_context_descriptor();
        native.size = (uint)sizeof(mln_webgl_context_descriptor);
        native.kind = (uint)value.Kind;
        native.context = value.Context;
        native.canvas_selector = scope.Utf8(value.CanvasSelector);
        return native;
    }

    internal static WebgpuBorrowedTextureDescriptor CopyWebgpuBorrowedTextureDescriptor(
        mln_webgpu_borrowed_texture_descriptor value
    ) =>
        new(
            CopyRenderTargetExtent(value.extent),
            value.physical_width,
            value.physical_height,
            CopyWebgpuContextDescriptor(value.context),
            NativePointer.FromNativeAddress((nint)value.texture),
            NativePointer.FromNativeAddress((nint)value.texture_view),
            value.format
        );

    internal static mln_webgpu_borrowed_texture_descriptor NativeWebgpuBorrowedTextureDescriptor(
        WebgpuBorrowedTextureDescriptor value
    )
    {
        var native = NativeMethods.mln_webgpu_borrowed_texture_descriptor_default();
        native.size = (uint)sizeof(mln_webgpu_borrowed_texture_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.physical_width = value.PhysicalWidth;
        native.physical_height = value.PhysicalHeight;
        native.context = NativeWebgpuContextDescriptor(value.Context);
        native.texture = (void*)value.Texture.Address;
        native.texture_view = (void*)value.TextureView.Address;
        native.format = value.Format;
        return native;
    }

    internal static WebgpuContextDescriptor CopyWebgpuContextDescriptor(
        mln_webgpu_context_descriptor value
    ) =>
        new(
            NativePointer.FromNativeAddress((nint)value.instance),
            NativePointer.FromNativeAddress((nint)value.device),
            NativePointer.FromNativeAddress((nint)value.queue)
        );

    internal static mln_webgpu_context_descriptor NativeWebgpuContextDescriptor(
        WebgpuContextDescriptor value
    )
    {
        var native = new mln_webgpu_context_descriptor();
        native.size = (uint)sizeof(mln_webgpu_context_descriptor);
        native.instance = (void*)value.Instance.Address;
        native.device = (void*)value.Device.Address;
        native.queue = (void*)value.Queue.Address;
        return native;
    }

    internal static WebgpuOwnedTextureDescriptor CopyWebgpuOwnedTextureDescriptor(
        mln_webgpu_owned_texture_descriptor value
    ) => new(CopyRenderTargetExtent(value.extent), CopyWebgpuContextDescriptor(value.context));

    internal static mln_webgpu_owned_texture_descriptor NativeWebgpuOwnedTextureDescriptor(
        WebgpuOwnedTextureDescriptor value
    )
    {
        var native = NativeMethods.mln_webgpu_owned_texture_descriptor_default();
        native.size = (uint)sizeof(mln_webgpu_owned_texture_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.context = NativeWebgpuContextDescriptor(value.Context);
        return native;
    }

    internal static WebgpuOwnedTextureFrame CopyWebgpuOwnedTextureFrame(
        mln_webgpu_owned_texture_frame value
    ) =>
        new(
            value.generation,
            value.width,
            value.height,
            value.scale_factor,
            value.frame_id,
            NativePointer.FromNativeAddress((nint)value.texture),
            NativePointer.FromNativeAddress((nint)value.texture_view),
            NativePointer.FromNativeAddress((nint)value.device),
            value.format
        );

    internal static mln_webgpu_owned_texture_frame NativeWebgpuOwnedTextureFrame(
        WebgpuOwnedTextureFrame value
    )
    {
        var native = new mln_webgpu_owned_texture_frame();
        native.size = (uint)sizeof(mln_webgpu_owned_texture_frame);
        native.generation = value.Generation;
        native.width = value.Width;
        native.height = value.Height;
        native.scale_factor = value.ScaleFactor;
        native.frame_id = value.FrameId;
        native.texture = (void*)value.Texture.Address;
        native.texture_view = (void*)value.TextureView.Address;
        native.device = (void*)value.Device.Address;
        native.format = value.Format;
        return native;
    }

    internal static WebgpuSurfaceDescriptor CopyWebgpuSurfaceDescriptor(
        mln_webgpu_surface_descriptor value
    ) =>
        new(
            CopyRenderTargetExtent(value.extent),
            CopyWebgpuContextDescriptor(value.context),
            NativePointer.FromNativeAddress((nint)value.surface),
            value.format
        );

    internal static mln_webgpu_surface_descriptor NativeWebgpuSurfaceDescriptor(
        WebgpuSurfaceDescriptor value
    )
    {
        var native = NativeMethods.mln_webgpu_surface_descriptor_default();
        native.size = (uint)sizeof(mln_webgpu_surface_descriptor);
        native.extent = NativeRenderTargetExtent(value.Extent);
        native.context = NativeWebgpuContextDescriptor(value.Context);
        native.surface = (void*)value.Surface.Address;
        native.format = value.Format;
        return native;
    }

    internal static WglContextDescriptor CopyWglContextDescriptor(
        mln_wgl_context_descriptor value
    ) =>
        new(
            NativePointer.FromNativeAddress((nint)value.device_context),
            NativePointer.FromNativeAddress((nint)value.share_context),
            NativePointer.FromNativeAddress((nint)value.get_proc_address)
        );

    internal static mln_wgl_context_descriptor NativeWglContextDescriptor(
        WglContextDescriptor value
    )
    {
        var native = new mln_wgl_context_descriptor();
        native.size = (uint)sizeof(mln_wgl_context_descriptor);
        native.device_context = (void*)value.DeviceContext.Address;
        native.share_context = (void*)value.ShareContext.Address;
        native.get_proc_address = (void*)value.GetProcAddress.Address;
        return native;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    internal static uint InvokeLogCallback(
        void* user_data,
        uint severity,
        uint @event,
        long code,
        sbyte* message
    )
    {
        try
        {
            using var restriction = NativeCallbackGuard.ForbidReentry();
            return (
                (Func<LogSeverity, LogEvent, long, string, uint>)NativeCallbackRoot.Value(user_data)
            )((LogSeverity)severity, (LogEvent)@event, code, NativeCallScope.CopyCString(message));
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_log_callback", error);
            return 0;
        }
    }

    private static readonly string[] AllowedResourceRequestCancelCallback =
    [
        "mln_resource_request_complete",
        "mln_resource_request_cancelled",
        "mln_resource_request_set_cancel_callback",
        "mln_resource_request_release",
    ];

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    internal static void InvokeResourceRequestCancelCallback(void* user_data)
    {
        try
        {
            var owned = (NativeOwnedCallback)NativeCallbackRoot.Value(user_data);
            using var restriction = NativeCallbackGuard.Restrict(
                owned.Owner,
                AllowedResourceRequestCancelCallback
            );
            ((Action)owned.Callback)();
        }
        catch (Exception error)
        {
            NativeCallbackFailure.Report("mln_resource_request_cancel_callback", error);
        }
    }
}
