// Measures command submission and completion latency, and allocations, for
// bursts of map commands. The target is outside the default build and CTest:
//
//   mise exec -- cmake --build build/linux-gnu-x64-vulkan --target
//   mln_command_bench mise exec --
//   build/linux-gnu-x64-vulkan/tests/native/bench/mln_command_bench 200
//
// The argument sets the measured repetitions, and the output is CSV. A gate
// holds the runtime worker while each burst is submitted, so the numbers
// measure command transport without worker scheduling races. Completion latency
// includes the time behind the gate, and a discarded command's completion ends
// at the query that fences the burst. Allocation counts come from the replaced
// operator new below and leave out the GPU driver and direct C or Rust
// allocations. Compare only runs built with the same toolchain.

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>
#include <thread>
#include <vector>

#include "maplibre_native_c.h"
#include "mln_test_graphics.h"
#include "runtime/runtime.hpp"

namespace {
using Clock = std::chrono::steady_clock;
std::atomic_bool counting = false;
std::atomic_uint64_t allocations = 0;
thread_local uint64_t local_allocations = 0;
thread_local bool measured_thread = false;
void count() noexcept {
  if (measured_thread && counting.load(std::memory_order_relaxed)) {
    allocations.fetch_add(1, std::memory_order_relaxed);
    ++local_allocations;
  }
}
void check(mln_status status) {
  if (status == MLN_STATUS_OK) return;
  std::fprintf(stderr, "native status: %d\n", status);
  std::abort();
}
auto view(const char* text) -> mln_buffer_view {
  return {text, std::strlen(text)};
}
auto ns(Clock::duration duration) -> double {
  return std::chrono::duration<double, std::nano>(duration).count();
}
struct Done {
  std::mutex mutex;
  std::condition_variable condition;
  bool ready = false;
  Clock::time_point at;
  mln_completion descriptor{
    .size = sizeof(mln_completion),
    .callback = callback,
    .user_data = this,
    .release_user_data = release
  };
  static void callback(void* context, const mln_completion_result* result) {
    auto& self = *static_cast<Done*>(context);
    check(static_cast<mln_status>(result->status));
    self.at = Clock::now();
  }
  static void release(void* context) {
    auto& self = *static_cast<Done*>(context);
    const auto lock = std::scoped_lock{self.mutex};
    self.ready = true;
    self.condition.notify_one();
  }
  void wait() {
    auto lock = std::unique_lock{mutex};
    if (!condition.wait_for(lock, std::chrono::seconds{30}, [&] {
          return ready;
        }))
      std::abort();
  }
};

struct Gate {
  std::mutex mutex;
  std::condition_variable condition;
  bool entered = false;
  bool released = false;
  void park() {
    measured_thread = true;
    auto lock = std::unique_lock{mutex};
    entered = true;
    condition.notify_all();
    condition.wait(lock, [&] { return released; });
  }
  void wait() {
    auto lock = std::unique_lock{mutex};
    condition.wait(lock, [&] { return entered; });
  }
  void open() {
    const auto lock = std::scoped_lock{mutex};
    released = true;
    condition.notify_all();
  }
};

class Host {
 public:
  explicit Host(mln_map map) : map_(map), thread_([this] { run(); }) {
    auto lock = std::unique_lock{mutex_};
    condition_.wait(lock, [&] { return started_; });
  }
  ~Host() {
    {
      const auto lock = std::scoped_lock{mutex_};
      stop_ = true;
      ++wakes_;
      condition_.notify_all();
    }
    thread_.join();
  }
  void settle() {
    auto done = Done{};
    check(
      mln_render_session_dump_debug_logs(session_, &done.descriptor, nullptr)
    );
    done.wait();
    auto lock = std::unique_lock{mutex_};
    const auto requested = ++fence_;
    ++wakes_;
    condition_.notify_all();
    condition_.wait(lock, [&] { return serviced_ >= requested; });
  }
  std::atomic_uint64_t frames = 0;

 private:
  static void wake(void* context) {
    auto& self = *static_cast<Host*>(context);
    const auto lock = std::scoped_lock{self.mutex_};
    ++self.wakes_;
    self.condition_.notify_all();
  }
  void arm() {
    auto demand = mln_frame_demand_default();
    demand.flags =
      MLN_FRAME_DEMAND_IF_NEEDED | MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
    check(mln_render_session_request_frame(session_, &demand, nullptr));
  }
  void service() {
    check(mln_render_session_service_driver_work(session_, nullptr));
    for (;;) {
      auto batch = mln_render_frame_batch{MLN_HANDLE_NULL};
      auto results = mln_render_frame_batch_view{
        .size = sizeof(mln_render_frame_batch_view)
      };
      const auto status = mln_render_session_drain_frame_results(
        session_, &batch, &results, nullptr
      );
      if (status == MLN_STATUS_NOT_READY) break;
      check(status);
      for (size_t i = 0; i < results.result_count; ++i) {
        const auto* result = reinterpret_cast<const mln_render_frame_result*>(
          reinterpret_cast<const char*>(results.results) +
          i * results.result_size
        );
        if (result->disposition == MLN_RENDER_RESULT_RENDERED) {
          auto frame = mln_acquired_frame{MLN_HANDLE_NULL};
          check(mln_render_session_acquire_frame(session_, &frame, nullptr));
          auto sync = mln_gpu_sync_default();
          check(mln_acquired_frame_release(frame, &sync, nullptr));
          frames.fetch_add(1);
        }
        arm();
      }
      mln_render_frame_batch_release(batch);
    }
  }
  void run() {
    auto* graphics = mln_test_graphics_create(MLN_TEST_GRAPHICS_BACKEND_VULKAN);
    if (!graphics) std::abort();
    auto context = mln_test_graphics_context{};
    if (!mln_test_graphics_get_context(graphics, &context)) std::abort();
    auto target = mln_vulkan_owned_texture_descriptor_default();
    target.extent = {.width = 64, .height = 64, .scale_factor = 1};
    target.context = {
      .instance = context.vulkan_instance,
      .physical_device = context.vulkan_physical_device,
      .device = context.vulkan_device,
      .graphics_queue = context.vulkan_queue,
      .graphics_queue_family_index = context.vulkan_queue_family_index,
      .get_instance_proc_addr = context.vulkan_get_instance_proc_addr,
      .get_device_proc_addr = context.vulkan_get_device_proc_addr
    };
    auto options = mln_render_session_attach_options_default();
    options.driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
    options.driver_work_wake = {.callback = wake, .user_data = this};
    options.frame_wake = {.callback = wake, .user_data = this};
    auto attached = Done{};
    check(mln_map_attach_vulkan_owned_texture(
      map_, &target, &options, &session_, &attached.descriptor, nullptr
    ));
    for (;;) {
      {
        const auto lock = std::scoped_lock{attached.mutex};
        if (attached.ready) break;
      }

      check(mln_render_session_service_driver_work(session_, nullptr));
    }
    arm();
    {
      const auto lock = std::scoped_lock{mutex_};
      started_ = true;
      condition_.notify_all();
    }
    auto seen = uint64_t{0};
    for (;;) {
      auto lock = std::unique_lock{mutex_};
      condition_.wait(lock, [&] { return wakes_ != seen; });
      if (stop_) break;
      seen = wakes_;
      const auto fence = fence_;
      lock.unlock();
      service();
      lock.lock();
      serviced_ = fence;
      condition_.notify_all();
    }
    auto detached = Done{};
    check(mln_render_session_detach(session_, &detached.descriptor, nullptr));
    while (true) {
      {
        const auto lock = std::scoped_lock{detached.mutex};
        if (detached.ready) break;
      }

      check(mln_render_session_service_driver_work(session_, nullptr));
    }
    check(mln_render_session_dispose(session_, nullptr, nullptr));
    mln_test_graphics_destroy(graphics);
  }
  mln_map map_;
  mln_render_session session_ = MLN_HANDLE_NULL;
  std::mutex mutex_;
  std::condition_variable condition_;
  bool started_ = false;
  bool stop_ = false;
  uint64_t wakes_ = 0;
  uint64_t fence_ = 0;
  uint64_t serviced_ = 0;
  std::thread thread_;
};

// A query on the same map runs after every earlier command on it.
void fence(mln_map map) {
  auto done = Done{};
  check(mln_map_get_style_url(map, &done.descriptor, nullptr));
  done.wait();
}
auto percentile(std::vector<double> values, double fraction) -> double {
  std::sort(values.begin(), values.end());
  return values[static_cast<size_t>((values.size() - 1) * fraction)];
}
void measure(
  mln_runtime runtime, mln_map map, Host& host, size_t burst, bool awaited,
  bool images, int repetitions, uint32_t image_size = 8
) {
  std::vector<double> submission;
  std::vector<double> completion;
  double total_allocs = 0;
  double submit_allocs = 0;
  uint64_t frames = 0;
  const auto live = mln::core::lease_runtime(runtime);
  std::vector<uint8_t> pixels(image_size * image_size * 4);
  auto image = mln_premultiplied_rgba8_image_default();
  image.width = image_size;
  image.height = image_size;
  image.stride = image_size * 4;
  image.pixels = pixels.data();
  image.byte_length = pixels.size();
  for (int rep = -5; rep < repetitions; ++rep) {
    host.settle();
    const auto before_frames = host.frames.load();
    auto gate = Gate{};
    live->executor.invoke([&] { gate.park(); });
    gate.wait();
    std::array<Done, 64> done;
    std::array<Clock::time_point, 64> starts;
    std::array<double, 64> submissions;
    allocations.store(0);
    local_allocations = 0;
    counting.store(true);
    for (size_t i = 0; i < burst; ++i) {
      auto* descriptor = awaited ? &done[i].descriptor : nullptr;
      char id[32];
      std::snprintf(id, sizeof(id), "image-%zu", i);
      starts[i] = Clock::now();
      if (images)
        check(mln_map_set_style_image(
          map, view(id), &image, nullptr, descriptor, nullptr
        ));
      else
        check(mln_map_set_style_layer_property(
          map, view("background"), view("background-opacity"),
          view((rep + static_cast<int>(i)) % 2 ? "0.5" : "1.0"), descriptor,
          nullptr
        ));
      submissions[i] = ns(Clock::now() - starts[i]);
    }
    const auto submitted_allocs = local_allocations;
    gate.open();
    if (awaited)
      for (size_t i = 0; i < burst; ++i) done[i].wait();
    // A discarded command has no terminal callback; a later query fences the
    // burst.
    fence(map);
    const auto fenced = Clock::now();
    counting.store(false);
    const auto allocated = allocations.load();
    host.settle();
    if (rep < 0) continue;
    frames += host.frames.load() - before_frames;
    total_allocs += allocated;
    submit_allocs += submitted_allocs;
    for (size_t i = 0; i < burst; ++i) {
      submission.push_back(submissions[i]);
      completion.push_back(ns((awaited ? done[i].at : fenced) - starts[i]));
    }
  }
  std::printf(
    "%s,%s,%zu,%d,%.0f,%.0f,%.0f,%.0f,%.2f,%.2f,%.3f\n",
    images ? (image_size == 8 ? "image" : "image-256") : "property",
    awaited ? "awaited" : "discarded", burst, repetitions,
    percentile(submission, .5), percentile(submission, .95),
    percentile(completion, .5), percentile(completion, .95),
    submit_allocs / (repetitions * burst), total_allocs / (repetitions * burst),
    static_cast<double>(frames) / repetitions
  );
  std::fflush(stdout);
}
}  // namespace

void* operator new(size_t size) {
  count();
  if (auto* p = std::malloc(size ? size : 1)) return p;
  throw std::bad_alloc{};
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* ptr) noexcept { std::free(ptr); }
void operator delete[](void* ptr) noexcept { std::free(ptr); }
void* operator new(size_t size, const std::nothrow_t&) noexcept {
  try {
    return ::operator new(size);
  } catch (...) {
    return nullptr;
  }
}
void* operator new[](size_t size, const std::nothrow_t&) noexcept {
  return ::operator new(size, std::nothrow);
}
void operator delete(void* ptr, const std::nothrow_t&) noexcept {
  std::free(ptr);
}
void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
  std::free(ptr);
}

int main(int argc, char** argv) {
  measured_thread = true;
  const auto repetitions = argc > 1 ? std::max(1, std::atoi(argv[1])) : 100;
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  auto options = mln_runtime_options_default();
  check(mln_runtime_create(&options, &runtime, nullptr));
  auto map_options = mln_map_options_default();
  map_options.map_mode = MLN_MAP_MODE_CONTINUOUS;
  map_options.initial_extent = {.width = 64, .height = 64, .scale_factor = 1};
  auto created = Done{};
  auto map = mln_map{MLN_HANDLE_NULL};
  check(mln_runtime_create_map(
    runtime, &map_options, &map, &created.descriptor, nullptr
  ));
  created.wait();
  auto style = Done{};
  check(mln_map_set_style_json(
    map,
    view(
      R"({"version":8,"transition":{"duration":0,"delay":0},"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"red"}}]})"
    ),
    &style.descriptor, nullptr
  ));
  style.wait();
  {
    auto host = Host{map};
    std::puts(
      "case,completion,burst,repetitions,submit_p50_ns,submit_p95_ns,complete_"
      "p50_ns,complete_p95_ns,submit_allocs_per_command,total_allocs_per_"
      "command,frames_per_burst"
    );
    for (const auto burst : {1U, 8U, 64U})
      for (const auto awaited : {true, false})
        measure(runtime, map, host, burst, awaited, false, repetitions);
    for (const auto awaited : {true, false})
      measure(runtime, map, host, 64, awaited, true, repetitions);
    for (const auto awaited : {true, false})
      measure(runtime, map, host, 1, awaited, true, repetitions, 256);
  }
  auto released = Done{};
  check(mln_map_release(map, &released.descriptor, nullptr));
  released.wait();
  auto stopped = Done{};
  check(mln_runtime_release(runtime, &stopped.descriptor, nullptr));
  stopped.wait();
}
