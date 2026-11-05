// mln_ffi_bench runs the cases with Google Benchmark's flags, plus:
//
//   --mln_fixture_dir=<path>      The prepared fixtures. Without it, the
//                                 directory that MLN_FFI_TEST_FIXTURE_DIR
//                                 names, which the device runners set.
//   --mln_iteration_scale=<x>     Multiplies every case's iteration count.
//   --mln_hz=<n>                  The display refresh rate of Frame/Paced, 60
//                                 by default.
//   --mln_images=0                Leaves out the cases' last frames.
//   --mln_target=offscreen        Renders offscreen instead of into a ring of
//                                 the host's textures.
//
// Progress goes to stderr. The results go to stdout as Google Benchmark's
// JSON, and then each kept frame as a base64 PNG, between marker lines,
// because some device runners mix other output into stdout.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "bench.hpp"

namespace {

// Writes the human-readable report to stderr and keeps the JSON report.
class Reporter : public benchmark::BenchmarkReporter {
 public:
  Reporter() : console_(benchmark::ConsoleReporter::OO_None) {
    console_.SetOutputStream(&std::cerr);
    console_.SetErrorStream(&std::cerr);
    json_.SetOutputStream(&json_text_);
    json_.SetErrorStream(&std::cerr);
  }

  auto ReportContext(const Context& context) -> bool override {
    return console_.ReportContext(context) && json_.ReportContext(context);
  }

  void ReportRuns(const std::vector<Run>& runs) override {
    // The console shows each case's median when a case repeats, and every
    // failure; the JSON report keeps every run.
    std::vector<Run> shown;
    for (const auto& run : runs) {
      if (
        run.repetitions <= 1 || run.skipped != 0 ||
        (run.run_type == Run::RT_Aggregate && run.aggregate_name == "median")
      ) {
        shown.push_back(run);
      }
    }
    if (!shown.empty()) console_.ReportRuns(shown);
    json_.ReportRuns(runs);
  }

  void Finalize() override {
    console_.Finalize();
    json_.Finalize();
  }

  [[nodiscard]] auto json() const -> std::string { return json_text_.str(); }

 private:
  benchmark::ConsoleReporter console_;
  benchmark::JSONReporter json_;
  std::ostringstream json_text_;
};

auto take_flag(int& argc, char** argv, const char* name) -> const char* {
  const size_t length = std::strlen(name);
  for (int index = 1; index < argc; ++index) {
    if (
      std::strncmp(argv[index], name, length) == 0 && argv[index][length] == '='
    ) {
      const char* value = argv[index] + length + 1;
      for (int rest = index; rest < argc - 1; ++rest)
        argv[rest] = argv[rest + 1];
      --argc;
      return value;
    }
  }
  return nullptr;
}

auto fixture_version(const std::string& directory) -> std::string {
  std::ifstream file(directory + "/version");
  std::string version;
  std::getline(file, version);
  return version.empty() ? std::string{"unknown"} : version;
}

auto backend_name() -> const char* {
#if defined(BENCH_BACKEND_VULKAN)
  return "vulkan";
#elif defined(BENCH_BACKEND_METAL)
  return "metal";
#elif defined(BENCH_BACKEND_WEBGPU)
  return "webgpu";
#elif defined(BENCH_OPENGL_WGL)
  return "opengl (wgl)";
#elif defined(BENCH_OPENGL_WEBGL)
  return "opengl (webgl)";
#else
  return "opengl (egl)";
#endif
}

}  // namespace

auto main(int argc, char** argv) -> int {
  auto& environment = bench::environment();
  if (const char* directory = take_flag(argc, argv, "--mln_fixture_dir")) {
    environment.fixture_dir = directory;
  } else if (
    const char* from_runner = std::getenv("MLN_FFI_TEST_FIXTURE_DIR")
  ) {
    environment.fixture_dir = from_runner;
  } else {
    bench::abort_run("pass --mln_fixture_dir, or set MLN_FFI_TEST_FIXTURE_DIR");
  }
  if (const char* scale = take_flag(argc, argv, "--mln_iteration_scale")) {
    const double value = std::strtod(scale, nullptr);
    if (!(value > 0.0))
      bench::abort_run("--mln_iteration_scale must be positive");
    bench::set_iteration_scale(value);
  }
  if (const char* hz = take_flag(argc, argv, "--mln_hz")) {
    environment.hz = std::strtod(hz, nullptr);
    if (!(environment.hz >= 1.0))
      bench::abort_run("--mln_hz must be at least 1");
  }
  if (const char* target = take_flag(argc, argv, "--mln_target")) {
    environment.offscreen = std::strcmp(target, "offscreen") == 0;
  }
  if (const char* images = take_flag(argc, argv, "--mln_images")) {
    environment.images = std::strcmp(images, "0") != 0;
  }

  benchmark::Initialize(&argc, argv);
  if (benchmark::ReportUnrecognizedArguments(argc, argv)) return EXIT_FAILURE;

  try {
    bench::install_log_handler();
  } catch (const bench::Failure& failure) {
    std::fprintf(stderr, "mln_ffi_bench: no log handler: %s\n", failure.what());
  }
  try {
    bench::gpu_describe(environment);
  } catch (const bench::Failure& failure) {
    bench::abort_run("%s", failure.what());
  }
  if (environment.offscreen) environment.render_target = "offscreen";
  if (environment.software_gpu) {
    std::fprintf(
      stderr,
      "mln_ffi_bench: warning: rendering on %s, a software rasterizer, whose "
      "cost hides changes on the CPU side\n",
      environment.gpu_device.c_str()
    );
  }

  benchmark::AddCustomContext("mln_backend", backend_name());
  benchmark::AddCustomContext("mln_render_target", environment.render_target);
  benchmark::AddCustomContext("mln_gpu_device", environment.gpu_device);
  benchmark::AddCustomContext("mln_gpu_type", environment.gpu_type);
  benchmark::AddCustomContext("mln_gpu_driver", environment.gpu_driver);
  benchmark::AddCustomContext(
    "mln_software_gpu", environment.software_gpu ? "true" : "false"
  );
  benchmark::AddCustomContext(
    "mln_fixtures", fixture_version(environment.fixture_dir)
  );
  benchmark::AddCustomContext(
    "mln_process_cpu", bench::process_cpu_measured() ? "true" : "false"
  );

  bench::register_lifecycle_cases();
  bench::register_command_cases();
  bench::register_frame_cases();
  bench::register_load_cases();
  bench::register_data_in_cases();
  bench::register_data_out_cases();
  bench::register_memory_cases();

  Reporter reporter;
  const auto ran = benchmark::RunSpecifiedBenchmarks(&reporter);
  benchmark::Shutdown();
  if (ran == 0) bench::abort_run("no case matches --benchmark_filter");
  const auto json = reporter.json();
  std::fputs("\nMLN_FFI_BENCH_JSON_BEGIN\n", stdout);
  std::fwrite(json.data(), 1, json.size(), stdout);
  std::fputs("\nMLN_FFI_BENCH_JSON_END\n", stdout);
  std::fflush(stdout);
  bench::print_images();
  return EXIT_SUCCESS;
}
