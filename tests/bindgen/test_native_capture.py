"""Compile native captures and verify their memory and ownership boundaries."""

from __future__ import annotations

import re
import shutil
import tempfile
import unittest
from dataclasses import replace
from pathlib import Path

from support import (
    REAL_CLANG_ARGS,
    ROOT,
    parse_directory,
    real_api,
    require_tool,
    run,
)

from tools.bindgen import copy_cases
from tools.bindgen.compiler import compile_api
from tools.bindgen.model import ModelError
from tools.bindgen.native_capture import capture_roots, copy_kind, generate


class NativeCaptureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.api = real_api()

    def test_added_header_result_gets_recursive_capture_without_emitter_entries(self):
        with tempfile.TemporaryDirectory() as directory:
            staging = Path(directory)
            include = staging / "include"
            shutil.copytree(ROOT / "include", include)
            (include / "capture_fixture.h").write_text(FIXTURE_HEADER)
            with (include / "maplibre_native_c.h").open("a") as umbrella:
                umbrella.write('\n#include "capture_fixture.h"\n')
            api = parse_directory(include, REAL_CLANG_ARGS)
            outputs = generate(api)
            for name, text in outputs.items():
                path = staging / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text)
            source = staging / "mutation.cpp"
            source.write_text(FIXTURE_TEST)
            executable = staging / "mutation"
            require_tool(self, "clang++").run(
                self,
                "-std=c++20",
                "-DMLN_STATIC",
                "-Wall",
                "-Wextra",
                "-Werror",
                f"-I{include}",
                f"-I{staging / 'src'}",
                f"-I{ROOT / 'src'}",
                f"-I{ROOT / 'third_party/maplibre-native/include'}",
                str(source),
                "-o",
                str(executable),
                cwd=staging,
            )
            run(self, [str(executable)], staging)
            fixture = include / "capture_fixture.h"
            fixture.write_text(
                FIXTURE_HEADER.replace(
                    "size_t count;",
                    'size_t count; const mln_map* owned_maps MLN_BINDING("length=count;ownership=owned");',
                )
            )
            mutated = parse_directory(include, REAL_CLANG_ARGS)
            with self.assertRaisesRegex(ModelError, "owned"):
                generate(mutated)

    def test_compiled_capture_copies_once_and_releases_unclaimed_handles(self):
        api = replace(
            self.api,
            functions=tuple(
                replace(function, metadata={**function.metadata, "shape": "array"})
                if function.name == "mln_map_create"
                else function
                for function in self.api.functions
            ),
        )
        compile_and_run(self, generate(api), CAPTURE_TEST)

    def test_every_copy_kind_has_one_copy_case(self):
        # tests/native/abi/adapter/copies.c runs each case through the adapter.
        bound = compile_api(self.api)
        roots, _, _ = capture_roots(bound)
        text = copy_cases.generate(bound)[copy_cases.PATH]
        table = text[text.index("mln_adapter_copy_cases[] = {") :]
        self.assertEqual(
            sorted(re.findall(r"\{(MLN_ADAPTER_COMPLETION_COPY_\w+),", table)),
            sorted(copy_kind(native) for native in roots),
        )

    def test_deferred_callbacks_answer_early_and_fail_unadopted_decisions(self):
        compile_and_run(self, generate(self.api), DEFERRED_TEST)


def compile_and_run(test, outputs, test_source):
    with tempfile.TemporaryDirectory() as directory:
        staging = Path(directory)
        for name, text in outputs.items():
            relative = name.removeprefix("src/").removeprefix("include/")
            path = staging / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
        source = staging / "capture.cpp"
        source.write_text(test_source)
        executable = staging / "capture"
        require_tool(test, "clang++").run(
            test,
            "-std=c++20",
            "-DMLN_STATIC",
            "-Wall",
            "-Wextra",
            "-Werror",
            f"-I{staging}",
            f"-I{ROOT / 'include'}",
            f"-I{ROOT / 'src'}",
            f"-I{ROOT / 'third_party/maplibre-native/include'}",
            str(source),
            "-o",
            str(executable),
            cwd=staging,
        )
        run(test, [str(executable)], staging)


DEFERRED_TEST = r"""
#include <cassert>
#include <cstring>
#include "c_api/callback_capture.hpp"
static mln::capture::DeferredRecord* delivered = nullptr;
static bool accepting = true;
auto mln::capture::deliver_deferred(void* context, std::uint32_t kind, DeferredRecord* record) noexcept -> bool {
  if (!accepting || context == nullptr || kind != record->view.callback) return false;
  delivered = record;
  return true;
}
static unsigned completions = 0, releases = 0;
static std::uint32_t completed_size = 1;
extern "C" mln_status mln_resource_request_complete(mln_resource_request_handle, const mln_resource_response* response, mln_diagnostic*) noexcept {
  ++completions; completed_size = response->size; return MLN_STATUS_OK;
}
extern "C" void mln_resource_request_release(mln_resource_request_handle) noexcept { ++releases; }
extern "C" mln_status mln_map_dispose(mln_map, mln_diagnostic*) noexcept { return MLN_STATUS_OK; }
extern "C" mln_status mln_map_projection_close(mln_map_projection, mln_diagnostic*) noexcept { return MLN_STATUS_OK; }

int main() {
  int context = 0;
  char message[] = "borrowed";
  auto* log = reinterpret_cast<mln_log_callback>(mln::capture::deferred_function(MLN_ADAPTER_DEFERRED_LOG_CALLBACK));
  assert(log(&context, 1, 2, 3, message) == 1);
  std::memset(message, 0, sizeof(message));
  const auto* copied = static_cast<const mln_adapter_log_callback_arguments*>(delivered->view.arguments);
  assert(copied->code == 3 && std::strcmp(copied->message, "borrowed") == 0);
  mln::capture::destroy_deferred(delivered);
  accepting = false;
  assert(log(&context, 1, 2, 3, message) == 0);
  accepting = true;

  char url[] = "custom://style.json";
  mln_resource_request request{};
  request.requested_url = url;
  auto* provider = reinterpret_cast<mln_resource_provider_callback>(
    mln::capture::deferred_function(MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK));
  assert(provider(&context, &request, 9) == MLN_RESOURCE_PROVIDER_DECISION_HANDLE);
  std::memset(url, 0, sizeof(url));
  const auto* arguments = static_cast<const mln_adapter_resource_provider_callback_arguments*>(delivered->view.arguments);
  assert(arguments->handle == 9 && std::strcmp(arguments->request->requested_url, "custom://style.json") == 0);
  delivered->claimed = true;
  mln::capture::destroy_deferred(delivered);
  assert(completions == 0 && releases == 0);

  assert(provider(&context, &request, 9) == MLN_RESOURCE_PROVIDER_DECISION_HANDLE);
  mln::capture::destroy_deferred(delivered);
  // A discarded decision only releases its handle; native fails the request.
  assert(completions == 0 && releases == 1);

  request.prior_data_size = 1;
  assert(provider(&context, &request, 9) == MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH);
  assert(mln::capture::deferred_function(0) == nullptr);
}
"""


CAPTURE_TEST = r"""
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>
static std::size_t allocations = 0;
static bool fail_allocation = false;
void* operator new(std::size_t size) {
  if (fail_allocation) throw std::bad_alloc{};
  ++allocations;
  if (auto* p = std::malloc(size)) return p;
  throw std::bad_alloc{};
}
void operator delete(void* p) noexcept { std::free(p); }
#include "c_api/callback_capture.hpp"
static unsigned disposed = 0;
extern "C" mln_status mln_map_dispose(mln_map, mln_diagnostic*) noexcept { ++disposed; return MLN_STATUS_OK; }
extern "C" mln_status mln_map_projection_close(mln_map_projection, mln_diagnostic*) noexcept { ++disposed; return MLN_STATUS_OK; }

int main() {
  char url[] = "retained URL";
  mln_buffer_view views[] = {{url, sizeof(url) - 1}, {url, 3}};
  mln_style_source_info source{};
  source.id = {url, 8};
  source.fields = MLN_STYLE_SOURCE_INFO_TILEJSON | MLN_STYLE_SOURCE_INFO_ATTRIBUTION;
  source.tilejson.tile_urls = views;
  source.tilejson.tile_url_count = 2;
  source.attribution = {url, 5};
  mln_completion_result result{};
  result.status = MLN_STATUS_OK;
  result.value = &source;
  result.value_count = 1;
  const auto before = allocations;
  auto* record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_INFO, 0);
  assert(allocations - before == 1);
  const auto* copied = static_cast<const mln_style_source_info*>(record->view.result.value);
  assert(reinterpret_cast<std::uintptr_t>(copied) % alignof(mln_style_source_info) == 0);
  assert(copied->tilejson.tile_urls != views && copied->tilejson.tile_urls[0].data != url);
  std::memset(url, 'X', sizeof(url));
  assert(std::memcmp(copied->tilejson.tile_urls[0].data, "retained URL", 12) == 0);
  assert(std::memcmp(copied->attribution.data, "retai", 5) == 0);
  assert(std::memcmp(copied->id.data, "retained", 8) == 0);
  mln::capture::destroy(record);

  // A clear bit leaves the embedded record unread, so the copy never follows
  // its pointer or trusts its count.
  mln_style_source_info absent{};
  absent.fields = MLN_STYLE_SOURCE_INFO_ATTRIBUTION;
  absent.attribution = {url, 5};
  absent.tilejson.tile_urls = reinterpret_cast<const mln_buffer_view*>(1);
  absent.tilejson.tile_url_count = std::numeric_limits<std::size_t>::max();
  absent.tilejson.min_zoom = 3.0;
  result.value = &absent;
  record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_INFO, 0);
  copied = static_cast<const mln_style_source_info*>(record->view.result.value);
  const mln_style_source_tile_info zero{};
  assert(std::memcmp(&copied->tilejson, &zero, sizeof(zero)) == 0);
  mln::capture::destroy(record);

  result.value = &source;
  result.value_count = 0;
  record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_INFO, 0);
  assert(record->view.result.value != nullptr);
  mln::capture::destroy(record);
  result.value = nullptr;
  record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_INFO, 0);
  assert(record->view.result.value == nullptr);
  mln::capture::destroy(record);

  result.value = &source;
  result.value_count = std::numeric_limits<std::size_t>::max();
  bool overflow = false;
  try { record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_INFO, 0); }
  catch (const std::bad_alloc&) { overflow = true; }
  assert(overflow);

  mln_offline_region_info offline{};
  offline.definition.type = std::numeric_limits<std::uint32_t>::max();
  result.value = &offline;
  result.value_count = 1;
  bool variant = false;
  try { record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_OFFLINE_REGION_INFO, 0); }
  catch (const std::invalid_argument&) { variant = true; }
  assert(variant);

  mln_map handle = 42;
  result.value = &handle;
  record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_MAP, 0);
  mln::capture::destroy(record);
  assert(disposed == 1);
  record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_MAP, 0);
  record->claimed = true;
  mln::capture::destroy(record);
  assert(disposed == 1);

  fail_allocation = true;
  bool failed = false;
  try { record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_MAP, 0); }
  catch (const std::bad_alloc&) { failed = true; }
  fail_allocation = false;
  assert(failed);
  // The callback adapter invokes discard when capture fails.
  mln::capture::discard(MLN_ADAPTER_COMPLETION_COPY_MAP, result);
  assert(disposed == 2);
  mln_map handles[] = {42, 43};
  result.value = handles;
  result.value_count = 2;
  record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_MAP, 0);
  mln::capture::destroy(record);
  assert(disposed == 4);
}
"""


FIXTURE_HEADER = r"""
#pragma once
#include "maplibre_native_c.h"
typedef enum mln_capture_fixture_fields : uint32_t { MLN_CAPTURE_FIXTURE_NAME = 1 } mln_capture_fixture_fields;
typedef struct mln_capture_fixture {
  mln_style_source_info nested;
  const mln_buffer_view* extra MLN_BINDING("length=count;nullable=true");
  size_t count;
  uint32_t fields;
  const char* name MLN_BINDING("mask=fields;bit=MLN_CAPTURE_FIXTURE_NAME");
} mln_capture_fixture;
MLN_BINDING("execution=query;result=mln_capture_fixture")
MLN_API mln_status mln_map_capture_fixture(mln_map map, const mln_completion* completion, mln_diagnostic *out_diagnostic) MLN_NOEXCEPT;
"""

FIXTURE_TEST = r"""
#include "capture_fixture.h"
#include "c_api/callback_capture.hpp"
#include <cassert>
extern "C" mln_status mln_map_dispose(mln_map, mln_diagnostic*) noexcept { return MLN_STATUS_OK; }
extern "C" mln_status mln_map_projection_close(mln_map_projection, mln_diagnostic*) noexcept { return MLN_STATUS_OK; }
int main() {
  char bytes[] = "nested";
  mln_buffer_view view{bytes, 6};
  mln_capture_fixture input{};
  input.nested.fields = MLN_STYLE_SOURCE_INFO_TILEJSON;
  input.nested.tilejson.tile_urls = &view;
  input.nested.tilejson.tile_url_count = 1;
  input.extra = &view;
  input.count = 1;
  input.fields = 0;
  input.name = reinterpret_cast<const char*>(1);
  mln_completion_result result{};
  result.status = MLN_STATUS_OK;
  result.value = &input;
  result.value_count = 1;
  auto* record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_CAPTURE_FIXTURE, 0);
  auto* copied = static_cast<const mln_capture_fixture*>(record->view.result.value);
  std::memset(bytes, 0, sizeof(bytes));
  assert(std::memcmp(copied->nested.tilejson.tile_urls[0].data, "nested", 6) == 0);
  assert(std::memcmp(copied->extra[0].data, "nested", 6) == 0);
  assert(copied->name == nullptr);
  mln::capture::destroy(record);

  input.count = 0;
  input.extra = &view;
  view.size = 0;
  record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_CAPTURE_FIXTURE, 0);
  copied = static_cast<const mln_capture_fixture*>(record->view.result.value);
  assert(copied->extra != nullptr);
  assert(reinterpret_cast<std::uintptr_t>(copied->extra) % alignof(mln_buffer_view) == 0);
  assert(copied->nested.tilejson.tile_urls[0].data != nullptr);
  assert(copied->nested.tilejson.tile_urls[0].size == 0);
  mln::capture::destroy(record);

  input.extra = nullptr;
  view.data = nullptr;
  record = mln::capture::copy(result, MLN_ADAPTER_COMPLETION_COPY_CAPTURE_FIXTURE, 0);
  copied = static_cast<const mln_capture_fixture*>(record->view.result.value);
  assert(copied->extra == nullptr);
  assert(copied->nested.tilejson.tile_urls[0].data == nullptr);
  mln::capture::destroy(record);
}
"""
