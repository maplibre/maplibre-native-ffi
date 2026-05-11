package org.maplibre.nativeffi.internal.struct;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import org.maplibre.nativeffi.camera.EdgeInsets;
import org.maplibre.nativeffi.camera.UnitBezier;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.geo.LatLngBounds;
import org.maplibre.nativeffi.geo.ProjectedMeters;
import org.maplibre.nativeffi.geo.Quaternion;
import org.maplibre.nativeffi.geo.ScreenPoint;
import org.maplibre.nativeffi.geo.Vec3;
import org.maplibre.nativeffi.internal.c.mln_edge_insets;
import org.maplibre.nativeffi.internal.c.mln_lat_lng;
import org.maplibre.nativeffi.internal.c.mln_lat_lng_bounds;
import org.maplibre.nativeffi.internal.c.mln_projected_meters;
import org.maplibre.nativeffi.internal.c.mln_quaternion;
import org.maplibre.nativeffi.internal.c.mln_screen_point;
import org.maplibre.nativeffi.internal.c.mln_string_view;
import org.maplibre.nativeffi.internal.c.mln_unit_bezier;
import org.maplibre.nativeffi.internal.c.mln_vec3;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;

/** Internal materializers and readers for shared value structs. */
public final class CoreStructs {
  private CoreStructs() {}

  public static MemorySegment latLng(LatLng coordinate, Arena arena) {
    var segment = mln_lat_lng.allocate(arena);
    mln_lat_lng.latitude(segment, coordinate.latitude());
    mln_lat_lng.longitude(segment, coordinate.longitude());
    return segment;
  }

  public static LatLng latLng(MemorySegment segment) {
    return new LatLng(mln_lat_lng.latitude(segment), mln_lat_lng.longitude(segment));
  }

  public static MemorySegment latLngBounds(LatLngBounds bounds, Arena arena) {
    var segment = mln_lat_lng_bounds.allocate(arena);
    mln_lat_lng_bounds.southwest(segment, latLng(bounds.southwest(), arena));
    mln_lat_lng_bounds.northeast(segment, latLng(bounds.northeast(), arena));
    return segment;
  }

  public static LatLngBounds latLngBounds(MemorySegment segment) {
    return new LatLngBounds(
        latLng(mln_lat_lng_bounds.southwest(segment)),
        latLng(mln_lat_lng_bounds.northeast(segment)));
  }

  public static MemorySegment latLngArray(List<LatLng> coordinates, Arena arena) {
    var array = mln_lat_lng.allocateArray(coordinates.size(), arena);
    for (var index = 0; index < coordinates.size(); index++) {
      var coordinate = coordinates.get(index);
      var element = mln_lat_lng.asSlice(array, index);
      mln_lat_lng.latitude(element, coordinate.latitude());
      mln_lat_lng.longitude(element, coordinate.longitude());
    }
    return array;
  }

  public static List<LatLng> latLngArray(MemorySegment segment, int count) {
    var coordinates = new ArrayList<LatLng>(count);
    for (var index = 0; index < count; index++) {
      coordinates.add(latLng(mln_lat_lng.asSlice(segment, index)));
    }
    return List.copyOf(coordinates);
  }

  public static MemorySegment screenPoint(ScreenPoint point, Arena arena) {
    var segment = mln_screen_point.allocate(arena);
    mln_screen_point.x(segment, point.x());
    mln_screen_point.y(segment, point.y());
    return segment;
  }

  public static ScreenPoint screenPoint(MemorySegment segment) {
    return new ScreenPoint(mln_screen_point.x(segment), mln_screen_point.y(segment));
  }

  public static MemorySegment screenPointArray(List<ScreenPoint> points, Arena arena) {
    var array = mln_screen_point.allocateArray(points.size(), arena);
    for (var index = 0; index < points.size(); index++) {
      var point = points.get(index);
      var element = mln_screen_point.asSlice(array, index);
      mln_screen_point.x(element, point.x());
      mln_screen_point.y(element, point.y());
    }
    return array;
  }

  public static List<ScreenPoint> screenPointArray(MemorySegment segment, int count) {
    var points = new ArrayList<ScreenPoint>(count);
    for (var index = 0; index < count; index++) {
      points.add(screenPoint(mln_screen_point.asSlice(segment, index)));
    }
    return List.copyOf(points);
  }

  public static MemorySegment unitBezier(UnitBezier easing, Arena arena) {
    var segment = mln_unit_bezier.allocate(arena);
    mln_unit_bezier.x1(segment, easing.x1());
    mln_unit_bezier.y1(segment, easing.y1());
    mln_unit_bezier.x2(segment, easing.x2());
    mln_unit_bezier.y2(segment, easing.y2());
    return segment;
  }

  public static MemorySegment vec3(Vec3 value, Arena arena) {
    var segment = mln_vec3.allocate(arena);
    mln_vec3.x(segment, value.x());
    mln_vec3.y(segment, value.y());
    mln_vec3.z(segment, value.z());
    return segment;
  }

  public static Vec3 vec3(MemorySegment segment) {
    return new Vec3(mln_vec3.x(segment), mln_vec3.y(segment), mln_vec3.z(segment));
  }

  public static MemorySegment quaternion(Quaternion value, Arena arena) {
    var segment = mln_quaternion.allocate(arena);
    mln_quaternion.x(segment, value.x());
    mln_quaternion.y(segment, value.y());
    mln_quaternion.z(segment, value.z());
    mln_quaternion.w(segment, value.w());
    return segment;
  }

  public static Quaternion quaternion(MemorySegment segment) {
    return new Quaternion(
        mln_quaternion.x(segment),
        mln_quaternion.y(segment),
        mln_quaternion.z(segment),
        mln_quaternion.w(segment));
  }

  public static MemorySegment projectedMeters(ProjectedMeters meters, Arena arena) {
    var segment = mln_projected_meters.allocate(arena);
    mln_projected_meters.northing(segment, meters.northing());
    mln_projected_meters.easting(segment, meters.easting());
    return segment;
  }

  public static ProjectedMeters projectedMeters(MemorySegment segment) {
    return new ProjectedMeters(
        mln_projected_meters.northing(segment), mln_projected_meters.easting(segment));
  }

  public static MemorySegment edgeInsets(EdgeInsets insets, Arena arena) {
    var segment = mln_edge_insets.allocate(arena);
    mln_edge_insets.top(segment, insets.top());
    mln_edge_insets.left(segment, insets.left());
    mln_edge_insets.bottom(segment, insets.bottom());
    mln_edge_insets.right(segment, insets.right());
    return segment;
  }

  public static EdgeInsets edgeInsets(MemorySegment segment) {
    return new EdgeInsets(
        mln_edge_insets.top(segment),
        mln_edge_insets.left(segment),
        mln_edge_insets.bottom(segment),
        mln_edge_insets.right(segment));
  }

  public static MemorySegment stringView(String value, Arena arena) {
    var bytes = value.getBytes(StandardCharsets.UTF_8);
    var view = mln_string_view.allocate(arena);
    if (bytes.length > 0) {
      var nativeBytes = arena.allocate(bytes.length);
      MemorySegment.copy(bytes, 0, nativeBytes, ValueLayout.JAVA_BYTE, 0, bytes.length);
      mln_string_view.data(view, nativeBytes);
    }
    mln_string_view.size(view, bytes.length);
    return view;
  }

  public static String stringView(MemorySegment view) {
    return MemoryUtil.copyStringView(mln_string_view.data(view), mln_string_view.size(view));
  }

  static MemorySegment sizedPointer(MemorySegment pointer, long byteSize, String name) {
    if (MemoryUtil.isNull(pointer)) {
      throw new IllegalArgumentException(name + " must not be null");
    }
    return pointer.reinterpret(byteSize);
  }

  static MemorySegment sizedArray(MemorySegment pointer, int count, long elementSize, String name) {
    if (count == 0) {
      return MemorySegment.NULL;
    }
    if (MemoryUtil.isNull(pointer)) {
      throw new IllegalArgumentException(name + " must not be null when count is non-zero");
    }
    return pointer.reinterpret(elementSize * count);
  }
}
