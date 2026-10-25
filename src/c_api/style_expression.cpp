#define MLN_BUILDING_C

#include <cstddef>

#include "style/style_expression.hpp"

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"

auto mln_style_expression_options_default(void) noexcept
  -> mln_style_expression_options {
  return mln::c_api::sized<mln_style_expression_options>();
}

auto mln_style_expression_inputs_default(void) noexcept
  -> mln_style_expression_inputs {
  return mln::c_api::sized<mln_style_expression_inputs>();
}

auto mln_style_expression_create(
  mln_buffer_view expression, const mln_style_expression_options* options,
  mln_style_expression* out_expression, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::style_expression_create(
      expression, options, out_expression
    );
  });
}

auto mln_style_expression_release(mln_style_expression expression) noexcept
  -> void {
  mln::core::style_expression_release(expression);
}

auto mln_style_expression_get_info(
  mln_style_expression expression, mln_style_expression_info* out_info,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::style_expression_get_info(expression, out_info);
  });
}

auto mln_style_expression_evaluate(
  mln_style_expression expression, const mln_style_expression_inputs* inputs,
  char* out_json, size_t json_capacity, size_t* out_json_length,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::style_expression_evaluate(
      expression, inputs, out_json, json_capacity, out_json_length
    );
  });
}
