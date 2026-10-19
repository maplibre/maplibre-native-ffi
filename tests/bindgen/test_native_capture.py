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
from tools.bindgen.native_capture import completion_values, generate


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
                if function.name == "mln_runtime_create_map"
                else function
                for function in self.api.functions
            ),
        )
        compile_and_run(self, generate(api), CAPTURE_TEST)

    def test_every_completion_value_type_has_one_copy_case(self):
        # tests/native/internal/delivery_copies.cpp runs each case through the
        # copy that a queue delivery makes.
        bound = compile_api(self.api)
        text = copy_cases.generate(bound)[copy_cases.PATH]
        table = text[text.index("mln_delivery_copy_cases[] = {") :]
        self.assertEqual(
            sorted(re.findall(r'\{"(\w+)",', table)),
            sorted(completion_values(bound)),
        )

    def test_delivered_callback_copies_outlive_their_call_and_release_handles(
        self,
    ):
        compile_and_run(self, generate(self.api), DELIVERY_TEST)


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


DELIVERY_TEST = r"""
#include <cassert>
#include <cstring>
#include "delivery/capture.hpp"
static unsigned releases = 0;
extern "C" void mln_resource_request_release(mln_resource_request_handle) noexcept { ++releases; }
extern "C" mln_status mln_map_dispose(mln_map, mln_diagnostic*) noexcept { return MLN_STATUS_OK; }

template <class T, class Capture>
auto copy(const T& source, Capture capture) -> T {
  mln::delivery::Arena<false> measure{nullptr, 0};
  static_cast<void>(capture(measure, source));
  auto* storage = static_cast<std::byte*>(::operator new(measure.offset));
  mln::delivery::Arena<true> write{storage, 0, measure.offset};
  return capture(write, source);
}

int main() {
  char message[] = "borrowed";
  const mln_log_delivery log{1, 2, 3, message};
  const auto copied = copy(log, [](auto& arena, const mln_log_delivery& value) {
    return mln::delivery::capture_mln_log_delivery(arena, value);
  });
  std::memset(message, 0, sizeof(message));
  assert(copied.code == 3 && std::strcmp(copied.message, "borrowed") == 0);
  mln::delivery::discard_mln_log_delivery(copied);
  assert(releases == 0);

  char url[] = "custom://style.json";
  mln_resource_request request{};
  request.requested_url = url;
  const mln_resource_request_delivery provided{&request, 9};
  const auto arguments = copy(provided, [](auto& arena, const mln_resource_request_delivery& value) {
    return mln::delivery::capture_mln_resource_request_delivery(arena, value);
  });
  std::memset(url, 0, sizeof(url));
  assert(arguments.handle == 9 && arguments.request != &request);
  assert(std::strcmp(arguments.request->requested_url, "custom://style.json") == 0);
  // An untaken request only releases its handle; native fails the request.
  mln::delivery::discard_mln_resource_request_delivery(arguments);
  assert(releases == 1);
  static_assert(mln::delivery::mln_resource_request_delivery_owns_handles());
  static_assert(!mln::delivery::mln_log_delivery_owns_handles());

  // A request whose byte count names no bytes cannot be copied.
  request.prior_data_size = 1;
  bool invalid = false;
  try { static_cast<void>(copy(provided, [](auto& arena, const mln_resource_request_delivery& value) {
    return mln::delivery::capture_mln_resource_request_delivery(arena, value);
  })); }
  catch (const std::invalid_argument&) { invalid = true; }
  assert(invalid);
}
"""
CAPTURE_TEST = r"""
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>
static std::size_t allocations = 0;
void* operator new(std::size_t size) {
  ++allocations;
  if (auto* p = std::malloc(size)) return p;
  throw std::bad_alloc{};
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
#include "delivery/delivery_queue.hpp"
static unsigned disposed = 0;
extern "C" mln_status mln_map_dispose(mln_map, mln_diagnostic*) noexcept { ++disposed; return MLN_STATUS_OK; }
extern "C" void mln_resource_request_release(mln_resource_request_handle) noexcept {}

// Copies count values into one allocation, as a queued completion does.
template <class T>
auto copy(const T* values, std::size_t count) -> const T* {
  const auto& capture = mln::delivery::completion_capture<T>;
  mln::delivery::Arena<false> measure{nullptr, 0};
  const auto before = allocations;
  capture.measure(measure, values, count);
  assert(allocations == before);
  auto* storage = static_cast<std::byte*>(::operator new(measure.offset == 0 ? 1 : measure.offset));
  mln::delivery::Arena<true> write{storage, 0, measure.offset};
  const auto* copied = static_cast<const T*>(capture.write(write, values, count));
  assert(allocations == before + 1);
  return copied;
}

int main() {
  char url[] = "retained URL";
  mln_buffer_view views[] = {{url, sizeof(url) - 1}, {url, 3}};
  mln_style_source_info source{};
  source.id = {url, 8};
  source.fields = MLN_STYLE_SOURCE_INFO_TILEJSON | MLN_STYLE_SOURCE_INFO_ATTRIBUTION;
  source.tilejson.tile_urls = views;
  source.tilejson.tile_url_count = 2;
  source.attribution = {url, 5};
  const auto* copied = copy(&source, 1);
  assert(reinterpret_cast<std::uintptr_t>(copied) % alignof(mln_style_source_info) == 0);
  assert(copied->tilejson.tile_urls != views && copied->tilejson.tile_urls[0].data != url);
  std::memset(url, 'X', sizeof(url));
  assert(std::memcmp(copied->tilejson.tile_urls[0].data, "retained URL", 12) == 0);
  assert(std::memcmp(copied->attribution.data, "retai", 5) == 0);
  assert(std::memcmp(copied->id.data, "retained", 8) == 0);

  // A clear bit leaves the embedded record unread, so the copy never follows
  // its pointer or trusts its count.
  mln_style_source_info absent{};
  absent.fields = MLN_STYLE_SOURCE_INFO_ATTRIBUTION;
  absent.attribution = {url, 5};
  absent.tilejson.tile_urls = reinterpret_cast<const mln_buffer_view*>(1);
  absent.tilejson.tile_url_count = std::numeric_limits<std::size_t>::max();
  absent.tilejson.min_zoom = 3.0;
  copied = copy(&absent, 1);
  const mln_style_source_tile_info zero{};
  assert(std::memcmp(&copied->tilejson, &zero, sizeof(zero)) == 0);

  // A present empty array keeps its presence, and an absent one stays absent.
  assert(copy(&source, 0) != nullptr);
  assert(copy<mln_style_source_info>(nullptr, 0) == nullptr);

  bool overflow = false;
  try { static_cast<void>(copy(&source, std::numeric_limits<std::size_t>::max())); }
  catch (const std::bad_alloc&) { overflow = true; }
  assert(overflow);

  mln_offline_region_info offline{};
  offline.definition.type = std::numeric_limits<std::uint32_t>::max();
  bool variant = false;
  try { static_cast<void>(copy(&offline, 1)); }
  catch (const std::invalid_argument&) { variant = true; }
  assert(variant);

  // A copied map is still the batch's: discarding it disposes the map, and a
  // value that owns no handle has nothing to discard.
  mln_map handles[] = {42, 43};
  const auto* maps = copy(handles, 2);
  assert(maps[0] == 42 && maps[1] == 43);
  mln::delivery::completion_capture<mln_map>.discard(maps, 2);
  assert(disposed == 2);
  assert(mln::delivery::completion_capture<mln_map>.discard_item != nullptr);
  assert(mln::delivery::completion_capture<mln_style_source_info>.discard_item == nullptr);
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
#include "delivery/delivery_queue.hpp"
#include <cassert>
extern "C" mln_status mln_map_dispose(mln_map, mln_diagnostic*) noexcept { return MLN_STATUS_OK; }
extern "C" void mln_resource_request_release(mln_resource_request_handle) noexcept {}
static auto copy(const mln_capture_fixture& input) -> const mln_capture_fixture* {
  const auto& capture = mln::delivery::completion_capture<mln_capture_fixture>;
  mln::delivery::Arena<false> measure{nullptr, 0};
  capture.measure(measure, &input, 1);
  auto* storage = static_cast<std::byte*>(::operator new(measure.offset));
  mln::delivery::Arena<true> write{storage, 0, measure.offset};
  return static_cast<const mln_capture_fixture*>(capture.write(write, &input, 1));
}
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
  auto* copied = copy(input);
  std::memset(bytes, 0, sizeof(bytes));
  assert(std::memcmp(copied->nested.tilejson.tile_urls[0].data, "nested", 6) == 0);
  assert(std::memcmp(copied->extra[0].data, "nested", 6) == 0);
  assert(copied->name == nullptr);

  input.count = 0;
  input.extra = &view;
  view.size = 0;
  copied = copy(input);
  assert(copied->extra != nullptr);
  assert(reinterpret_cast<std::uintptr_t>(copied->extra) % alignof(mln_buffer_view) == 0);
  assert(copied->nested.tilejson.tile_urls[0].data != nullptr);
  assert(copied->nested.tilejson.tile_urls[0].size == 0);

  input.extra = nullptr;
  view.data = nullptr;
  copied = copy(input);
  assert(copied->extra == nullptr);
  assert(copied->nested.tilejson.tile_urls[0].data == nullptr);
}
"""
