#pragma once

#include <maplibre_native_c/style.h>

// JavaCPP borrows each const byte array with GetByteArrayElements and releases
// it with JNI_ABORT after the C API has copied the accepted pixels.
inline mln_status mln_android_set_style_image(
  mln_map map, const mln_buffer_view* image_id,
  const mln_premultiplied_rgba8_image* image, const uint8_t* pixels,
  const mln_style_image_options* options, const mln_completion* completion
) {
  auto borrowed = *image;
  borrowed.pixels = pixels;
  return mln_map_set_style_image(
    map, *image_id, &borrowed, options, completion
  );
}

inline mln_status mln_android_add_image_source_image(
  mln_map map, const mln_buffer_view* source_id, const mln_lat_lng* coordinates,
  size_t coordinate_count, const mln_premultiplied_rgba8_image* image,
  const uint8_t* pixels, const mln_completion* completion
) {
  auto borrowed = *image;
  borrowed.pixels = pixels;
  return mln_map_add_image_source_image(
    map, *source_id, coordinates, coordinate_count, &borrowed, completion
  );
}

inline mln_status mln_android_set_image_source_image(
  mln_map map, const mln_buffer_view* source_id,
  const mln_premultiplied_rgba8_image* image, const uint8_t* pixels,
  const mln_completion* completion
) {
  auto borrowed = *image;
  borrowed.pixels = pixels;
  return mln_map_set_image_source_image(map, *source_id, &borrowed, completion);
}
