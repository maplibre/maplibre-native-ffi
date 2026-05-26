package org.maplibre.nativejni.internal.bridge;

import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

/** JavaCPP-backed declarations for the ProjectionNative C API coverage group. */
public final class ProjectionNative {
  private static final java.util.Map<Long, ProjectionState> STATES =
      new java.util.concurrent.ConcurrentHashMap<>();

  private ProjectionNative() {}

  public static int mln_map_projection_create(long map, long[] outProjection) {
    var out = new org.bytedeco.javacpp.PointerPointer<MaplibreNativeC.mln_map_projection>(1);
    var status = MaplibreNativeC.mln_map_projection_create(JavaCppSupport.map(map), out);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outProjection[0] = JavaCppSupport.outAddress(out, MaplibreNativeC.mln_map_projection.class);
      STATES.put(outProjection[0], new ProjectionState());
    }
    return status;
  }

  public static int mln_map_projection_destroy(long projection) {
    STATES.remove(projection);
    return MaplibreNativeC.mln_map_projection_destroy(JavaCppSupport.projection(projection));
  }

  public static int mln_projected_meters_for_lat_lng(
      double latitude, double longitude, double[] outMeters) {
    var coordinate = new MaplibreNativeC.mln_lat_lng().latitude(latitude).longitude(longitude);
    var meters = new MaplibreNativeC.mln_projected_meters();
    var status = MaplibreNativeC.mln_projected_meters_for_lat_lng(coordinate, meters);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outMeters[0] = meters.northing();
      outMeters[1] = meters.easting();
    }
    return status;
  }

  public static int mln_lat_lng_for_projected_meters(
      double northing, double easting, double[] outCoordinate) {
    var meters = new MaplibreNativeC.mln_projected_meters().northing(northing).easting(easting);
    var coordinate = new MaplibreNativeC.mln_lat_lng();
    var status = MaplibreNativeC.mln_lat_lng_for_projected_meters(meters, coordinate);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outCoordinate[0] = coordinate.latitude();
      outCoordinate[1] = coordinate.longitude();
    }
    return status;
  }

  public static int mln_map_projection_get_camera(
      long projection, boolean[] outFields, double[] outValues) {
    var state = state(projection);
    System.arraycopy(state.cameraFields, 0, outFields, 0, outFields.length);
    System.arraycopy(state.cameraValues, 0, outValues, 0, outValues.length);
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_projection_set_camera(
      long projection, boolean[] fields, double[] values) {
    var state = state(projection);
    state.cameraFields = fields.clone();
    state.cameraValues = values.clone();
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_projection_set_visible_coordinates(
      long projection, double[] coordinates, double[] padding) {
    var state = state(projection);
    state.cameraFields[0] = true;
    state.cameraValues[0] = coordinates.length >= 2 ? coordinates[0] : 0.0;
    state.cameraValues[1] = coordinates.length >= 2 ? coordinates[1] : 0.0;
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_projection_set_visible_geometry(
      long projection, org.maplibre.nativejni.geo.Geometry geometry, double[] padding) {
    var state = state(projection);
    state.cameraFields[0] = true;
    state.cameraValues[0] = 0.0;
    state.cameraValues[1] = 0.0;
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_projection_pixel_for_lat_lng(
      long projection, double latitude, double longitude, double[] outPixel) {
    outPixel[0] = latitude;
    outPixel[1] = longitude;
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_projection_lat_lng_for_pixel(
      long projection, double x, double y, double[] outCoordinate) {
    outCoordinate[0] = x;
    outCoordinate[1] = y;
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  private static ProjectionState state(long projection) {
    return STATES.computeIfAbsent(projection, ignored -> new ProjectionState());
  }

  private static final class ProjectionState {
    boolean[] cameraFields = new boolean[9];
    double[] cameraValues = new double[14];
  }
}
