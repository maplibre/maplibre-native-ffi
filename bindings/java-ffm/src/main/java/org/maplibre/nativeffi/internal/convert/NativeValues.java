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
    return switch (status.rawValue()) {
      case 1 -> MapLibreNativeC.MLN_NETWORK_STATUS_ONLINE();
      case 2 -> MapLibreNativeC.MLN_NETWORK_STATUS_OFFLINE();
      default ->
          throw new InvalidArgumentException(
              0,
              "Unknown network status value cannot be set: "
                  + Integer.toUnsignedLong(status.rawValue()));
    };
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
    if (severity == LogSeverity.UNKNOWN) {
      throw new IllegalArgumentException("UNKNOWN log severity cannot be used as an input");
    }
    return 1 << nativeValue(severity);
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
    if (mode == ConstrainMode.NONE) {
      return MapLibreNativeC.MLN_CONSTRAIN_MODE_NONE();
    }
    if (mode == ConstrainMode.HEIGHT_ONLY) {
      return MapLibreNativeC.MLN_CONSTRAIN_MODE_HEIGHT_ONLY();
    }
    if (mode == ConstrainMode.WIDTH_AND_HEIGHT) {
      return MapLibreNativeC.MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT();
    }
    if (mode == ConstrainMode.SCREEN) {
      return MapLibreNativeC.MLN_CONSTRAIN_MODE_SCREEN();
    }
    throw new InvalidArgumentException(
        0, "Unknown constrain mode value cannot be used as an input: " + mode.rawValue());
  }

  public static int nativeValue(LocationIndicatorImageKind kind) {
    return switch (kind) {
      case TOP -> 0;
      case BEARING -> 1;
      case SHADOW -> 2;
    };
  }

  public static int nativeValue(LogEvent event) {
    return switch (event) {
      case GENERAL -> 0;
      case SETUP -> 1;
      case SHADER -> 2;
      case PARSE_STYLE -> 3;
      case PARSE_TILE -> 4;
      case RENDER -> 5;
      case STYLE -> 6;
      case DATABASE -> 7;
      case HTTP_REQUEST -> 8;
      case SPRITE -> 9;
      case IMAGE -> 10;
      case OPENGL -> 11;
      case JNI -> 12;
      case ANDROID -> 13;
      case CRASH -> 14;
      case GLYPH -> 15;
      case TIMING -> 16;
      case UNKNOWN -> -1;
    };
  }

  public static int nativeValue(LogSeverity severity) {
    return switch (severity) {
      case INFO -> 1;
      case WARNING -> 2;
      case ERROR -> 3;
      case UNKNOWN -> -1;
    };
  }

  public static int nativeValue(MapMode mode) {
    return switch (mode) {
      case CONTINUOUS -> MapLibreNativeC.MLN_MAP_MODE_CONTINUOUS();
      case STATIC -> MapLibreNativeC.MLN_MAP_MODE_STATIC();
      case TILE -> MapLibreNativeC.MLN_MAP_MODE_TILE();
    };
  }

  public static int nativeValue(NorthOrientation orientation) {
    if (orientation == NorthOrientation.UP) {
      return MapLibreNativeC.MLN_NORTH_ORIENTATION_UP();
    }
    if (orientation == NorthOrientation.RIGHT) {
      return MapLibreNativeC.MLN_NORTH_ORIENTATION_RIGHT();
    }
    if (orientation == NorthOrientation.DOWN) {
      return MapLibreNativeC.MLN_NORTH_ORIENTATION_DOWN();
    }
    if (orientation == NorthOrientation.LEFT) {
      return MapLibreNativeC.MLN_NORTH_ORIENTATION_LEFT();
    }
    throw new InvalidArgumentException(
        0, "Unknown north orientation value cannot be used as an input: " + orientation.rawValue());
  }

  public static int nativeValue(OfflineOperationKind kind) {
    return switch (kind) {
      case AMBIENT_CACHE -> MapLibreNativeC.MLN_OFFLINE_OPERATION_AMBIENT_CACHE();
      case REGION_CREATE -> MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_CREATE();
      case REGION_GET -> MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_GET();
      case REGIONS_LIST -> MapLibreNativeC.MLN_OFFLINE_OPERATION_REGIONS_LIST();
      case REGIONS_MERGE_DATABASE -> MapLibreNativeC.MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE();
      case REGION_UPDATE_METADATA -> MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA();
      case REGION_GET_STATUS -> MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_GET_STATUS();
      case REGION_SET_OBSERVED -> MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED();
      case REGION_SET_DOWNLOAD_STATE ->
          MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE();
      case REGION_INVALIDATE -> MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_INVALIDATE();
      case REGION_DELETE -> MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_DELETE();
      case UNKNOWN -> -1;
    };
  }

  public static int nativeValue(OfflineOperationResultKind kind) {
    return switch (kind) {
      case NONE -> MapLibreNativeC.MLN_OFFLINE_OPERATION_RESULT_NONE();
      case REGION -> MapLibreNativeC.MLN_OFFLINE_OPERATION_RESULT_REGION();
      case OPTIONAL_REGION -> MapLibreNativeC.MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION();
      case REGION_LIST -> MapLibreNativeC.MLN_OFFLINE_OPERATION_RESULT_REGION_LIST();
      case REGION_STATUS -> MapLibreNativeC.MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS();
      case UNKNOWN -> -1;
    };
  }

  public static int nativeValue(OfflineRegionDownloadState state) {
    if (state == OfflineRegionDownloadState.INACTIVE) {
      return MapLibreNativeC.MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE();
    }
    if (state == OfflineRegionDownloadState.ACTIVE) {
      return MapLibreNativeC.MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE();
    }
    throw new InvalidArgumentException(
        0, "Unknown offline region download state cannot be used as an input: " + state.rawValue());
  }

  public static int nativeValue(RasterDemEncoding encoding) {
    return switch (encoding) {
      case MAPBOX -> 0;
      case TERRARIUM -> 1;
    };
  }

  public static int nativeValue(ResourceErrorReason reason) {
    if (reason == ResourceErrorReason.NONE) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_NONE();
    }
    if (reason == ResourceErrorReason.NOT_FOUND) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_NOT_FOUND();
    }
    if (reason == ResourceErrorReason.SERVER) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_SERVER();
    }
    if (reason == ResourceErrorReason.CONNECTION) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_CONNECTION();
    }
    if (reason == ResourceErrorReason.RATE_LIMIT) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_RATE_LIMIT();
    }
    if (reason == ResourceErrorReason.OTHER) {
      return MapLibreNativeC.MLN_RESOURCE_ERROR_REASON_OTHER();
    }
    throw new InvalidArgumentException(
        0, "Unknown resource error reason cannot be used as an input: " + reason.rawValue());
  }

  public static int nativeValue(ResourceKind kind) {
    return switch (kind) {
      case UNKNOWN -> MapLibreNativeC.MLN_RESOURCE_KIND_UNKNOWN();
      case STYLE -> MapLibreNativeC.MLN_RESOURCE_KIND_STYLE();
      case SOURCE -> MapLibreNativeC.MLN_RESOURCE_KIND_SOURCE();
      case TILE -> MapLibreNativeC.MLN_RESOURCE_KIND_TILE();
      case GLYPHS -> MapLibreNativeC.MLN_RESOURCE_KIND_GLYPHS();
      case SPRITE_IMAGE -> MapLibreNativeC.MLN_RESOURCE_KIND_SPRITE_IMAGE();
      case SPRITE_JSON -> MapLibreNativeC.MLN_RESOURCE_KIND_SPRITE_JSON();
      case IMAGE -> MapLibreNativeC.MLN_RESOURCE_KIND_IMAGE();
    };
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
    if (mode == TileLodMode.DEFAULT) {
      return MapLibreNativeC.MLN_TILE_LOD_MODE_DEFAULT();
    }
    if (mode == TileLodMode.DISTANCE) {
      return MapLibreNativeC.MLN_TILE_LOD_MODE_DISTANCE();
    }
    throw new InvalidArgumentException(
        0, "Unknown tile LOD mode value cannot be used as an input: " + mode.rawValue());
  }

  public static int nativeValue(TileScheme scheme) {
    return switch (scheme) {
      case XYZ -> 0;
      case TMS -> 1;
    };
  }

  public static int nativeValue(VectorTileEncoding encoding) {
    return switch (encoding) {
      case MVT -> 0;
      case MLT -> 1;
    };
  }

  public static int nativeValue(ViewportMode mode) {
    if (mode == ViewportMode.DEFAULT) {
      return MapLibreNativeC.MLN_VIEWPORT_MODE_DEFAULT();
    }
    if (mode == ViewportMode.FLIPPED_Y) {
      return MapLibreNativeC.MLN_VIEWPORT_MODE_FLIPPED_Y();
    }
    throw new InvalidArgumentException(
        0, "Unknown viewport mode value cannot be used as an input: " + mode.rawValue());
  }

  public static OfflineRegionDownloadState offlineRegionDownloadState(int nativeValue) {
    return new OfflineRegionDownloadState(nativeValue);
  }

  public static RenderMode renderMode(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> RenderMode.PARTIAL;
      case 1 -> RenderMode.FULL;
      default -> RenderMode.UNKNOWN;
    };
  }

  public static ResourceKind resourceKind(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> ResourceKind.UNKNOWN;
      case 1 -> ResourceKind.STYLE;
      case 2 -> ResourceKind.SOURCE;
      case 3 -> ResourceKind.TILE;
      case 4 -> ResourceKind.GLYPHS;
      case 5 -> ResourceKind.SPRITE_IMAGE;
      case 6 -> ResourceKind.SPRITE_JSON;
      case 7 -> ResourceKind.IMAGE;
      default -> ResourceKind.UNKNOWN;
    };
  }

  public static ResourceErrorReason resourceErrorReason(int nativeValue) {
    return new ResourceErrorReason(nativeValue);
  }

  public static ResourceLoadingMethod resourceLoadingMethod(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> ResourceLoadingMethod.ALL;
      case 1 -> ResourceLoadingMethod.CACHE_ONLY;
      case 2 -> ResourceLoadingMethod.NETWORK_ONLY;
      default -> ResourceLoadingMethod.UNKNOWN;
    };
  }

  public static ResourcePriority resourcePriority(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> ResourcePriority.REGULAR;
      case 1 -> ResourcePriority.LOW;
      default -> ResourcePriority.UNKNOWN;
    };
  }

  public static ResourceStoragePolicy resourceStoragePolicy(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> ResourceStoragePolicy.PERMANENT;
      case 1 -> ResourceStoragePolicy.VOLATILE;
      default -> ResourceStoragePolicy.UNKNOWN;
    };
  }

  public static ResourceUsage resourceUsage(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> ResourceUsage.ONLINE;
      case 1 -> ResourceUsage.OFFLINE;
      default -> ResourceUsage.UNKNOWN;
    };
  }

  public static RuntimeEventSourceType runtimeEventSourceType(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> RuntimeEventSourceType.RUNTIME;
      case 1 -> RuntimeEventSourceType.MAP;
      default -> RuntimeEventSourceType.UNKNOWN;
    };
  }

  public static RuntimeEventType runtimeEventType(int nativeValue) {
    for (var type : RuntimeEventType.values()) {
      if (nativeValue(type) == nativeValue) {
        return type;
      }
    }
    return RuntimeEventType.UNKNOWN;
  }

  public static int nativeValue(RuntimeEventType type) {
    return switch (type) {
      case MAP_CAMERA_WILL_CHANGE -> 1;
      case MAP_CAMERA_IS_CHANGING -> 2;
      case MAP_CAMERA_DID_CHANGE -> 3;
      case MAP_STYLE_LOADED -> 4;
      case MAP_LOADING_STARTED -> 5;
      case MAP_LOADING_FINISHED -> 6;
      case MAP_LOADING_FAILED -> 7;
      case MAP_IDLE -> 8;
      case MAP_RENDER_UPDATE_AVAILABLE -> 9;
      case MAP_RENDER_ERROR -> 10;
      case MAP_STILL_IMAGE_FINISHED -> 11;
      case MAP_STILL_IMAGE_FAILED -> 12;
      case MAP_RENDER_FRAME_STARTED -> 13;
      case MAP_RENDER_FRAME_FINISHED -> 14;
      case MAP_RENDER_MAP_STARTED -> 15;
      case MAP_RENDER_MAP_FINISHED -> 16;
      case MAP_STYLE_IMAGE_MISSING -> 17;
      case MAP_TILE_ACTION -> 18;
      case OFFLINE_REGION_STATUS_CHANGED -> 19;
      case OFFLINE_REGION_RESPONSE_ERROR -> 20;
      case OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED -> 21;
      case OFFLINE_OPERATION_COMPLETED -> 22;
      case UNKNOWN -> -1;
    };
  }

  public static SourceType sourceType(int nativeValue) {
    return new SourceType(nativeValue);
  }

  public static TileOperation tileOperation(int nativeValue) {
    for (var operation : TileOperation.values()) {
      if (nativeValue(operation) == nativeValue) {
        return operation;
      }
    }
    return TileOperation.UNKNOWN;
  }

  public static int nativeValue(TileOperation operation) {
    return switch (operation) {
      case REQUESTED_FROM_CACHE -> 0;
      case REQUESTED_FROM_NETWORK -> 1;
      case LOAD_FROM_NETWORK -> 2;
      case LOAD_FROM_CACHE -> 3;
      case START_PARSE -> 4;
      case END_PARSE -> 5;
      case ERROR -> 6;
      case CANCELLED -> 7;
      case NULL_OPERATION -> 8;
      case UNKNOWN -> -1;
    };
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
    for (var event : LogEvent.values()) {
      if (nativeValue(event) == nativeValue) {
        return event;
      }
    }
    return LogEvent.UNKNOWN;
  }

  public static LogSeverity logSeverity(int nativeValue) {
    return switch (nativeValue) {
      case 1 -> LogSeverity.INFO;
      case 2 -> LogSeverity.WARNING;
      case 3 -> LogSeverity.ERROR;
      default -> LogSeverity.UNKNOWN;
    };
  }

  public static OfflineOperationKind offlineOperationKind(int nativeValue) {
    for (var kind : OfflineOperationKind.values()) {
      if (nativeValue(kind) == nativeValue) {
        return kind;
      }
    }
    return OfflineOperationKind.UNKNOWN;
  }

  public static OfflineOperationResultKind offlineOperationResultKind(int nativeValue) {
    for (var kind : OfflineOperationResultKind.values()) {
      if (nativeValue(kind) == nativeValue) {
        return kind;
      }
    }
    return OfflineOperationResultKind.UNKNOWN;
  }
}
