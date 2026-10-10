#pragma once

namespace mln::core {

// Process exit with live runtimes.
//
// A process may exit while runtimes and maps are live and native threads are
// at work. Nothing at exit stops or joins those threads, so they keep running
// until the operating system ends the process. Three rules keep that safe:
//
// - Nothing that a native thread reads is destroyed at exit. The library
//   compiles without static destructors, so every object with static storage
//   duration outlives the threads; see mln_ffi_no_static_destructors_option.
// - Nothing at exit waits for a native thread.
// - Once exit begins, native code starts no host callback. Every call into a
//   host function pointer checks process_exiting() first and skips the call.
//   A callback whose check passed just before exit began may still start.
//
// Graphics drivers are host state that the library cannot keep alive: some
// tear themselves down in exit handlers registered after this one, which run
// first. So the host ends every render session's graphics calls before it
// exits, and nothing here waits for a driver call, which can be inside a host
// callback that blocks once exit begins.

// Registers the exit handler that marks the start of process exit, once per
// process. Called before the library first hands host callbacks to native
// threads: when it creates a runtime or installs the log callback. The C
// runtime runs exit handlers in reverse order of registration, so this one
// runs before every exit handler and static destructor registered earlier.
//
// On Windows, the library links the C runtime statically, so the handler
// joins the library's own exit list, which runs when the library unloads at
// process exit. The operating system has ended every other thread by then, so
// no native thread is left to start a callback, and the host's exit handlers
// and static destructors have already run while native threads still could.
auto watch_process_exit() noexcept -> void;

// Whether the process has begun to exit.
[[nodiscard]] auto process_exiting() noexcept -> bool;

}  // namespace mln::core
