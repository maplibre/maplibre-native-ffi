#pragma once

namespace mln::core {

// Process exit with live runtimes.
//
// A process may exit while runtimes, maps, and render sessions are live and
// native threads are at work. Nothing at exit stops or joins those threads;
// they keep running until the operating system ends the process. Three rules
// keep that safe:
//
// - Nothing that a native thread reads is destroyed at exit. The library
//   compiles without static destructors, so every object with static storage
//   duration outlives the threads; see mln_ffi_no_static_destructors_option.
// - Nothing at exit waits for a native thread.
// - Once exit begins, native code starts no host callback. Every call into a
//   host function pointer checks process_exiting() first and skips the call.
//   A callback that is already running when exit begins may still be running.

// Registers the exit handler that marks the start of process exit, once per
// process. Called before the library first hands host callbacks to native
// threads: when it creates a runtime or installs the log callback. The C
// runtime runs exit handlers in reverse order of registration, so this one
// runs before every exit handler and static destructor registered earlier.
auto watch_process_exit() noexcept -> void;

// Whether the process has begun to exit.
[[nodiscard]] auto process_exiting() noexcept -> bool;

}  // namespace mln::core
