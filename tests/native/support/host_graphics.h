#ifndef MLN_NATIVE_TESTS_HOST_GRAPHICS_H
#define MLN_NATIVE_TESTS_HOST_GRAPHICS_H

// Render fixtures whose target the host creates: a borrowed texture or a
// presentation surface from tests/graphics, for the preset's backend. The
// browser presets have neither, because their contexts come from JavaScript.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"
#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__EMSCRIPTEN__)
// The size of every fixture target, in pixels, at a scale factor of 1.
#define MLN_TEST_HOST_TARGET_SIZE 64U

// Attaches a session to a borrowed texture that the fixture creates, with the
// preset's driver. The fixture owns the texture until it is destroyed.
bool mln_test_render_fixture_create_borrowed_texture(
  mln_map map, mln_test_render_fixture* fixture
);

// Attaches a session to a presentation surface that the fixture creates, with
// the preset's driver.
bool mln_test_render_fixture_create_surface(
  mln_map map, mln_test_render_fixture* fixture
);

// Copies the borrowed texture of a fixture that
// mln_test_render_fixture_create_borrowed_texture() made, as described for
// mln_test_graphics_texture_read_rgba8().
bool mln_test_render_fixture_read_texture(
  const mln_test_render_fixture* fixture, uint8_t* pixels, size_t size
);
#endif

#ifdef __cplusplus
}
#endif

#endif
