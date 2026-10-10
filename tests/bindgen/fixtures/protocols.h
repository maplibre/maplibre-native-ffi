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
#if defined(MLN_PROTOCOL_CONVENTIONS) || defined(MLN_PROTOCOL_PLAN_NAMES)
#define MLN_PROTOCOL_STANDARD_TYPES
#endif
#if defined(MLN_PROTOCOL_VALUES) || defined(MLN_PROTOCOL_KEYWORDS) ||    \
  defined(MLN_PROTOCOL_PRESENCE_MASK) ||                                 \
  defined(MLN_PROTOCOL_COMPLETION_RUNTIME) ||                            \
  defined(MLN_PROTOCOL_ABI_VERSION) || defined(MLN_PROTOCOL_DEFAULTS) || \
  defined(MLN_PROTOCOL_DEFAULT_REGISTRATION) ||                          \
  defined(MLN_PROTOCOL_STRIDED_RECORDS)
#define MLN_PROTOCOL_STANDARD_TYPES
#endif
#if defined(MLN_PROTOCOL_DECISION)
#define MLN_PROTOCOL_DIRECT_REGISTRATION
#endif
#if defined(MLN_PROTOCOL_DIRECT_REGISTRATION) || \
  defined(MLN_PROTOCOL_DEFERRED_CALLBACK)
#define MLN_PROTOCOL_STANDARD_TYPES
#endif
#if defined(MLN_PROTOCOL_RETAINED_REGISTRATION) && \
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
  uint32_t reserved BIND("kind=reserved");
  uint64_t generation;
  mln_buffer_view diagnostic;
  const void* value BIND("kind=erased");
  size_t value_count;
} mln_completion_result;
typedef void (*mln_completion_callback)(
  void* user_data, const mln_completion_result* result
);
typedef void (*mln_completion_release)(void* user_data);
typedef struct mln_completion {
  uint32_t size;
  mln_completion_callback callback;
  void* user_data BIND("kind=context");
  mln_completion_release release_user_data;
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
// counted arrays with narrow counts, and nullable UTF-8 with explicit length.
#ifndef MLN_PROTOCOL_GAIN_TYPE
#define MLN_PROTOCOL_GAIN_TYPE double
#endif
typedef struct mln_probe_point {
  double type;
  MLN_PROTOCOL_GAIN_TYPE gain;
} mln_probe_point;
typedef enum BIND("kind=bitmask") mln_probe_option_field : uint32_t {
  MLN_PROBE_OPTION_POINT = 1u << 0u,
} mln_probe_option_field;
typedef struct mln_probe_options {
  mln_buffer_view title BIND("nullable=true");
  uint32_t fields BIND("enum=mln_probe_option_field");
  mln_probe_point point BIND("mask=fields;bit=MLN_PROBE_OPTION_POINT");
  const mln_probe_point* left BIND("length=left_count;nullable=true");
  uint16_t left_count;
  const mln_probe_point* right BIND("length=right_count");
  uint32_t right_count;
} mln_probe_options;
mln_status mln_probe_roundtrip(
  mln_probe_options input, mln_probe_options* out_options BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
typedef struct mln_probe_text_result {
  mln_buffer_view text BIND("nullable=true");
} mln_probe_text_result;
mln_status mln_probe_nullable_text(
  const char* text BIND("length=text_size;nullable=true"), uint16_t text_size,
  mln_probe_text_result* out_result BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_DEFAULTS
// A record whose native default holds nonzero values: a scalar of each kind,
// an enumerator, a bitmask with one flag and with several, and a nested record
// with defaults of its own. A binding builds it from language defaults.
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
  uint32_t width BIND("default=256");
  double scale BIND("default=1.5");
} mln_probe_extent;
typedef struct mln_probe_settings {
  uint32_t size;
  mln_probe_extent extent;
  uint32_t mode BIND("enum=mln_probe_mode;default=MLN_PROBE_MODE_SECOND");
  uint32_t flags BIND("enum=mln_probe_flag;default=MLN_PROBE_FLAG_ALL");
  uint32_t heading BIND("enum=mln_probe_flag;default=MLN_PROBE_FLAG_SOUTH");
  float ratio BIND("default=0.25");
  int32_t offset BIND("default=-3");
  bool enabled BIND("default=true");
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
typedef void (*mln_probe_notify)(void* user_data, uint32_t count);
typedef void (*mln_probe_notify_release)(void* user_data);
typedef struct mln_probe_signal {
  uint32_t size;
  mln_probe_notify callback BIND("nullable=true");
  void* user_data BIND("kind=context");
  mln_probe_notify_release release_user_data;
} mln_probe_signal BIND("kind=callback_registration;release=release_user_data");
typedef struct mln_probe_hooks {
  uint32_t size;
  uint32_t limit BIND("default=4");
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

#ifdef MLN_PROTOCOL_PRESENCE_MASK
// A bitmask presence group with a default constructor, nested in a snapshot
// that also carries a borrowed array.
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
  double latitude
    BIND("mask=fields;bit=MLN_CAMERA_CENTER;group_type=mln_lat_lng");
  double longitude
    BIND("mask=fields;bit=MLN_CAMERA_CENTER;group_type=mln_lat_lng");
  double zoom BIND("mask=fields;bit=MLN_CAMERA_ZOOM");
} mln_camera;
typedef struct mln_snapshot {
  mln_camera camera;
  uint64_t generation;
  const mln_lat_lng* coordinates BIND("length=coordinate_count");
  size_t coordinate_count;
} mln_snapshot;
mln_camera mln_camera_default(void);
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
// operations begin with a prefix other than its type name, a record whose field
// order is its meaning, and presence groups whose record name shares no prefix
// with its bit constants.
typedef uint64_t mln_pass_handle BIND(
  "kind=handle;release=mln_pass_close;dispose=mln_pass_close;prefix=mln_pass"
);
void mln_pass_close(mln_pass_handle pass);
mln_status mln_pass_redeem(
  mln_pass_handle pass, mln_diagnostic* out_diagnostic
);
mln_status mln_pass_stamp(mln_pass_handle pass, mln_diagnostic* out_diagnostic);
typedef struct mln_point {
  double x;
  double y;
} mln_point BIND("fields=ordered");
typedef enum BIND("kind=bitmask") mln_frame_window_field : uint32_t {
  MLN_FRAME_WINDOW_FIELD_VIEW_ORIGIN = 1u << 0u,
  MLN_FRAME_WINDOW_FIELD_SCALE = 1u << 1u,
  MLN_FRAME_WINDOW_FIELD_LOCKED = 1u << 2u,
  MLN_FRAME_WINDOW_FIELD_EXTENT = 1u << 3u,
} mln_frame_window_field;
typedef struct mln_frame_window {
  uint32_t size;
  uint32_t fields BIND("enum=mln_frame_window_field");
  double x BIND(
    "mask=fields;bit=MLN_FRAME_WINDOW_FIELD_VIEW_ORIGIN;group_type=mln_point"
  );
  double y BIND(
    "mask=fields;bit=MLN_FRAME_WINDOW_FIELD_VIEW_ORIGIN;group_type=mln_point"
  );
  double scale BIND("mask=fields;bit=MLN_FRAME_WINDOW_FIELD_SCALE");
  double width BIND("mask=fields;bit=MLN_FRAME_WINDOW_FIELD_EXTENT");
  double height BIND("mask=fields;bit=MLN_FRAME_WINDOW_FIELD_EXTENT");
} mln_frame_window;
mln_status mln_pass_set_window(
  mln_pass_handle pass, const mln_frame_window* window,
  mln_diagnostic* out_diagnostic
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

#ifdef MLN_PROTOCOL_OWNED_OUTPUT
// An owned output whose parent is an input other than the receiver.
typedef unsigned long long mln_forest BIND(
  "kind=handle;release=mln_forest_close;"
  "dispose=mln_forest_close"
);
typedef unsigned long long mln_seed
  BIND("kind=handle;release=mln_seed_close;dispose=mln_seed_close");
typedef unsigned long long mln_tree BIND(
  "kind=handle;release=mln_tree_close;dispose=mln_tree_close;parent=mln_forest"
);
void mln_forest_close(mln_forest forest);
void mln_seed_close(mln_seed seed);
void mln_tree_close(mln_tree tree);
mln_status mln_seed_plant(
  mln_seed seed, mln_forest forest, mln_tree* out_tree BIND("direction=out"),
  mln_diagnostic* out_diagnostic
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
typedef void (*mln_sample_notification)(void* context, mln_sample_point point);
typedef void (*mln_sample_release)(void* context);
typedef struct mln_sample_options {
  unsigned int size;
  mln_sample_notification changed;
  void* context BIND("kind=context");
  mln_sample_release release;
} mln_sample_options BIND("kind=callback_registration;release=release");
BIND("execution=command")
mln_status mln_map_observe_sample(
  mln_map map, const mln_sample_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_DIRECT_REGISTRATION
// A direct callback registration that native may decline through an output.
typedef unsigned long long mln_ticket
  BIND("kind=handle;release=mln_ticket_release");
typedef void (*mln_runtime_callback_release)(void* context);
// The cancel callback may call back into the ticket's own protocol.
#ifdef MLN_PROTOCOL_DECISION
#define MLN_PROTOCOL_TICKET_CALLS                                \
  "mln_ticket_answer,mln_ticket_cancelled,mln_ticket_on_cancel," \
  "mln_ticket_release"
#else
#define MLN_PROTOCOL_TICKET_CALLS "mln_ticket_on_cancel,mln_ticket_release"
#endif
typedef void (*mln_ticket_cancel)(void* context) BIND(
  "reentry=protocol;reentry_owner=registration;"
  "reentry_calls=" MLN_PROTOCOL_TICKET_CALLS
);
void mln_ticket_release(mln_ticket ticket);
BIND(
  "registration=callback;release_callback=release;"
  "accepted_unless=cancelled"
)
mln_status mln_ticket_on_cancel(
  mln_ticket ticket, mln_ticket_cancel callback,
  void* context BIND("kind=context"), mln_runtime_callback_release release,
  bool* cancelled BIND("direction=out"), mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_DECISION
// A provider callback that claims or passes through an issued decision handle.
typedef unsigned long long mln_host BIND(
  "kind=handle;release=mln_host_destroy;"
  "dispose=mln_host_destroy"
);
typedef enum mln_decision : unsigned {
  MLN_DECISION_DELEGATE = 0,
  MLN_DECISION_CLAIM = 1
} mln_decision;
typedef struct mln_ticket_response {
  unsigned code;
} mln_ticket_response;
typedef unsigned (*mln_ticket_provider_callback)(void* context, mln_ticket ticket) BIND(
  "enum=mln_decision;failure=MLN_DECISION_DELEGATE;"
  "decision_handle=ticket;decision_accept=MLN_DECISION_CLAIM;"
  "decision_pass=MLN_DECISION_DELEGATE;"
  "complete=mln_ticket_answer;cancelled=mln_ticket_cancelled;"
  "cancel_registration=mln_ticket_on_cancel;"
  "wait_retired=mln_ticket_await"
);
typedef struct mln_ticket_provider {
  mln_ticket_provider_callback callback;
  void* user_data BIND("kind=context");
  mln_runtime_callback_release release;
} mln_ticket_provider BIND("kind=callback_registration;release=release");
mln_status mln_host_destroy(mln_host host, mln_diagnostic* out_diagnostic);
BIND("execution=command")
mln_status mln_host_set_provider(
  mln_host host, const mln_ticket_provider* provider,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
);
mln_status mln_ticket_answer(
  mln_ticket ticket, const mln_ticket_response* response,
  mln_diagnostic* out_diagnostic
);
mln_status mln_ticket_cancelled(
  mln_ticket ticket, bool* result BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
mln_status mln_ticket_await(
  mln_ticket ticket BIND("handle_access=issued"), mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_DEFERRED_CALLBACK
// A callback that a native adapter answers at once and delivers to the host
// later with copied arguments.
typedef void (*mln_notice_release)(void* context);
typedef unsigned (*mln_notice_callback)(
  void* context, int code, const char* text
) BIND("failure=0;deferred=1");
BIND("registration=callback;release_callback=release")
mln_status mln_notice_set_callback(
  mln_notice_callback callback, void* context BIND("kind=context"),
  mln_notice_release release, mln_diagnostic* out_diagnostic
);
#endif

// Outputs that a failure status reports as absent: each call first finds
// nothing, then publishes its output, and then fails. The parcel drain is
// absent on MLN_STATUS_NOT_READY by convention, and the level names its status.
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
// exceed the record a binding compiled.
#ifdef MLN_PROTOCOL_STRIDED_RECORDS
typedef struct mln_probe_reading {
  uint32_t size;
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

#endif
