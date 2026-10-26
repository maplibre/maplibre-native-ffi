"""Render the Node-API addon: one export function per operation, the export
table, and the converters.

Each export function converts its arguments through the runtime helpers and
the generated converters, calls its C function directly with a diagnostic of
its own, and throws a MaplibreError for a failed status. The converters also
read the deliveries that the realm drains from its queue: each completion's
value through the decoder that its export recorded, and every other payload by
its type.
"""

from __future__ import annotations

from .typescript import NATIVE_DIRECTORY, Lowered
from .typescript_native_values import NativeValues

HEADER = "// Generated from the C headers by tools/bindgen. Do not edit.\n"


def render_export(entry: Lowered) -> str:
    name = entry.plan.name
    lines = [
        f"static napi_value mln_ts_export_{name}(napi_env env, napi_callback_info info) {{",
        "  mln_ts_call call_state;",
        "  mln_ts_call* call = &call_state;",
    ]
    lines += [f"  {line}" for line in entry.c_locals]
    count = len(entry.native_parameters)
    lines.append(f"  if (!mln_ts_begin(call, env, info, {count})) return NULL;")
    lines += [f"  {line}" for line in entry.c_body]
    if any("goto done" in line for line in entry.c_body):
        lines.append("done:")
    lines += ["  return mln_ts_end(call);", "}"]
    helpers = entry.c_helpers + "\n" if entry.c_helpers else ""
    return helpers + "\n".join(lines) + "\n"


def render_view_scope(bound, begin: str, end: str) -> str:
    """The exports that begin and end one borrowed view scope."""
    function = bound.source.functions_by_name[begin]
    handle = function.parameters[0].type.spelling
    return (
        f"static napi_value mln_ts_export_{begin}(napi_env env, napi_callback_info info) {{\n"
        "  mln_ts_call call_state;\n"
        "  mln_ts_call* call = &call_state;\n"
        "  uint64_t handle = 0;\n"
        "  void* scope = NULL;\n"
        "  mln_diagnostic diagnostic;\n"
        "  mln_status status;\n"
        "  if (!mln_ts_begin(call, env, info, 1)) return NULL;\n"
        '  if (!mln_ts_read_u64(call, call->argv[0], &handle, "handle")) goto done;\n'
        "  diagnostic.size = sizeof diagnostic;\n"
        "  diagnostic.message[0] = '\\0';\n"
        f"  status = {begin}(({handle})handle, &scope, &diagnostic);\n"
        "  if (!mln_ts_check(call, status, &diagnostic)) goto done;\n"
        "  call->result = mln_ts_u64(env, (uint64_t)(uintptr_t)scope);\n"
        "done:\n"
        "  return mln_ts_end(call);\n"
        "}\n\n"
        f"static napi_value mln_ts_export_{end}(napi_env env, napi_callback_info info) {{\n"
        "  mln_ts_call call_state;\n"
        "  mln_ts_call* call = &call_state;\n"
        "  uint64_t scope = 0;\n"
        "  if (!mln_ts_begin(call, env, info, 1)) return NULL;\n"
        '  if (mln_ts_read_u64(call, call->argv[0], &scope, "scope")) {\n'
        f"    {end}((void*)(uintptr_t)scope);\n"
        "  }\n"
        "  return mln_ts_end(call);\n"
        "}\n"
    )


def render_native(
    bound,
    natives: NativeValues,
    lowered: list[Lowered],
    scopes: set[tuple[str, str]],
):
    entries = sorted(lowered, key=lambda e: e.plan.name)
    header = (
        HEADER
        + "#ifndef MAPLIBRE_NATIVE_FFI_NODE_GENERATED_ADDON_H\n"
        + "#define MAPLIBRE_NATIVE_FFI_NODE_GENERATED_ADDON_H\n\n"
        + '#include "runtime.h"\n'
        # The headers that the umbrella header leaves out, such as plugin.h.
        + "".join(
            f'#include "{path}"\n'
            for path in sorted(
                {e.plan.function.location.path for e in entries}
                - {"maplibre_native_c.h"}
            )
        )
        + "\n"
        + natives.decoder_ids()
        + "\n"
        + "".join(f"{prototype};\n" for prototype, _ in natives.prototypes())
        + "\n#endif\n"
    )
    exports = "".join(render_export(e) + "\n" for e in entries)
    exports += "".join(
        render_view_scope(bound, begin, end) + "\n" for begin, end in sorted(scopes)
    )
    names = sorted(
        [e.plan.name for e in entries] + [name for pair in scopes for name in pair]
    )
    table = "".join(f'  {{"{name}", mln_ts_export_{name}}},\n' for name in names)
    addon = (
        HEADER
        + "#include <string.h>\n\n"
        + '#include "generated_addon.h"\n\n'
        + exports
        + "static const mln_ts_export exports[] = {\n"
        + table
        + "};\n\n"
        + "NAPI_MODULE_EXPORT napi_value napi_register_module_v1(napi_env env, napi_value exports_object) {\n"
        + "  return mln_ts_module_init(env, exports_object, exports, sizeof exports / sizeof exports[0]);\n"
        + "}\n"
    )
    values = (
        HEADER
        + "#include <stdlib.h>\n#include <string.h>\n\n"
        + '#include "generated_addon.h"\n\n'
        + natives.render_bodies()
    )
    return {
        f"{NATIVE_DIRECTORY}/generated_addon.h": header,
        f"{NATIVE_DIRECTORY}/generated_addon.c": addon,
        f"{NATIVE_DIRECTORY}/generated_values.c": values,
    }
