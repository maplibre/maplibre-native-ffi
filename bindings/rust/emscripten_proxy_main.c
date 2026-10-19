// Bridges Rust's test-harness entry to Emscripten's pthread proxy CRT.
//
// wasm-ld --wrap=main redirects every wasm reference to `main` onto
// `__wrap_main` and exposes Rust 1.99's harness as `__real_main`. That lets
// this object call `emscripten_force_exit` after libtest returns, without
// defining `main` or `__main_argc_argv` (the wasm32-unknown-emscripten C ABI
// aliases those two names and would collide with the harness).
#include <emscripten.h>

extern int __real_main(int argc, char** argv);

int __wrap_main(int argc, char** argv) {
  const int status = __real_main(argc, argv);

  // Do not let backend keepalives hide libtest's completed status.
  emscripten_force_exit(status);
  return status;
}
