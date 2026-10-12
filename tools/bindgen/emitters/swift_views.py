"""Render callback-scoped graphics views guarded by the native session token."""

from .swift import camel, identifier, native_call
from .swift_ownership import owner_name


def operation(plan, values):
    from .swift_dynamic_values import dynamic

    view = plan.view
    output = view.output.value
    value = output.element if output.kind == "reference" else output
    values.add(value)
    public = values.public(value)
    scoped = public + "View"
    members = []
    for field in value.fields:
        if not field.public:
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
    method = camel("with_" + view.stem)
    copied = f"{'try ' if dynamic(value) else ''}{public}(raw: raw)"
    functions = values.bound.source.functions_by_name
    begin = native_call(functions[view.begin], "raw", "token")
    read = native_call(plan.function, "raw", "value")
    initial = value.default + "()" if value.default else value.native + "()"
    return (
        f"""func {method}<Result>(_ body: ({scoped}) throws -> Result) throws -> Result {{
  var raw = {initial}
  raw.size = UInt32(MemoryLayout<{value.native}>.size)
  return try nativeView("{plan.function.name}", reading: raw, begin: {{ raw, token, diagnostic in {begin} }}, end: {{ {view.end}($0) }}, get: {{ raw, value, diagnostic in {read} }}) {{ raw, scope in try body({scoped}({copied}, scope: scope)) }}
}}
""",
        owner_name(view.owner),
    )
