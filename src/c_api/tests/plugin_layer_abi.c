// Raw C ABI coverage: MapLibre Native's plugin registration reaches the
// library's exports, and a layer type registered through it renders through
// the C API on every backend the plugin API declares shaders for.

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "abi_tests.h"
#include "maplibre_native_c/plugin.h"
#include "test_support.h"
#include "unity.h"

#define SQUARE_STRING(literal) {(literal), sizeof(literal) - 1}

// The plugin draws a viewport-aligned square of `square-radius` logical pixels
// around each point feature, filled with `square-color`. Both properties may
// be data-driven, which exercises the host's attribute and uniform bindings.
enum {
  SQUARE_ATTRIBUTE_POSITION = 0,
  SQUARE_ATTRIBUTE_RADIUS = 1,
  SQUARE_ATTRIBUTE_COLOR_MIN = 2,
  SQUARE_ATTRIBUTE_COLOR_MAX = 3,
  SQUARE_VERTEX_STREAM = 0,
  SQUARE_UNIFORM_BLOCK = 0,
  SQUARE_MAX_POINTS = 64,
};

static const uint64_t square_drawable_key = 1;

typedef struct square_vertex {
  int16_t position[2];
} square_vertex;

// std140 layout of the drawable uniform block declared in the shaders.
typedef struct square_uniforms {
  float matrix[16];
  float pixels_to_gl_units[2];
  float padding0[2];
  float color[4];
  float radius;
  float radius_t;
  float color_t;
  float padding1;
} square_uniforms;

static_assert(sizeof(square_uniforms) == 112, "uniform block is std140");
static_assert(offsetof(square_uniforms, color) == 80, "color is vec4 aligned");
static_assert(offsetof(square_uniforms, radius) == 96, "radius follows color");

typedef struct square_layout {
  uint32_t extent;
  square_vertex vertices[SQUARE_MAX_POINTS * 4];
  uint16_t indices[SQUARE_MAX_POINTS * 6];
  mln_plugin_feature_vertex_range_v1 ranges[SQUARE_MAX_POINTS];
  uint32_t vertex_count;
  uint32_t index_count;
  size_t range_count;
  mln_plugin_segment_v1 segment;
  mln_plugin_vertex_stream_v1 stream;
  mln_plugin_attribute_binding_v1 attribute;
  mln_plugin_drawable_descriptor_v1 drawable;
} square_layout;

static mln_plugin_status square_create_layout(
  const mln_plugin_layout_context_v1* context, void** instance
) {
  if (
    context == NULL || context->struct_size < sizeof(*context) ||
    instance == NULL
  ) {
    return MLN_PLUGIN_STATUS_INVALID_ARGUMENT;
  }
  square_layout* layout = calloc(1, sizeof(*layout));
  if (layout == NULL) {
    return MLN_PLUGIN_STATUS_CALLBACK_ERROR;
  }
  layout->extent = context->extent;
  *instance = layout;
  return MLN_PLUGIN_STATUS_OK;
}

static mln_plugin_status square_layout_feature(
  void* instance, const mln_plugin_feature_v1* feature
) {
  if (
    instance == NULL || feature == NULL ||
    feature->struct_size < sizeof(*feature) ||
    feature->geometry_type != MLN_PLUGIN_GEOMETRY_POINT ||
    (feature->point_count != 0 && feature->points == NULL)
  ) {
    return MLN_PLUGIN_STATUS_INVALID_ARGUMENT;
  }
  square_layout* layout = instance;
  const uint32_t first_vertex = layout->vertex_count;
  for (size_t index = 0; index < feature->point_count; index += 1) {
    const mln_plugin_tile_point_v1 point = feature->points[index];
    // Half-open tile ownership keeps a point on a boundary in one tile.
    if (
      point.x < 0 || point.y < 0 || point.x >= (int32_t)layout->extent ||
      point.y >= (int32_t)layout->extent
    ) {
      continue;
    }
    if (layout->vertex_count + 4 > SQUARE_MAX_POINTS * 4) {
      return MLN_PLUGIN_STATUS_CALLBACK_ERROR;
    }
    const uint16_t base = (uint16_t)layout->vertex_count;
    static const int16_t corners[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    for (size_t corner = 0; corner < 4; corner += 1) {
      // The shader recovers the center and corner from one packed attribute.
      layout->vertices[layout->vertex_count].position[0] =
        (int16_t)(point.x * 2 + corners[corner][0]);
      layout->vertices[layout->vertex_count].position[1] =
        (int16_t)(point.y * 2 + corners[corner][1]);
      layout->vertex_count += 1;
    }
    const uint16_t quad[6] = {base, (uint16_t)(base + 1), (uint16_t)(base + 2),
                              base, (uint16_t)(base + 2), (uint16_t)(base + 3)};
    memcpy(layout->indices + layout->index_count, quad, sizeof(quad));
    layout->index_count += 6;
  }
  if (layout->vertex_count > first_vertex) {
    layout->ranges[layout->range_count] = (mln_plugin_feature_vertex_range_v1){
      .struct_size = sizeof(mln_plugin_feature_vertex_range_v1),
      .feature_index = feature->feature_index,
      .drawable_key = square_drawable_key,
      .first_vertex = first_vertex,
      .vertex_count = layout->vertex_count - first_vertex,
    };
    layout->range_count += 1;
  }
  return MLN_PLUGIN_STATUS_OK;
}

static mln_plugin_status square_finish_layout(
  void* instance, mln_plugin_bucket_v1* bucket
) {
  if (
    instance == NULL || bucket == NULL || bucket->struct_size < sizeof(*bucket)
  ) {
    return MLN_PLUGIN_STATUS_INVALID_ARGUMENT;
  }
  square_layout* layout = instance;
  layout->segment = (mln_plugin_segment_v1){
    .struct_size = sizeof(mln_plugin_segment_v1),
    .vertex_length = layout->vertex_count,
    .index_length = layout->index_count,
  };
  layout->stream = (mln_plugin_vertex_stream_v1){
    .struct_size = sizeof(mln_plugin_vertex_stream_v1),
    .stream_id = SQUARE_VERTEX_STREAM,
    .data = (const uint8_t*)layout->vertices,
    .data_size = layout->vertex_count * sizeof(square_vertex),
    .vertex_count = layout->vertex_count,
    .stride = sizeof(square_vertex),
  };
  layout->attribute = (mln_plugin_attribute_binding_v1){
    .struct_size = sizeof(mln_plugin_attribute_binding_v1),
    .attribute_id = SQUARE_ATTRIBUTE_POSITION,
    .stream_id = SQUARE_VERTEX_STREAM,
    .byte_offset = offsetof(square_vertex, position),
  };
  layout->drawable = (mln_plugin_drawable_descriptor_v1){
    .struct_size = sizeof(mln_plugin_drawable_descriptor_v1),
    .drawable_key = square_drawable_key,
    .shader_id = SQUARE_STRING("square"),
    .attributes = &layout->attribute,
    .attribute_count = 1,
    .segments = &layout->segment,
    .segment_count = 1,
  };
  const bool empty = layout->index_count == 0;
  bucket->vertex_streams = empty ? NULL : &layout->stream;
  bucket->vertex_stream_count = empty ? 0 : 1;
  bucket->indices = layout->indices;
  bucket->index_count = layout->index_count;
  bucket->drawables = empty ? NULL : &layout->drawable;
  bucket->drawable_count = empty ? 0 : 1;
  bucket->query_radius = 0.0f;
  bucket->feature_vertex_ranges = layout->ranges;
  bucket->feature_vertex_range_count = layout->range_count;
  return MLN_PLUGIN_STATUS_OK;
}

static void square_destroy_layout(void* instance) { free(instance); }

static mln_plugin_status square_update_uniform_block(
  const mln_plugin_uniform_context_v1* context, uint32_t uniform_id,
  uint8_t* output, size_t output_size
) {
  if (
    context == NULL || context->struct_size < sizeof(*context) ||
    uniform_id != SQUARE_UNIFORM_BLOCK || output == NULL ||
    output_size != sizeof(square_uniforms)
  ) {
    return MLN_PLUGIN_STATUS_INVALID_ARGUMENT;
  }
  // The host writes the bound paint properties after this returns.
  square_uniforms value = {0};
  memcpy(value.matrix, context->tile_matrix, sizeof(value.matrix));
  value.pixels_to_gl_units[0] = context->pixels_to_gl_units[0];
  value.pixels_to_gl_units[1] = context->pixels_to_gl_units[1];
  memcpy(output, &value, sizeof(value));
  return MLN_PLUGIN_STATUS_OK;
}

// One algorithm in the three shading languages the plugin API accepts. The
// host prepends the binding macros and, per property, an `_IS_UNIFORM` macro
// that selects the uniform or the attribute pair.
#define SQUARE_GLSL_EVALUATE                                \
  "    float radius = u.radius;\n"                          \
  "#if !MLN_PLUGIN_PROPERTY_SQUARE_RADIUS_IS_UNIFORM\n"     \
  "    radius = mix(a_radius.x, a_radius.y, u.radius_t);\n" \
  "#endif\n"                                                \
  "    vec4 color = u.color;\n"                             \
  "#if !MLN_PLUGIN_PROPERTY_SQUARE_COLOR_IS_UNIFORM\n"      \
  "    color = mix(a_color_min, a_color_max, u.color_t);\n" \
  "#endif\n"

#define SQUARE_GLSL_PROJECT                                             \
  "    vec2 center = floor(encoded * 0.5);\n"                           \
  "    vec2 corner = (encoded - 2.0 * center) * 2.0 - 1.0;\n"           \
  "    vec4 projected = u.matrix * vec4(center, 0.0, 1.0);\n"           \
  "    gl_Position = projected + vec4(corner * radius * u.camera.xy * " \
  "projected.w, 0.0, 0.0);\n"                                           \
  "    v_color = color;\n"

#define SQUARE_GLSL_UNIFORMS \
  "    mat4 matrix;\n"       \
  "    vec4 camera;\n"       \
  "    vec4 color;\n"        \
  "    float radius;\n"      \
  "    float radius_t;\n"    \
  "    float color_t;\n"     \
  "    float padding;\n"

static const char square_opengl_vertex[] =
  "in vec2 a_position;\n"
  "#if !MLN_PLUGIN_PROPERTY_SQUARE_RADIUS_IS_UNIFORM\n"
  "in vec2 a_radius;\n"
  "#endif\n"
  "#if !MLN_PLUGIN_PROPERTY_SQUARE_COLOR_IS_UNIFORM\n"
  "in vec4 a_color_min;\n"
  "in vec4 a_color_max;\n"
  "#endif\n"
  "layout(std140) uniform SquareDrawableUBO {\n" SQUARE_GLSL_UNIFORMS
  "} u;\n"
  "out vec4 v_color;\n"
  "void main() {\n" SQUARE_GLSL_EVALUATE
  "    vec2 encoded = a_position;\n" SQUARE_GLSL_PROJECT "}\n";

static const char square_opengl_fragment[] =
  "in vec4 v_color;\n"
  "void main() {\n"
  "    fragColor = v_color;\n"
  "}\n";

static const char square_vulkan_vertex[] =
  "layout(location = 0) in ivec2 a_position;\n"
  "#if !MLN_PLUGIN_PROPERTY_SQUARE_RADIUS_IS_UNIFORM\n"
  "layout(location = 1) in vec2 a_radius;\n"
  "#endif\n"
  "#if !MLN_PLUGIN_PROPERTY_SQUARE_COLOR_IS_UNIFORM\n"
  "layout(location = 2) in vec4 a_color_min;\n"
  "layout(location = 3) in vec4 a_color_max;\n"
  "#endif\n"
  "layout(std140, set = DRAWABLE_UBO_SET_INDEX, binding = "
  "MLN_PLUGIN_UNIFORM_0_BINDING) uniform SquareDrawableUBO "
  "{\n" SQUARE_GLSL_UNIFORMS
  "} u;\n"
  "layout(location = 0) out vec4 v_color;\n"
  "void main() {\n" SQUARE_GLSL_EVALUATE
  "    vec2 encoded = vec2(a_position);\n" SQUARE_GLSL_PROJECT
  "    applySurfaceTransform();\n"
  "}\n";

static const char square_vulkan_fragment[] =
  "layout(location = 0) in vec4 v_color;\n"
  "layout(location = 0) out vec4 fragColor;\n"
  "void main() {\n"
  "    fragColor = v_color;\n"
  "}\n";

static const char square_metal_source[] =
  "struct alignas(16) SquareDrawableUBO {\n"
  "    float4x4 matrix;\n"
  "    float4 camera;\n"
  "    float4 color;\n"
  "    float radius;\n"
  "    float radius_t;\n"
  "    float color_t;\n"
  "    float padding;\n"
  "};\n"
  "struct SquareVertex {\n"
  "    short2 a_position [[attribute(0)]];\n"
  "#if !MLN_PLUGIN_PROPERTY_SQUARE_RADIUS_IS_UNIFORM\n"
  "    float2 a_radius [[attribute(1)]];\n"
  "#endif\n"
  "#if !MLN_PLUGIN_PROPERTY_SQUARE_COLOR_IS_UNIFORM\n"
  "    float4 a_color_min [[attribute(2)]];\n"
  "    float4 a_color_max [[attribute(3)]];\n"
  "#endif\n"
  "};\n"
  "struct SquareVaryings {\n"
  "    float4 position [[position]];\n"
  "    float4 color;\n"
  "};\n"
  "vertex SquareVaryings squareVertex(SquareVertex in [[stage_in]],\n"
  "    constant SquareDrawableUBO& u [[buffer(MLN_PLUGIN_UNIFORM_0_BINDING)]]) "
  "{\n"
  "    SquareVaryings out;\n"
  "    float radius = u.radius;\n"
  "#if !MLN_PLUGIN_PROPERTY_SQUARE_RADIUS_IS_UNIFORM\n"
  "    radius = mix(in.a_radius.x, in.a_radius.y, u.radius_t);\n"
  "#endif\n"
  "    float4 color = u.color;\n"
  "#if !MLN_PLUGIN_PROPERTY_SQUARE_COLOR_IS_UNIFORM\n"
  "    color = mix(in.a_color_min, in.a_color_max, u.color_t);\n"
  "#endif\n"
  "    float2 encoded = float2(in.a_position);\n"
  "    float2 center = floor(encoded * 0.5);\n"
  "    float2 corner = (encoded - 2.0 * center) * 2.0 - 1.0;\n"
  "    float4 projected = u.matrix * float4(center, 0.0, 1.0);\n"
  "    out.position = projected + float4(corner * radius * u.camera.xy * "
  "projected.w, 0.0, 0.0);\n"
  "    out.color = color;\n"
  "    return out;\n"
  "}\n"
  "fragment half4 squareFragment(SquareVaryings in [[stage_in]]) {\n"
  "    return half4(in.color);\n"
  "}\n";

static const mln_plugin_shader_source_v1 square_shader_sources[] = {
  {
    .struct_size = sizeof(mln_plugin_shader_source_v1),
    .backend = MLN_PLUGIN_BACKEND_OPENGL,
    .vertex_source = SQUARE_STRING(square_opengl_vertex),
    .fragment_source = SQUARE_STRING(square_opengl_fragment),
  },
  {
    .struct_size = sizeof(mln_plugin_shader_source_v1),
    .backend = MLN_PLUGIN_BACKEND_VULKAN,
    .vertex_source = SQUARE_STRING(square_vulkan_vertex),
    .fragment_source = SQUARE_STRING(square_vulkan_fragment),
  },
  {
    .struct_size = sizeof(mln_plugin_shader_source_v1),
    .backend = MLN_PLUGIN_BACKEND_METAL,
    .vertex_source = SQUARE_STRING(square_metal_source),
    .vertex_entry_point = SQUARE_STRING("squareVertex"),
    .fragment_entry_point = SQUARE_STRING("squareFragment"),
  },
};

static const mln_plugin_shader_attribute_v1 square_shader_attributes[] = {
  {sizeof(mln_plugin_shader_attribute_v1), SQUARE_ATTRIBUTE_POSITION, 0,
   SQUARE_STRING("a_position"), MLN_PLUGIN_VERTEX_INT16_X2},
  {sizeof(mln_plugin_shader_attribute_v1), SQUARE_ATTRIBUTE_RADIUS, 1,
   SQUARE_STRING("a_radius"), MLN_PLUGIN_VERTEX_FLOAT_X2},
  {sizeof(mln_plugin_shader_attribute_v1), SQUARE_ATTRIBUTE_COLOR_MIN, 2,
   SQUARE_STRING("a_color_min"), MLN_PLUGIN_VERTEX_FLOAT_X4},
  {sizeof(mln_plugin_shader_attribute_v1), SQUARE_ATTRIBUTE_COLOR_MAX, 3,
   SQUARE_STRING("a_color_max"), MLN_PLUGIN_VERTEX_FLOAT_X4},
};

static const mln_plugin_uniform_block_descriptor_v1 square_uniform_blocks[] = {
  {
    .struct_size = sizeof(mln_plugin_uniform_block_descriptor_v1),
    .uniform_id = SQUARE_UNIFORM_BLOCK,
    .name = SQUARE_STRING("SquareDrawableUBO"),
    .byte_size = sizeof(square_uniforms),
    .stage_mask = MLN_PLUGIN_SHADER_STAGE_VERTEX,
    .scope = MLN_PLUGIN_UNIFORM_DRAWABLE,
  },
};

static const mln_plugin_shader_property_binding_v1 square_property_bindings[] =
  {
    {
      .struct_size = sizeof(mln_plugin_shader_property_binding_v1),
      .property_name = SQUARE_STRING("square-radius"),
      .encoding = MLN_PLUGIN_PROPERTY_ENCODING_FLOAT,
      .uniform_id = SQUARE_UNIFORM_BLOCK,
      .uniform_byte_offset = offsetof(square_uniforms, radius),
      .minimum_attribute_id = SQUARE_ATTRIBUTE_RADIUS,
      .maximum_attribute_id = SQUARE_ATTRIBUTE_RADIUS,
      .interpolation_uniform_id = SQUARE_UNIFORM_BLOCK,
      .interpolation_uniform_byte_offset = offsetof(square_uniforms, radius_t),
    },
    {
      .struct_size = sizeof(mln_plugin_shader_property_binding_v1),
      .property_name = SQUARE_STRING("square-color"),
      .encoding = MLN_PLUGIN_PROPERTY_ENCODING_COLOR,
      .uniform_id = SQUARE_UNIFORM_BLOCK,
      .uniform_byte_offset = offsetof(square_uniforms, color),
      .minimum_attribute_id = SQUARE_ATTRIBUTE_COLOR_MIN,
      .maximum_attribute_id = SQUARE_ATTRIBUTE_COLOR_MAX,
      .interpolation_uniform_id = SQUARE_UNIFORM_BLOCK,
      .interpolation_uniform_byte_offset = offsetof(square_uniforms, color_t),
    },
};

static const mln_plugin_shader_descriptor_v1 square_shader = {
  .struct_size = sizeof(mln_plugin_shader_descriptor_v1),
  .shader_id = SQUARE_STRING("square"),
  .sources = square_shader_sources,
  .source_count =
    sizeof(square_shader_sources) / sizeof(*square_shader_sources),
  .attributes = square_shader_attributes,
  .attribute_count =
    sizeof(square_shader_attributes) / sizeof(*square_shader_attributes),
  .uniform_blocks = square_uniform_blocks,
  .uniform_block_count =
    sizeof(square_uniform_blocks) / sizeof(*square_uniform_blocks),
  .property_bindings = square_property_bindings,
  .property_binding_count =
    sizeof(square_property_bindings) / sizeof(*square_property_bindings),
};

static const uint32_t square_expression_capabilities =
  MLN_PLUGIN_EXPRESSION_CAMERA | MLN_PLUGIN_EXPRESSION_FEATURE |
  MLN_PLUGIN_EXPRESSION_COMPOSITE | MLN_PLUGIN_EXPRESSION_FEATURE_STATE;

static const mln_plugin_property_descriptor_v1 square_properties[] = {
  {
    .struct_size = sizeof(mln_plugin_property_descriptor_v1),
    .name = SQUARE_STRING("square-radius"),
    .type = MLN_PLUGIN_VALUE_FLOAT,
    .default_value =
      {sizeof(mln_plugin_value), MLN_PLUGIN_VALUE_FLOAT, {.float_value = 4.0f}},
    .expression_capabilities = square_expression_capabilities,
    .supports_transitions = 1,
    .has_minimum = 1,
    .minimum = 0.0f,
  },
  {
    .struct_size = sizeof(mln_plugin_property_descriptor_v1),
    .name = SQUARE_STRING("square-color"),
    .type = MLN_PLUGIN_VALUE_COLOR,
    .default_value =
      {sizeof(mln_plugin_value),
       MLN_PLUGIN_VALUE_COLOR,
       {.color_value = {0.0f, 0.0f, 0.0f, 1.0f}}},
    .expression_capabilities = square_expression_capabilities,
    .supports_transitions = 1,
  },
};

static const mln_plugin_layer_type_v1 square_layer_type = {
  .struct_size = sizeof(mln_plugin_layer_type_v1),
  .layer_type = SQUARE_STRING("ffi-test-square"),
  .backend_mask = MLN_PLUGIN_BACKEND_OPENGL | MLN_PLUGIN_BACKEND_VULKAN |
                  MLN_PLUGIN_BACKEND_METAL,
  .properties = square_properties,
  .property_count = sizeof(square_properties) / sizeof(*square_properties),
  .geometry_type_mask = MLN_PLUGIN_GEOMETRY_POINT,
  .shaders = &square_shader,
  .shader_count = 1,
  .create_layout = square_create_layout,
  .layout_feature = square_layout_feature,
  .finish_layout = square_finish_layout,
  .destroy_layout = square_destroy_layout,
  .update_uniform_block = square_update_uniform_block,
};

static const mln_plugin_descriptor_v1 square_descriptor = {
  .struct_size = sizeof(mln_plugin_descriptor_v1),
  .abi_version = MLN_PLUGIN_ABI_VERSION_1,
  .plugin_id = SQUARE_STRING("org.maplibre.maplibre-native-ffi.test-square"),
  .plugin_version = SQUARE_STRING("0.0.0"),
  .minimum_host_abi = MLN_PLUGIN_ABI_VERSION_1,
  .maximum_host_abi = MLN_PLUGIN_ABI_VERSION_1,
  .layer_types = &square_layer_type,
  .layer_type_count = 1,
};

// One point at the camera center, which the 64x64 fixture places at (32, 32),
// under a square wide enough to cover the center pixel and no corner.
static const char square_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#ff0000\"}},"
  "{\"id\":\"square\",\"type\":\"ffi-test-square\",\"source\":\"points\","
  "\"paint\":{\"square-color\":\"#00ff00\",\"square-radius\":8}}]}";

static mln_plugin_status register_square_plugin(void) {
  char error[256] = "";
  const mln_plugin_status status = mln_plugin_get_register_function_v1()(
    &square_descriptor, error, sizeof(error)
  );
  TEST_ASSERT_EQUAL_STRING("", error);
  return status;
}

#if defined(MLN_FFI_TEST_BACKEND_WEBGPU)

// The plugin API declares shaders for OpenGL, Vulkan, and Metal, so a WebGPU
// build registers a plugin but has no shader path to render its layers.
static void registration_reaches_the_library_exports(void) {
  TEST_ASSERT_EQUAL_INT(MLN_PLUGIN_STATUS_OK, register_square_plugin());
}

#else

static void a_registered_layer_type_renders_through_the_c_api(void) {
  TEST_ASSERT_EQUAL_INT(MLN_PLUGIN_STATUS_OK, register_square_plugin());

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL(square_style_json))
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  // The source tiles on a worker, so the first rendered frames show only the
  // background. Render until the square lands on the center pixel.
  static uint8_t pixels[64 * 64 * 4];
  const uint8_t* center = pixels + ((32 * 64) + 32) * 4;
  bool square_rendered = false;
  for (unsigned int attempt = 0; attempt < 2000 && !square_rendered;
       attempt += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0, -1));
    mln_render_result result = MLN_RENDER_RESULT_NO_UPDATE;
    bool needs_repaint = false;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_render_session_render_update(fixture.session, &result, &needs_repaint)
    );
    if (result == MLN_RENDER_RESULT_RENDERED) {
      mln_texture_image_info info = {.size = sizeof(mln_texture_image_info)};
      TEST_ASSERT_EQUAL_INT(
        MLN_STATUS_OK, mln_texture_read_premultiplied_rgba8(
                         fixture.session, pixels, sizeof(pixels), &info
                       )
      );
      TEST_ASSERT_EQUAL_UINT32(64, info.width);
      TEST_ASSERT_EQUAL_UINT32(64, info.height);
      square_rendered = center[1] == 255;
    }
    if (!square_rendered) {
      mln_test_sleep_millisecond();
    }
  }
  TEST_ASSERT_TRUE_MESSAGE(
    square_rendered, "the plugin layer never covered the center pixel"
  );
  TEST_ASSERT_EQUAL_UINT8(0, center[0]);
  TEST_ASSERT_EQUAL_UINT8(255, center[1]);
  TEST_ASSERT_EQUAL_UINT8(0, center[2]);
  TEST_ASSERT_EQUAL_UINT8(255, center[3]);
  // The square stops short of the corner, which keeps the background.
  TEST_ASSERT_EQUAL_UINT8(255, pixels[0]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels[1]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels[2]);
  TEST_ASSERT_EQUAL_UINT8(255, pixels[3]);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#endif

void run_plugin_layer_abi_tests(void) {
  UnitySetTestFile(__FILE__);
#if defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  RUN_TEST(registration_reaches_the_library_exports);
#else
  RUN_TEST(a_registered_layer_type_renders_through_the_c_api);
#endif
}
