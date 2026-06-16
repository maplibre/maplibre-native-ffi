package org.maplibre.nativejni.internal.struct;

import java.util.ArrayList;
import java.util.List;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.bytedeco.javacpp.PointerPointer;
import org.bytedeco.javacpp.SizeTPointer;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.LatLngBounds;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.offline.OfflineRegionDefinition;
import org.maplibre.nativejni.offline.OfflineRegionInfo;

/** JavaCPP-backed materializers and readers for offline region JNI calls. */
public final class OfflineStructs {
  private OfflineStructs() {}

  public static OfflineRegionInfo offlineRegionSnapshot(
      PointerPointer<MaplibreNativeC.mln_offline_region_snapshot> outSnapshot) {
    var snapshotAddress =
        JavaCppSupport.outAddress(outSnapshot, MaplibreNativeC.mln_offline_region_snapshot.class);
    var snapshot =
        new MaplibreNativeC.mln_offline_region_snapshot(JavaCppSupport.pointer(snapshotAddress));
    try {
      try (var info = new MaplibreNativeC.mln_offline_region_info()) {
        info.size(info.sizeof());
        var status = MaplibreNativeC.mln_offline_region_snapshot_get(snapshot, info);
        org.maplibre.nativejni.internal.status.Status.check(status);
        return regionInfo(info);
      }
    } finally {
      MaplibreNativeC.mln_offline_region_snapshot_destroy(snapshot);
    }
  }

  public static List<OfflineRegionInfo> offlineRegionList(
      PointerPointer<MaplibreNativeC.mln_offline_region_list> outList) {
    var listAddress =
        JavaCppSupport.outAddress(outList, MaplibreNativeC.mln_offline_region_list.class);
    var list = new MaplibreNativeC.mln_offline_region_list(JavaCppSupport.pointer(listAddress));
    try (var count = new SizeTPointer(1)) {
      var status = MaplibreNativeC.mln_offline_region_list_count(list, count);
      org.maplibre.nativejni.internal.status.Status.check(status);
      var regions = new ArrayList<OfflineRegionInfo>(Math.toIntExact(count.get()));
      for (var i = 0; i < Math.toIntExact(count.get()); i++) {
        var info = new MaplibreNativeC.mln_offline_region_info();
        try {
          info.size(info.sizeof());
          status = MaplibreNativeC.mln_offline_region_list_get(list, i, info);
          org.maplibre.nativejni.internal.status.Status.check(status);
          regions.add(regionInfo(info));
        } finally {
          info.close();
        }
      }
      return List.copyOf(regions);
    } finally {
      MaplibreNativeC.mln_offline_region_list_destroy(list);
    }
  }

  private static OfflineRegionInfo regionInfo(MaplibreNativeC.mln_offline_region_info info) {
    return new OfflineRegionInfo(info.id(), definition(info.definition()), metadata(info));
  }

  private static byte[] metadata(MaplibreNativeC.mln_offline_region_info info) {
    var metadata = new byte[Math.toIntExact(info.metadata_size())];
    if (metadata.length > 0) {
      info.metadata().get(metadata, 0, metadata.length);
    }
    return metadata;
  }

  private static OfflineRegionDefinition definition(
      MaplibreNativeC.mln_offline_region_definition definition) {
    return switch (definition.type()) {
      case MaplibreNativeC.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID ->
          tilePyramid(definition.data_tile_pyramid());
      case MaplibreNativeC.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY ->
          geometryRegion(definition.data_geometry());
      default ->
          throw new IllegalArgumentException(
              "unknown offline region definition type: " + definition.type());
    };
  }

  private static OfflineRegionDefinition.TilePyramid tilePyramid(
      MaplibreNativeC.mln_offline_tile_pyramid_region_definition definition) {
    return new OfflineRegionDefinition.TilePyramid(
        JavaCppSupport.cString(definition.style_url()),
        bounds(definition.bounds()),
        definition.min_zoom(),
        definition.max_zoom(),
        definition.pixel_ratio(),
        definition.include_ideographs());
  }

  private static OfflineRegionDefinition.GeometryRegion geometryRegion(
      MaplibreNativeC.mln_offline_geometry_region_definition definition) {
    return new OfflineRegionDefinition.GeometryRegion(
        JavaCppSupport.cString(definition.style_url()),
        geometry(definition.geometry()),
        definition.min_zoom(),
        definition.max_zoom(),
        definition.pixel_ratio(),
        definition.include_ideographs());
  }

  private static LatLngBounds bounds(MaplibreNativeC.mln_lat_lng_bounds bounds) {
    return new LatLngBounds(coordinate(bounds.southwest()), coordinate(bounds.northeast()));
  }

  private static LatLng coordinate(MaplibreNativeC.mln_lat_lng coordinate) {
    return new LatLng(coordinate.latitude(), coordinate.longitude());
  }

  private static Geometry geometry(MaplibreNativeC.mln_geometry geometry) {
    return switch (geometry.type()) {
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_EMPTY -> Geometry.empty();
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_POINT ->
          Geometry.point(coordinate(geometry.data_point()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_LINE_STRING ->
          Geometry.lineString(coordinates(geometry.data_line_string()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_POLYGON ->
          Geometry.polygon(polygon(geometry.data_polygon()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POINT ->
          Geometry.multiPoint(coordinates(geometry.data_multi_point()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING ->
          Geometry.multiLineString(multiLine(geometry.data_multi_line_string()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POLYGON ->
          Geometry.multiPolygon(multiPolygon(geometry.data_multi_polygon()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION ->
          Geometry.collection(collection(geometry.data_geometry_collection()));
      default -> throw new IllegalArgumentException("unknown geometry type: " + geometry.type());
    };
  }

  private static List<LatLng> coordinates(MaplibreNativeC.mln_coordinate_span span) {
    var coordinates = new ArrayList<LatLng>(Math.toIntExact(span.coordinate_count()));
    var nativeCoordinates = span.coordinates();
    for (var i = 0; i < span.coordinate_count(); i++) {
      coordinates.add(coordinate(nativeCoordinates.getPointer(i)));
    }
    return coordinates;
  }

  private static List<List<LatLng>> polygon(MaplibreNativeC.mln_polygon_geometry polygon) {
    var rings = new ArrayList<List<LatLng>>(Math.toIntExact(polygon.ring_count()));
    var nativeRings = polygon.rings();
    for (var i = 0; i < polygon.ring_count(); i++) {
      rings.add(coordinates(nativeRings.getPointer(i)));
    }
    return rings;
  }

  private static List<List<LatLng>> multiLine(MaplibreNativeC.mln_multi_line_geometry multiLine) {
    var lines = new ArrayList<List<LatLng>>(Math.toIntExact(multiLine.line_count()));
    var nativeLines = multiLine.lines();
    for (var i = 0; i < multiLine.line_count(); i++) {
      lines.add(coordinates(nativeLines.getPointer(i)));
    }
    return lines;
  }

  private static List<List<List<LatLng>>> multiPolygon(
      MaplibreNativeC.mln_multi_polygon_geometry multiPolygon) {
    var polygons = new ArrayList<List<List<LatLng>>>(Math.toIntExact(multiPolygon.polygon_count()));
    var nativePolygons = multiPolygon.polygons();
    for (var i = 0; i < multiPolygon.polygon_count(); i++) {
      polygons.add(polygon(nativePolygons.getPointer(i)));
    }
    return polygons;
  }

  private static List<Geometry> collection(MaplibreNativeC.mln_geometry_collection collection) {
    var geometries = new ArrayList<Geometry>(Math.toIntExact(collection.geometry_count()));
    var nativeGeometries = collection.geometries();
    for (var i = 0; i < collection.geometry_count(); i++) {
      geometries.add(geometry(nativeGeometries.getPointer(i)));
    }
    return geometries;
  }

  public static final class DefinitionScope implements AutoCloseable {
    private final ArrayList<Pointer> owned = new ArrayList<>();
    private final MaplibreNativeC.mln_offline_region_definition definition;
    private final GeometryScope geometryScope;

    public DefinitionScope(OfflineRegionDefinition value) {
      definition = own(new MaplibreNativeC.mln_offline_region_definition());
      definition.size(definition.sizeof());
      if (value instanceof OfflineRegionDefinition.TilePyramid tilePyramid) {
        geometryScope = null;
        definition.type(MaplibreNativeC.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID);
        definition.data_tile_pyramid(tilePyramid(tilePyramid));
      } else if (value instanceof OfflineRegionDefinition.GeometryRegion geometryRegion) {
        geometryScope = new GeometryScope(geometryRegion.geometry());
        definition.type(MaplibreNativeC.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY);
        definition.data_geometry(geometryRegion(geometryRegion));
      } else {
        throw new IllegalArgumentException("unknown offline region definition type");
      }
    }

    public MaplibreNativeC.mln_offline_region_definition definition() {
      return definition;
    }

    @Override
    public void close() {
      if (geometryScope != null) {
        geometryScope.close();
      }
      for (var i = owned.size() - 1; i >= 0; i--) {
        owned.get(i).close();
      }
    }

    private MaplibreNativeC.mln_offline_tile_pyramid_region_definition tilePyramid(
        OfflineRegionDefinition.TilePyramid value) {
      var out = own(new MaplibreNativeC.mln_offline_tile_pyramid_region_definition());
      out.size(out.sizeof());
      out.style_url(utf8(value.styleUrl()));
      out.bounds(bounds(value.bounds()));
      out.min_zoom(value.minZoom());
      out.max_zoom(value.maxZoom());
      out.pixel_ratio(value.pixelRatio());
      out.include_ideographs(value.includeIdeographs());
      return out;
    }

    private MaplibreNativeC.mln_offline_geometry_region_definition geometryRegion(
        OfflineRegionDefinition.GeometryRegion value) {
      var out = own(new MaplibreNativeC.mln_offline_geometry_region_definition());
      out.size(out.sizeof());
      out.style_url(utf8(value.styleUrl()));
      out.geometry(geometryScope.geometry());
      out.min_zoom(value.minZoom());
      out.max_zoom(value.maxZoom());
      out.pixel_ratio(value.pixelRatio());
      out.include_ideographs(value.includeIdeographs());
      return out;
    }

    private MaplibreNativeC.mln_lat_lng_bounds bounds(LatLngBounds value) {
      var out = own(new MaplibreNativeC.mln_lat_lng_bounds());
      out.southwest().latitude(value.southwest().latitude());
      out.southwest().longitude(value.southwest().longitude());
      out.northeast().latitude(value.northeast().latitude());
      out.northeast().longitude(value.northeast().longitude());
      return out;
    }

    private BytePointer utf8(String value) {
      return own(JavaCppSupport.utf8(value));
    }

    private <T extends Pointer> T own(T pointer) {
      owned.add(pointer);
      return pointer;
    }
  }

  private static final class GeometryScope implements AutoCloseable {
    private final ArrayList<Pointer> owned = new ArrayList<>();
    private final MaplibreNativeC.mln_geometry geometry;

    GeometryScope(Geometry value) {
      geometry = geometry(value);
    }

    MaplibreNativeC.mln_geometry geometry() {
      return geometry;
    }

    @Override
    public void close() {
      for (var i = owned.size() - 1; i >= 0; i--) {
        owned.get(i).close();
      }
    }

    private MaplibreNativeC.mln_geometry geometry(Geometry value) {
      var out = own(new MaplibreNativeC.mln_geometry());
      out.size(out.sizeof());
      switch (value) {
        case Geometry.Empty ignored -> out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_EMPTY);
        case Geometry.Point node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_POINT);
          out.data_point(coordinate(node.coordinate()));
        }
        case Geometry.LineString node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_LINE_STRING);
          out.data_line_string(coordinateSpan(node.coordinates()));
        }
        case Geometry.Polygon node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_POLYGON);
          out.data_polygon(polygon(node.rings()));
        }
        case Geometry.MultiPoint node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POINT);
          out.data_multi_point(coordinateSpan(node.coordinates()));
        }
        case Geometry.MultiLineString node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING);
          out.data_multi_line_string(multiLine(node.lines()));
        }
        case Geometry.MultiPolygon node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POLYGON);
          out.data_multi_polygon(multiPolygon(node.polygons()));
        }
        case Geometry.Collection node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION);
          var geometries = node.geometries();
          var collection = own(new MaplibreNativeC.mln_geometry_collection());
          if (!geometries.isEmpty()) {
            var nativeGeometries = own(new MaplibreNativeC.mln_geometry(geometries.size()));
            for (var i = 0; i < geometries.size(); i++) {
              nativeGeometries.position(i).put(geometry(geometries.get(i)));
            }
            nativeGeometries.position(0);
            collection.geometries(nativeGeometries);
          }
          collection.geometry_count(geometries.size());
          out.data_geometry_collection(collection);
        }
      }
      return out;
    }

    private MaplibreNativeC.mln_coordinate_span coordinateSpan(List<LatLng> values) {
      var span = own(new MaplibreNativeC.mln_coordinate_span());
      if (!values.isEmpty()) {
        var nativeCoordinates = own(new MaplibreNativeC.mln_lat_lng(values.size()));
        for (var i = 0; i < values.size(); i++) {
          var coordinate = values.get(i);
          nativeCoordinates
              .position(i)
              .latitude(coordinate.latitude())
              .longitude(coordinate.longitude());
        }
        nativeCoordinates.position(0);
        span.coordinates(nativeCoordinates);
      }
      span.coordinate_count(values.size());
      return span;
    }

    private MaplibreNativeC.mln_polygon_geometry polygon(List<List<LatLng>> rings) {
      var out = own(new MaplibreNativeC.mln_polygon_geometry());
      if (!rings.isEmpty()) {
        var nativeRings = own(new MaplibreNativeC.mln_coordinate_span(rings.size()));
        for (var i = 0; i < rings.size(); i++) {
          nativeRings.position(i).put(coordinateSpan(rings.get(i)));
        }
        nativeRings.position(0);
        out.rings(nativeRings);
      }
      out.ring_count(rings.size());
      return out;
    }

    private MaplibreNativeC.mln_multi_line_geometry multiLine(List<List<LatLng>> lines) {
      var out = own(new MaplibreNativeC.mln_multi_line_geometry());
      if (!lines.isEmpty()) {
        var nativeLines = own(new MaplibreNativeC.mln_coordinate_span(lines.size()));
        for (var i = 0; i < lines.size(); i++) {
          nativeLines.position(i).put(coordinateSpan(lines.get(i)));
        }
        nativeLines.position(0);
        out.lines(nativeLines);
      }
      out.line_count(lines.size());
      return out;
    }

    private MaplibreNativeC.mln_multi_polygon_geometry multiPolygon(
        List<List<List<LatLng>>> polygons) {
      var out = own(new MaplibreNativeC.mln_multi_polygon_geometry());
      if (!polygons.isEmpty()) {
        var nativePolygons = own(new MaplibreNativeC.mln_polygon_geometry(polygons.size()));
        for (var i = 0; i < polygons.size(); i++) {
          nativePolygons.position(i).put(polygon(polygons.get(i)));
        }
        nativePolygons.position(0);
        out.polygons(nativePolygons);
      }
      out.polygon_count(polygons.size());
      return out;
    }

    private MaplibreNativeC.mln_lat_lng coordinate(LatLng value) {
      var out = own(new MaplibreNativeC.mln_lat_lng());
      out.latitude(value.latitude());
      out.longitude(value.longitude());
      return out;
    }

    private <T extends Pointer> T own(T pointer) {
      owned.add(pointer);
      return pointer;
    }
  }
}
