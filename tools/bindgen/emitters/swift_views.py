"""Render callback-scoped graphics views guarded by the native session token."""

from .swift import camel, identifier, native_call
from .swift_ownership import owner_name


def operation(plan, values):
    from .swift_dynamic_values import dynamic

    view = plan.view
    output = view.output.value
    value = output.unwrapped
    values.add(value)
    public = values.public(value)
    scoped = public + "View"
    members = []
    for field in value.fields:
        if not field.public:
            continue
        local = identifier(camel(field.name))
        typ = values.public(field.value)
        # The scope guards only what the view's owner lends; a copy of any
        # other member stays readable after the scope ends.
        if field.scoped:
            members.append(
                f"  public var {local}: {typ} {{ get throws {{ try scope.check(); return snapshot.{local} }} }}"
            )
        else:
            members.append(f"  public var {local}: {typ} {{ snapshot.{local} }}")
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
    return (
        f"""func {method}<Result>(_ body: ({scoped}) throws -> Result) throws -> Result {{
  var raw = {value.native}()
  raw.size = UInt32(MemoryLayout<{value.native}>.size)
  return try nativeView(reading: raw, begin: {{ raw, token, diagnostic in {begin} }}, end: {{ {view.end}($0) }}, get: {{ raw, value, diagnostic in {read} }}) {{ raw, scope in try body({scoped}({copied}, scope: scope)) }}
}}
""",
        owner_name(view.owner),
    )
