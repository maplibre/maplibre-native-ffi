#include <algorithm>
#include <exception>
#include <memory>
#include <system_error>
#include <utility>

#if defined(__EMSCRIPTEN__)
#include <emscripten/threading.h>
#endif

#include "execution/worker_thread.hpp"

namespace mln::core {

auto on_browser_main_thread() noexcept -> bool {
#if defined(__EMSCRIPTEN__)
  return emscripten_is_main_browser_thread() != 0;
#else
  return false;
#endif
}

#if defined(_WIN32) || defined(__EMSCRIPTEN__)

// Windows sizes thread stacks from the executable's own header, and
// Emscripten sizes them from the link-time pthread stack setting, so both keep
// std::thread.
WorkerThread::WorkerThread(std::function<void()> body)
    : thread_{std::move(body)} {}

WorkerThread::WorkerThread(WorkerThread&& other) noexcept = default;

auto WorkerThread::operator=(WorkerThread&& other) noexcept
  -> WorkerThread& = default;

WorkerThread::~WorkerThread() = default;

auto WorkerThread::joinable() const noexcept -> bool {
  return thread_.joinable();
}

auto WorkerThread::is_current() const noexcept -> bool {
  return thread_.get_id() == std::this_thread::get_id();
}

auto WorkerThread::join() -> void { thread_.join(); }

auto WorkerThread::detach() -> void { thread_.detach(); }

#else

namespace {

auto run_worker(void* opaque) -> void* {
  auto body = std::unique_ptr<std::function<void()> >{
    static_cast<std::function<void()>*>(opaque)
  };
  (*body)();
  return nullptr;
}

}  // namespace

WorkerThread::WorkerThread(std::function<void()> body) {
  auto attributes = pthread_attr_t{};
  if (const auto error = pthread_attr_init(&attributes); error != 0) {
    throw std::system_error{
      error, std::generic_category(), "initializing worker thread attributes"
    };
  }
  auto stack_size = std::size_t{0};
  static_cast<void>(pthread_attr_getstacksize(&attributes, &stack_size));
  static_cast<void>(
    pthread_attr_setstacksize(&attributes, std::max(stack_size, kStackSize))
  );
  auto call = std::make_unique<std::function<void()> >(std::move(body));
  const auto error =
    pthread_create(&handle_, &attributes, run_worker, call.get());
  pthread_attr_destroy(&attributes);
  if (error != 0) {
    throw std::system_error{
      error, std::generic_category(), "creating a worker thread"
    };
  }
  static_cast<void>(call.release());
  joinable_ = true;
}

WorkerThread::WorkerThread(WorkerThread&& other) noexcept
    : handle_{other.handle_}, joinable_{other.joinable_} {
  other.joinable_ = false;
}

auto WorkerThread::operator=(WorkerThread&& other) noexcept -> WorkerThread& {
  if (this != &other) {
    if (joinable_) {
      std::terminate();
    }
    handle_ = other.handle_;
    joinable_ = other.joinable_;
    other.joinable_ = false;
  }
  return *this;
}

WorkerThread::~WorkerThread() {
  if (joinable_) {
    std::terminate();
  }
}

auto WorkerThread::joinable() const noexcept -> bool { return joinable_; }

auto WorkerThread::is_current() const noexcept -> bool {
  return joinable_ && pthread_equal(handle_, pthread_self()) != 0;
}

auto WorkerThread::detach() -> void {
  if (!joinable_) {
    throw std::system_error{
      std::make_error_code(std::errc::invalid_argument),
      "detaching a worker thread that is not joinable"
    };
  }
  joinable_ = false;
  static_cast<void>(pthread_detach(handle_));
}

auto WorkerThread::join() -> void {
  if (!joinable_) {
    throw std::system_error{
      std::make_error_code(std::errc::invalid_argument),
      "joining a worker thread that is not joinable"
    };
  }
  joinable_ = false;
  static_cast<void>(pthread_join(handle_, nullptr));
}

#endif

}  // namespace mln::core

#ifdef _WIN32
// Throwaway diagnostics for #899: print a symbolized stack when this DLL's C
// runtime aborts or terminates.
#include <windows.h>
// dbghelp.h needs windows.h first.
#include <dbghelp.h>

#include <csignal>
#include <cstdio>
#include <cstring>
#include <exception>
#pragma comment(lib, "dbghelp.lib")

namespace {

void repro_print_exception() {
  try {
    if (auto current = std::current_exception()) std::rethrow_exception(current);
    std::fprintf(stderr, "repro: no exception in flight\n");
  } catch (const std::exception& error) {
    std::fprintf(stderr, "repro: exception in flight: %s\n", error.what());
  } catch (...) {
    std::fprintf(stderr, "repro: a non-std exception in flight\n");
  }
}

void repro_print_stack(const char* why) {
  void* frames[96];
  const auto count = CaptureStackBackTrace(0, 96, frames, nullptr);
  const auto process = GetCurrentProcess();
  SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
  SymInitialize(process, nullptr, TRUE);
  std::fprintf(stderr, "repro: %s on thread %lu\n", why, GetCurrentThreadId());
  repro_print_exception();
  alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 512];
  for (USHORT index = 0; index < count; ++index) {
    const auto address = reinterpret_cast<DWORD64>(frames[index]);
    HMODULE module = nullptr;
    char path[MAX_PATH] = "?";
    GetModuleHandleExA(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      static_cast<LPCSTR>(frames[index]), &module
    );
    if (module != nullptr) GetModuleFileNameA(module, path, MAX_PATH);
    const char* name = std::strrchr(path, '\\');
    name = name != nullptr ? name + 1 : path;
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = 511;
    DWORD64 displacement = 0;
    const bool has_symbol =
      SymFromAddr(process, address, &displacement, symbol) != FALSE;
    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof line;
    DWORD line_displacement = 0;
    const bool has_line =
      SymGetLineFromAddr64(process, address, &line_displacement, &line) !=
      FALSE;
    std::fprintf(
      stderr, "repro:   #%02u %s+0x%llx %s+0x%llx %s:%lu\n", index, name,
      static_cast<unsigned long long>(
        address - reinterpret_cast<DWORD64>(module)
      ),
      has_symbol ? symbol->Name : "?",
      static_cast<unsigned long long>(displacement),
      has_line ? line.FileName : "", has_line ? line.LineNumber : 0UL
    );
  }
  std::fflush(stderr);
}

void repro_on_abort(int) { repro_print_stack("abort"); }

void repro_on_terminate() {
  repro_print_stack("terminate");
  std::abort();
}

const bool repro_installed = [] {
  std::signal(SIGABRT, repro_on_abort);
  std::set_terminate(repro_on_terminate);
  return true;
}();

}  // namespace
#endif
