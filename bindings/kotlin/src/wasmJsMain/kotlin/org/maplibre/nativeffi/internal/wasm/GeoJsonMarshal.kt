package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.generated.MlnFeature
import org.maplibre.nativeffi.internal.wasm.generated.MlnFeatureCollection
import org.maplibre.nativeffi.internal.wasm.generated.MlnFeatureIdentifierType
import org.maplibre.nativeffi.internal.wasm.generated.MlnGeojson
import org.maplibre.nativeffi.internal.wasm.generated.MlnGeojsonType

/**
 * Places a [GeoJson] or [Feature] tree into the Emscripten heap.
 *
 * A GeoJSON descriptor is the outermost of the three trees this binding writes: its arms reach into
 * geometry through [GeometryMarshal] and into feature properties through [JsonMarshal], and all
 * three share one arena so that the whole graph is one allocation and one release. The arena
 * arithmetic comes from [JsonMarshal] for the same reason — the checked measure lives in one place.
 *
 * Depth is bounded by whichever marshaller owns the nesting: a GeoJSON descriptor is itself flat,
 * so a feature collection adds no level of its own and nothing here needs a depth of its own.
 *
 * Measuring and writing walk the same shape in the same order, and each pair sits together so that
 * a change to one is visible against the other.
 */
internal object GeoJsonMarshal {
  /** Bytes [value] needs, including its root descriptor. */
  fun measure(value: GeoJson): Int =
    JsonMarshal.plus(JsonMarshal.measureBlock(MlnGeojson.SIZEOF), measurePayload(value)).toInt()

  private fun measurePayload(value: GeoJson): Long =
    when (value) {
      // The geometry and feature arms hold a pointer, so the thing pointed at needs a block of its
      // own; only the collection arm is held in place inside the root descriptor.
      is GeoJson.GeometryValue -> measureGeometry(value.geometry)
      is GeoJson.FeatureValue -> measureFeatureValue(value.feature)
      is GeoJson.FeatureCollection ->
        value.features.fold(JsonMarshal.measureArray(MlnFeature.SIZEOF, value.features.size)) {
          total,
          feature ->
          JsonMarshal.plus(total, measureFeaturePayload(feature))
        }
    }

  /** Writes [value] into [arena] and returns the root descriptor's address. */
  fun write(arena: HeapArena, value: GeoJson): HeapPointer {
    val base = JsonMarshal.allocateBlock(arena, MlnGeojson.SIZEOF)
    // The leading size field is how the C API versions a descriptor: it carries the size this
    // binding was generated against so native can tell which fields it may read.
    MlnGeojson.setSize(base, MlnGeojson.SIZEOF)
    val data = base + MlnGeojson.OFFSET_DATA
    when (value) {
      is GeoJson.GeometryValue -> {
        MlnGeojson.setType(base, MlnGeojsonType.MLN_GEOJSON_TYPE_GEOMETRY)
        // The arm is a bare pointer rather than a struct, so there is no generated field
        // accessor to name; what is written is the union's own address.
        Heap.storeInt(data, GeometryMarshal.write(arena, value.geometry).address)
      }
      is GeoJson.FeatureValue -> {
        MlnGeojson.setType(base, MlnGeojsonType.MLN_GEOJSON_TYPE_FEATURE)
        Heap.storeInt(data, writeFeature(arena, value.feature).address)
      }
      is GeoJson.FeatureCollection -> {
        MlnGeojson.setType(base, MlnGeojsonType.MLN_GEOJSON_TYPE_FEATURE_COLLECTION)
        val features = JsonMarshal.allocateArray(arena, MlnFeature.SIZEOF, value.features.size)
        value.features.forEachIndexed { index, feature ->
          writeFeatureInto(arena, features + index * MlnFeature.SIZEOF, feature)
        }
        MlnFeatureCollection.setFeatures(data, features)
        MlnFeatureCollection.setFeatureCount(data, value.features.size)
      }
    }
    return base
  }

  /** Bytes [feature] needs, including its root descriptor. */
  fun measureFeature(feature: Feature): Int = measureFeatureValue(feature).toInt()

  /** Writes [feature] into [arena] and returns its descriptor's address. */
  fun writeFeature(arena: HeapArena, feature: Feature): HeapPointer {
    val base = JsonMarshal.allocateBlock(arena, MlnFeature.SIZEOF)
    writeFeatureInto(arena, base, feature)
    return base
  }

  private fun measureFeatureValue(feature: Feature): Long =
    JsonMarshal.plus(JsonMarshal.measureBlock(MlnFeature.SIZEOF), measureFeaturePayload(feature))

  /**
   * Bytes [feature] needs below its own descriptor.
   *
   * Every addition is bounded as it is taken rather than only at the end. A subtotal that wrapped
   * would produce a small positive size that passed both this check and the arena's, and the write
   * that followed would run past the block.
   */
  private fun measureFeaturePayload(feature: Feature): Long {
    val geometry = measureGeometry(feature.geometry)
    // Properties are a root member array rather than an object's, so their values start at depth 0.
    val properties = JsonMarshal.measureMembers(feature.properties, 0)
    return JsonMarshal.plus(
      JsonMarshal.plus(geometry, properties),
      measureIdentifier(feature.identifier),
    )
  }

  private fun writeFeatureInto(arena: HeapArena, base: HeapPointer, feature: Feature) {
    MlnFeature.setSize(base, MlnFeature.SIZEOF)
    MlnFeature.setGeometry(base, GeometryMarshal.write(arena, feature.geometry))
    MlnFeature.setProperties(base, JsonMarshal.writeMembers(arena, feature.properties, 0))
    MlnFeature.setPropertyCount(base, feature.properties.size)
    writeIdentifier(arena, base, feature.identifier)
  }

  private fun measureIdentifier(identifier: FeatureIdentifier): Long =
    when (identifier) {
      // Scalars live in the descriptor's own union arm, so they need no storage of their own.
      FeatureIdentifier.Null -> 0L
      is FeatureIdentifier.UInt -> 0L
      is FeatureIdentifier.Int -> 0L
      is FeatureIdentifier.DoubleValue -> 0L
      is FeatureIdentifier.StringValue -> JsonMarshal.measureText(identifier.value)
      // An identifier read back from a native tag this binding did not recognise. Its shape is
      // unknown, so there is nothing to measure and nothing that could be written back.
      is FeatureIdentifier.Unknown -> throw unknownIdentifier(identifier)
    }

  private fun writeIdentifier(arena: HeapArena, base: HeapPointer, identifier: FeatureIdentifier) {
    val data = base + MlnFeature.OFFSET_IDENTIFIER
    when (identifier) {
      FeatureIdentifier.Null ->
        MlnFeature.setIdentifierType(
          base,
          MlnFeatureIdentifierType.MLN_FEATURE_IDENTIFIER_TYPE_NULL,
        )
      is FeatureIdentifier.UInt -> {
        MlnFeature.setIdentifierType(
          base,
          MlnFeatureIdentifierType.MLN_FEATURE_IDENTIFIER_TYPE_UINT,
        )
        // Carried as the bit pattern it was read as. The C arm is unsigned and Kotlin's Long
        // is not, so reinterpreting here would change the identifier rather than preserve it.
        Heap.storeLong(data, identifier.value)
      }
      is FeatureIdentifier.Int -> {
        MlnFeature.setIdentifierType(base, MlnFeatureIdentifierType.MLN_FEATURE_IDENTIFIER_TYPE_INT)
        Heap.storeLong(data, identifier.value)
      }
      is FeatureIdentifier.DoubleValue -> {
        MlnFeature.setIdentifierType(
          base,
          MlnFeatureIdentifierType.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE,
        )
        Heap.storeDouble(data, identifier.value)
      }
      is FeatureIdentifier.StringValue -> {
        MlnFeature.setIdentifierType(
          base,
          MlnFeatureIdentifierType.MLN_FEATURE_IDENTIFIER_TYPE_STRING,
        )
        JsonMarshal.writeText(arena, data, identifier.value)
      }
      is FeatureIdentifier.Unknown -> throw unknownIdentifier(identifier)
    }
  }

  /**
   * Bytes a geometry tree occupies here, paired with [GeometryMarshal.write].
   *
   * Rounded up even though a geometry tree already measures to a multiple of this arena's alignment
   * today: that marshaller measures against its own struct widths, and a header change there must
   * not start costing this measure padding it never accounted for.
   */
  private fun measureGeometry(geometry: Geometry): Long =
    JsonMarshal.measureBlock(GeometryMarshal.measure(geometry))

  private fun unknownIdentifier(identifier: FeatureIdentifier.Unknown) =
    Status.invalidArgument(
      "A feature identifier of unknown native type ${identifier.rawType} cannot be sent to native; " +
        "it was read from a tag this binding does not recognise."
    )
}
