// Bridges Rust's test-harness entry to Emscripten's pthread proxy CRT.
//
// The wasm32-unknown-emscripten C ABI names argc/argv `main` as
// `__main_argc_argv`. Defining that symbol also defines `main`, which collides
// with the `main` Rust 1.99 emits. This function stays under its own name; the
// linker wrap in scripts/test-rust-browser.sh binds `__main_argc_argv` to it.
#include <emscripten.h>

extern int mln_rust_main(int argc, char** argv) __asm__("main");

int mln_ffi_proxy_main(int argc, char** argv) {
  const int status = mln_rust_main(argc, argv);

  // Do not let backend keepalives hide libtest's completed status.
  emscripten_force_exit(status);
  return status;
}
