/**
 * @file maplibre_native_c/gesture.h
 * Recognize gestures from a host's input, and move a map's camera with them.
 *
 * The host translates each platform event on its own, with no memory: which
 * buttons and held keys select which modes, and how far a wheel, a dial, or a
 * key moves the camera. The recognizer interprets the inputs over time:
 * drags, pinches, twists, tilts, taps, releases that glide, and snaps.
 *
 * Each recognizer belongs to one mln_map and moves its camera directly, so
 * the host never relays camera changes.
 */

#ifndef MAPLIBRE_NATIVE_C_GESTURE_H
#define MAPLIBRE_NATIVE_C_GESTURE_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "map.h"
#include "query.h"

#ifdef __cplusplus
extern "C" {
#endif

/** How a part's glide unfolds from its release speed. */
typedef enum mln_momentum_decay_model : uint32_t {
  /**
   * The speed halves every `half_life_ms`, until it drops to `stop_speed`.
   * UIScrollView's normal deceleration is a half-life of 346 ms, and its fast
   * deceleration 69 ms.
   */
  MLN_MOMENTUM_DECAY_MODEL_FRICTION = 0,
  /**
   * The glide lasts `duration_ms` × (speed / `reference_speed`) to the power
   * of `duration_exponent`, and follows `easing`, starting at the release
   * speed. Its defaults are Android's OverScroller fling, in its
   * density-independent pixels: a reference speed of 1000, 555.182 ms, an
   * exponent of 0.7362676, the curve (0.175, 0.5, 0.35, 1), a minimum speed
   * of 50 and a maximum of 8000.
   */
  MLN_MOMENTUM_DECAY_MODEL_CURVE = 1,
} mln_momentum_decay_model;

/** The fields that mln_momentum_decay sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_momentum_decay_field : uint32_t {
  MLN_MOMENTUM_DECAY_MODEL = 1U << 0U,
  MLN_MOMENTUM_DECAY_HALF_LIFE_MS = 1U << 1U,
  MLN_MOMENTUM_DECAY_STOP_SPEED = 1U << 2U,
  MLN_MOMENTUM_DECAY_REFERENCE_SPEED = 1U << 3U,
  MLN_MOMENTUM_DECAY_DURATION_MS = 1U << 4U,
  MLN_MOMENTUM_DECAY_DURATION_EXPONENT = 1U << 5U,
  MLN_MOMENTUM_DECAY_EASING = 1U << 6U,
  MLN_MOMENTUM_DECAY_MIN_SPEED = 1U << 7U,
  MLN_MOMENTUM_DECAY_MAX_SPEED = 1U << 8U,
  MLN_MOMENTUM_DECAY_MAX_DISTANCE = 1U << 9U,
} mln_momentum_decay_field;

/** A set of mln_momentum_decay_field bits. */
typedef uint32_t mln_momentum_decay_fields;

/**
 * How one part of a gesture glides after release.
 *
 * Speeds and distances are in the part's unit: logical pixels for pan, zoom
 * levels for zoom, and degrees for bearing and pitch, with speeds per second.
 * A setting whose bit isn't set keeps its current value. A new `model` starts
 * from its defaults: friction from the part's, which
 * mln_gesture_recognizer_create() lists, and a curve from Android's fling.
 * Settings of the other model are an error. `min_speed` is finite
 * and not negative, `duration_exponent` finite and above -1, and every other
 * number positive; the maximums may be infinite.
 */
typedef struct mln_momentum_decay {
  mln_momentum_decay_fields fields;
  mln_momentum_decay_model model
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_MODEL");
  /** For MLN_MOMENTUM_DECAY_MODEL_FRICTION. */
  double half_life_ms
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_HALF_LIFE_MS");
  /** For MLN_MOMENTUM_DECAY_MODEL_FRICTION, the speed at which it ends. */
  double stop_speed
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_STOP_SPEED");
  /** For MLN_MOMENTUM_DECAY_MODEL_CURVE, the speed that lasts `duration_ms`. */
  double reference_speed
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_REFERENCE_SPEED");
  /** For MLN_MOMENTUM_DECAY_MODEL_CURVE. */
  double duration_ms
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_DURATION_MS");
  /**
   * For MLN_MOMENTUM_DECAY_MODEL_CURVE, how the duration grows with the
   * speed.
   */
  double duration_exponent
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_DURATION_EXPONENT");
  /**
   * For MLN_MOMENTUM_DECAY_MODEL_CURVE, the share of the glide covered over
   * the share of its duration. Its control points are between 0 and 1, and
   * `x1` and `y1` above 0, so the glide starts moving and never passes its
   * end.
   */
  mln_unit_bezier easing
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_EASING");
  /** A slower release doesn't glide. */
  double min_speed MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_MIN_SPEED");
  /** A faster release counts as this. */
  double max_speed MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_MAX_SPEED");
  /**
   * The farthest the part glides. A release that would glide farther counts
   * as the speed that glides this far.
   */
  double max_distance
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_MAX_DISTANCE");
} mln_momentum_decay;

/** The fields that mln_momentum_snap sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_momentum_snap_field : uint32_t {
  MLN_MOMENTUM_SNAP_SPACING = 1U << 0U,
  MLN_MOMENTUM_SNAP_OFFSET = 1U << 1U,
  MLN_MOMENTUM_SNAP_TOLERANCE = 1U << 2U,
  MLN_MOMENTUM_SNAP_DURATION_MS = 1U << 3U,
} mln_momentum_snap_field;

/** A set of mln_momentum_snap_field bits. */
typedef uint32_t mln_momentum_snap_fields;

/**
 * Where a part settles after a gesture that changed it.
 *
 * The targets are every `spacing` from `offset`, in the part's unit. Bearing
 * targets lie within one turn from `offset`, and the bearing takes the
 * shorter way around to the nearest. Once the part's glide ends, or right
 * away when it has none, the part eases to the nearest target if it is within
 * `tolerance` of it, and otherwise stays where it is. A setting whose bit
 * isn't set keeps its current value.
 */
typedef struct mln_momentum_snap {
  mln_momentum_snap_fields fields;
  /** Positive. */
  double spacing MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_SNAP_SPACING");
  /** Finite. */
  double offset MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_SNAP_OFFSET");
  /** Not negative. */
  double tolerance MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_SNAP_TOLERANCE");
  /** Zero settles without easing. */
  double duration_ms
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_SNAP_DURATION_MS");
} mln_momentum_snap;

/**
 * Values every `spacing` from `offset`, in the part's unit, that report an
 * MLN_GESTURE_EVENT_NOTCH as a gesture moves the part to or across one, for
 * the host to play as haptic feedback. Bearing notches lie within one turn
 * from the offset.
 */
typedef struct mln_gesture_notch {
  /** Positive. */
  double spacing;
  /** Finite. */
  double offset;
  /** The host's name for the feedback, which the notch's events carry. */
  uint32_t kind;
} mln_gesture_notch;

/** The fields that mln_gesture_pan_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_gesture_pan_option_field : uint32_t {
  MLN_GESTURE_PAN_OPTION_ENABLED = 1U << 0U,
  MLN_GESTURE_PAN_OPTION_GLIDES = 1U << 1U,
  MLN_GESTURE_PAN_OPTION_MOMENTUM = 1U << 2U,
} mln_gesture_pan_option_field;

/** A set of mln_gesture_pan_option_field bits. */
typedef uint32_t mln_gesture_pan_option_fields;

/** How gestures move the map's content. */
typedef struct mln_gesture_pan_options {
  mln_gesture_pan_option_fields fields;
  /** Whether drags, two fingers moving together, and deltas pan. */
  bool enabled MLN_BINDING("mask=fields;bit=MLN_GESTURE_PAN_OPTION_ENABLED");
  /** Whether a release glides on. */
  bool glides MLN_BINDING("mask=fields;bit=MLN_GESTURE_PAN_OPTION_GLIDES");
  mln_momentum_decay momentum
    MLN_BINDING("mask=fields;bit=MLN_GESTURE_PAN_OPTION_MOMENTUM");
} mln_gesture_pan_options;

/** The fields that mln_gesture_axis_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_gesture_axis_option_field : uint32_t {
  MLN_GESTURE_AXIS_OPTION_ENABLED = 1U << 0U,
  MLN_GESTURE_AXIS_OPTION_GLIDES = 1U << 1U,
  MLN_GESTURE_AXIS_OPTION_MOMENTUM = 1U << 2U,
  MLN_GESTURE_AXIS_OPTION_SNAPS = 1U << 3U,
  MLN_GESTURE_AXIS_OPTION_SNAP = 1U << 4U,
  MLN_GESTURE_AXIS_OPTION_NOTCHES = 1U << 5U,
} mln_gesture_axis_option_field;

/** A set of mln_gesture_axis_option_field bits. */
typedef uint32_t mln_gesture_axis_option_fields;

/** How gestures move the zoom, the bearing, or the pitch. */
typedef struct mln_gesture_axis_options {
  mln_gesture_axis_option_fields fields;
  /** Whether gestures move the part. */
  bool enabled MLN_BINDING("mask=fields;bit=MLN_GESTURE_AXIS_OPTION_ENABLED");
  /** Whether a release glides on. */
  bool glides MLN_BINDING("mask=fields;bit=MLN_GESTURE_AXIS_OPTION_GLIDES");
  mln_momentum_decay momentum
    MLN_BINDING("mask=fields;bit=MLN_GESTURE_AXIS_OPTION_MOMENTUM");
  /** Whether the part settles once its glide ends. */
  bool snaps MLN_BINDING("mask=fields;bit=MLN_GESTURE_AXIS_OPTION_SNAPS");
  mln_momentum_snap snap
    MLN_BINDING("mask=fields;bit=MLN_GESTURE_AXIS_OPTION_SNAP");
  /**
   * Where the part reports notches while gestures, their glides, and their
   * snaps move it, replacing the ones it had. Several notches at one value
   * report one event, with the kind of the first in the list.
   */
  const mln_gesture_notch* notches MLN_BINDING(
    "length=notch_count;mask=fields;bit=MLN_GESTURE_AXIS_OPTION_NOTCHES"
  );
  size_t notch_count;
} mln_gesture_axis_options;

/** The fields that mln_gesture_options sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_gesture_option_field : uint32_t {
  MLN_GESTURE_OPTION_DOUBLE_TAP_ZOOM = 1U << 0U,
  MLN_GESTURE_OPTION_TWO_FINGER_TAP_ZOOM = 1U << 1U,
} mln_gesture_option_field;

/** A set of mln_gesture_option_field bits. */
typedef uint32_t mln_gesture_option_fields;

/**
 * How a gesture recognizer moves each part of the camera.
 *
 * Settings whose bits aren't set keep their values. A new
 * recognizer moves every part, and glides each by friction with a half-life
 * of 140 ms, a minimum speed of 0, and no maximum speed or distance, with
 * these exceptions:
 *
 * - Pan stops at 10 logical pixels per second, needs a release of at least
 *   200, and counts a release faster than 10000 as 10000.
 * - Zoom stops at 0.01 levels per second and glides at most 2.5 levels. Its
 *   snap, when on, settles on the nearest whole level within half a level.
 * - Bearing stops at 1 degree per second and needs a release of at least 60.
 *   It snaps to north within 7 degrees, and has a notch of kind 0 at north.
 * - Pitch stops at 1 degree per second and needs a release of at least 5. Its
 *   snap, when on, settles flat within 7 degrees.
 *
 * Snaps take 300 ms. A double tap zooms in a level and a two-finger tap out.
 */
typedef struct mln_gesture_options {
  uint32_t size;
  mln_gesture_option_fields fields;
  /**
   * Zoom levels that a double tap zooms in, eased; with
   * MLN_GESTURE_MODE_ZOOM_OUT it zooms out instead. 0 leaves double taps to
   * the host. Finite.
   */
  double double_tap_zoom
    MLN_BINDING("mask=fields;bit=MLN_GESTURE_OPTION_DOUBLE_TAP_ZOOM");
  /**
   * Zoom levels that a two-finger tap zooms, eased; negative zooms out. 0
   * leaves two-finger taps to the host. Finite.
   */
  double two_finger_tap_zoom
    MLN_BINDING("mask=fields;bit=MLN_GESTURE_OPTION_TWO_FINGER_TAP_ZOOM");
  mln_gesture_pan_options pan;
  mln_gesture_axis_options zoom;
  mln_gesture_axis_options bearing;
  mln_gesture_axis_options pitch;
} mln_gesture_options;

/** What kind of device a pointer is. */
typedef enum mln_pointer_kind : uint32_t {
  MLN_POINTER_KIND_MOUSE = 0,
  /** Two touches pinch, twist, and tilt, and a held second tap zooms. */
  MLN_POINTER_KIND_TOUCH = 1,
  MLN_POINTER_KIND_PEN = 2,
} mln_pointer_kind;

/**
 * What a one-pointer drag or a double tap does instead of its default, from
 * the pointer's held buttons or from held keys. A drag reads its modes when
 * it starts and keeps them until it ends, and a double tap when it lifts;
 * rotating wins over a box zoom.
 */
typedef enum MLN_BINDING("kind=bitmask") mln_gesture_mode : uint32_t {
  /** Sideways turns the map and up and down tilts it. */
  MLN_GESTURE_MODE_ROTATE = 1U << 0U,
  /** Draws a box that the map zooms to when the pointer lifts. */
  MLN_GESTURE_MODE_BOX_ZOOM = 1U << 1U,
  /** A double tap zooms out instead of in. */
  MLN_GESTURE_MODE_ZOOM_OUT = 1U << 2U,
} mln_gesture_mode;

/** A set of mln_gesture_mode bits. */
typedef uint32_t mln_gesture_modes;

/** The kind of an mln_gesture_input. */
typedef enum mln_gesture_input_type : uint32_t {
  MLN_GESTURE_INPUT_POINTER_DOWN = 1,
  MLN_GESTURE_INPUT_POINTER_MOVE = 2,
  MLN_GESTURE_INPUT_POINTER_UP = 3,
  /**
   * The platform or the host took the pointer away, as on a system gesture,
   * lost capture, or Escape during a box zoom: no tap and no glide.
   */
  MLN_GESTURE_INPUT_POINTER_CANCEL = 4,
  /** The modes that held keys select, for every pointer. */
  MLN_GESTURE_INPUT_MODES = 5,
  MLN_GESTURE_INPUT_DELTA = 6,
  MLN_GESTURE_INPUT_PINCH = 7,
} mln_gesture_input_type;

/**
 * A pointer went down. The host sends no pointer whose buttons mean nothing,
 * such as a mouse's middle button.
 */
typedef struct mln_gesture_pointer_down {
  uint32_t id;
  mln_pointer_kind kind;
  mln_screen_point position;
  /** The modes of the pointer's held buttons. */
  mln_gesture_modes modes;
} mln_gesture_pointer_down;

/** A pointer moved, went up, or was taken away. */
typedef struct mln_gesture_pointer {
  uint32_t id;
  /** Unused for MLN_GESTURE_INPUT_POINTER_CANCEL. */
  mln_screen_point position;
} mln_gesture_pointer;

/**
 * Camera movement from anything but a pointer, such as a wheel, a trackpad
 * scroll, a dial, a crown, keys, or buttons, which the host converts into
 * camera units. Zooms and turns hold `position` in place.
 *
 * A continuous delta, such as a trackpad scroll, applies at once. A discrete
 * one, such as a wheel detent or a key press, eases, and a quick run of them
 * merges into one ease that speeds up. Deltas are one gesture until they
 * idle. The recommended amounts are a third of a level per wheel detent,
 * 1/300 of a level per pixel of trackpad scrolling, and, for keys, 100 logical
 * pixels of pan, a level of zoom, 15 degrees of bearing, and 10 of pitch.
 */
typedef struct mln_gesture_delta {
  mln_screen_point position;
  bool discrete;
  /** How far the map's content moves on the screen, in logical pixels. */
  mln_screen_point pan;
  /** Zoom levels to add. */
  double zoom;
  /** Degrees to turn clockwise. */
  double bearing;
  /** Degrees to tilt. */
  double pitch;
} mln_gesture_delta;

/** Where a platform-recognized pinch is. */
typedef enum mln_pinch_phase : uint32_t {
  MLN_PINCH_PHASE_BEGIN = 0,
  MLN_PINCH_PHASE_UPDATE = 1,
  /** The fingers lifted: the pinch glides on. */
  MLN_PINCH_PHASE_END = 2,
  /**
   * The platform took the pinch away: nothing continues, and the camera's
   * animations stop.
   */
  MLN_PINCH_PHASE_CANCEL = 3,
} mln_pinch_phase;

/**
 * A pinch the platform recognized, such as a macOS trackpad's magnify and
 * rotate or a browser's Ctrl+wheel. Each update zooms by `scale` (above 1
 * zooms in) and turns `rotation` degrees clockwise on the screen since the
 * last, around `center`. It glides on when it ends, as a touch pinch does.
 */
typedef struct mln_gesture_pinch {
  mln_screen_point center;
  double scale;
  double rotation;
  mln_pinch_phase phase;
} mln_gesture_pinch;

/** The input of an mln_gesture_input. */
typedef union mln_gesture_input_data {
  mln_gesture_pointer_down pointer_down
    MLN_BINDING("variant=MLN_GESTURE_INPUT_POINTER_DOWN");
  mln_gesture_pointer pointer_move
    MLN_BINDING("variant=MLN_GESTURE_INPUT_POINTER_MOVE");
  mln_gesture_pointer pointer_up
    MLN_BINDING("variant=MLN_GESTURE_INPUT_POINTER_UP");
  mln_gesture_pointer pointer_cancel
    MLN_BINDING("variant=MLN_GESTURE_INPUT_POINTER_CANCEL");
  mln_gesture_modes modes MLN_BINDING("variant=MLN_GESTURE_INPUT_MODES");
  mln_gesture_delta delta MLN_BINDING("variant=MLN_GESTURE_INPUT_DELTA");
  mln_gesture_pinch pinch MLN_BINDING("variant=MLN_GESTURE_INPUT_PINCH");
} mln_gesture_input_data;

/**
 * One input, in logical pixels from the map's top-left corner.
 *
 * When the view loses focus, the host cancels the pointers it sent and sends
 * empty modes.
 */
typedef struct mln_gesture_input {
  uint32_t size;
  mln_gesture_input_type type;
  mln_gesture_input_data data MLN_BINDING("tag=type");
} mln_gesture_input;

/** The kind of an mln_gesture_event. */
typedef enum mln_gesture_event_type : uint32_t {
  /** A pointer pressed and lifted without moving the map, before 500 ms. */
  MLN_GESTURE_EVENT_TAP = 1,
  /** The second tap of a double tap. */
  MLN_GESTURE_EVENT_DOUBLE_TAP = 2,
  /** Two fingers tapped together. */
  MLN_GESTURE_EVENT_TWO_FINGER_TAP = 3,
  /**
   * A gesture, its glide, or its snap moved a part to or across a notch. A
   * gesture that starts at a notch reports it only once it leaves and comes
   * back, a part reports at most one notch every 50 ms, and box zooms report
   * none.
   */
  MLN_GESTURE_EVENT_NOTCH = 4,
  /**
   * A pointer stayed pressed and still for 500 ms. Its lift makes no tap. The
   * recognizer reports it from mln_gesture_recognizer_tick() while the
   * pointer is down, or on its lift without a tick in time.
   */
  MLN_GESTURE_EVENT_LONG_PRESS = 5,
} mln_gesture_event_type;

/** A tap, at `position`, by the pointer `id`. */
typedef struct mln_gesture_tap {
  /** Unused for MLN_GESTURE_EVENT_TWO_FINGER_TAP. */
  uint32_t id;
  mln_screen_point position;
} mln_gesture_tap;

/** The part of the camera that a notch is on. */
typedef enum mln_gesture_axis : uint32_t {
  MLN_GESTURE_AXIS_ZOOM = 0,
  MLN_GESTURE_AXIS_BEARING = 1,
  MLN_GESTURE_AXIS_PITCH = 2,
} mln_gesture_axis;

/** A notch that a part reached. */
typedef struct mln_gesture_notch_event {
  mln_gesture_axis axis;
  /** The notch's kind, from the options. */
  uint32_t kind;
  /** In the part's unit; a bearing from 0 up to 360 degrees. */
  double value;
} mln_gesture_notch_event;

/** The event of an mln_gesture_event. */
typedef union mln_gesture_event_data {
  mln_gesture_tap tap MLN_BINDING("variant=MLN_GESTURE_EVENT_TAP");
  mln_gesture_tap double_tap
    MLN_BINDING("variant=MLN_GESTURE_EVENT_DOUBLE_TAP");
  mln_gesture_tap two_finger_tap
    MLN_BINDING("variant=MLN_GESTURE_EVENT_TWO_FINGER_TAP");
  mln_gesture_notch_event notch MLN_BINDING("variant=MLN_GESTURE_EVENT_NOTCH");
  mln_gesture_tap long_press
    MLN_BINDING("variant=MLN_GESTURE_EVENT_LONG_PRESS");
} mln_gesture_event_data;

/**
 * What a gesture means beyond moving the camera. Double taps and two-finger
 * taps also zoom, as the options say.
 */
typedef struct mln_gesture_event {
  mln_gesture_event_type type;
  mln_gesture_event_data data MLN_BINDING("tag=type");
} mln_gesture_event;

/** The most events that one input or tick reports. */
#define MLN_GESTURE_RESULT_EVENT_CAPACITY 4

/** When a gesture recognizer next needs mln_gesture_recognizer_tick(). */
typedef enum mln_gesture_next_tick : uint32_t {
  /** Not until the next input. */
  MLN_GESTURE_NEXT_TICK_NONE = 0,
  /**
   * Every display frame, while a delta eases or a gesture, its glide, or its
   * snap may reach a notch.
   */
  MLN_GESTURE_NEXT_TICK_FRAME = 1,
  /**
   * At `next_tick_ms`, on the inputs' clock: when a press becomes a long
   * press, or idle deltas end.
   */
  MLN_GESTURE_NEXT_TICK_AT = 2,
} mln_gesture_next_tick;

/** What an input or a tick did. */
typedef struct mln_gesture_result {
  uint32_t size;
  /**
   * Whether the recognizer used the input, so the host stops the platform's
   * default for it. Ticks report false.
   */
  bool handled;
  mln_gesture_next_tick next_tick;
  /** For MLN_GESTURE_NEXT_TICK_AT. */
  double next_tick_ms;
  /**
   * Whether a box-zoom drag is drawing `box_zoom`, which the host shows, in
   * logical pixels corner to corner.
   */
  bool box_zoom_active;
  mln_screen_box box_zoom;
  /** The events that this input or tick reported, in order. */
  size_t event_count;
  mln_gesture_event events[MLN_GESTURE_RESULT_EVENT_CAPACITY] MLN_BINDING(
    "length=event_count"
  );
} mln_gesture_result;

/**
 * Creates a gesture recognizer that moves `map`'s camera.
 *
 * Its moves go to the map as camera commands, which apply asynchronously, and
 * the map's snapshot and camera events report them as gestures. After the map
 * is released, its moves do nothing. `options` may be null for every default.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when an option is out of range.
 */
MLN_API mln_status mln_gesture_recognizer_create(
  mln_map map, const mln_gesture_options* options MLN_BINDING("nullable=true"),
  mln_gesture_recognizer* out_recognizer MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Releases a gesture recognizer. A glide it started keeps going. */
MLN_API void mln_gesture_recognizer_release(
  mln_gesture_recognizer recognizer
) MLN_NOEXCEPT;

/**
 * Changes the settings whose bits are set, from the next input on.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when an option is out of range.
 */
MLN_API mln_status mln_gesture_recognizer_set_options(
  mln_gesture_recognizer recognizer, const mln_gesture_options* options,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Handles one input at `time_ms`, in milliseconds on any clock that only goes
 * forward, the same for every input and frame.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when `input` isn't a valid input.
 */
MLN_API mln_status mln_gesture_recognizer_handle(
  mln_gesture_recognizer recognizer, const mln_gesture_input* input,
  double time_ms, mln_gesture_result* out_result MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Times long presses, steps the eases of discrete deltas, ends deltas once
 * they idle, and samples the camera for notches, at `time_ms` on the inputs'
 * clock. Call it when the last result's `next_tick` says: on the next display
 * frame, or at its time with a frame or a timer. A tick at any other time does
 * no harm.
 */
MLN_API mln_status mln_gesture_recognizer_tick(
  mln_gesture_recognizer recognizer, double time_ms,
  mln_gesture_result* out_result MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_GESTURE_H
