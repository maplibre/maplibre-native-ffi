// The frame rate limit that every platform's frame pacer shares, driven by a
// vsync source that the case runs by hand.

#include <chrono>

#include "render/frame_pacer.hpp"

#include "maplibre_native_c.h"
#include "support/test_support.h"

namespace {

using namespace std::chrono_literals;

// A pacer whose vsyncs the case delivers.
class ManualPacer final : public mln::core::FramePacer {
 public:
  using FramePacer::FramePacer;
  ManualPacer(const ManualPacer&) = delete;
  ManualPacer(ManualPacer&&) = delete;
  auto operator=(const ManualPacer&) -> ManualPacer& = delete;
  auto operator=(ManualPacer&&) -> ManualPacer& = delete;
  ~ManualPacer() override = default;

  // Asks for a frame and reports whether the pacer asked for a vsync.
  auto request(mln_render_session session) -> bool {
    scheduled_ = false;
    request_frame(session);
    return scheduled_;
  }

  // Delivers a vsync at `time` and reports whether the pacer asked for the
  // next one, which it does when it skips this one.
  auto vsync(std::chrono::nanoseconds time) -> bool {
    scheduled_ = false;
    on_vsync(time);
    return scheduled_;
  }

 protected:
  auto schedule_vsync() noexcept -> void override { scheduled_ = true; }

 private:
  bool scheduled_ = false;
};

// Names no session, so the pacer's demands for it fail without effect.
constexpr auto session = mln_render_session{1};

void a_limit_skips_a_vsync_that_comes_too_soon() {
  auto options =
    mln_frame_pacer_options{.size = sizeof(mln_frame_pacer_options)};
  options.fields = MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND;
  options.maximum_frames_per_second = 60;
  auto minimum_interval = std::chrono::nanoseconds{};
  MLN_TEST_OK(
    mln::core::frame_pacer_minimum_interval(&options, minimum_interval)
  );
  auto pacer = ManualPacer{minimum_interval};
  pacer.add_session();

  TEST_ASSERT_TRUE(pacer.request(session));
  TEST_ASSERT_FALSE(pacer.vsync(0ms));
  TEST_ASSERT_TRUE(pacer.request(session));
  // Too soon after the last demand for 60 frames per second.
  TEST_ASSERT_TRUE(pacer.vsync(5ms));
  // A vsync that comes a little early still demands, so a limit at the
  // display's rate skips none.
  TEST_ASSERT_FALSE(pacer.vsync(16ms));
  // The demand took the request, so the next one asks for a vsync again.
  TEST_ASSERT_TRUE(pacer.request(session));

  pacer.remove_session(session);
}

}  // namespace

MLN_TEST_GROUP { RUN_TEST(a_limit_skips_a_vsync_that_comes_too_soon); }
