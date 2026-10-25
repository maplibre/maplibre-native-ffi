// A zeroed struct is the default. Every record default sets only the sizes,
// its own and those of the sized records it embeds, and leaves every other
// member zero. These cases check each member that once defaulted to something
// else.

#include "support/test_support.h"

#define EXPECT_SIZED(type, value) \
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(sizeof(type), (value).size, #type)

static void every_record_default_sets_its_size(void) {
  EXPECT_SIZED(mln_runtime_options, mln_runtime_options_default());
  EXPECT_SIZED(mln_resource_route, mln_resource_route_default());
  EXPECT_SIZED(mln_http_header_rule, mln_http_header_rule_default());
  EXPECT_SIZED(mln_map_options, mln_map_options_default());
  EXPECT_SIZED(mln_camera_options, mln_camera_options_default());
  EXPECT_SIZED(mln_camera_delta, mln_camera_delta_default());
  EXPECT_SIZED(mln_camera_update, mln_camera_update_default());
  EXPECT_SIZED(mln_camera_fit_options, mln_camera_fit_options_default());
  EXPECT_SIZED(mln_bound_options, mln_bound_options_default());
  EXPECT_SIZED(mln_free_camera_options, mln_free_camera_options_default());
  EXPECT_SIZED(mln_projection_mode, mln_projection_mode_default());
  EXPECT_SIZED(mln_map_viewport_options, mln_map_viewport_options_default());
  EXPECT_SIZED(mln_map_tile_options, mln_map_tile_options_default());
  EXPECT_SIZED(
    mln_rendered_feature_query_options,
    mln_rendered_feature_query_options_default()
  );
  EXPECT_SIZED(
    mln_source_feature_query_options, mln_source_feature_query_options_default()
  );
  EXPECT_SIZED(
    mln_geojson_source_options, mln_geojson_source_options_default()
  );
  EXPECT_SIZED(
    mln_custom_geometry_source_options,
    mln_custom_geometry_source_options_default()
  );
  EXPECT_SIZED(
    mln_custom_mvt_vector_source_options,
    mln_custom_mvt_vector_source_options_default()
  );
  EXPECT_SIZED(
    mln_premultiplied_rgba8_image, mln_premultiplied_rgba8_image_default()
  );
  EXPECT_SIZED(mln_style_image_options, mln_style_image_options_default());
  EXPECT_SIZED(
    mln_style_transition_options, mln_style_transition_options_default()
  );
  EXPECT_SIZED(
    mln_style_expression_options, mln_style_expression_options_default()
  );
  EXPECT_SIZED(
    mln_style_expression_inputs, mln_style_expression_inputs_default()
  );
  EXPECT_SIZED(mln_frame_demand, mln_frame_demand_default());
  EXPECT_SIZED(mln_gpu_sync, mln_gpu_sync_default());
  EXPECT_SIZED(
    mln_render_session_attach_options,
    mln_render_session_attach_options_default()
  );
  EXPECT_SIZED(
    mln_metal_surface_descriptor, mln_metal_surface_descriptor_default()
  );
  EXPECT_SIZED(
    mln_vulkan_surface_descriptor, mln_vulkan_surface_descriptor_default()
  );
  EXPECT_SIZED(
    mln_opengl_surface_descriptor, mln_opengl_surface_descriptor_default()
  );
  EXPECT_SIZED(
    mln_webgpu_surface_descriptor, mln_webgpu_surface_descriptor_default()
  );
  EXPECT_SIZED(
    mln_metal_owned_texture_descriptor,
    mln_metal_owned_texture_descriptor_default()
  );
  EXPECT_SIZED(
    mln_metal_borrowed_texture_descriptor,
    mln_metal_borrowed_texture_descriptor_default()
  );
  EXPECT_SIZED(
    mln_vulkan_owned_texture_descriptor,
    mln_vulkan_owned_texture_descriptor_default()
  );
  EXPECT_SIZED(
    mln_vulkan_borrowed_texture_descriptor,
    mln_vulkan_borrowed_texture_descriptor_default()
  );
  EXPECT_SIZED(
    mln_opengl_owned_texture_descriptor,
    mln_opengl_owned_texture_descriptor_default()
  );
  EXPECT_SIZED(
    mln_opengl_borrowed_texture_descriptor,
    mln_opengl_borrowed_texture_descriptor_default()
  );
  EXPECT_SIZED(
    mln_webgpu_owned_texture_descriptor,
    mln_webgpu_owned_texture_descriptor_default()
  );
  EXPECT_SIZED(
    mln_webgpu_borrowed_texture_descriptor,
    mln_webgpu_borrowed_texture_descriptor_default()
  );
  // A camera update embeds sized camera options, whose size it also sets.
  EXPECT_SIZED(mln_camera_options, mln_camera_update_default().camera);
}

static void map_and_render_defaults_leave_every_member_zero(void) {
  const mln_logical_extent extent = mln_logical_extent_default();
  TEST_ASSERT_EQUAL_UINT32(0, extent.width);
  TEST_ASSERT_EQUAL_UINT32(0, extent.height);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, extent.scale_factor);

  const mln_map_options map = mln_map_options_default();
  TEST_ASSERT_EQUAL_UINT32(0, map.fields);
  TEST_ASSERT_EQUAL_UINT32(0, map.initial_extent.width);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, map.initial_extent.scale_factor);
  TEST_ASSERT_EQUAL_UINT64(0, map.event_mask);

  const mln_frame_demand demand = mln_frame_demand_default();
  TEST_ASSERT_EQUAL_UINT32(0, demand.flags);
  TEST_ASSERT_EQUAL_UINT64(0, demand.token);

  const mln_render_session_attach_options attach =
    mln_render_session_attach_options_default();
  TEST_ASSERT_EQUAL_UINT32(0, attach.driver);
  TEST_ASSERT_EQUAL_UINT32(0, attach.requested_texture_ring_depth);
  TEST_ASSERT_EQUAL_UINT32(0, attach.producer_sync_kind);

  const mln_metal_owned_texture_descriptor metal =
    mln_metal_owned_texture_descriptor_default();
  TEST_ASSERT_EQUAL_UINT32(0, metal.extent.width);
  TEST_ASSERT_EQUAL_UINT64(0, metal.pixel_format);
  const mln_metal_borrowed_texture_descriptor metal_borrowed =
    mln_metal_borrowed_texture_descriptor_default();
  TEST_ASSERT_EQUAL_UINT32(0, metal_borrowed.physical_width);
  TEST_ASSERT_EQUAL_UINT32(0, metal_borrowed.physical_height);
  const mln_vulkan_borrowed_texture_descriptor vulkan =
    mln_vulkan_borrowed_texture_descriptor_default();
  TEST_ASSERT_EQUAL_UINT32(0, vulkan.final_layout);
  TEST_ASSERT_EQUAL_UINT32(0, vulkan.physical_width);
  const mln_opengl_owned_texture_descriptor opengl =
    mln_opengl_owned_texture_descriptor_default();
  TEST_ASSERT_EQUAL_UINT32(
    MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED, opengl.context.platform
  );
  TEST_ASSERT_EQUAL_DOUBLE(0.0, opengl.extent.scale_factor);
  const mln_opengl_surface_descriptor opengl_surface =
    mln_opengl_surface_descriptor_default();
  TEST_ASSERT_EQUAL_UINT32(
    MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED, opengl_surface.context.platform
  );
  const mln_webgpu_borrowed_texture_descriptor webgpu =
    mln_webgpu_borrowed_texture_descriptor_default();
  TEST_ASSERT_EQUAL_UINT32(0, webgpu.physical_width);
  TEST_ASSERT_EQUAL_UINT32(
    0, mln_vulkan_surface_descriptor_default().extent.width
  );
}

static void resource_and_style_defaults_leave_every_member_zero(void) {
  TEST_ASSERT_EQUAL_UINT32(0, mln_resource_route_default().kinds);
  TEST_ASSERT_EQUAL_UINT32(0, mln_http_header_rule_default().kinds);

  const mln_geojson_source_options geojson =
    mln_geojson_source_options_default();
  TEST_ASSERT_EQUAL_DOUBLE(0.0, geojson.max_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, geojson.tolerance);
  TEST_ASSERT_EQUAL_UINT32(0, geojson.buffer);
  TEST_ASSERT_EQUAL_UINT32(0, geojson.cluster_radius);
  TEST_ASSERT_EQUAL_UINT32(0, geojson.cluster_min_points);

  const mln_custom_geometry_source_options geometry =
    mln_custom_geometry_source_options_default();
  TEST_ASSERT_EQUAL_DOUBLE(0.0, geometry.max_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, geometry.tolerance);
  TEST_ASSERT_EQUAL_UINT32(0, geometry.tile_size);
  TEST_ASSERT_EQUAL_UINT32(0, geometry.buffer);
  TEST_ASSERT_EQUAL_DOUBLE(
    0.0, mln_custom_mvt_vector_source_options_default().max_zoom
  );

  const mln_style_image_options image = mln_style_image_options_default();
  TEST_ASSERT_EQUAL_UINT32(0, image.text_fit_width);
  TEST_ASSERT_EQUAL_UINT32(0, image.text_fit_height);
  TEST_ASSERT_EQUAL_FLOAT(0.0F, image.pixel_ratio);
  TEST_ASSERT_FALSE(
    mln_style_transition_options_default().enable_placement_transitions
  );

  const mln_animation_options animation = mln_animation_options_default();
  TEST_ASSERT_EQUAL_UINT32(0, animation.fields);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, animation.easing.x2);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, animation.easing.y2);
  TEST_ASSERT_EQUAL_DOUBLE(
    0.0, mln_free_camera_options_default().orientation.w
  );
  TEST_ASSERT_EQUAL_DOUBLE(0.0, mln_camera_delta_default().animation.easing.y2);
  TEST_ASSERT_EQUAL_DOUBLE(
    0.0, mln_camera_update_default().animation.easing.y2
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_CONSTRAIN_MODE_NONE, mln_map_viewport_options_default().constrain_mode
  );
}

MLN_TEST_GROUP {
  RUN_TEST(every_record_default_sets_its_size);
  RUN_TEST(map_and_render_defaults_leave_every_member_zero);
  RUN_TEST(resource_and_style_defaults_leave_every_member_zero);
}
