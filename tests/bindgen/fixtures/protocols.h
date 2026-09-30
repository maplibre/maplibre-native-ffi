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
#if defined(MLN_PROTOCOL_VALUES) || defined(MLN_PROTOCOL_KEYWORDS) || \
  defined(MLN_PROTOCOL_PRESENCE_MASK) ||                              \
  defined(MLN_PROTOCOL_COMPLETION_RUNTIME)
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
  uint32_t size BIND("kind=size;default=sizeof");
  mln_status status;
  uint32_t disposition BIND("enum=mln_command_disposition");
  uint32_t reserved BIND("kind=reserved;default=0");
  uint64_t generation;
  mln_buffer_view diagnostic BIND("encoding=utf8");
  const void* value BIND("kind=erased;ownership=borrowed");
  size_t value_count;
} mln_completion_result;
typedef void (*mln_completion_callback)(
  void* user_data BIND("kind=context;lifetime=owner"),
  const mln_completion_result* result
    BIND("ownership=borrowed;lifetime=call;direction=in;length=1")
) BIND("thread=native;failure=contain");
typedef void (*mln_completion_release)(
  void* user_data BIND("kind=context;lifetime=owner")
) BIND("thread=native;failure=contain");
typedef struct mln_completion {
  uint32_t size BIND("kind=size;default=sizeof");
  mln_completion_callback callback;
  void* user_data BIND("kind=context;ownership=borrowed");
  mln_completion_release release_user_data;
} mln_completion BIND(
  "kind=callback_registration;user_data=user_data;release=release_user_data"
);
#else
typedef struct mln_completion {
  void* state;
} mln_completion;
#endif
typedef unsigned long long mln_runtime;

// The map is a plain value unless a test asks for one of its owner forms.
#if defined(MLN_PROTOCOL_MAP_CLOSE)
typedef unsigned long long mln_map
  BIND("kind=handle;release=mln_map_close;dispose=mln_map_close;parent=none");
BIND("execution=immediate") void mln_map_close(mln_map map);
#elif defined(MLN_PROTOCOL_MAP_RELEASE)
typedef unsigned long long mln_map
  BIND("kind=handle;release=mln_map_release;parent=none");
BIND("execution=immediate")
void mln_map_release(mln_map map BIND("consumes=always"));
#else
typedef unsigned long long mln_map;
#endif

#ifdef MLN_PROTOCOL_VALUES
// Copied values: a nested record, boolean presence, nullable and required
// counted arrays with narrow counts, and nullable UTF-8 with explicit length.
#ifndef MLN_PROTOCOL_GAIN_TYPE
#define MLN_PROTOCOL_GAIN_TYPE double
#endif
typedef struct mln_probe_point {
  double type;
  MLN_PROTOCOL_GAIN_TYPE gain;
} mln_probe_point;
typedef struct mln_probe_options {
  mln_buffer_view title BIND("encoding=utf8;nullable=true");
  bool has_point BIND("kind=presence_mask");
  mln_probe_point point BIND("mask=has_point");
  const mln_probe_point* left
    BIND("length=left_count;ownership=borrowed;nullable=true");
  uint16_t left_count BIND("kind=count");
  const mln_probe_point* right BIND("length=right_count;ownership=borrowed");
  uint32_t right_count BIND("kind=count");
} mln_probe_options;
BIND("execution=immediate")
mln_status mln_probe_roundtrip(
  mln_probe_options input, mln_probe_options* out_options BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
typedef struct mln_probe_text_result {
  mln_buffer_view text BIND("encoding=utf8;nullable=true");
} mln_probe_text_result;
BIND("execution=immediate")
mln_status mln_probe_nullable_text(
  const char* text
    BIND("length=text_size;encoding=utf8;nullable=true;ownership=borrowed"),
  uint16_t text_size BIND("kind=count"),
  mln_probe_text_result* out_result BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_KEYWORDS
// Parameter and field names that are target-language keywords or collide
// with generated locals.
typedef struct mln_keyword_entry {
  double type;
  double defer;
  double raw;
} mln_keyword_entry;
BIND("execution=immediate")
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
  uint32_t abi_size BIND("kind=size;default=sizeof");
  uint64_t fields BIND("kind=presence_mask;enum=mln_camera_field");
  double latitude
    BIND("mask=fields;bit=MLN_CAMERA_CENTER;group_type=mln_lat_lng");
  double longitude
    BIND("mask=fields;bit=MLN_CAMERA_CENTER;group_type=mln_lat_lng");
  double zoom BIND("mask=fields;bit=MLN_CAMERA_ZOOM");
} mln_camera BIND("default=mln_camera_default");
typedef struct mln_snapshot {
  mln_camera camera;
  uint64_t generation;
  const mln_lat_lng* coordinates
    BIND("length=coordinate_count;ownership=borrowed");
  size_t coordinate_count BIND("kind=count");
} mln_snapshot;
BIND("execution=immediate") mln_camera mln_camera_default(void);
BIND("execution=query;result=mln_snapshot;shape=value;ownership=borrowed")
mln_status mln_map_snapshot(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
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
  unsigned int kind BIND("kind=tag;enum=mln_event_kind");
  mln_event_payload payload BIND("tag=kind");
} mln_event;
BIND("execution=query;result=mln_event;shape=value;ownership=borrowed")
mln_status mln_map_event(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_OWNED_OUTPUT
// An owned output whose parent is an input other than the receiver.
typedef unsigned long long mln_forest BIND(
  "kind=handle;release=mln_forest_close;dispose=mln_forest_close;parent=none"
);
typedef unsigned long long mln_seed
  BIND("kind=handle;release=mln_seed_close;dispose=mln_seed_close;parent=none");
typedef unsigned long long mln_tree BIND(
  "kind=handle;release=mln_tree_close;dispose=mln_tree_close;parent=mln_forest"
);
BIND("execution=immediate") void mln_forest_close(mln_forest forest);
BIND("execution=immediate") void mln_seed_close(mln_seed seed);
BIND("execution=immediate") void mln_tree_close(mln_tree tree);
BIND("execution=immediate")
mln_status mln_seed_plant(
  mln_seed seed, mln_forest forest,
  mln_tree* out_tree BIND("direction=out;ownership=owned"),
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_CHILD_OWNER
// A child owner created by its parent's operation.
typedef unsigned long long mln_measurement BIND(
  "kind=handle;release=mln_measurement_close;dispose=mln_measurement_close;"
  "parent=none"
);
typedef unsigned long long mln_sample_handle BIND(
  "kind=handle;release=mln_sample_close;dispose=mln_sample_close;"
  "parent=mln_measurement"
);
BIND("execution=immediate")
mln_status mln_measurement_create(
  mln_measurement* out_owner BIND("direction=out;ownership=owned"),
  mln_diagnostic* out_diagnostic
);
BIND("execution=immediate") void mln_measurement_close(mln_measurement owner);
BIND("execution=immediate") void mln_sample_close(mln_sample_handle sample);
BIND("execution=immediate")
mln_status mln_measurement_read(
  mln_measurement owner, double* out_value BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
BIND("receiver=measurement;execution=immediate")
mln_status mln_measurement_take_sample(
  mln_measurement measurement,
  mln_sample_handle* out_sample BIND("direction=out;ownership=owned"),
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_RETAINED_REGISTRATION
// A callback registration record retained until native quiescence.
typedef struct mln_sample_point {
  unsigned int x;
  unsigned int y;
} mln_sample_point;
typedef void (*mln_sample_notification)(
  void* context BIND("kind=context;lifetime=owner"), mln_sample_point point
) BIND("thread=native;failure=contain");
typedef void (*mln_sample_release)(
  void* context BIND("kind=context;lifetime=owner")
) BIND("thread=native;failure=contain");
typedef struct mln_sample_options {
  unsigned int size BIND("kind=size;default=sizeof");
  mln_sample_notification changed;
  void* context BIND("kind=context;lifetime=owner");
  mln_sample_release release;
} mln_sample_options BIND(
  "kind=callback_registration;user_data=context;release=release"
);
BIND("receiver=map;execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_observe_sample(
  mln_map map, const mln_sample_options* options BIND("length=1"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_DIRECT_REGISTRATION
// A direct callback registration that native may decline through an output.
typedef unsigned long long mln_ticket
  BIND("kind=handle;release=mln_ticket_release;parent=none");
typedef void (*mln_runtime_callback_release)(
  void* context BIND("kind=context;lifetime=owner")
) BIND("thread=native;failure=contain");
// The cancel callback may call back into the ticket's own protocol.
#ifdef MLN_PROTOCOL_DECISION
#define MLN_PROTOCOL_TICKET_CALLS                                \
  "mln_ticket_answer,mln_ticket_cancelled,mln_ticket_on_cancel," \
  "mln_ticket_release"
#else
#define MLN_PROTOCOL_TICKET_CALLS "mln_ticket_on_cancel,mln_ticket_release"
#endif
typedef void (*mln_ticket_cancel)(
  void* context BIND("kind=context;lifetime=owner")
)
  BIND(
    "reentry=protocol;reentry_owner=registration;"
    "reentry_calls=" MLN_PROTOCOL_TICKET_CALLS ";thread=native;failure=contain"
  );
BIND("execution=immediate") void mln_ticket_release(mln_ticket ticket);
BIND(
  "execution=immediate;registration=callback;user_data=context;"
  "release_callback=release;accepted_unless=cancelled"
)
mln_status mln_ticket_on_cancel(
  mln_ticket ticket, mln_ticket_cancel callback,
  void* context BIND("kind=context;ownership=borrowed"),
  mln_runtime_callback_release release, bool* cancelled BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_DECISION
// A provider callback that claims or passes through an issued decision handle.
typedef unsigned long long mln_host BIND(
  "kind=handle;release=mln_host_destroy;dispose=mln_host_destroy;parent=none"
);
typedef enum mln_decision : unsigned {
  MLN_DECISION_DELEGATE = 0,
  MLN_DECISION_CLAIM = 1
} mln_decision;
typedef struct mln_ticket_response {
  unsigned code;
} mln_ticket_response;
typedef unsigned (*mln_ticket_provider_callback)(void* context BIND("kind=context;lifetime=owner"), mln_ticket ticket) BIND(
  "thread=native;enum=mln_decision;failure=MLN_DECISION_DELEGATE;"
  "decision_handle=ticket;decision_accept=MLN_DECISION_CLAIM;"
  "decision_pass=MLN_DECISION_DELEGATE;complete=mln_ticket_answer;"
  "cancelled=mln_ticket_cancelled;cancel_registration=mln_ticket_on_cancel;"
  "wait_retired=mln_ticket_await"
);
typedef struct mln_ticket_provider {
  mln_ticket_provider_callback callback;
  void* user_data BIND("kind=context;ownership=borrowed");
  mln_runtime_callback_release release;
} mln_ticket_provider BIND(
  "kind=callback_registration;user_data=user_data;release=release"
);
BIND("execution=immediate")
mln_status mln_host_destroy(mln_host host, mln_diagnostic* out_diagnostic);
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_host_set_provider(
  mln_host host, const mln_ticket_provider* provider BIND("length=1"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
);
BIND("execution=immediate")
mln_status mln_ticket_answer(
  mln_ticket ticket, const mln_ticket_response* response BIND("length=1"),
  mln_diagnostic* out_diagnostic
);
BIND("execution=immediate")
mln_status mln_ticket_cancelled(
  mln_ticket ticket, bool* result BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
BIND("execution=immediate")
mln_status mln_ticket_await(
  mln_ticket ticket BIND("handle_access=issued"), mln_diagnostic* out_diagnostic
);
#endif

#ifdef MLN_PROTOCOL_DEFERRED_CALLBACK
// A callback that a native adapter answers at once and delivers to the host
// later with copied arguments.
typedef void (*mln_notice_release)(
  void* context BIND("kind=context;lifetime=owner")
) BIND("thread=native;failure=contain");
typedef unsigned (*mln_notice_callback)(
  void* context BIND("kind=context;lifetime=owner"), int code,
  const char* text BIND("length=nul;encoding=utf8;lifetime=call")
) BIND("thread=native;failure=0;deferred=1");
BIND(
  "execution=immediate;registration=callback;user_data=context;"
  "release_callback=release"
)
mln_status mln_notice_set_callback(
  mln_notice_callback callback,
  void* context BIND("kind=context;ownership=borrowed"),
  mln_notice_release release, mln_diagnostic* out_diagnostic
);
#endif

#endif
