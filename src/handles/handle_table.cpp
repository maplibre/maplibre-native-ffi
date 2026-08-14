#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "handles/handle_table.hpp"

#include "diagnostics/diagnostics.hpp"

namespace mln::core {

namespace {

auto handle_to_hex(std::uint64_t handle) -> std::string {
  constexpr auto digits = std::string_view{"0123456789abcdef"};
  auto text = std::string{"0x"};
  auto leading = true;
  for (auto shift = std::uint32_t{60};; shift -= 4) {
    const auto digit = static_cast<std::size_t>((handle >> shift) & 0xf);
    if (digit != 0 || !leading || shift == 0) {
      leading = false;
      text.push_back(digits[digit]);
    }
    if (shift == 0) {
      break;
    }
  }
  return text;
}

}  // namespace

auto handle_kind_name(std::uint8_t kind) noexcept -> const char* {
  switch (static_cast<HandleKind>(kind)) {
    case HandleKind::Runtime:
      return "mln_runtime";
    case HandleKind::Map:
      return "mln_map";
    case HandleKind::MapProjection:
      return "mln_map_projection";
    case HandleKind::RenderSession:
      return "mln_render_session";
    case HandleKind::OfflineRegionSnapshot:
      return "mln_offline_region_snapshot";
    case HandleKind::OfflineRegionList:
      return "mln_offline_region_list";
    case HandleKind::Buffer:
      return "mln_buffer";
    case HandleKind::StyleIdList:
      return "mln_style_id_list";
    case HandleKind::WakeSource:
      return "mln_wake_source";
    case HandleKind::ResourceRequest:
      return "mln_resource_request_handle";
    case HandleKind::StyleStringList:
      return "mln_style_string_list";
    case HandleKind::GeoJsonSourceData:
      return "mln_geojson_source_data";
  }
  return nullptr;
}

auto classify_handle_fault(
  HandleKind expected, std::uint64_t handle, bool index_in_range
) noexcept -> HandleFault {
  if (handle == 0) {
    return HandleFault::Null;
  }
  const auto kind = handle_kind_of(handle);
  if (handle_kind_name(kind) == nullptr) {
    return HandleFault::NotAHandle;
  }
  if (kind != static_cast<std::uint8_t>(expected)) {
    return HandleFault::WrongKind;
  }
  if (!index_in_range) {
    return HandleFault::Unknown;
  }
  return HandleFault::Stale;
}

auto set_handle_fault_error(
  HandleKind expected, std::uint64_t handle, HandleFault fault
) noexcept -> void {
  try {
    const auto* expected_name =
      handle_kind_name(static_cast<std::uint8_t>(expected));
    auto message = std::string{};
    switch (fault) {
      case HandleFault::Null:
        message = std::string{expected_name} + " handle must not be null";
        break;
      case HandleFault::NotAHandle:
        message = handle_to_hex(handle) + " is not a valid " + expected_name +
                  " handle";
        break;
      case HandleFault::WrongKind:
        message = std::string{"handle "} + handle_to_hex(handle) + " is an " +
                  handle_kind_name(handle_kind_of(handle)) +
                  " handle, not an " + expected_name + " handle";
        break;
      case HandleFault::Unknown:
        message = std::string{expected_name} + " handle " +
                  handle_to_hex(handle) + " was never created by this process";
        break;
      case HandleFault::Stale:
        message = std::string{expected_name} + " handle " +
                  handle_to_hex(handle) +
                  " is stale; the object it named was destroyed";
        break;
    }
    set_thread_error(message.c_str());
  } catch (...) {
    set_thread_error("handle is not live");
  }
}

}  // namespace mln::core
