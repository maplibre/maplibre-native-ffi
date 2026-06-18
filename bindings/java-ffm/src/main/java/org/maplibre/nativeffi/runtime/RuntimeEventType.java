package org.maplibre.nativeffi.runtime;

/** Runtime event type copied from the native event queue. */
public final class RuntimeEventType {
  public static final RuntimeEventType MAP_CAMERA_WILL_CHANGE =
      new RuntimeEventType(1, "MAP_CAMERA_WILL_CHANGE");
  public static final RuntimeEventType MAP_CAMERA_IS_CHANGING =
      new RuntimeEventType(2, "MAP_CAMERA_IS_CHANGING");
  public static final RuntimeEventType MAP_CAMERA_DID_CHANGE =
      new RuntimeEventType(3, "MAP_CAMERA_DID_CHANGE");
  public static final RuntimeEventType MAP_STYLE_LOADED =
      new RuntimeEventType(4, "MAP_STYLE_LOADED");
  public static final RuntimeEventType MAP_LOADING_STARTED =
      new RuntimeEventType(5, "MAP_LOADING_STARTED");
  public static final RuntimeEventType MAP_LOADING_FINISHED =
      new RuntimeEventType(6, "MAP_LOADING_FINISHED");
  public static final RuntimeEventType MAP_LOADING_FAILED =
      new RuntimeEventType(7, "MAP_LOADING_FAILED");
  public static final RuntimeEventType MAP_IDLE = new RuntimeEventType(8, "MAP_IDLE");
  public static final RuntimeEventType MAP_RENDER_UPDATE_AVAILABLE =
      new RuntimeEventType(9, "MAP_RENDER_UPDATE_AVAILABLE");
  public static final RuntimeEventType MAP_RENDER_ERROR =
      new RuntimeEventType(10, "MAP_RENDER_ERROR");
  public static final RuntimeEventType MAP_STILL_IMAGE_FINISHED =
      new RuntimeEventType(11, "MAP_STILL_IMAGE_FINISHED");
  public static final RuntimeEventType MAP_STILL_IMAGE_FAILED =
      new RuntimeEventType(12, "MAP_STILL_IMAGE_FAILED");
  public static final RuntimeEventType MAP_RENDER_FRAME_STARTED =
      new RuntimeEventType(13, "MAP_RENDER_FRAME_STARTED");
  public static final RuntimeEventType MAP_RENDER_FRAME_FINISHED =
      new RuntimeEventType(14, "MAP_RENDER_FRAME_FINISHED");
  public static final RuntimeEventType MAP_RENDER_MAP_STARTED =
      new RuntimeEventType(15, "MAP_RENDER_MAP_STARTED");
  public static final RuntimeEventType MAP_RENDER_MAP_FINISHED =
      new RuntimeEventType(16, "MAP_RENDER_MAP_FINISHED");
  public static final RuntimeEventType MAP_STYLE_IMAGE_MISSING =
      new RuntimeEventType(17, "MAP_STYLE_IMAGE_MISSING");
  public static final RuntimeEventType MAP_TILE_ACTION =
      new RuntimeEventType(18, "MAP_TILE_ACTION");
  public static final RuntimeEventType OFFLINE_REGION_STATUS_CHANGED =
      new RuntimeEventType(19, "OFFLINE_REGION_STATUS_CHANGED");
  public static final RuntimeEventType OFFLINE_REGION_RESPONSE_ERROR =
      new RuntimeEventType(20, "OFFLINE_REGION_RESPONSE_ERROR");
  public static final RuntimeEventType OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED =
      new RuntimeEventType(21, "OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED");
  public static final RuntimeEventType OFFLINE_OPERATION_COMPLETED =
      new RuntimeEventType(22, "OFFLINE_OPERATION_COMPLETED");

  private final int nativeValue;
  private final String name;

  private RuntimeEventType(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static RuntimeEventType fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 1 -> MAP_CAMERA_WILL_CHANGE;
      case 2 -> MAP_CAMERA_IS_CHANGING;
      case 3 -> MAP_CAMERA_DID_CHANGE;
      case 4 -> MAP_STYLE_LOADED;
      case 5 -> MAP_LOADING_STARTED;
      case 6 -> MAP_LOADING_FINISHED;
      case 7 -> MAP_LOADING_FAILED;
      case 8 -> MAP_IDLE;
      case 9 -> MAP_RENDER_UPDATE_AVAILABLE;
      case 10 -> MAP_RENDER_ERROR;
      case 11 -> MAP_STILL_IMAGE_FINISHED;
      case 12 -> MAP_STILL_IMAGE_FAILED;
      case 13 -> MAP_RENDER_FRAME_STARTED;
      case 14 -> MAP_RENDER_FRAME_FINISHED;
      case 15 -> MAP_RENDER_MAP_STARTED;
      case 16 -> MAP_RENDER_MAP_FINISHED;
      case 17 -> MAP_STYLE_IMAGE_MISSING;
      case 18 -> MAP_TILE_ACTION;
      case 19 -> OFFLINE_REGION_STATUS_CHANGED;
      case 20 -> OFFLINE_REGION_RESPONSE_ERROR;
      case 21 -> OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED;
      case 22 -> OFFLINE_OPERATION_COMPLETED;
      default -> new RuntimeEventType(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof RuntimeEventType value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "RuntimeEventType(" + nativeValue + ")";
  }
}
