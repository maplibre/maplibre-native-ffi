"""Render callback-scoped graphics views guarded by the native session token."""

from .swift import camel, identifier, native_call
from .swift_ownership import owner_name


def operation(plan, values):
    from .swift_dynamic_values import dynamic

    view = plan.view
    output = plan.outputs[0].value
    value = output.element if output.kind == "reference" else output
    values.add(value)
    public = values.public(value)
    scoped = public + "View"
    members = []
    for field in value.fields:
        if field.role in {"size", "count", "reserved", "presence_mask", "tag"}:
            continue
        local = identifier(camel(field.name))
        members.append(
            f"  public var {local}: {values.public(field.value)} {{ get throws {{ try scope.check(); return snapshot.{local} }} }}"
        )
    values.views[scoped] = f"""public final class {scoped} {{
  private let snapshot: {public}
  private let scope: NativeViewScope
  init(_ value: {public}, scope: NativeViewScope) {{ self.snapshot = value; self.scope = scope }}
{chr(10).join(members)}
}}
"""
    method = camel(
        plan.function.name.removeprefix(view.owner.native + "_").replace(
            "get_", "with_", 1
        )
    )
    copied = f"{'try ' if dynamic(value) else ''}{public}(raw: raw)"
    functions = values.bound.source.functions_by_name
    begin = native_call(functions[view.owner.view_begin], "raw", "token")
    read = native_call(plan.function, "raw", "value")
    initial = value.default + "()" if value.default else value.native + "()"
    return (
        f"""func {method}<Result>(_ body: ({scoped}) throws -> Result) throws -> Result {{
  var raw = {initial}
  raw.size = UInt32(MemoryLayout<{value.native}>.size)
  return try nativeView("{plan.function.name}", reading: raw, begin: {{ raw, token, diagnostic in {begin} }}, end: {{ {view.owner.view_end}($0) }}, get: {{ raw, value, diagnostic in {read} }}) {{ raw, scope in try body({scoped}({copied}, scope: scope)) }}
}}
""",
        owner_name(view.owner.native),
    )
