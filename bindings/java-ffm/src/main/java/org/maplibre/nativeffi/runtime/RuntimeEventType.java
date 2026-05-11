package org.maplibre.nativeffi.runtime;

/** Runtime event type copied from the native event queue. */
public enum RuntimeEventType {
  MAP_CAMERA_WILL_CHANGE(1),
  MAP_CAMERA_IS_CHANGING(2),
  MAP_CAMERA_DID_CHANGE(3),
  MAP_STYLE_LOADED(4),
  MAP_LOADING_STARTED(5),
  MAP_LOADING_FINISHED(6),
  MAP_LOADING_FAILED(7),
  MAP_IDLE(8),
  MAP_RENDER_UPDATE_AVAILABLE(9),
  MAP_RENDER_ERROR(10),
  MAP_STILL_IMAGE_FINISHED(11),
  MAP_STILL_IMAGE_FAILED(12),
  MAP_RENDER_FRAME_STARTED(13),
  MAP_RENDER_FRAME_FINISHED(14),
  MAP_RENDER_MAP_STARTED(15),
  MAP_RENDER_MAP_FINISHED(16),
  MAP_STYLE_IMAGE_MISSING(17),
  MAP_TILE_ACTION(18),
  OFFLINE_REGION_STATUS_CHANGED(19),
  OFFLINE_REGION_RESPONSE_ERROR(20),
  OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED(21),
  UNKNOWN(-1);

  private final int nativeValue;

  RuntimeEventType(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public static RuntimeEventType fromNative(int nativeValue) {
    for (var type : values()) {
      if (type.nativeValue == nativeValue) {
        return type;
      }
    }
    return UNKNOWN;
  }
}
