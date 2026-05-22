package org.maplibre.nativejni.internal.struct;

import java.util.Objects;
import org.maplibre.nativejni.camera.EdgeInsets;
import org.maplibre.nativejni.camera.UnitBezier;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.LatLngBounds;
import org.maplibre.nativejni.geo.ProjectedMeters;
import org.maplibre.nativejni.geo.Quaternion;
import org.maplibre.nativejni.geo.ScreenBox;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.geo.Vec3;

/** Internal copied-value materializers shared by JNI descriptor helpers. */
public final class CoreStructs {
  private CoreStructs() {}

  public record LatLngValue(double latitude, double longitude) {}

  public record LatLngBoundsValue(LatLngValue southwest, LatLngValue northeast) {}

  public record ProjectedMetersValue(double northing, double easting) {}

  public record ScreenPointValue(double x, double y) {}

  public record ScreenBoxValue(ScreenPointValue min, ScreenPointValue max) {}

  public record EdgeInsetsValue(double top, double left, double bottom, double right) {}

  public record Vec3Value(double x, double y, double z) {}

  public record QuaternionValue(double x, double y, double z, double w) {}

  public record UnitBezierValue(double x1, double y1, double x2, double y2) {}

  public static LatLngValue latLng(LatLng value) {
    Objects.requireNonNull(value, "value");
    return new LatLngValue(value.latitude(), value.longitude());
  }

  public static LatLng latLng(LatLngValue value) {
    Objects.requireNonNull(value, "value");
    return new LatLng(value.latitude(), value.longitude());
  }

  public static LatLngBoundsValue latLngBounds(LatLngBounds value) {
    Objects.requireNonNull(value, "value");
    return new LatLngBoundsValue(latLng(value.southwest()), latLng(value.northeast()));
  }

  public static LatLngBounds latLngBounds(LatLngBoundsValue value) {
    Objects.requireNonNull(value, "value");
    return new LatLngBounds(latLng(value.southwest()), latLng(value.northeast()));
  }

  public static ProjectedMetersValue projectedMeters(ProjectedMeters value) {
    Objects.requireNonNull(value, "value");
    return new ProjectedMetersValue(value.northing(), value.easting());
  }

  public static ProjectedMeters projectedMeters(ProjectedMetersValue value) {
    Objects.requireNonNull(value, "value");
    return new ProjectedMeters(value.northing(), value.easting());
  }

  public static ScreenPointValue screenPoint(ScreenPoint value) {
    Objects.requireNonNull(value, "value");
    return new ScreenPointValue(value.x(), value.y());
  }

  public static ScreenPoint screenPoint(ScreenPointValue value) {
    Objects.requireNonNull(value, "value");
    return new ScreenPoint(value.x(), value.y());
  }

  public static ScreenBoxValue screenBox(ScreenBox value) {
    Objects.requireNonNull(value, "value");
    return new ScreenBoxValue(screenPoint(value.min()), screenPoint(value.max()));
  }

  public static ScreenBox screenBox(ScreenBoxValue value) {
    Objects.requireNonNull(value, "value");
    return new ScreenBox(screenPoint(value.min()), screenPoint(value.max()));
  }

  public static EdgeInsetsValue edgeInsets(EdgeInsets value) {
    Objects.requireNonNull(value, "value");
    return new EdgeInsetsValue(value.top(), value.left(), value.bottom(), value.right());
  }

  public static EdgeInsets edgeInsets(EdgeInsetsValue value) {
    Objects.requireNonNull(value, "value");
    return new EdgeInsets(value.top(), value.left(), value.bottom(), value.right());
  }

  public static Vec3Value vec3(Vec3 value) {
    Objects.requireNonNull(value, "value");
    return new Vec3Value(value.x(), value.y(), value.z());
  }

  public static Vec3 vec3(Vec3Value value) {
    Objects.requireNonNull(value, "value");
    return new Vec3(value.x(), value.y(), value.z());
  }

  public static QuaternionValue quaternion(Quaternion value) {
    Objects.requireNonNull(value, "value");
    return new QuaternionValue(value.x(), value.y(), value.z(), value.w());
  }

  public static Quaternion quaternion(QuaternionValue value) {
    Objects.requireNonNull(value, "value");
    return new Quaternion(value.x(), value.y(), value.z(), value.w());
  }

  public static UnitBezierValue unitBezier(UnitBezier value) {
    Objects.requireNonNull(value, "value");
    return new UnitBezierValue(value.x1(), value.y1(), value.x2(), value.y2());
  }

  public static UnitBezier unitBezier(UnitBezierValue value) {
    Objects.requireNonNull(value, "value");
    return new UnitBezier(value.x1(), value.y1(), value.x2(), value.y2());
  }
}
