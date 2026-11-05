#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <numbers>
#include <sstream>

#include "bench.hpp"

namespace bench {

namespace {

auto format(const char* format, va_list arguments) -> std::string {
  char buffer[1024];
  std::vsnprintf(buffer, sizeof(buffer), format, arguments);
  return buffer;
}

thread_local mln_diagnostic last_diagnostic{};

// Sessions that rendered offscreen because the library rejected the ring as
// unsupported, and offscreen sessions on a GPU that the implementation chose
// because it couldn't create a device on the host's.
std::atomic_uint64_t ring_fallbacks{0};
std::atomic_uint64_t unverified_offscreen_gpus{0};

}  // namespace

void fail(const char* format_text, ...) {
  va_list arguments;
  va_start(arguments, format_text);
  auto message = format(format_text, arguments);
  va_end(arguments);
  throw Failure(message, false);
}

void abort_run(const char* format_text, ...) {
  va_list arguments;
  va_start(arguments, format_text);
  const auto message = format(format_text, arguments);
  va_end(arguments);
  std::fprintf(stderr, "mln_ffi_bench: %s\n", message.c_str());
  std::fflush(stderr);
  std::_Exit(EXIT_FAILURE);
}

auto diag() -> mln_diagnostic* {
  last_diagnostic = {};
  last_diagnostic.size = sizeof(last_diagnostic);
  return &last_diagnostic;
}

void check(mln_status status, const char* what) {
  if (status == MLN_STATUS_OK) return;
  std::string message = what;
  message += " returned status " + std::to_string(status);
  if (last_diagnostic.message[0] != '\0') {
    message += ": ";
    message += last_diagnostic.message;
  }
  last_diagnostic.message[0] = '\0';
  throw Failure(message, status == MLN_STATUS_UNSUPPORTED);
}

auto view(const std::string& text) -> mln_buffer_view {
  return {text.data(), text.size()};
}

auto view(const char* text) -> mln_buffer_view {
  return {text, std::strlen(text)};
}

auto environment() -> Environment& {
  static Environment value;
  return value;
}

namespace {

// The point at fractional tile coordinates `x` and `y` of zoom 14.
auto tile_point(double x, double y) -> mln_lat_lng {
  constexpr double tiles = 16384.0;
  const double longitude = x / tiles * 360.0 - 180.0;
  const double latitude =
    std::atan(std::sinh(std::numbers::pi * (1.0 - 2.0 * y / tiles))) * 180.0 /
    std::numbers::pi;
  return {latitude, longitude};
}

// The fixture directory as a file URL, without a trailing slash. A Windows
// path has no leading slash and uses backslashes.
auto fixture_url() -> const std::string& {
  static const std::string url = [] {
    static const char hex[] = "0123456789ABCDEF";
    std::string out = "file://";
    const std::string& path = environment().fixture_dir;
    if (path.empty() || path.front() != '/') out += '/';
    for (const char raw : path) {
      const auto byte = static_cast<unsigned char>(raw == '\\' ? '/' : raw);
      if (std::isalnum(byte) != 0 || std::strchr("-._~/:", byte) != nullptr) {
        out += static_cast<char>(byte);
      } else {
        out += '%';
        out += hex[byte >> 4U];
        out += hex[byte & 0x0FU];
      }
    }
    while (out.size() > 7 && out.back() == '/') out.pop_back();
    return out;
  }();
  return url;
}

}  // namespace

// The fixture's tiles span x 2618 to 2622 and y 6330 to 6334 at zoom 14.
auto scene_center() -> mln_lat_lng { return tile_point(2620.5, 6332.5); }
auto north_west() -> mln_lat_lng { return tile_point(2619.0, 6331.0); }
auto south_east() -> mln_lat_lng { return tile_point(2622.0, 6334.0); }

auto with_fixture_urls(std::string style) -> std::string {
  const std::string placeholder = "mln-bench-fixtures:";
  for (size_t at = style.find(placeholder); at != std::string::npos;
       at = style.find(placeholder, at)) {
    style.replace(at, placeholder.size(), fixture_url());
    at += fixture_url().size();
  }
  return style;
}

auto benchmark_style() -> const std::string& {
  static const std::string style = [] {
    const std::string path = environment().fixture_dir + "/style.json";
    std::ifstream file(path, std::ios::binary);
    if (!file)
      abort_run("cannot read %s; run `mise run bench:fixtures`", path.c_str());
    std::stringstream buffer;
    buffer << file.rdbuf();
    return with_fixture_urls(buffer.str());
  }();
  return style;
}

void Pump::notify() {
  {
    const std::scoped_lock lock{mutex_};
    ++generation_;
  }
  condition_.notify_all();
}

auto Pump::generation() -> uint64_t {
  const std::scoped_lock lock{mutex_};
  return generation_;
}

auto Pump::wait(uint64_t seen, Clock::time_point until) -> bool {
  // A slice bounds the wait, so a state change that comes without a wake is
  // still seen.
  const auto slice =
    std::min(until, Clock::now() + std::chrono::milliseconds{100});
  std::unique_lock lock{mutex_};
  return condition_.wait_until(lock, slice, [&] {
    return generation_ != seen;
  });
}

auto pump() -> Pump& {
  static Pump value;
  return value;
}

auto wakes() -> WakeCounts& {
  static WakeCounts value;
  return value;
}

auto events() -> EventLog& {
  static EventLog value;
  return value;
}

void EventLog::record(const mln_event_batch_view& batch) {
  count_.fetch_add(batch.event_count, std::memory_order_relaxed);
  bool idle = false;
  {
    const std::scoped_lock lock{mutex_};
    for (size_t index = 0; index < batch.event_count; ++index) {
      const auto* event = MLN_STRIDED_AT(
        mln_runtime_event, batch.events, batch.event_size, index
      );
      switch (event->type) {
        case MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED:
          if (
            event->source == collecting_ &&
            event->payload_type == MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME
          ) {
            const auto& stats = event->payload.render_frame.stats;
            frames_.push_back(
              {stats.encoding_time_ms, stats.rendering_time_ms}
            );
          }
          break;
        case MLN_RUNTIME_EVENT_MAP_IDLE: {
          idle = true;
          auto found =
            std::find_if(idle_.begin(), idle_.end(), [&](const Idle& entry) {
              return entry.map == event->source;
            });
          if (found == idle_.end()) {
            idle_.push_back({event->source, 0, {}});
            found = idle_.end() - 1;
          }
          found->count += 1;
          found->at = Clock::now();
          break;
        }
        case MLN_RUNTIME_EVENT_MAP_LOADING_FAILED:
        case MLN_RUNTIME_EVENT_MAP_RENDER_ERROR: {
          // A broken fixture shows here first.
          static std::atomic_int printed{0};
          if (printed.fetch_add(1) < 10) {
            std::fprintf(
              stderr, "mln_ffi_bench: map event %u: %.*s\n", event->type,
              static_cast<int>(event->message.size),
              static_cast<const char*>(event->message.data)
            );
          }
          break;
        }
        default:
          break;
      }
    }
  }
  if (idle) pump().notify();
}

void EventLog::collect_frames(mln_map map) {
  const std::scoped_lock lock{mutex_};
  collecting_ = map;
  frames_.clear();
}

auto EventLog::take_frames() -> std::vector<FrameEvent> {
  const std::scoped_lock lock{mutex_};
  collecting_ = MLN_HANDLE_NULL;
  return std::move(frames_);
}

auto EventLog::idle_count(mln_map map) -> uint64_t {
  const std::scoped_lock lock{mutex_};
  for (const auto& entry : idle_) {
    if (entry.map == map) return entry.count;
  }
  return 0;
}

auto EventLog::last_idle(mln_map map) -> Clock::time_point {
  const std::scoped_lock lock{mutex_};
  for (const auto& entry : idle_) {
    if (entry.map == map) return entry.at;
  }
  return {};
}

namespace {

void on_log(
  void*, mln_log_severity severity, mln_log_event, int64_t,
  mln_buffer_view message
) {
  note_callback_thread(CallbackKind::log);
  static std::atomic_int printed{0};
  if (severity == MLN_LOG_SEVERITY_ERROR && printed.fetch_add(1) < 10) {
    std::fprintf(
      stderr, "mln_ffi_bench: library error: %.*s\n",
      static_cast<int>(message.size), static_cast<const char*>(message.data)
    );
  }
}

void on_events(void*, const mln_event_batch_view* batch) {
  note_callback_thread(CallbackKind::events);
  events().record(*batch);
}

void on_frame_wake(void*) {
  note_callback_thread(CallbackKind::frame_wake);
  wakes().frame.fetch_add(1, std::memory_order_relaxed);
  pump().notify();
}

void on_redraw_wake(void*) {
  note_callback_thread(CallbackKind::redraw_wake);
  wakes().redraw.fetch_add(1, std::memory_order_relaxed);
  pump().notify();
}

void on_driver_wake(void*) {
  note_callback_thread(CallbackKind::driver_wake);
  wakes().driver.fetch_add(1, std::memory_order_relaxed);
  pump().notify();
}

}  // namespace

void install_log_handler() {
  mln_log_handler handler{};
  handler.size = sizeof(handler);
  handler.callback = on_log;
  BENCH_CHECK(mln_log_set_callback(&handler, diag()));
}

auto Completion::descriptor() -> const mln_completion* {
  descriptor_ = {};
  descriptor_.size = sizeof(mln_completion);
  descriptor_.callback = callback;
  descriptor_.user_data = this;
  descriptor_.queue = queue;
  return &descriptor_;
}

void Completion::reset() {
  done.store(false, std::memory_order_relaxed);
  status = MLN_STATUS_OK;
  render_update_generation = 0;
  value_count = 0;
  diagnostic[0] = '\0';
}

void Completion::callback(
  void* user_data, const mln_completion_result* result
) {
  auto& self = *static_cast<Completion*>(user_data);
  self.at = Clock::now();
  note_callback_thread(CallbackKind::completion);
  self.status = result->status;
  self.render_update_generation = result->render_update_generation;
  self.value_count = result->value_count;
  if (result->status != MLN_STATUS_OK) {
    const auto length =
      std::min(result->diagnostic.size, sizeof(self.diagnostic) - 1);
    std::memcpy(self.diagnostic, result->diagnostic.data, length);
    self.diagnostic[length] = '\0';
  } else if (self.hook != nullptr) {
    self.hook(self, *result);
  }
  self.done.store(true, std::memory_order_release);
  pump().notify();
}

void finish(Completion& completion, const char* what, Session* session) {
  pump_until(
    [&] { return completion.done.load(std::memory_order_acquire); }, what,
    session
  );
  if (completion.status != MLN_STATUS_OK) {
    throw Failure(
      std::string{what} + " completed with status " +
        std::to_string(completion.status) + ": " + completion.diagnostic,
      completion.status == MLN_STATUS_UNSUPPORTED
    );
  }
}

void copy_readback(
  Completion& completion, const mln_completion_result& result
) {
  auto& image = *static_cast<Image*>(completion.context);
  if (result.value == nullptr || result.value_count == 0) return;
  const auto* readback =
    static_cast<const mln_texture_readback_result*>(result.value);
  image.width = readback->info.width;
  image.height = readback->info.height;
  const auto* bytes = static_cast<const uint8_t*>(readback->data.data);
  image.rgba.assign(bytes, bytes + readback->data.size);
}

void start_event_handler(mln_runtime runtime, Completion& completion) {
  mln_runtime_event_handler handler{};
  handler.size = sizeof(handler);
  handler.callback = on_events;
  BENCH_CHECK(mln_runtime_set_event_handler(
    runtime, &handler, completion.descriptor(), diag()
  ));
}

auto map_options() -> mln_map_options {
  mln_map_options options{};
  options.size = sizeof(options);
  options.initial_extent = {kWidth, kHeight, 1.0};
  options.map_mode = MLN_MAP_MODE_CONTINUOUS;
  // The events that a host's map view typically follows. Tile and glyph
  // events cost a delivery each, and few hosts want them.
  options.fields = MLN_MAP_OPTION_EVENT_MASK;
  options.event_mask = MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_WILL_CHANGE |
                       MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_DID_CHANGE |
                       MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED |
                       MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_STARTED |
                       MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_FINISHED |
                       MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_FAILED |
                       MLN_RUNTIME_EVENT_FLAG_MAP_IDLE |
                       MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_ERROR |
                       MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_FINISHED |
                       MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_IMAGE_MISSING;
  return options;
}

auto jump_to(mln_lat_lng center, double zoom) -> mln_camera_update {
  mln_camera_update update{};
  update.size = sizeof(update);
  update.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
  update.camera.size = sizeof(update.camera);
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.center = center;
  update.camera.zoom = zoom;
  return update;
}

namespace {

auto attach_options(bool caller_driven) -> mln_render_session_attach_options {
  mln_render_session_attach_options options{};
  options.size = sizeof(options);
  options.driver = caller_driven ? MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
                                 : MLN_RENDER_DRIVER_CORE_WORKER;
  options.producer_sync_kinds = gpu_producer_sync_kinds();
  options.frame_wake.callback = on_frame_wake;
  options.redraw_wake.callback = on_redraw_wake;
  if (caller_driven) options.driver_work_wake.callback = on_driver_wake;
  return options;
}

}  // namespace

Ring::~Ring() { destroy(); }

void Ring::create() {
  if (count == 0 && gpu_has_ring() && !environment().offscreen) {
    gpu_create_ring(textures, &count);
  }
}

void Ring::destroy() {
  gpu_destroy_ring(textures, count);
  count = 0;
}

void Session::start_attach(mln_map map, Completion& completion) {
  last_slot_ = -1;
  ring_ = nullptr;
  auto& ring = borrowed_ring_ != nullptr ? *borrowed_ring_ : own_ring_;
  ring.create();
  if (ring.count != 0) {
    caller_driven_ = gpu_driver() == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
    const auto status = gpu_attach_ring(
      map, attach_options(caller_driven_), ring.textures, &session_,
      completion.descriptor(), diag()
    );
    if (status == MLN_STATUS_OK) {
      ring_ = &ring;
      return;
    }
    own_ring_.destroy();
    if (status != MLN_STATUS_UNSUPPORTED) {
      check(status, "the borrowed texture attach");
    }
    ring_fallbacks.fetch_add(1, std::memory_order_relaxed);
  }
  start_offscreen(map, completion);
}

void Session::start_offscreen(mln_map map, Completion& completion) {
  caller_driven_ = false;
  last_slot_ = -1;
  mln_offscreen_device device = MLN_HANDLE_NULL;
  const auto& env = environment();
  if (env.has_offscreen_adapter) {
    // The host's GPU, rather than whichever the library would pick.
    mln_offscreen_device_options options{};
    options.size = sizeof(options);
    options.fields = MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX;
    options.adapter_index = env.offscreen_adapter_index;
    const auto status = mln_offscreen_device_create(&options, &device, diag());
    if (status != MLN_STATUS_UNSUPPORTED) {
      check(status, "mln_offscreen_device_create");
    }
  }
  if (device == MLN_HANDLE_NULL) {
    unverified_offscreen_gpus.fetch_add(1, std::memory_order_relaxed);
  }
  mln_offscreen_descriptor descriptor{};
  descriptor.size = sizeof(descriptor);
  descriptor.logical_size = {kWidth, kHeight};
  const auto options = attach_options(false);
  const auto status = mln_map_attach_offscreen(
    map, &descriptor, &options, device, &session_, completion.descriptor(),
    diag()
  );
  // Sessions keep the device until they detach.
  if (device != MLN_HANDLE_NULL) mln_offscreen_device_release(device);
  check(status, "mln_map_attach_offscreen");
}

void Session::attached() {
  mln_render_session_capabilities capabilities{};
  capabilities.size = sizeof(capabilities);
  BENCH_CHECK(
    mln_render_session_get_capabilities(session_, &capabilities, diag())
  );
  acquires_ =
    (capabilities.flags & MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION) != 0;
  reads_back_ =
    (capabilities.flags & MLN_RENDER_SESSION_CAPABILITY_READBACK) != 0;
}

void Session::attach(mln_map map) {
  Completion done;
  start_attach(map, done);
  finish(done, "the render session attach", this);
  attached();
}

void Session::attach_offscreen(mln_map map) {
  Completion done;
  start_offscreen(map, done);
  finish(done, "the offscreen attach", this);
  attached();
}

void Session::release() {
  Completion done;
  BENCH_CHECK(mln_render_session_release(session_, done.descriptor(), diag()));
  releasing_ = true;
  try {
    finish(done, "the render session release", this);
  } catch (...) {
    releasing_ = false;
    throw;
  }
  releasing_ = false;
  session_ = MLN_HANDLE_NULL;
  ring_ = nullptr;
  own_ring_.destroy();
}

void Session::close() {
  if (session_ == MLN_HANDLE_NULL) return;
  try {
    release();
  } catch (const Failure& failure) {
    std::fprintf(
      stderr, "mln_ffi_bench: %s; disposing of the session\n", failure.what()
    );
    mln_render_session_dispose(session_, nullptr);
    session_ = MLN_HANDLE_NULL;
  }
}

void Session::service() {
  if (!caller_driven_ || session_ == MLN_HANDLE_NULL) return;
  const auto status = mln_render_session_service_driver_work(session_, nullptr);
  // A released session stops accepting service once its release finishes,
  // which can happen between the completion and this call.
  if (status != MLN_STATUS_OK && !releasing_) {
    fail("servicing the render session returned status %d", status);
  }
}

void Session::demand(mln_frame_demand_flags flags, uint64_t token) {
  mln_frame_demand demand{};
  demand.size = sizeof(demand);
  demand.flags = flags;
  demand.token = token;
  BENCH_CHECK(mln_render_session_demand_frame(session_, &demand, diag()));
}

auto Session::poll(Frame& frame) -> bool {
  bool found = false;
  bool rendered = false;
  for (;;) {
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    mln_render_frame_batch_view results{};
    results.size = sizeof(results);
    BENCH_CHECK(
      mln_render_session_drain_frame_results(session_, &batch, &results, diag())
    );
    if (batch == MLN_HANDLE_NULL) break;
    for (size_t index = 0; index < results.result_count; ++index) {
      const auto* result = MLN_STRIDED_AT(
        mln_render_frame_result, results.results, results.result_size, index
      );
      if (result->disposition == MLN_RENDER_RESULT_FAILED) {
        char message[512] = {};
        size_t length = 0;
        mln_render_session_get_failure(
          session_, message, sizeof(message), &length, nullptr
        );
        mln_render_frame_batch_release(batch);
        fail("the render session failed: %s", message);
      }
      rendered = rendered || result->disposition == MLN_RENDER_RESULT_RENDERED;
      if (result->token == frame.result.token) {
        frame.result = *result;
        found = true;
      }
    }
    mln_render_frame_batch_release(batch);
  }
  if (found) frame.drained = Clock::now();
  frame.completed = frame.drained;
  if (rendered && acquires_) {
    // A host shows the newest frame, waits for its GPU work before it reads
    // the texture, and gives the texture back.
    mln_acquired_frame acquired = MLN_HANDLE_NULL;
    BENCH_CHECK(mln_render_session_acquire_frame(session_, &acquired, diag()));
    if (acquired != MLN_HANDLE_NULL) {
      last_slot_ = gpu_frame_slot(acquired);
      mln_gpu_sync producer{};
      producer.size = sizeof(producer);
      if (
        mln_acquired_frame_get_producer_sync(acquired, &producer, nullptr) ==
        MLN_STATUS_OK
      ) {
        const bool waited = gpu_wait(producer);
        if (found) {
          frame.gpu_waited = waited;
          frame.completed = Clock::now();
        }
      }
      mln_gpu_sync sync{};
      sync.size = sizeof(sync);
      sync.kind = MLN_GPU_SYNC_CPU_COMPLETE;
      BENCH_CHECK(mln_acquired_frame_release(acquired, &sync, diag()));
    }
  }
  return found;
}

auto Session::render(mln_frame_demand_flags flags) -> Frame {
  Frame frame;
  frame.result.token = next_token();
  frame.demanded = Clock::now();
  demand(flags, frame.result.token);
  pump_until([&] { return poll(frame); }, "a frame result", this);
  return frame;
}

auto refresh_period() -> Clock::duration {
  return std::chrono::duration_cast<Clock::duration>(
    std::chrono::duration<double>(1.0 / environment().hz)
  );
}

auto Session::present_until_full(uint64_t generation) -> Presented {
  const auto period = refresh_period();
  const auto start = Clock::now();
  Presented presented;
  Frame frame;
  bool in_flight = false;
  // The caller's change is new, so the first frame starts at once.
  bool redraw = true;
  bool force = false;
  auto next_start = start;
  uint64_t redraws = wakes().redraw.load();
  for (;;) {
    const auto seen = pump().generation();
    service();
    if (in_flight && poll(frame)) {
      in_flight = false;
      const auto& result = frame.result;
      if (result.disposition == MLN_RENDER_RESULT_RENDERED) {
        ++presented.frames;
        if (
          result.mode == MLN_RENDER_MODE_FULL &&
          result.map_update_generation >= generation
        ) {
          presented.frame = frame;
          presented.refreshes =
            (frame.completed - start + period - Clock::duration{1}) / period;
          return presented;
        }
        // A fade or transition draws again without another redraw wake.
        force = result.needs_repaint;
      } else if (result.disposition == MLN_RENDER_RESULT_NO_UPDATE) {
        // The map drew everything it has, so a forced frame shows whether
        // that was complete.
        force = true;
      }
      continue;
    }
    const auto now_redraws = wakes().redraw.load();
    if (now_redraws != redraws) {
      redraws = now_redraws;
      redraw = true;
    }
    const auto now = Clock::now();
    if (!in_flight && (redraw || force) && now >= next_start) {
      frame = Frame{};
      frame.result.token = next_token();
      frame.demanded = now;
      demand(force ? MLN_FRAME_DEMAND_FORCE : 0, frame.result.token);
      redraw = false;
      force = false;
      in_flight = true;
      next_start = now + period;
      continue;
    }
    if (now - start >= kWaitLimit) fail("the map never rendered complete");
    pump().wait(
      seen, !in_flight && (redraw || force) ? next_start : now + kWaitLimit
    );
  }
}

void Session::settle(mln_map map) {
  // The first demand renders a pending update. When there is none, a forced
  // frame shows whether the last update settled the map; after an unsettled
  // frame, the next update brings what the map lacked.
  mln_frame_demand_flags flags = 0;
  const auto until = Clock::now() + kWaitLimit;
  for (;;) {
    const auto frame = render(flags);
    const auto& result = frame.result;
    if (result.disposition == MLN_RENDER_RESULT_RENDERED) {
      mln_map_snapshot snapshot{};
      snapshot.size = sizeof(snapshot);
      BENCH_CHECK(mln_map_get_snapshot(map, &snapshot, diag()));
      if (
        result.mode == MLN_RENDER_MODE_FULL && !result.needs_repaint &&
        !snapshot.transition_in_progress &&
        result.map_update_generation >= snapshot.latest_render_update_generation
      ) {
        return;
      }
      flags = result.needs_repaint ? MLN_FRAME_DEMAND_FORCE
                                   : MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
    } else {
      flags = MLN_FRAME_DEMAND_FORCE;
    }
    if (Clock::now() >= until) fail("the map never settled");
  }
}

auto Session::capture() -> Image {
  Image image;
  if (reads_back_) {
    Completion done;
    done.hook = copy_readback;
    done.context = &image;
    BENCH_CHECK(
      mln_render_session_read_texture(session_, done.descriptor(), diag())
    );
    finish(done, "the readback", this);
    return image;
  }
  if (
    ring_ == nullptr || last_slot_ < 0 ||
    static_cast<uint32_t>(last_slot_) >= ring_->count
  ) {
    throw Failure(
      "the session reports neither readback nor a frame's slot", true
    );
  }
  // tests/graphics reads on the session's own queue or share group, so the
  // session must be idle first.
  Completion done;
  BENCH_CHECK(mln_render_session_barrier(session_, done.descriptor(), diag()));
  finish(done, "the render barrier", this);
  if (!gpu_read_texture(ring_->textures[last_slot_], image)) {
    fail("tests/graphics could not read the frame's texture");
  }
  return image;
}

namespace {

void release_runtime(mln_runtime runtime) {
  Completion done;
  BENCH_CHECK(mln_runtime_release(runtime, done.descriptor(), diag()));
  finish(done, "the runtime release");
}

auto create_runtime() -> mln_runtime {
  mln_runtime runtime = MLN_HANDLE_NULL;
  BENCH_CHECK(mln_runtime_create(nullptr, &runtime, diag()));
  try {
    Completion done;
    start_event_handler(runtime, done);
    finish(done, "the event handler");
  } catch (...) {
    mln_runtime_dispose(runtime, nullptr);
    throw;
  }
  return runtime;
}

}  // namespace

Scene::Scene() : runtime_(create_runtime()) {}

Scene::~Scene() {
  session_.close();
  try {
    if (map_ != MLN_HANDLE_NULL) {
      Completion done;
      BENCH_CHECK(mln_map_release(map_, done.descriptor(), diag()));
      finish(done, "the map release");
      map_ = MLN_HANDLE_NULL;
    }
    release_runtime(runtime_);
  } catch (const Failure& failure) {
    std::fprintf(
      stderr, "mln_ffi_bench: %s; disposing of the map\n", failure.what()
    );
    if (map_ != MLN_HANDLE_NULL) mln_map_dispose(map_, nullptr);
    mln_runtime_dispose(runtime_, nullptr);
  }
}

void Scene::create_map() { create_map(map_options()); }

void Scene::create_map(const mln_map_options& options) {
  Completion done;
  BENCH_CHECK(
    mln_runtime_create_map(runtime_, &options, &map_, done.descriptor(), diag())
  );
  finish(done, "the map creation");
  const auto update = jump_to(scene_center());
  BENCH_CHECK(mln_map_update_camera(map_, &update, nullptr, diag()));
}

void Scene::load_style(const std::string& json) {
  Completion done;
  BENCH_CHECK(
    mln_map_set_style_json(map_, view(json), done.descriptor(), diag())
  );
  finish(done, "the style load", &session_);
}

void Scene::load_benchmark_scene() {
  create_map();
  load_style(benchmark_style());
  session_.attach(map_);
  session_.settle(map_);
}

auto Scene::jump(mln_lat_lng center, double zoom) -> uint64_t {
  const auto update = jump_to(center, zoom);
  Completion done;
  BENCH_CHECK(mln_map_update_camera(map_, &update, done.descriptor(), diag()));
  finish(done, "the camera jump", &session_);
  return done.render_update_generation;
}

void Scene::save_frame(benchmark::State& state, const std::string& name) {
  if (!environment().images) return;
  try {
    session_.settle(map_);
    keep_image(name.empty() ? case_key(state) : name, session_.capture());
  } catch (const Failure& failure) {
    // The frame is a sanity check beside the numbers, which stand without it.
    state.SetLabel(std::string{"no frame: "} + failure.what());
  }
}

void Series::add(Clock::duration duration) {
  samples_.push_back(
    std::chrono::duration<double, std::micro>(duration).count()
  );
}

auto Series::quantile(double q) -> double {
  const auto index = static_cast<size_t>(
    std::lround(q * static_cast<double>(samples_.size() - 1))
  );
  return samples_[index];
}

void Series::report(benchmark::State& state, const std::string& name) {
  if (samples_.empty()) return;
  std::sort(samples_.begin(), samples_.end());
  state.counters[name + "_p50_us"] = quantile(0.50);
  state.counters[name + "_p95_us"] = quantile(0.95);
}

void Series::report_distribution(
  benchmark::State& state, const std::string& name
) {
  if (samples_.empty()) return;
  std::sort(samples_.begin(), samples_.end());
  state.counters[name + "_p50_ms"] = quantile(0.50) / 1000.0;
  state.counters[name + "_p95_ms"] = quantile(0.95) / 1000.0;
  state.counters[name + "_p99_ms"] = quantile(0.99) / 1000.0;
  state.counters[name + "_max_ms"] = samples_.back() / 1000.0;
}

namespace {
double iteration_scale = 1.0;
}  // namespace

void set_iteration_scale(double scale) { iteration_scale = scale; }

auto case_key(const benchmark::State& state) -> std::string {
  auto name = state.name();
  for (const char* suffix :
       {"/iterations:", "/repeats:", "/min_time:", "/process_time",
        "/real_time"}) {
    if (const auto at = name.find(suffix); at != std::string::npos)
      name.erase(at);
  }
  return name;
}

auto add_case(const char* name, Case function, int64_t iterations)
  -> benchmark::internal::Benchmark* {
  auto* registered =
    benchmark::RegisterBenchmark(name, [function](benchmark::State& state) {
      const auto fallbacks = ring_fallbacks.load();
      const auto unverified = unverified_offscreen_gpus.load();
      try {
        function(state);
        // A side whose label differs measured different work, which the
        // compare task reports instead of comparing.
        std::string label;
        if (ring_fallbacks.load() != fallbacks) {
          label = "offscreen: the borrowed texture ring is unsupported";
        }
        if (unverified_offscreen_gpus.load() != unverified) {
          label += label.empty() ? "" : "; ";
          label += "offscreen on a GPU that the implementation chose";
        }
        if (!label.empty()) state.SetLabel(label);
      } catch (const Failure& failure) {
        if (failure.unsupported()) {
          state.SkipWithMessage(std::string{"unsupported: "} + failure.what());
        } else {
          state.SkipWithError(failure.what());
        }
      }
    });
  registered->UseRealTime()->Unit(benchmark::kMicrosecond);
  if (process_cpu_measured()) registered->MeasureProcessCPUTime();
  if (iterations > 0) {
    const auto scaled = static_cast<int64_t>(
      std::llround(static_cast<double>(iterations) * iteration_scale)
    );
    registered->Iterations(std::max<int64_t>(scaled, 1));
  }
  return registered;
}

}  // namespace bench
