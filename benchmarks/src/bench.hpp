#pragma once

// What every case shares: a host that drives the C API from one thread, as an
// app's UI or graphics thread does, and the meter that turns a case's loop into
// counters.
//
// A failed call throws Failure, which reports the case as an error, or as
// skipped for MLN_STATUS_UNSUPPORTED, and the run goes on.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include "maplibre_native_c.h"

namespace bench {

using Clock = std::chrono::steady_clock;

inline constexpr auto kWaitLimit = std::chrono::seconds{60};

// The view every map case starts from: the middle of the fixture's 5 x 5 block
// of zoom 14 tiles over central San Francisco, at one device pixel per logical
// pixel. The viewport shows 2 x 1.5 of those tiles.
inline constexpr uint32_t kWidth = 1024;
inline constexpr uint32_t kHeight = 768;
inline constexpr double kZoom = 14.0;
auto scene_center() -> mln_lat_lng;
// Two views that share no tile: the block's north-west and south-east corners.
auto north_west() -> mln_lat_lng;
auto south_east() -> mln_lat_lng;

class Failure : public std::runtime_error {
 public:
  Failure(const std::string& message, bool unsupported)
      : std::runtime_error(message), unsupported_(unsupported) {}
  [[nodiscard]] auto unsupported() const -> bool { return unsupported_; }

 private:
  bool unsupported_;
};

[[noreturn]] void fail(const char* format, ...);
// Ends the process, for a failure outside any case.
[[noreturn]] void abort_run(const char* format, ...);
// A diagnostic for the next call, which check() reads when the call fails.
auto diag() -> mln_diagnostic*;
void check(mln_status status, const char* what);
#define BENCH_CHECK(expression) ::bench::check((expression), #expression)

auto view(const std::string& text) -> mln_buffer_view;
auto view(const char* text) -> mln_buffer_view;

// What the harness found at startup, for the run's context.
struct Environment {
  std::string fixture_dir;
  std::string render_target;
  std::string gpu_device;
  std::string gpu_type;
  std::string gpu_driver;
  bool software_gpu = false;
  bool has_offscreen_adapter = false;
  uint32_t offscreen_adapter_index = 0;
  bool images = true;
  // Renders every session offscreen instead of into a ring.
  bool offscreen = false;
  double hz = 60.0;
};
auto environment() -> Environment&;

// A style document with its `mln-bench-fixtures:` URLs pointing at the fixture
// files.
auto with_fixture_urls(std::string style) -> std::string;
// benchmarks/style.json from the fixture directory.
auto benchmark_style() -> const std::string&;

// A deterministic hash, for generated data without the stripes that a linear
// sequence draws.
inline auto mix(uint32_t seed) -> uint32_t {
  seed = (seed ^ (seed >> 16U)) * 0x7FEB352DU;
  seed = (seed ^ (seed >> 15U)) * 0x846CA68BU;
  return seed ^ (seed >> 16U);
}

// One process-wide signal that library threads notify from wakes, events, and
// completions, and that the benchmark thread waits on between checks.
class Pump {
 public:
  void notify();
  [[nodiscard]] auto generation() -> uint64_t;
  // Waits until the generation passes `seen`, or until `until`. Reports
  // whether it passed.
  auto wait(uint64_t seen, Clock::time_point until) -> bool;

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  uint64_t generation_ = 0;
};
auto pump() -> Pump&;

// The kinds of callback that the harness receives. The CPU report names a
// thread that the library left unnamed by the kinds it ran.
enum class CallbackKind : uint32_t {
  completion = 1U << 0U,
  events = 1U << 1U,
  frame_wake = 1U << 2U,
  redraw_wake = 1U << 3U,
  driver_wake = 1U << 4U,
  log = 1U << 5U,
};
// Notes that the calling thread runs callbacks of `kind`, without allocating.
void note_callback_thread(CallbackKind kind);

struct WakeCounts {
  std::atomic_uint64_t frame{0};
  std::atomic_uint64_t redraw{0};
  std::atomic_uint64_t driver{0};
};
auto wakes() -> WakeCounts&;
void install_log_handler();

// The engine's own report of one rendered frame.
struct FrameEvent {
  double encoding_ms = 0.0;
  double rendering_ms = 0.0;
};

// The runtime events that cases read: frame statistics, and when each map
// last became idle.
class EventLog {
 public:
  void record(const mln_event_batch_view& batch);
  [[nodiscard]] auto count() const -> uint64_t {
    return count_.load(std::memory_order_relaxed);
  }
  // Starts collecting the frame events of `map`, dropping any collected.
  void collect_frames(mln_map map);
  auto take_frames() -> std::vector<FrameEvent>;
  // How many times `map` has become idle, and when it last did.
  auto idle_count(mln_map map) -> uint64_t;
  auto last_idle(mln_map map) -> Clock::time_point;

 private:
  struct Idle {
    mln_map map;
    uint64_t count;
    Clock::time_point at;
  };
  std::mutex mutex_;
  std::atomic_uint64_t count_{0};
  mln_map collecting_ = MLN_HANDLE_NULL;
  std::vector<FrameEvent> frames_;
  std::vector<Idle> idle_;
};
auto events() -> EventLog&;

// One completion delivery. The callback sets `done` last, so a case can reuse
// the object as soon as it sees `done`.
struct Completion {
  using ValueHook = void (*)(Completion& self, const mln_completion_result&);

  std::atomic_bool done{false};
  mln_status status = MLN_STATUS_OK;
  uint64_t render_update_generation = 0;
  size_t value_count = 0;
  Clock::time_point at;
  ValueHook hook = nullptr;
  void* context = nullptr;
  mln_delivery_queue queue = MLN_HANDLE_NULL;
  char diagnostic[256] = {};

  auto descriptor() -> const mln_completion*;
  void reset();

 private:
  static void callback(void* user_data, const mln_completion_result* result);
  mln_completion descriptor_{};
};

class Session;

// Blocks until `ready()` holds, servicing `session` between checks when the
// caller drives it.
template <typename Ready>
void pump_until(Ready ready, const char* what, Session* session = nullptr);

// Waits for a completion and expects MLN_STATUS_OK.
void finish(
  Completion& completion, const char* what, Session* session = nullptr
);

// Pixels that a case read back: 8-bit RGBA, `width` x `height`.
struct Image {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<uint8_t> rgba;
};
// A completion hook that copies a texture readback into the Image in
// `context`.
void copy_readback(Completion& completion, const mln_completion_result& result);

// One frame, from its demand to the host's handling of its result.
struct Frame {
  mln_render_frame_result result{};
  Clock::time_point demanded;
  // When the drain took the result.
  Clock::time_point drained;
  // When the frame's GPU work had finished, as far as the host can tell: after
  // a wait on its producer sync, or at the drain when there is none.
  Clock::time_point completed;
  bool gpu_waited = false;
};

// A frame that present_until_full() waited for, and how many frames and
// display refreshes it took.
struct Presented {
  Frame frame;
  int64_t frames = 0;
  int64_t refreshes = 0;
};

// The time between two refreshes of the display that the run simulates.
auto refresh_period() -> Clock::duration;

// A ring of two of the host's textures, from tests/graphics, for a session to
// render into.
struct Ring {
  Ring() = default;
  Ring(const Ring&) = delete;
  auto operator=(const Ring&) -> Ring& = delete;
  ~Ring();
  // Creates the textures, unless they exist or the run renders offscreen.
  void create();
  void destroy();

  void* textures[2] = {};
  uint32_t count = 0;
};

// A render session into a ring of the host's textures, or offscreen where the
// target has no ring, the run asks for offscreen, or the implementation
// rejects the ring as unsupported.
class Session {
 public:
  Session() = default;
  Session(const Session&) = delete;
  auto operator=(const Session&) -> Session& = delete;
  ~Session() { close(); }

  // Submits the attach, into the borrowed ring when there is one and
  // otherwise into a ring of the session's own.
  void start_attach(mln_map map, Completion& completion);
  // Makes the next attach render into `ring`, which must outlive the session.
  void borrow_ring(Ring& ring) { borrowed_ring_ = &ring; }
  // Reads the session's capabilities once the attach has completed.
  void attached();
  void attach(mln_map map);
  void attach_offscreen(mln_map map);
  void release();
  // Releases the session if it is attached, and disposes of it when the
  // release fails.
  void close();

  [[nodiscard]] auto handle() const -> mln_render_session { return session_; }
  void service();

  void demand(mln_frame_demand_flags flags, uint64_t token);
  [[nodiscard]] auto next_token() -> uint64_t { return next_token_++; }
  // Drains the queued results, and acquires, waits for, and releases the
  // newest rendered frame. Fills `frame` and reports whether the result of
  // its token came.
  auto poll(Frame& frame) -> bool;
  // Demands one frame and waits for it.
  auto render(mln_frame_demand_flags flags) -> Frame;
  // Renders as a host does on its display's clock until a frame shows
  // `generation` or a later update complete: a frame starts on a redraw wake,
  // or when the last one asked to draw again, but no sooner than one refresh
  // period after the previous one started.
  auto present_until_full(uint64_t generation) -> Presented;
  // Renders until a frame shows the map settled: complete, with no transition
  // or fade running and no update left to draw.
  void settle(mln_map map);
  // Renders a frame at each redraw wake until `done()` holds.
  template <typename Done>
  void follow_redraws_until(Done done, const char* what);
  // Reads the pixels of the last rendered frame.
  auto capture() -> Image;

 private:
  void start_offscreen(mln_map map, Completion& completion);

  mln_render_session session_ = MLN_HANDLE_NULL;
  bool caller_driven_ = false;
  bool acquires_ = false;
  bool reads_back_ = false;
  bool releasing_ = false;
  uint64_t next_token_ = 1;
  int last_slot_ = -1;
  Ring own_ring_;
  Ring* borrowed_ring_ = nullptr;
  // The ring that the session renders into, if it does.
  Ring* ring_ = nullptr;
};

// A runtime with an event handler, and optionally a map in continuous mode at
// the benchmark view and a session.
class Scene {
 public:
  Scene();
  Scene(const Scene&) = delete;
  auto operator=(const Scene&) -> Scene& = delete;
  ~Scene();

  void create_map();
  void create_map(const mln_map_options& options);
  void load_style(const std::string& json);
  // Loads the benchmark style, attaches a session, and renders until settled.
  void load_benchmark_scene();
  // Moves the camera and waits for the command, returning its render update
  // generation.
  auto jump(mln_lat_lng center, double zoom = kZoom) -> uint64_t;
  // Settles the map and keeps its frame for the run's report, under the
  // case's name by default.
  void save_frame(benchmark::State& state, const std::string& name = {});

  [[nodiscard]] auto runtime() const -> mln_runtime { return runtime_; }
  [[nodiscard]] auto map() const -> mln_map { return map_; }
  auto session() -> Session& { return session_; }

 private:
  mln_runtime runtime_ = MLN_HANDLE_NULL;
  mln_map map_ = MLN_HANDLE_NULL;
  Session session_;
};

// Installs the runtime's event handler, which feeds events().
void start_event_handler(mln_runtime runtime, Completion& completion);
auto map_options() -> mln_map_options;
auto jump_to(mln_lat_lng center, double zoom = kZoom) -> mln_camera_update;

// Samples of one quantity. report() writes p50 and p95 in microseconds;
// report_distribution() writes p50, p95, p99, and the maximum in
// milliseconds.
class Series {
 public:
  void reserve(size_t count) { samples_.reserve(count); }
  void add(Clock::duration duration);
  void add_ms(double milliseconds) {
    samples_.push_back(milliseconds * 1000.0);
  }
  void add_since(Clock::time_point start) { add(Clock::now() - start); }
  void report(benchmark::State& state, const std::string& name);
  void report_distribution(benchmark::State& state, const std::string& name);

 private:
  auto quantile(double q) -> double;
  std::vector<double> samples_;
};

// Measures what a case's loop costs beyond the time and CPU that Google
// Benchmark measures: allocations, CPU by thread, peak resident memory,
// runtime events, and latencies. Construct it just before the loop and call
// finish() just after.
class Meter {
 public:
  explicit Meter(benchmark::State& state);
  auto series(const char* name) -> Series&;
  // Leaves what happens until resume() out of the counters, as
  // State::PauseTiming() leaves it out of the times.
  void pause();
  void resume();
  void finish();

 private:
  struct Named {
    std::string name;
    Series series;
  };
  void start_span();
  void end_span();

  benchmark::State& state_;
  std::vector<Named> series_;
  bool peak_reset_ = false;
  // Totals over the measured spans.
  uint64_t allocations_ = 0;
  uint64_t allocated_bytes_ = 0;
  uint64_t events_ = 0;
  uint64_t process_cpu_ns_ = 0;
  uint64_t named_cpu_ns_ = 0;
  std::map<std::string, uint64_t> thread_cpu_ns_;
  // Readings at the start of the current span.
  uint64_t span_allocations_ = 0;
  uint64_t span_bytes_ = 0;
  uint64_t span_events_ = 0;
  uint64_t span_process_cpu_ns_ = 0;
  uint64_t span_own_cpu_ns_ = 0;
  size_t span_ended_from_ = 0;
  std::vector<std::pair<uint64_t, uint64_t>> span_threads_;
};

// The malloc heap in use and the resident memory, in KiB, or 0 where the
// target can't tell.
auto heap_in_use_kib() -> uint64_t;
auto resident_kib() -> uint64_t;
auto process_cpu_measured() -> bool;

// Registers a case with the settings every case shares, and `iterations`
// times the run's iteration scale, or Google Benchmark's own choice for 0.
using Case = void (*)(benchmark::State&);
auto add_case(const char* name, Case function, int64_t iterations)
  -> benchmark::internal::Benchmark*;
void set_iteration_scale(double scale);
// The running case's name and arguments, without the settings that every case
// shares.
auto case_key(const benchmark::State& state) -> std::string;

void register_lifecycle_cases();
void register_command_cases();
void register_frame_cases();
void register_load_cases();
void register_data_in_cases();
void register_data_out_cases();
void register_memory_cases();

// Keeps a case's last frame as a PNG, which main() prints with the report.
void keep_image(const std::string& name, const Image& image);
void print_images();

// Defined in gpu.cpp, per backend.
void gpu_describe(Environment& environment);
auto gpu_has_ring() -> bool;
auto gpu_driver() -> mln_render_driver_kind;
auto gpu_producer_sync_kinds() -> mln_gpu_sync_kind_flags;
void gpu_create_ring(void** textures, uint32_t* texture_count);
auto gpu_attach_ring(
  mln_map map, const mln_render_session_attach_options& options,
  void* const* textures, mln_render_session* out_session,
  const mln_completion* completion, mln_diagnostic* diagnostic
) -> mln_status;
void gpu_destroy_ring(void** textures, uint32_t texture_count);
// The ring slot of an acquired frame, or -1 when the session won't say.
auto gpu_frame_slot(mln_acquired_frame frame) -> int;
// Waits on the CPU until `sync` signals. Reports false for a kind that the
// host can't wait on.
auto gpu_wait(const mln_gpu_sync& sync) -> bool;
// Reads a ring texture back once its session is idle.
auto gpu_read_texture(void* texture, Image& image) -> bool;

template <typename Done>
void Session::follow_redraws_until(Done done, const char* what) {
  if (done()) return;
  uint64_t seen = wakes().redraw.load();
  render(0);
  pump_until(
    [&] {
      if (done()) return true;
      const auto redraws = wakes().redraw.load();
      if (redraws != seen) {
        seen = redraws;
        render(0);
      }
      return done();
    },
    what, this
  );
}

template <typename Ready>
void pump_until(Ready ready, const char* what, Session* session) {
  const auto until = Clock::now() + kWaitLimit;
  for (;;) {
    const auto seen = pump().generation();
    if (session != nullptr) session->service();
    if (ready()) return;
    if (!pump().wait(seen, until) && Clock::now() >= until) {
      fail("timed out waiting for %s", what);
    }
  }
}

}  // namespace bench
