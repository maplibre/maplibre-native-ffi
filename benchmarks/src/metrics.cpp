// What a case costs beyond time: allocations, CPU by thread, and peak and
// resident memory. A target without a source reports no counter for it, rather
// than a value that means something else.

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iterator>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "bench.hpp"

#if defined(__linux__) || defined(__ANDROID__) || defined(__OHOS__)
#define BENCH_PROC_FS 1
#include <dirent.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <mach/mach.h>
#include <pthread.h>
#endif

// glibc lets an executable replace malloc and forwards to its own allocator
// through the __libc_ entry points, so every allocation in the process passes
// through these definitions: the library, its static C++ runtime, Rust, and
// the graphics driver. A replaced operator new would see none of the library's
// allocations, because the library links its C++ runtime statically. Memory a
// driver maps without malloc, such as GPU memory, stays uncounted.
//
// The executable's pthread_create stands in for the library's and the
// driver's the same way, so each thread reports its name and CPU time as it
// ends, and threads that a case starts and stops keep their names. Threads
// that the kernel starts for the process, such as io_uring's, end unseen.
#if defined(__GLIBC__) && !defined(__EMSCRIPTEN__)
#define BENCH_GLIBC 1
#include <dlfcn.h>
#include <pthread.h>
#include <sys/prctl.h>

namespace {
std::atomic_uint64_t allocation_count{0};
std::atomic_uint64_t allocation_bytes{0};

inline void count(size_t bytes) {
  allocation_count.fetch_add(1, std::memory_order_relaxed);
  allocation_bytes.fetch_add(bytes, std::memory_order_relaxed);
}
}  // namespace

extern "C" {
void* __libc_malloc(size_t size);
void* __libc_calloc(size_t count, size_t size);
void* __libc_realloc(void* pointer, size_t size);
void* __libc_memalign(size_t alignment, size_t size);
void __libc_free(void* pointer);

void* malloc(size_t size) {
  count(size);
  return __libc_malloc(size);
}

void* calloc(size_t count_, size_t size) {
  count(count_ * size);
  return __libc_calloc(count_, size);
}

void* realloc(void* pointer, size_t size) {
  count(size);
  return __libc_realloc(pointer, size);
}

void free(void* pointer) { __libc_free(pointer); }

void* memalign(size_t alignment, size_t size) {
  count(size);
  return __libc_memalign(alignment, size);
}

void* aligned_alloc(size_t alignment, size_t size) {
  count(size);
  return __libc_memalign(alignment, size);
}

int posix_memalign(void** out, size_t alignment, size_t size) {
  if (alignment < sizeof(void*) || (alignment & (alignment - 1)) != 0) {
    return EINVAL;
  }
  count(size);
  void* pointer = __libc_memalign(alignment, size);
  if (pointer == nullptr) return ENOMEM;
  *out = pointer;
  return 0;
}
}
#endif

namespace bench {

namespace {

auto to_ns(const timespec& time) -> uint64_t {
  return static_cast<uint64_t>(time.tv_sec) * 1000000000ULL +
         static_cast<uint64_t>(time.tv_nsec);
}

#if defined(CLOCK_PROCESS_CPUTIME_ID) && !defined(__EMSCRIPTEN__)
auto clock_ns(clockid_t clock) -> uint64_t {
  timespec now{};
  return clock_gettime(clock, &now) == 0 ? to_ns(now) : 0;
}
auto own_cpu_ns() -> uint64_t { return clock_ns(CLOCK_THREAD_CPUTIME_ID); }
auto process_cpu_ns() -> uint64_t { return clock_ns(CLOCK_PROCESS_CPUTIME_ID); }
#else
auto own_cpu_ns() -> uint64_t { return 0; }
auto process_cpu_ns() -> uint64_t { return 0; }
#endif

auto current_thread_id() -> uint64_t {
#if defined(BENCH_PROC_FS)
  return static_cast<uint64_t>(syscall(SYS_gettid));
#elif defined(__APPLE__)
  uint64_t id = 0;
  pthread_threadid_np(nullptr, &id);
  return id;
#else
  return 0;
#endif
}

// The kinds of callback that each thread ran, so that a thread the library
// didn't name still says what it does. A fixed table keeps the callbacks from
// allocating.
struct CallbackThread {
  std::atomic_uint64_t id{0};
  std::atomic_uint32_t kinds{0};
};
CallbackThread callback_threads[1024];
thread_local uint32_t noted_kinds = 0;

auto find_callback_thread(uint64_t id, bool insert) -> CallbackThread* {
  for (auto& slot : callback_threads) {
    uint64_t seen = slot.id.load(std::memory_order_acquire);
    if (seen == id) return &slot;
    if (seen == 0) {
      if (!insert) return nullptr;
      if (slot.id.compare_exchange_strong(seen, id)) return &slot;
      if (seen == id) return &slot;
    }
  }
  return nullptr;
}

auto callback_kinds(uint64_t id) -> uint32_t {
  const auto* slot = find_callback_thread(id, false);
  return slot == nullptr ? 0 : slot->kinds.load(std::memory_order_relaxed);
}

auto read_file(const std::string& path) -> std::string {
  std::string text;
  if (FILE* file = std::fopen(path.c_str(), "re")) {
    char buffer[512];
    size_t read = 0;
    while ((read = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
      text.append(buffer, read);
    }
    std::fclose(file);
  }
  return text;
}

// Folds a thread's name into its pool's, so "Worker 1" and "Worker 2" add up.
auto thread_group(std::string name) -> std::string {
  while (!name.empty() &&
         (std::isdigit(static_cast<unsigned char>(name.back())) != 0 ||
          std::strchr(" -_#:\n", name.back()) != nullptr)) {
    name.pop_back();
  }
  return name;
}

// A thread that nobody named carries the process's name.
auto process_group() -> const std::string& {
#if defined(BENCH_PROC_FS)
  static const std::string name = thread_group(read_file(
    "/proc/self/task/" + std::to_string(static_cast<uint64_t>(getpid())) +
    "/comm"
  ));
#else
  static const std::string name;
#endif
  return name;
}

// A thread's group: the library's name for it, or else the callbacks it ran.
auto group_of(const std::string& name, uint32_t kinds) -> std::string {
  auto group = thread_group(name);
  if (!group.empty() && group != process_group()) return group;
  static constexpr const char* names[] = {
    "completion", "events", "frame-wake", "redraw-wake", "driver-wake", "log"
  };
  group = "unnamed";
  char separator = ':';
  for (uint32_t bit = 0; bit < std::size(names); ++bit) {
    if ((kinds & (1U << bit)) != 0) {
      group += separator;
      group += names[bit];
      separator = '+';
    }
  }
  return group;
}

auto allocation_totals() -> std::pair<uint64_t, uint64_t> {
#if defined(BENCH_GLIBC)
  return {
    allocation_count.load(std::memory_order_relaxed),
    allocation_bytes.load(std::memory_order_relaxed)
  };
#else
  return {0, 0};
#endif
}

struct EndedThread {
  uint64_t id = 0;
  uint64_t cpu_ns = 0;
  uint32_t kinds = 0;
  char name[16] = {};
};

#if defined(BENCH_GLIBC)
constexpr size_t kEndedThreads = 16384;
EndedThread ended_threads[kEndedThreads];
size_t ended_count = 0;
std::mutex ended_mutex;

void record_thread_end(void*) {
  EndedThread entry;
  entry.id = current_thread_id();
  entry.cpu_ns = own_cpu_ns();
  prctl(PR_GET_NAME, entry.name);
  if (auto* slot = find_callback_thread(entry.id, false)) {
    // A later thread may take the ID.
    entry.kinds = slot->kinds.exchange(0, std::memory_order_relaxed);
  }
  const std::scoped_lock lock{ended_mutex};
  if (ended_count < kEndedThreads) ended_threads[ended_count++] = entry;
}

struct ThreadStart {
  void* (*start)(void*);
  void* argument;
};

auto start_thread(void* raw) -> void* {
  const ThreadStart start = *static_cast<ThreadStart*>(raw);
  __libc_free(raw);
  void* result = nullptr;
  // Runs when the thread returns, and when it exits or is cancelled.
  pthread_cleanup_push(record_thread_end, nullptr);
  result = start.start(start.argument);
  pthread_cleanup_pop(1);
  return result;
}
#endif

auto ended_thread_count() -> size_t {
#if defined(BENCH_GLIBC)
  const std::scoped_lock lock{ended_mutex};
  return ended_count;
#else
  return 0;
#endif
}

auto ended_threads_since(size_t from) -> std::vector<EndedThread> {
#if defined(BENCH_GLIBC)
  const std::scoped_lock lock{ended_mutex};
  return {
    ended_threads + std::min(from, ended_count), ended_threads + ended_count
  };
#else
  (void)from;
  return {};
#endif
}

}  // namespace

void note_callback_thread(CallbackKind kind) {
  const auto bit = static_cast<uint32_t>(kind);
  if ((noted_kinds & bit) != 0) return;
  noted_kinds |= bit;
  const auto id = current_thread_id();
  if (id == 0) return;
  if (auto* slot = find_callback_thread(id, true)) {
    slot->kinds.fetch_or(bit, std::memory_order_relaxed);
  }
}

}  // namespace bench

#if defined(BENCH_GLIBC)
extern "C" int pthread_create(
  pthread_t* thread, const pthread_attr_t* attributes, void* (*start)(void*),
  void* argument
) {
  using Create =
    int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
  static const auto real =
    reinterpret_cast<Create>(dlsym(RTLD_NEXT, "pthread_create"));
  // Outside the allocation count, as the harness's own.
  auto* wrapped =
    static_cast<bench::ThreadStart*>(__libc_malloc(sizeof(bench::ThreadStart)));
  if (wrapped == nullptr) return EAGAIN;
  *wrapped = {start, argument};
  const int result = real(thread, attributes, bench::start_thread, wrapped);
  if (result != 0) __libc_free(wrapped);
  return result;
}
#endif

namespace bench {

namespace {

struct ThreadTime {
  uint64_t id;
  std::string group;
  uint64_t cpu_ns;
};

#if defined(BENCH_PROC_FS)
// schedstat counts nanoseconds on the CPU. A kernel without it leaves the
// clock ticks in stat.
auto thread_cpu_ns(const std::string& task) -> uint64_t {
  const auto schedstat = read_file(task + "/schedstat");
  if (!schedstat.empty()) return std::strtoull(schedstat.c_str(), nullptr, 10);
  const auto stat = read_file(task + "/stat");
  const auto close = stat.rfind(')');
  if (close == std::string::npos) return 0;
  // After the name: state, then fields 4 to 13, then utime and stime.
  const char* cursor = stat.c_str() + close + 2;
  uint64_t fields[15] = {};
  for (auto& field : fields) {
    char* end = nullptr;
    while (*cursor == ' ') ++cursor;
    if (*cursor == '\0') break;
    field = std::strtoull(cursor, &end, 10);
    if (end == cursor) {
      while (*cursor != ' ' && *cursor != '\0') ++cursor;
    } else {
      cursor = end;
    }
  }
  const auto ticks = fields[11] + fields[12];
  return ticks * (1000000000ULL / static_cast<uint64_t>(sysconf(_SC_CLK_TCK)));
}
#endif

constexpr bool kThreadCpu =
#if defined(BENCH_PROC_FS) || defined(__APPLE__)
  true;
#else
  false;
#endif

// Every thread but the calling one, whose CPU time the meter reads from its
// own clock so that these scans stay out of it.
auto thread_times() -> std::vector<ThreadTime> {
  std::vector<ThreadTime> threads;
  const auto self = current_thread_id();
#if defined(BENCH_PROC_FS)
  DIR* directory = opendir("/proc/self/task");
  if (directory == nullptr) return threads;
  while (const dirent* entry = readdir(directory)) {
    if (entry->d_name[0] == '.') continue;
    const auto id = std::strtoull(entry->d_name, nullptr, 10);
    if (id == self) continue;
    const std::string task = std::string{"/proc/self/task/"} + entry->d_name;
    threads.push_back(
      {id, group_of(read_file(task + "/comm"), callback_kinds(id)),
       thread_cpu_ns(task)}
    );
  }
  closedir(directory);
#elif defined(__APPLE__)
  thread_act_array_t list = nullptr;
  mach_msg_type_number_t count = 0;
  if (task_threads(mach_task_self(), &list, &count) != KERN_SUCCESS) {
    return threads;
  }
  for (mach_msg_type_number_t index = 0; index < count; ++index) {
    thread_basic_info_data_t basic{};
    mach_msg_type_number_t basic_count = THREAD_BASIC_INFO_COUNT;
    thread_identifier_info_data_t identifier{};
    mach_msg_type_number_t identifier_count = THREAD_IDENTIFIER_INFO_COUNT;
    if (
      thread_info(
        list[index], THREAD_BASIC_INFO, reinterpret_cast<thread_info_t>(&basic),
        &basic_count
      ) == KERN_SUCCESS &&
      thread_info(
        list[index], THREAD_IDENTIFIER_INFO,
        reinterpret_cast<thread_info_t>(&identifier), &identifier_count
      ) == KERN_SUCCESS &&
      identifier.thread_id != self
    ) {
      char name[64] = {};
      if (pthread_t thread = pthread_from_mach_thread_np(list[index])) {
        pthread_getname_np(thread, name, sizeof(name));
      }
      const auto cpu_ns =
        (static_cast<uint64_t>(basic.user_time.seconds) +
         static_cast<uint64_t>(basic.system_time.seconds)) *
          1000000000ULL +
        (static_cast<uint64_t>(basic.user_time.microseconds) +
         static_cast<uint64_t>(basic.system_time.microseconds)) *
          1000ULL;
      threads.push_back(
        {identifier.thread_id,
         group_of(name, callback_kinds(identifier.thread_id)), cpu_ns}
      );
    }
    mach_port_deallocate(mach_task_self(), list[index]);
  }
  vm_deallocate(
    mach_task_self(), reinterpret_cast<vm_address_t>(list),
    count * sizeof(thread_act_t)
  );
#else
  (void)self;
#endif
  return threads;
}

auto status_kib(const char* field) -> uint64_t {
#if defined(BENCH_PROC_FS)
  const auto status = read_file("/proc/self/status");
  const auto at = status.find(field);
  if (at != std::string::npos) {
    return std::strtoull(status.c_str() + at + std::strlen(field), nullptr, 10);
  }
#else
  (void)field;
#endif
  return 0;
}

auto reset_peak_rss() -> bool {
#if defined(BENCH_PROC_FS)
  // Writing 5 resets the peak that VmHWM reports.
  if (FILE* file = std::fopen("/proc/self/clear_refs", "we")) {
    const bool written = std::fputs("5", file) >= 0;
    return std::fclose(file) == 0 && written;
  }
#endif
  return false;
}

#if defined(BENCH_GLIBC)
// glibc's mallinfo2(), which the 2.17 headers that the artifacts build against
// lack, looked up at run time. It sums every arena.
struct Mallinfo2 {
  size_t arena, ordblks, smblks, hblks, hblkhd, usmblks, fsmblks, uordblks,
    fordblks, keepcost;
};
#endif

}  // namespace

auto heap_in_use_kib() -> uint64_t {
#if defined(BENCH_GLIBC)
  using Function = Mallinfo2 (*)();
  static const auto mallinfo2 =
    reinterpret_cast<Function>(dlsym(RTLD_DEFAULT, "mallinfo2"));
  if (mallinfo2 != nullptr) {
    const auto info = mallinfo2();
    // Chunks in use in the arenas, and the chunks that malloc mapped alone.
    return (info.uordblks + info.hblkhd) / 1024;
  }
#endif
  return 0;
}

auto resident_kib() -> uint64_t { return status_kib("VmRSS:"); }

auto process_cpu_measured() -> bool {
#if defined(__EMSCRIPTEN__)
  // The browser has no CPU clock for the page's threads.
  return false;
#else
  return true;
#endif
}

Meter::Meter(benchmark::State& state) : state_(state) {
  series_.reserve(8);
  peak_reset_ = reset_peak_rss();
  start_span();
}

auto Meter::series(const char* name) -> Series& {
  for (auto& named : series_) {
    if (named.name == name) return named.series;
  }
  auto& named = series_.emplace_back();
  named.name = name;
  named.series.reserve(static_cast<size_t>(state_.max_iterations));
  return named.series;
}

void Meter::pause() { end_span(); }
void Meter::resume() { start_span(); }

// The meter's own readings come last when a span starts and first when it
// ends, so that its own work stays out of them.
void Meter::start_span() {
  span_threads_.clear();
  for (const auto& thread : thread_times()) {
    span_threads_.emplace_back(thread.id, thread.cpu_ns);
  }
  span_ended_from_ = ended_thread_count();
  span_events_ = events().count();
  span_process_cpu_ns_ = process_cpu_ns();
  span_own_cpu_ns_ = own_cpu_ns();
  std::tie(span_allocations_, span_bytes_) = allocation_totals();
}

void Meter::end_span() {
  const auto [allocations, bytes] = allocation_totals();
  const auto own = own_cpu_ns() - span_own_cpu_ns_;
  const auto process = process_cpu_ns() - span_process_cpu_ns_;
  allocations_ += allocations - span_allocations_;
  allocated_bytes_ += bytes - span_bytes_;
  events_ += events().count() - span_events_;
  if (!kThreadCpu) return;
  process_cpu_ns_ += process;
  const auto add = [&](const std::string& group, uint64_t cpu_ns) {
    if (cpu_ns == 0) return;
    thread_cpu_ns_[group] += cpu_ns;
    named_cpu_ns_ += cpu_ns;
  };
  const auto since_start = [&](uint64_t id, uint64_t cpu_ns) {
    for (const auto& [known, at_start] : span_threads_) {
      if (known == id) return cpu_ns - std::min(cpu_ns, at_start);
    }
    return cpu_ns;
  };
  add("main", own);
  const auto ended = ended_threads_since(span_ended_from_);
  for (const auto& thread : thread_times()) {
    // A thread that has recorded its end stays in /proc/self/task until it
    // has exited, and counts once, as ended.
    if (std::none_of(ended.begin(), ended.end(), [&](const auto& entry) {
          return entry.id == thread.id;
        })) {
      add(thread.group, since_start(thread.id, thread.cpu_ns));
    }
  }
  for (const auto& thread : ended) {
    add(
      group_of(thread.name, thread.kinds), since_start(thread.id, thread.cpu_ns)
    );
  }
}

void Meter::finish() {
  end_span();
  const auto per_iteration = benchmark::Counter::kAvgIterations;
#if defined(BENCH_GLIBC)
  state_.counters["allocs"] =
    benchmark::Counter(static_cast<double>(allocations_), per_iteration);
  state_.counters["alloc_kib"] = benchmark::Counter(
    static_cast<double>(allocated_bytes_) / 1024.0, per_iteration
  );
#endif
  state_.counters["events"] =
    benchmark::Counter(static_cast<double>(events_), per_iteration);
  for (const auto& [group, cpu_ns] : thread_cpu_ns_) {
    state_.counters["thread_cpu_us/" + group] =
      benchmark::Counter(static_cast<double>(cpu_ns) / 1000.0, per_iteration);
  }
  // What no thread above accounts for: threads that ended unseen, such as the
  // io_uring threads that the kernel starts and stops with each ring.
  if (process_cpu_ns_ > named_cpu_ns_) {
    state_.counters["thread_cpu_us/(ended unseen)"] = benchmark::Counter(
      static_cast<double>(process_cpu_ns_ - named_cpu_ns_) / 1000.0,
      per_iteration
    );
  }
  if (peak_reset_) {
    state_.counters["peak_rss_mib"] =
      static_cast<double>(status_kib("VmHWM:")) / 1024.0;
  }
  for (auto& named : series_) named.series.report(state_, named.name);
}

}  // namespace bench
