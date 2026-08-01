// Applying one camera two ways: immediately, and over 800 milliseconds.

#include <maplibre_native_c.h>

static mln_camera_options downtown(void) {
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  camera.latitude = 37.7749;
  camera.longitude = -122.4194;
  camera.zoom = 13.0;
  camera.bearing = 12.0;
  camera.pitch = 30.0;
  return camera;
}

mln_status jump_downtown(mln_map map) {
  const mln_camera_options camera = downtown();
  return mln_map_jump_to(map, &camera);
}

mln_status ease_downtown(mln_map map, uint64_t transition_id) {
  const mln_camera_options camera = downtown();

  mln_animation_options animation = mln_animation_options_default();
  animation.fields = MLN_ANIMATION_OPTION_DURATION |
                     MLN_ANIMATION_OPTION_EASING |
                     MLN_ANIMATION_OPTION_TRANSITION_ID;
  animation.duration_ms = 800.0;
  animation.easing =
    (mln_unit_bezier){.x1 = 0.25, .y1 = 0.1, .x2 = 0.25, .y2 = 1.0};
  animation.transition_id = transition_id;

  return mln_map_ease_to(map, &camera, &animation);
}

bool transition_finished(
  const mln_runtime_event* event, uint64_t transition_id
) {
  if (
    event->type != MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED ||
    event->payload_type != MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED
  ) {
    return false;
  }
  const mln_runtime_event_camera_transition_finished* finished = event->payload;
  return finished->transition_id == transition_id;
}
