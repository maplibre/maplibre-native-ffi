package org.maplibre.nativeffi.internal.convert;

import java.util.EnumSet;
import org.maplibre.nativeffi.error.InvalidArgumentException;
import org.maplibre.nativeffi.error.InvalidStateException;
import org.maplibre.nativeffi.error.MaplibreException;
import org.maplibre.nativeffi.error.MaplibreStatus;
import org.maplibre.nativeffi.error.NativeErrorException;
import org.maplibre.nativeffi.error.UnsupportedFeatureException;
import org.maplibre.nativeffi.error.WrongThreadException;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.log.LogEvent;
import org.maplibre.nativeffi.log.LogSeverity;
import org.maplibre.nativeffi.map.ConstrainMode;
import org.maplibre.nativeffi.map.DebugOption;
import org.maplibre.nativeffi.map.MapMode;
import org.maplibre.nativeffi.map.NorthOrientation;
import org.maplibre.nativeffi.map.TileLodMode;
import org.maplibre.nativeffi.map.TileOperation;
import org.maplibre.nativeffi.map.ViewportMode;
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState;
import org.maplibre.nativeffi.render.OpenGLContextProvider;
import org.maplibre.nativeffi.render.RenderBackend;
import org.maplibre.nativeffi.render.RenderMode;
import org.maplibre.nativeffi.resource.ResourceErrorReason;
import org.maplibre.nativeffi.resource.ResourceKind;
import org.maplibre.nativeffi.resource.ResourceLoadingMethod;
import org.maplibre.nativeffi.resource.ResourcePriority;
import org.maplibre.nativeffi.resource.ResourceProviderDecision;
import org.maplibre.nativeffi.resource.ResourceResponseStatus;
import org.maplibre.nativeffi.resource.ResourceStoragePolicy;
import org.maplibre.nativeffi.resource.ResourceUsage;
import org.maplibre.nativeffi.runtime.AmbientCacheOperation;
import org.maplibre.nativeffi.runtime.NetworkStatus;
import org.maplibre.nativeffi.runtime.OfflineOperationKind;
import org.maplibre.nativeffi.runtime.OfflineOperationResultKind;
import org.maplibre.nativeffi.runtime.RuntimeEventSourceType;
import org.maplibre.nativeffi.runtime.RuntimeEventType;
import org.maplibre.nativeffi.style.LocationIndicatorImageKind;
import org.maplibre.nativeffi.style.RasterDemEncoding;
import org.maplibre.nativeffi.style.SourceType;
import org.maplibre.nativeffi.style.TileScheme;
import org.maplibre.nativeffi.style.VectorTileEncoding;

/** Internal C enum and mask conversions for the public semantic value types. */
public final class NativeValues {
  private NativeValues() {}

  public static MaplibreStatus maplibreStatus(int nativeCode) {
    return switch (nativeCode) {
      case 0 -> MaplibreStatus.OK;
      case -1 -> MaplibreStatus.INVALID_ARGUMENT;
      case -2 -> MaplibreStatus.INVALID_STATE;
      case -3 -> MaplibreStatus.WRONG_THREAD;
      case -4 -> MaplibreStatus.UNSUPPORTED;
      case -5 -> MaplibreStatus.NATIVE_ERROR;
      default -> MaplibreStatus.UNKNOWN;
    };
  }

  public static int nativeCode(MaplibreStatus status) {
    return switch (status) {
      case OK -> 0;
      case INVALID_ARGUMENT -> -1;
      case INVALID_STATE -> -2;
      case WRONG_THREAD -> -3;
      case UNSUPPORTED -> -4;
      case NATIVE_ERROR -> -5;
      case UNKNOWN -> Integer.MIN_VALUE;
    };
  }

  public static MaplibreException exceptionForStatus(
      MaplibreStatus status, int nativeStatusCode, String diagnostic) {
    return switch (status) {
      case INVALID_ARGUMENT -> new InvalidArgumentException(nativeStatusCode, diagnostic);
      case INVALID_STATE -> new InvalidStateException(nativeStatusCode, diagnostic);
      case WRONG_THREAD -> new WrongThreadException(nativeStatusCode, diagnostic);
      case UNSUPPORTED -> new UnsupportedFeatureException(nativeStatusCode, diagnostic);
      case NATIVE_ERROR -> new NativeErrorException(nativeStatusCode, diagnostic);
      case OK, UNKNOWN -> new MaplibreException(status, nativeStatusCode, diagnostic);
    };
  }

  public static NetworkStatus networkStatus(int nativeValue) {
    return new NetworkStatus(nativeValue);
  }

  public static int nativeValue(NetworkStatus status) {
    if (status.equals(NetworkStatus.ONLINE)) {
      return MapLibreNativeC.MLN_NETWORK_STATUS_ONLINE();
    }
    if (status.equals(NetworkStatus.OFFLINE)) {
      return MapLibreNativeC.MLN_NETWORK_STATUS_OFFLINE();
    }
    throw new InvalidArgumentException(
        0,
        "Unknown network status value cannot be set: " + Integer.toUnsignedLong(status.rawValue()));
  }

  public static EnumSet<RenderBackend> renderBackendsFromMask(int mask) {
    var backends = EnumSet.noneOf(RenderBackend.class);
    if ((mask & 1) != 0) {
      backends.add(RenderBackend.METAL);
    }
    if ((mask & (1 << 1)) != 0) {
      backends.add(RenderBackend.VULKAN);
    }
    if ((mask & (1 << 2)) != 0) {
      backends.add(RenderBackend.OPENGL);
    }
    return backends;
  }

  public static EnumSet<OpenGLContextProvider> openGLContextProvidersFromMask(int mask) {
    var providers = EnumSet.noneOf(OpenGLContextProvider.class);
    if ((mask & 1) != 0) {
      providers.add(OpenGLContextProvider.WGL);
    }
    if ((mask & (1 << 1)) != 0) {
      providers.add(OpenGLContextProvider.EGL);
    }
    return providers;
  }

  public static int nativeMask(LogSeverity severity) {
    return severity.nativeMask();
  }

  public static int nativeMask(DebugOption option) {
    return switch (option) {
      case TILE_BORDERS -> MapLibreNativeC.MLN_MAP_DEBUG_TILE_BORDERS();
      case PARSE_STATUS -> MapLibreNativeC.MLN_MAP_DEBUG_PARSE_STATUS();
      case TIMESTAMPS -> MapLibreNativeC.MLN_MAP_DEBUG_TIMESTAMPS();
      case COLLISION -> MapLibreNativeC.MLN_MAP_DEBUG_COLLISION();
      case OVERDRAW -> MapLibreNativeC.MLN_MAP_DEBUG_OVERDRAW();
      case STENCIL_CLIP -> MapLibreNativeC.MLN_MAP_DEBUG_STENCIL_CLIP();
      case DEPTH_BUFFER -> MapLibreNativeC.MLN_MAP_DEBUG_DEPTH_BUFFER();
    };
  }

  public static int nativeValue(AmbientCacheOperation operation) {
    return switch (operation) {
      case RESET_DATABASE -> MapLibreNativeC.MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE();
      case PACK_DATABASE -> MapLibreNativeC.MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE();
      case INVALIDATE -> MapLibreNativeC.MLN_AMBIENT_CACHE_OPERATION_INVALIDATE();
      case CLEAR -> MapLibreNativeC.MLN_AMBIENT_CACHE_OPERATION_CLEAR();
    };
  }

  public static int nativeValue(ConstrainMode mode) {
    if (mode.equals(ConstrainMode.NONE)) {
      return MapLibreNativeC.MLN_CONSTRAIN_MODE_NONE();
    }
    if (mode.equals(ConstrainMode.HEIGHT_ONLY)) {
      return MapLibreNativeC.MLN_CONSTRAIN_MODE_HEIGHT_ONLY();
    }
    if (mode.equals(ConstrainMode.WIDTH_AND_HEIGHT)) {
      return MapLibreNativeC.MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT();
    }
    if (mode.equals(ConstrainMode.SCREEN)) {
      return MapLibreNativeC.MLN_CONSTRAIN_MODE_SCREEN();
    }
    throw new InvalidArgumentException(
        0, "Unknown constrain mode value cannot be used as an input: " + mode.rawValue());
  }

  public static int nativeValue(LocationIndicatorImageKind kind) {
    return switch (kind) {
      case TOP -> MapLibreNativeC.MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP();
      case BEARING -> MapLibreNativeC.MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING();
      case SHADOW -> MapLibreNativeC.MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW();
    };
  }

  public static int nativeValue(LogEvent event) {
    return event.nativeValue();
  }

  public static int nativeValue(LogSeverity severity) {
    return severity.nativeValue();
  }

  public static int nativeValue(MapMode mode) {
    return switch (mode) {
      case CONTINUOUS -> MapLibreNativeC.MLN_MAP_MODE_CONTINUOUS();
      case STATIC -> MapLibreNativeC.MLN_MAP_MODE_STATIC();
      case TILE -> MapLibreNativeC.MLN_MAP_MODE_TILE();
    };
  }

  public static int nativeValue(NorthOrientation orientation) {
    if (orientation.equals(NorthOrientation.UP)) {
      return MapLibreNativeC.MLN_NORTH_ORIENTATION_UP();
    }
    if (orientation.equals(NorthOrientation.RIGHT)) {
      return MapLibreNativeC.MLN_NORTH_ORIENTATION_RIGHT();
    }
    if (orientation.equals(NorthOrientation.DOWN)) {
      return MapLibreNativeC.MLN_NORTH_ORIENTATION_DOWN();
    }
    if (orientation.equals(NorthOrientation.LEFT)) {
      return MapLibreNativeC.MLN_NORTH_ORIENTATION_LEFT();
    }
    throw new InvalidArgumentException(
        0, "Unknown north orientation value cannot be used as an input: " + orientation.rawValue());
  }

  public static int nativeValue(OfflineOperationKind kind) {
    return kind.nativeValue();
  }

  public static int nativeValue(OfflineOperationResultKind kind) {
    return kind.nativeValue();
  }

  public static int nativeValue(OfflineRegionDownloadState state) {
    if (state.equals(OfflineRegionDownloadState.INACTIVE)) {
      return MapLibreNativeC.MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE();
    }
    if (state.equals(OfflineRegionDownloadState.ACTIVE)) {
      return MapLibreNativeC.MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE();
    }
    throw new InvalidArgumentException(
        0, "Unknown offline region download state cannot be used as an input: " + state.rawValue());
  }

  public static int nativeValue(RasterDemEncoding encoding) {
    return switch (encoding) {
      case MAPBOX -> MapLibreNativeC.MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX();
      case TERRARIUM -> MapLibreNativeC.MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM();
    };
  }

  public static int nativeValue(ResourceErrorReason reason) {
    if (reason.equals(ResourceErrorReason.NONE)) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_NONE();
    }
    if (reason.equals(ResourceErrorReason.NOT_FOUND)) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_NOT_FOUND();
    }
    if (reason.equals(ResourceErrorReason.SERVER)) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_SERVER();
    }
    if (reason.equals(ResourceErrorReason.CONNECTION)) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_CONNECTION();
    }
    if (reason.equals(ResourceErrorReason.RATE_LIMIT)) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_RATE_LIMIT();
    }
    if (reason.equals(ResourceErrorReason.OTHER)) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_OTHER();
    }
    throw new InvalidArgumentException(
        0, "Unknown resource error reason cannot be used as an input: " + reason.rawValue());
  }

  public static int nativeValue(ResourceKind kind) {
    return kind.nativeValue();
  }

  public static int nativeValue(ResourceProviderDecision decision) {
    return switch (decision) {
      case PASS_THROUGH -> MapLibreNativeC.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH();
      case HANDLE -> MapLibreNativeC.MLN_RESOURCE_PROVIDER_DECISION_HANDLE();
    };
  }

  public static int nativeValue(ResourceResponseStatus status) {
    return switch (status) {
      case OK -> MapLibreNativeC.MLN_RESOURCE_RESPONSE_STATUS_OK();
      case ERROR -> MapLibreNativeC.MLN_RESOURCE_RESPONSE_STATUS_ERROR();
      case NO_CONTENT -> MapLibreNativeC.MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT();
      case NOT_MODIFIED -> MapLibreNativeC.MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED();
    };
  }

  public static int nativeValue(TileLodMode mode) {
    if (mode.equals(TileLodMode.DEFAULT)) {
      return MapLibreNativeC.MLN_TILE_LOD_MODE_DEFAULT();
    }
    if (mode.equals(TileLodMode.DISTANCE)) {
      return MapLibreNativeC.MLN_TILE_LOD_MODE_DISTANCE();
    }
    throw new InvalidArgumentException(
        0, "Unknown tile LOD mode value cannot be used as an input: " + mode.rawValue());
  }

  public static int nativeValue(TileScheme scheme) {
    return switch (scheme) {
      case XYZ -> MapLibreNativeC.MLN_STYLE_TILE_SCHEME_XYZ();
      case TMS -> MapLibreNativeC.MLN_STYLE_TILE_SCHEME_TMS();
    };
  }

  public static int nativeValue(VectorTileEncoding encoding) {
    return switch (encoding) {
      case MVT -> MapLibreNativeC.MLN_STYLE_VECTOR_TILE_ENCODING_MVT();
      case MLT -> MapLibreNativeC.MLN_STYLE_VECTOR_TILE_ENCODING_MLT();
    };
  }

  public static int nativeValue(ViewportMode mode) {
    if (mode.equals(ViewportMode.DEFAULT)) {
      return MapLibreNativeC.MLN_VIEWPORT_MODE_DEFAULT();
    }
    if (mode.equals(ViewportMode.FLIPPED_Y)) {
      return MapLibreNativeC.MLN_VIEWPORT_MODE_FLIPPED_Y();
    }
    throw new InvalidArgumentException(
        0, "Unknown viewport mode value cannot be used as an input: " + mode.rawValue());
  }

  public static OfflineRegionDownloadState offlineRegionDownloadState(int nativeValue) {
    return new OfflineRegionDownloadState(nativeValue);
  }

  public static RenderMode renderMode(int nativeValue) {
    return RenderMode.fromNative(nativeValue);
  }

  public static ResourceKind resourceKind(int nativeValue) {
    return ResourceKind.fromNative(nativeValue);
  }

  public static ResourceErrorReason resourceErrorReason(int nativeValue) {
    return new ResourceErrorReason(nativeValue);
  }

  public static ResourceLoadingMethod resourceLoadingMethod(int nativeValue) {
    return ResourceLoadingMethod.fromNative(nativeValue);
  }

  public static ResourcePriority resourcePriority(int nativeValue) {
    return ResourcePriority.fromNative(nativeValue);
  }

  public static ResourceStoragePolicy resourceStoragePolicy(int nativeValue) {
    return ResourceStoragePolicy.fromNative(nativeValue);
  }

  public static ResourceUsage resourceUsage(int nativeValue) {
    return ResourceUsage.fromNative(nativeValue);
  }

  public static RuntimeEventSourceType runtimeEventSourceType(int nativeValue) {
    return RuntimeEventSourceType.fromNative(nativeValue);
  }

  public static RuntimeEventType runtimeEventType(int nativeValue) {
    return RuntimeEventType.fromNative(nativeValue);
  }

  public static int nativeValue(RuntimeEventType type) {
    return type.rawValue();
  }

  public static SourceType sourceType(int nativeValue) {
    return new SourceType(nativeValue);
  }

  public static TileOperation tileOperation(int nativeValue) {
    return TileOperation.fromNative(nativeValue);
  }

  public static int nativeValue(TileOperation operation) {
    return operation.rawValue();
  }

  public static NorthOrientation northOrientation(int nativeValue) {
    return new NorthOrientation(nativeValue);
  }

  public static ConstrainMode constrainMode(int nativeValue) {
    return new ConstrainMode(nativeValue);
  }

  public static TileLodMode tileLodMode(int nativeValue) {
    return new TileLodMode(nativeValue);
  }

  public static ViewportMode viewportMode(int nativeValue) {
    return new ViewportMode(nativeValue);
  }

  public static LogEvent logEvent(int nativeValue) {
    return LogEvent.fromNative(nativeValue);
  }

  public static LogSeverity logSeverity(int nativeValue) {
    return LogSeverity.fromNative(nativeValue);
  }

  public static OfflineOperationKind offlineOperationKind(int nativeValue) {
    return OfflineOperationKind.fromNative(nativeValue);
  }

  public static OfflineOperationResultKind offlineOperationResultKind(int nativeValue) {
    return OfflineOperationResultKind.fromNative(nativeValue);
  }
}
