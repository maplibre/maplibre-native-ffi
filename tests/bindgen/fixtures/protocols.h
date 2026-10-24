/*
 * Protocol fixtures for the binding generator tests.
 *
 * The base declarations below stand in for the C API's shared types. Each
 * group after them declares one representative of a generated protocol shape
 * and is off unless its MLN_PROTOCOL_* macro is defined, so a test parses only
 * the shapes it exercises. tests/bindgen/support.py prepends the defines, and
 * protocols_stub.c implements the groups the executed probes call.
 */
#ifndef MLN_BINDGEN_PROTOCOLS_H
#define MLN_BINDGEN_PROTOCOLS_H

// Groups that declare fixed-width types include the standard headers.
#if defined(MLN_PROTOCOL_CONVENTIONS) || defined(MLN_PROTOCOL_PLAN_NAMES) || \
  defined(MLN_PROTOCOL_CALLER_SIZED_OUTPUT) ||                               \
  defined(MLN_PROTOCOL_CALLER_SIZED_TEXT)
#define MLN_PROTOCOL_STANDARD_TYPES
#endif
#if defined(MLN_PROTOCOL_VALUES) || defined(MLN_PROTOCOL_KEYWORDS) ||    \
  defined(MLN_PROTOCOL_PRESENCE_MASK) ||                                 \
  defined(MLN_PROTOCOL_COMPLETION_RUNTIME) ||                            \
  defined(MLN_PROTOCOL_ABI_VERSION) || defined(MLN_PROTOCOL_DEFAULTS) || \
  defined(MLN_PROTOCOL_DEFAULT_REGISTRATION) ||                          \
  defined(MLN_PROTOCOL_STRIDED_RECORDS) || defined(MLN_PROTOCOL_VERSIONING)
#define MLN_PROTOCOL_STANDARD_TYPES
#endif
#if defined(MLN_PROTOCOL_DECISION)
#define MLN_PROTOCOL_DECLINABLE_REGISTRATION
#endif
// Groups whose registrations carry arguments to the host offer queue delivery,
// as every such registration in the C API does.
#if defined(MLN_PROTOCOL_RETAINED_REGISTRATION) || \
  defined(MLN_PROTOCOL_OWNED_ARGUMENT) ||          \
  defined(MLN_PROTOCOL_RECORD_NOTIFICATION) ||     \
  defined(MLN_PROTOCOL_DECLINABLE_REGISTRATION) || \
  defined(MLN_PROTOCOL_DELIVERY)
#define MLN_PROTOCOL_QUEUE
#endif
#if defined(MLN_PROTOCOL_QUEUE)
#define MLN_PROTOCOL_STANDARD_TYPES
#endif
#if (                                            \
  defined(MLN_PROTOCOL_RETAINED_REGISTRATION) || \
  defined(MLN_PROTOCOL_OWNED_ARGUMENT)           \
) &&                                             \
  !defined(MLN_PROTOCOL_MAP_RELEASE)
#define MLN_PROTOCOL_MAP_CLOSE
#endif
#if defined(MLN_PROTOCOL_RECORD_NOTIFICATION) && \
  !defined(MLN_PROTOCOL_MAP_RELEASE)
#define MLN_PROTOCOL_MAP_CLOSE
#endif

#define BIND(x) __attribute__((annotate("mln:" x)))

// Tests that redefine the portable scalar typedefs leave these headers out.
#ifdef MLN_PROTOCOL_STANDARD_TYPES
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MLN_PROTOCOL_SIZE size_t
#else
#define MLN_PROTOCOL_SIZE unsigned long
#endif

typedef enum mln_status : int {
  MLN_STATUS_OK = 0,
  MLN_STATUS_INVALID_ARGUMENT = -1,
  MLN_STATUS_INVALID_STATE = -2,
  MLN_STATUS_WRONG_THREAD = -3,
  MLN_STATUS_UNSUPPORTED = -4,
  MLN_STATUS_NATIVE_ERROR = -5,
  MLN_STATUS_CANCELLED = -6,
  MLN_STATUS_BUSY = -7,
  MLN_STATUS_TARGET_LOST = -8,
  MLN_STATUS_NOT_READY = -9,
  MLN_STATUS_NOT_FOUND = -10,
} mln_status;
typedef struct mln_diagnostic {
  unsigned int size;
  char message[4096];
} mln_diagnostic;
typedef struct mln_buffer_view {
  const void* data;
  MLN_PROTOCOL_SIZE size;
} mln_buffer_view;

#if defined(MLN_PROTOCOL_QUEUE) || defined(MLN_PROTOCOL_COMPLETION_RUNTIME)
typedef unsigned long long mln_delivery_queue BIND(
  "kind=handle;release=mln_delivery_queue_release;"
  "dispose=mln_delivery_queue_release"
);
void mln_delivery_queue_release(mln_delivery_queue queue);
#endif

#ifdef MLN_PROTOCOL_QUEUE
// The delivery batch and payload kinds that the registrations below use.
typedef unsigned long long mln_delivery_batch BIND(
  "kind=handle;release=mln_delivery_batch_release;"
  "dispose=mln_delivery_batch_release"
);
typedef enum mln_delivery_type : uint32_t {
  MLN_DELIVERY_TYPE_NOTICE = 1,
  MLN_DELIVERY_TYPE_TICKET = 2,
  MLN_DELIVERY_TYPE_RETIRED = 3,
  MLN_DELIVERY_TYPE_SAMPLE = 4,
  MLN_DELIVERY_TYPE_QUERY = 5,
  MLN_DELIVERY_TYPE_READING = 6,
  MLN_DELIVERY_TYPE_TICKET_CANCEL = 7,
} mln_delivery_type;
#endif

#ifdef MLN_PROTOCOL_COMPLETION_RUNTIME
// The completion types as the C API declares them, for a probe that compiles
// a binding's handwritten completion runtime.
typedef enum mln_command_disposition : uint32_t {
  MLN_COMMAND_DISPOSITION_COMMITTED = 0,
  MLN_COMMAND_DISPOSITION_SUPERSEDED = 1,
  MLN_COMMAND_DISPOSITION_FAILED = 2,
  MLN_COMMAND_DISPOSITION_CANCELLED = 3,
} mln_command_disposition;
typedef struct mln_completion_result {
  uint32_t size;
  int status BIND("enum=mln_status");
  uint32_t disposition BIND("enum=mln_command_disposition");
  uint32_t value_size;
  uint64_t generation;
  uint64_t render_update_generation;
  mln_buffer_view diagnostic;
  const void* value;
  size_t value_count;
} mln_completion_result;
typedef void (*mln_completion_callback)(
  void* user_data, const mln_completion_result* result
);
typedef void (*mln_user_data_release)(void* user_data);
typedef struct mln_completion {
  uint32_t size;
  mln_completion_callback callback;
  void* user_data BIND("kind=context");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue BIND("kind=delivery_queue");
} mln_completion BIND("kind=callback_registration;release=release_user_data");
#else
typedef struct mln_completion {
  void* state;
} mln_completion;
#endif
typedef unsigned long long mln_runtime;

#ifdef MLN_PROTOCOL_ABI_VERSION
// The C ABI version query, for a probe that compiles a binding's handwritten
// runtime, which checks the version before its first call.
uint32_t mln_c_version(void);
#endif

// The map is a plain value unless a test asks for one of its owner forms.
#if defined(MLN_PROTOCOL_MAP_CLOSE)
typedef unsigned long long mln_map
  BIND("kind=handle;release=mln_map_close;dispose=mln_map_close");
void mln_map_close(mln_map map);
#elif defined(MLN_PROTOCOL_MAP_RELEASE)
typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_release");
void mln_map_release(mln_map map);
#else
typedef unsigned long long mln_map;
#endif

#ifdef MLN_PROTOCOL_VALUES
// Copied values: a nested record behind a presence bit, nullable and required
// counted arrays.
typedef struct mln_probe_point {
  double type;
  double gain;
} mln_probe_point;
typedef enum BIND("kind=bitmask") mln_probe_option_field : uint32_t {
  MLN_PROBE_OPTION_POINT = 1u << 0u,
} mln_probe_option_field;
typedef struct mln_probe_options {
  mln_buffer_view title BIND("nullable=true");
  uint32_t fields BIND("enum=mln_probe_option_field");
  mln_probe_point point BIND("mask=fields;bit=MLN_PROBE_OPTION_POINT");
  const mln_probe_point* left BIND("length=left_count;nullable=true");
  size_t left_count;
  const mln_probe_point* right BIND("length=right_count");
  size_t right_count;
} mln_probe_options;
mln_status mln_probe_roundtrip(
  mln_probe_options input, mln_probe_options* out_options BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_DEFAULTS
// A record whose native default holds nonzero values: a scalar of each kind,
// an enumerator, a bitmask with one flag and with several, and a nested record
// with defaults of its own. Bindings copy the native default.
typedef enum mln_probe_mode : uint32_t {
  MLN_PROBE_MODE_FIRST = 1,
  MLN_PROBE_MODE_SECOND = 2,
} mln_probe_mode;
typedef enum BIND("kind=bitmask") mln_probe_flag : uint32_t {
  MLN_PROBE_FLAG_NONE = 0,
  MLN_PROBE_FLAG_NORTH = 1,
  MLN_PROBE_FLAG_SOUTH = 2,
  MLN_PROBE_FLAG_ALL = 3,
} mln_probe_flag;
typedef struct mln_probe_extent {
  uint32_t width;
  double scale;
  uint32_t fit BIND("enum=mln_probe_mode");
} mln_probe_extent;
typedef struct mln_probe_settings {
  uint32_t size;
  mln_probe_extent extent;
  uint32_t mode BIND("enum=mln_probe_mode");
  uint32_t flags BIND("enum=mln_probe_flag");
  uint32_t heading BIND("enum=mln_probe_flag");
  float ratio;
  int32_t offset;
  bool enabled;
  int32_t count;
} mln_probe_settings;
mln_probe_settings mln_probe_settings_default(void);
// Succeeds when every field of `settings` equals the native default, and
// otherwise names the first field that differs.
mln_status mln_probe_settings_check(
  mln_probe_settings settings, mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_DEFAULT_REGISTRATION
// A record default that holds a callback registration with its callback null.
// A copy of the default keeps the limit and leaves the registration unset.
typedef void (*mln_probe_notify)(void* user_data);
typedef void (*mln_probe_notify_release)(void* user_data);
typedef struct mln_probe_signal {
  mln_probe_notify callback BIND("nullable=true");
  void* user_data BIND("kind=context");
  mln_probe_notify_release release_user_data;
} mln_probe_signal BIND("kind=callback_registration;release=release_user_data");
typedef struct mln_probe_hooks {
  uint32_t size;
  uint32_t limit;
  mln_probe_signal signal;
} mln_probe_hooks;
mln_probe_hooks mln_probe_hooks_default(void);
#endif

#ifdef MLN_PROTOCOL_KEYWORDS
// Parameter and field names that are target-language keywords or collide
// with generated locals.
typedef struct mln_keyword_entry {
  double type;
  double defer;
  double raw;
} mln_keyword_entry;
mln_status mln_keyword_combine(
  double defer, double self, double raw, double bindingArg0,
  mln_keyword_entry* out_entry BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_CALLER_SIZED_OUTPUT
// Output arrays that the caller sizes from an input array's count, with record
// and scalar elements.
typedef struct mln_probe_batch_point {
  double x;
  double y;
} mln_probe_batch_point;
mln_status mln_probe_batch_points(
  const mln_probe_batch_point* points BIND("length=point_count"),
  uint32_t point_count,
  mln_probe_batch_point* out_points BIND("direction=out;length=point_count"),
  mln_diagnostic* out_diagnostic
);
mln_status mln_probe_batch_scores(
  const mln_probe_batch_point* points BIND("length=point_count"),
  uint32_t point_count,
  double* out_scores BIND("direction=out;length=point_count"),
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_CALLER_SIZED_TEXT
// JSON text in storage that the caller sizes. Native reports the text's length
// whether or not it fits, so a binding calls again with enough storage.
mln_status mln_probe_zeros_json(
  uint32_t count,
  char* out_json BIND(
    "direction=out;encoding=json;length=json_capacity;"
    "result_length=out_json_length"
  ),
  size_t json_capacity, size_t* out_json_length BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_PRESENCE_MASK
// A bitmask presence mask whose bit guards an embedded record, in a record with
// a default constructor that a caller passes by pointer. A snapshot nests it
// beside a borrowed array.
typedef enum BIND("kind=bitmask") mln_camera_field : uint64_t {
  MLN_CAMERA_CENTER = 1ULL << 40,
  MLN_CAMERA_ZOOM = 2
} mln_camera_field;
typedef struct mln_lat_lng {
  double latitude;
  double longitude;
} mln_lat_lng;
typedef struct mln_camera {
  uint32_t abi_size BIND("kind=size");
  uint64_t fields BIND("enum=mln_camera_field");
  mln_lat_lng center BIND("mask=fields;bit=MLN_CAMERA_CENTER");
  double zoom BIND("mask=fields;bit=MLN_CAMERA_ZOOM");
} mln_camera;
typedef struct mln_snapshot {
  mln_camera camera;
  uint64_t generation;
  const mln_lat_lng* coordinates BIND("length=coordinate_count");
  size_t coordinate_count;
} mln_snapshot;
mln_camera mln_camera_default(void);
mln_status mln_map_jump(
  mln_map map, const mln_camera* camera, mln_diagnostic* out_diagnostic
);
BIND("execution=query;result=mln_snapshot")
mln_status mln_map_snapshot(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_COMPLETION_RESULTS
// Completions in each shape of the native result table: one value, a nullable
// value, an array, a nullable array, an operation that delivers only a status,
// and a command, which the table leaves out.
typedef struct mln_tile_id {
  double x;
  double y;
} mln_tile_id;
BIND("execution=query;result=double")
mln_status mln_map_zoom(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
);
BIND("execution=query;result=mln_tile_id;nullable=true")
mln_status mln_map_find_tile(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
);
BIND("execution=query;result=mln_buffer_view;shape=array")
mln_status mln_map_list_names(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
);
BIND("execution=query;result=mln_tile_id;shape=array;nullable=true")
mln_status mln_map_visible_tiles(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
);
BIND("execution=operation")
mln_status mln_map_reload(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
);
BIND("execution=command")
mln_status mln_map_refresh(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_CONVENTIONS
// Declarations annotated only where they depart from convention: a versioned
// struct with its size and reserved members, UTF-8 text, a presence mask that
// a member names, a default constructor, a record passed by pointer, and a
// command that completes without a value.
typedef enum BIND("kind=bitmask") mln_label_field : uint32_t {
  MLN_LABEL_FIELD_TEXT_SIZE = 1
} mln_label_field;
typedef struct mln_label {
  uint32_t size;
  uint32_t fields BIND("enum=mln_label_field");
  mln_buffer_view text;
  double text_size BIND("mask=fields;bit=MLN_LABEL_FIELD_TEXT_SIZE");
  uint32_t reserved BIND("kind=reserved");
} mln_label;
mln_label mln_label_default(void);
BIND("execution=command")
mln_status mln_map_set_label(
  mln_map map, const mln_label* label, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_PLAN_NAMES
// Names that the semantic plan derives once for every binding: a handle whose
// operations begin with its type name, a record whose field
// order is its meaning, and a mask flag whose record name shares no prefix with
// its bit constants.
typedef uint64_t mln_pass
  BIND("kind=handle;release=mln_pass_close;dispose=mln_pass_close");
void mln_pass_close(mln_pass pass);
mln_status mln_pass_redeem(mln_pass pass, mln_diagnostic* out_diagnostic);
mln_status mln_pass_stamp(mln_pass pass, mln_diagnostic* out_diagnostic);
typedef struct mln_point {
  double x;
  double y;
} mln_point;
typedef enum BIND("kind=bitmask") mln_frame_window_field : uint32_t {
  MLN_FRAME_WINDOW_FIELD_VIEW_ORIGIN = 1u << 0u,
  MLN_FRAME_WINDOW_FIELD_SCALE = 1u << 1u,
  MLN_FRAME_WINDOW_FIELD_LOCKED = 1u << 2u,
} mln_frame_window_field;
typedef struct mln_frame_window {
  uint32_t size;
  uint32_t fields BIND("enum=mln_frame_window_field");
  mln_point view_origin
    BIND("mask=fields;bit=MLN_FRAME_WINDOW_FIELD_VIEW_ORIGIN");
  double scale BIND("mask=fields;bit=MLN_FRAME_WINDOW_FIELD_SCALE");
} mln_frame_window;
mln_status mln_pass_set_window(
  mln_pass pass, const mln_frame_window* window, mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_TAGGED_UNION
// A tagged union whose tag may hold a value this header does not declare.
typedef enum mln_event_kind {
  MLN_EVENT_NONE = 0,
  MLN_EVENT_FRAME = 1
} mln_event_kind;
typedef struct mln_event_frame {
  double timestamp;
} mln_event_frame;
typedef union mln_event_payload {
  mln_event_frame frame BIND("variant=MLN_EVENT_FRAME");
} mln_event_payload;
typedef struct mln_event {
  unsigned int kind BIND("enum=mln_event_kind");
  mln_event_payload payload BIND("tag=kind");
} mln_event;
BIND("execution=query;result=mln_event")
mln_status mln_map_event(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_CHILD_OWNER
// A child owner created by its parent's operation.
typedef unsigned long long mln_measurement BIND(
  "kind=handle;release=mln_measurement_close;"
  "dispose=mln_measurement_close"
);
typedef unsigned long long mln_sample_handle BIND(
  "kind=handle;release=mln_sample_close;dispose=mln_sample_close;"
  "parent=mln_measurement"
);
mln_status mln_measurement_create(
  mln_measurement* out_owner BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
void mln_measurement_close(mln_measurement owner);
void mln_sample_close(mln_sample_handle sample);
mln_status mln_measurement_read(
  mln_measurement owner, double* out_value BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
mln_status mln_measurement_take_sample(
  mln_measurement measurement,
  mln_sample_handle* out_sample BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_RETAINED_REGISTRATION
// A callback registration record retained until native quiescence.
typedef struct mln_sample_point {
  unsigned int x;
  unsigned int y;
} mln_sample_point;
BIND("delivery=MLN_DELIVERY_TYPE_SAMPLE")
typedef void (*mln_sample_notification)(void* context, mln_sample_point point);
typedef void (*mln_sample_release)(void* context);
typedef struct mln_sample_options {
  unsigned int size;
  mln_sample_notification changed;
  void* context BIND("kind=context");
  mln_sample_release release;
  mln_delivery_queue queue BIND("kind=delivery_queue");
} mln_sample_options BIND("kind=callback_registration;release=release");
BIND("execution=command")
mln_status mln_map_observe_sample(
  mln_map map, const mln_sample_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
);
typedef struct mln_sample_delivery {
  mln_sample_point point;
} mln_sample_delivery;
#endif

#ifdef MLN_PROTOCOL_OWNED_ARGUMENT
// A callback that owns a handle it receives, answers through it inline or
// later, and releases it.
typedef unsigned long long mln_query
  BIND("kind=handle;release=mln_query_release;dispose=mln_query_release");
typedef void (*mln_query_callback)(
  void* context, const char* text, mln_query query BIND("ownership=owned")
) BIND("delivery=MLN_DELIVERY_TYPE_QUERY");
typedef void (*mln_query_release_context)(void* context);
typedef struct mln_query_handler {
  unsigned int size;
  mln_query_callback callback;
  void* context BIND("kind=context");
  mln_query_release_context release;
  mln_delivery_queue queue BIND("kind=delivery_queue");
} mln_query_handler BIND("kind=callback_registration;release=release");
BIND("execution=command")
mln_status mln_map_set_query_handler(
  mln_map map, const mln_query_handler* handler,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
);
mln_status mln_query_answer(
  mln_query query, mln_buffer_view text, mln_diagnostic* out_diagnostic
);
void mln_query_release(mln_query query);
typedef struct mln_query_delivery {
  const char* text BIND("lifetime=owner");
  mln_query query BIND("ownership=owned");
} mln_query_delivery;
#endif

#ifdef MLN_PROTOCOL_RECORD_NOTIFICATION
// A notification that borrows a sized record for the callback, from a
// registration that a command copies inside the record it takes.
typedef struct mln_reading {
  uint32_t size;
  uint32_t level;
  uint64_t stamp;
} mln_reading;
typedef void (*mln_reading_callback)(
  void* context, const mln_reading* reading
) BIND("delivery=MLN_DELIVERY_TYPE_READING");
typedef void (*mln_reading_release)(void* context);
typedef struct mln_reading_handler {
  mln_reading_callback callback BIND("nullable=true");
  void* context BIND("kind=context");
  mln_reading_release release;
  mln_delivery_queue queue BIND("kind=delivery_queue");
} mln_reading_handler BIND("kind=callback_registration;release=release");
typedef struct mln_reading_request {
  uint32_t size;
  uint32_t level;
  mln_reading_handler handler;
} mln_reading_request;
BIND("execution=command")
mln_status mln_map_request_reading(
  mln_map map, const mln_reading_request* request,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
);
typedef struct mln_reading_delivery {
  const mln_reading* reading BIND("lifetime=owner");
} mln_reading_delivery;
#endif

#ifdef MLN_PROTOCOL_DECLINABLE_REGISTRATION
// A callback registration that native may decline through an output. The
// cancel callback borrows the ticket that registers it.
typedef unsigned long long mln_ticket
  BIND("kind=handle;release=mln_ticket_release");
typedef void (*mln_ticket_release_context)(void* context);
typedef void (*mln_ticket_cancel)(void* context, mln_ticket ticket) BIND(
  "delivery=MLN_DELIVERY_TYPE_TICKET_CANCEL;borrowed_owner=ticket"
);
typedef struct mln_ticket_cancel_handler {
  unsigned int size;
  mln_ticket_cancel callback;
  void* context BIND("kind=context");
  mln_ticket_release_context release;
  mln_delivery_queue queue BIND("kind=delivery_queue");
} mln_ticket_cancel_handler BIND("kind=callback_registration;release=release");
void mln_ticket_release(mln_ticket ticket);
BIND("accepted_unless=cancelled")
mln_status mln_ticket_on_cancel(
  mln_ticket ticket, const mln_ticket_cancel_handler* handler,
  bool* cancelled BIND("direction=out"), mln_diagnostic* out_diagnostic
);
typedef struct mln_ticket_cancel_delivery {
  mln_ticket ticket;
} mln_ticket_cancel_delivery;
#endif

#ifdef MLN_PROTOCOL_DELIVERY
// A queue registration outside any command, whose callback returns a result
// that native code supplies in queue mode.
typedef void (*mln_notice_release)(void* context);
typedef unsigned (*mln_notice_callback)(
  void* context, int code, const char* text
) BIND("failure=0;delivery=MLN_DELIVERY_TYPE_NOTICE;queue_result=1");
// Its release must not call back into the API.
typedef struct mln_notice_handler {
  unsigned int size;
  mln_notice_callback callback;
  void* context BIND("kind=context");
  mln_notice_release release;
  mln_delivery_queue queue BIND("kind=delivery_queue");
} mln_notice_handler BIND("kind=callback_registration;release=release");
mln_status mln_notice_set_callback(
  const mln_notice_handler* handler, mln_diagnostic* out_diagnostic
);
typedef struct mln_notice_delivery {
  int code;
  const char* text BIND("lifetime=owner");
} mln_notice_delivery;
#endif

#ifdef MLN_PROTOCOL_QUEUE
// Queue delivery: a registration names a delivery queue in place of its
// callback, and native code queues a copy of each call's arguments for the
// host to drain. Each enabled group adds the payload variant that carries its
// copies.
typedef union mln_delivery_payload {
#ifdef MLN_PROTOCOL_RETAINED_REGISTRATION
  mln_sample_delivery sample BIND("variant=MLN_DELIVERY_TYPE_SAMPLE");
#endif
#ifdef MLN_PROTOCOL_OWNED_ARGUMENT
  mln_query_delivery query BIND("variant=MLN_DELIVERY_TYPE_QUERY");
#endif
#ifdef MLN_PROTOCOL_RECORD_NOTIFICATION
  mln_reading_delivery reading BIND("variant=MLN_DELIVERY_TYPE_READING");
#endif
#ifdef MLN_PROTOCOL_DECLINABLE_REGISTRATION
  mln_ticket_cancel_delivery ticket_cancel
    BIND("variant=MLN_DELIVERY_TYPE_TICKET_CANCEL");
#endif
#ifdef MLN_PROTOCOL_DELIVERY
  mln_notice_delivery notice BIND("variant=MLN_DELIVERY_TYPE_NOTICE");
#endif
} mln_delivery_payload;
typedef struct mln_delivery {
  uint32_t type BIND("enum=mln_delivery_type");
  uint32_t reserved BIND("kind=reserved");
  void* user_data BIND("kind=context");
  mln_delivery_payload payload
    BIND("tag=type;empty_variant=MLN_DELIVERY_TYPE_RETIRED");
} mln_delivery;
typedef struct mln_delivery_batch_view {
  uint32_t size;
  uint32_t delivery_size;
  const mln_delivery* deliveries
    BIND("length=delivery_count;stride=delivery_size");
  size_t delivery_count;
} mln_delivery_batch_view;
mln_status mln_delivery_queue_drain(
  mln_delivery_queue queue, mln_delivery_batch* out_batch BIND("direction=out"),
  mln_delivery_batch_view* out_view BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
mln_status mln_delivery_batch_take(
  mln_delivery_batch batch, size_t index, mln_diagnostic* out_diagnostic
);
void mln_delivery_batch_release(mln_delivery_batch batch);
#endif

// Outputs that a failure status reports as absent: each call first finds
// nothing, then publishes its output, and then fails. The parcel drain is
// absent on MLN_STATUS_NOT_READY by convention; mln_probe_read_level is not a
// drain, so it names the status with absent_on.
#ifdef MLN_PROTOCOL_ABSENT_HANDLE
typedef unsigned long long mln_probe_parcel BIND(
  "kind=handle;release=mln_probe_parcel_release;dispose=mln_probe_parcel_"
  "release"
);
void mln_probe_parcel_release(mln_probe_parcel parcel);
BIND("execution=event_batch")
mln_status mln_probe_take_parcel(
  mln_probe_parcel* out_parcel BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_ABSENT_VALUE
BIND("absent_on=MLN_STATUS_NOT_READY")
mln_status mln_probe_read_level(
  double* out_level BIND("direction=out"), mln_diagnostic* out_diagnostic
);
#endif

// Plain records that native steps through by a stride it reports, which may
// exceed the record a binding compiled. The stride versions each record, so a
// record carries no size of its own.
#ifdef MLN_PROTOCOL_STRIDED_RECORDS
typedef struct mln_probe_reading {
  uint64_t value;
} mln_probe_reading;
typedef struct mln_probe_reading_view {
  uint32_t size;
  uint32_t reading_size;
  const mln_probe_reading* readings
    BIND("length=reading_count;stride=reading_size");
  size_t reading_count;
} mln_probe_reading_view;
typedef unsigned long long mln_probe_ledger BIND(
  "kind=handle;release=mln_probe_ledger_release;dispose=mln_probe_ledger_"
  "release"
);
void mln_probe_ledger_release(mln_probe_ledger ledger);
mln_status mln_probe_ledger_open(
  mln_probe_ledger* out_ledger BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
mln_status mln_probe_ledger_get(
  mln_probe_ledger ledger,
  mln_probe_reading_view* out_view BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
#endif

// Each struct is versioned by one thing. A window that a caller passes by
// pointer begins with its size and keeps it where a survey embeds it. The span
// that only a window embeds, the survey that only a completion delivers, and
// the tallies that the survey borrows without a stride carry none.
#ifdef MLN_PROTOCOL_VERSIONING
typedef struct mln_probe_span {
  double low;
  double high;
} mln_probe_span;
typedef struct mln_probe_window {
  uint32_t size;
  mln_probe_span span;
} mln_probe_window;
typedef struct mln_probe_tally {
  uint64_t hits;
} mln_probe_tally;
typedef struct mln_probe_survey {
  mln_probe_window window;
  uint64_t samples;
  const mln_probe_tally* tallies BIND("length=tally_count");
  size_t tally_count;
} mln_probe_survey;
mln_status mln_map_set_window(
  mln_map map, const mln_probe_window* window, mln_diagnostic* out_diagnostic
);
BIND("execution=query;result=mln_probe_survey")
mln_status mln_map_survey(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
);
#endif

#endif
