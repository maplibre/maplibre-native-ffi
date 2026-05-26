#ifndef MLN_JAVACPP_BOUNDARY_TEST_H
#define MLN_JAVACPP_BOUNDARY_TEST_H

#include <thread>

typedef int (*mln_javacpp_test_callback)(void* user_data);

static inline int mln_javacpp_test_invoke_on_native_thread(
  mln_javacpp_test_callback callback, void* user_data
) {
  if (callback == nullptr) {
    return -1;
  }
  int result = 0;
  std::thread worker([&]() { result = callback(user_data); });
  worker.join();
  return result;
}

static inline int mln_javacpp_test_repeat_callback(
  mln_javacpp_test_callback callback, void* user_data, int count
) {
  if (callback == nullptr || count < 0) {
    return -1;
  }
  int result = 0;
  std::thread worker([&]() {
    for (int index = 0; index < count; ++index) {
      result += callback(user_data);
    }
  });
  worker.join();
  return result;
}

#endif
