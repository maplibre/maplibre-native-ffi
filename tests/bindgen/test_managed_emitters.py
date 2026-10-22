"""Managed emitters must reject contracts their runtime cannot represent."""

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import ROOT, parse, parse_sources, protocol_header, require_tool

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters import dart, dart_native, dotnet, dotnet_native, kotlin
from tools.bindgen.emitters.dotnet_values import Values
from tools.bindgen.schema import validate


class ManagedEmitterTests(unittest.TestCase):
    def parse(self, source="", header="metrics.h", map_handle=False, groups=()):
        api = parse(
            source,
            groups=groups,
            defines=("MLN_PROTOCOL_MAP_CLOSE",) if map_handle else (),
            header=header,
        )
        validate(api)
        return api

    def test_reserved_parameter_names_are_rejected_by_dotnet_and_generate_elsewhere(
        self,
    ):
        for name in ("class", "completion_value", "arena"):
            parameter = "completion" if name == "completion_value" else name
            source = f"""
BIND("execution=command")
mln_status mln_map_run(mln_map map, double {parameter}, const mln_completion *done, mln_diagnostic *out_diagnostic);
"""
            api = self.parse(source)
            for emitter in (dotnet,):
                with self.subTest(name=name, emitter=emitter.__name__):
                    self.assertEqual(emitter.coverage(api)["generated"], [])
                    self.assertIn(
                        "identifier",
                        emitter.coverage(api)["unsupported"]["mln_map_run"],
                    )

            owned = self.parse(source, map_handle=True)
            for emitter in (dart, kotlin):
                with self.subTest(name=name, emitter=emitter.__name__):
                    self.assertIn("mln_map_run", emitter.coverage(owned)["generated"])

    def test_native_declarations_keep_inline_array_fields_inline(self):
        # A pointer in place of an inline array would change the struct size,
        # so an array field declares its elements inline, through a typedef too.
        outputs = {}
        for field in ("float m[4];", "mln_mat2 m;"):
            api = self.parse(f"""
typedef float mln_mat2[4];
typedef struct mln_matrix {{ {field} }} mln_matrix;
""")
            bound = compile_api(api)
            outputs[field] = (
                dotnet_native.generate(bound),
                dart_native.generate(bound),
            )
        self.assertEqual(outputs["float m[4];"], outputs["mln_mat2 m;"])

    def test_dart_commands_generate_on_any_owner(self):
        api = self.parse("""
typedef unsigned long long mln_measurement BIND("kind=handle;release=mln_measurement_close;dispose=mln_measurement_close");
void mln_measurement_close(mln_measurement owner);
BIND("execution=command")
mln_status mln_measurement_change(mln_measurement measurement, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertIn("mln_measurement_change", dart.coverage(api)["generated"])

    def test_dart_generates_callback_registrations_only_with_queue_delivery(
        self,
    ):
        text = protocol_header(groups=("delivery", "decision"))
        api = parse_sources({"api.h": text})
        generated = dart.coverage(api)["generated"]
        self.assertIn("mln_notice_set_callback", generated)
        self.assertIn("mln_host_set_provider", generated)
        # Without a queue, Dart has no way to answer the callback.
        synchronous = parse_sources(
            {
                "api.h": text.replace(
                    ' BIND("failure=0;delivery=MLN_DELIVERY_TYPE_NOTICE;queue_result=1")',
                    ' BIND("failure=0")',
                ).replace(
                    '  mln_delivery_queue queue BIND("kind=delivery_queue");\n} mln_notice_handler',
                    "} mln_notice_handler",
                )
            }
        )
        self.assertIn(
            "mln_notice_set_callback", dart.coverage(synchronous)["unsupported"]
        )

    def test_reserved_method_identifiers_generate(self):
        source = """
BIND("execution=query;result=double")
mln_status mln_map_class(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        owned = self.parse(source, map_handle=True)
        for emitter in (dart, kotlin):
            with self.subTest(emitter=emitter.__name__):
                self.assertIn("mln_map_class", emitter.coverage(owned)["generated"])

    def test_dotnet_new_record_generates_in_its_header_namespace(self):
        api = self.parse("""
typedef struct mln_metric { double event; } mln_metric;
BIND("execution=query;result=mln_metric")
mln_status mln_map_metric(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=command")
mln_status mln_map_set_metric(mln_map map, mln_metric metric, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        # The record's header names its public namespace.
        self.assertIn("Metrics/Metric.g.cs", dotnet.generate(api))
        self.assertEqual(len(dotnet.coverage(api)["generated"]), 2)

    def test_dotnet_record_field_name_collisions_are_rejected(self):
        api = self.parse("""
typedef struct mln_metric { double some_value; double someValue; } mln_metric;
BIND("execution=query;result=mln_metric")
mln_status mln_map_metric(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(dotnet.coverage(api)["generated"], [])

    def test_dotnet_generates_boolean_outputs(self):
        api = self.parse("""
mln_status mln_map_ready(mln_map map, bool *ready BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(dotnet.emit(api).functions, ("mln_map_ready",))

    def test_dotnet_scalar_absence_cannot_disappear(self):
        source = """
typedef struct mln_metric { double value; } mln_metric;
BIND("execution=query;result=mln_metric")
mln_status mln_map_metric(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        self.assertIn(
            "mln_map_metric", dotnet.coverage(self.parse(source))["generated"]
        )
        changed = source.replace("double value;", 'double value BIND("nullable=true");')
        self.assertNotIn(
            "mln_map_metric", dotnet.coverage(self.parse(changed))["generated"]
        )

    def test_dotnet_field_enum_cannot_generate_colliding_member_names(self):
        api = self.parse("""
typedef enum mln_mode : unsigned { MLN_MODE_X = 0, MLN_MODE_x = 1 } mln_mode;
typedef struct mln_metric { unsigned mode BIND("enum=mln_mode"); } mln_metric;
BIND("execution=query;result=mln_metric")
mln_status mln_map_metric(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertNotIn("mln_map_metric", dotnet.coverage(api)["generated"])

    def test_dotnet_values_preserve_embedded_presence_defaults_and_keyword_fields(self):
        api = self.parse(
            """
typedef struct mln_metric { double event; } mln_metric;
mln_status mln_map_set_metric(mln_map map, mln_metric metric, mln_diagnostic *out_diagnostic);
mln_status mln_map_set_snapshot(mln_map map, const mln_snapshot *snapshot, mln_diagnostic *out_diagnostic);
BIND("execution=query;result=mln_metric")
mln_status mln_map_metric(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""",
            groups=("presence_mask",),
        )
        values = Values(api)
        values.record("mln_snapshot")
        values.record("mln_metric")
        plans = list(values.plans.values())
        declarations = "\n".join(values.declaration(plan) for plan in plans)
        methods = "\n".join(
            values.encoder(plan) + values.decoder(plan) for plan in plans
        )
        with TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "Probe.csproj").write_text("""
<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup>
<TargetFramework>net10.0</TargetFramework><OutputType>Exe</OutputType>
<AllowUnsafeBlocks>true</AllowUnsafeBlocks><ImplicitUsings>enable</ImplicitUsings>
<Nullable>enable</Nullable>
</PropertyGroup></Project>
""")
            runtime = (
                ROOT
                / "bindings/dotnet/src/Maplibre.NativeFfi/Internal/Memory/NativeCallScope.cs"
            )
            (root / "NativeCallScope.cs").write_text(runtime.read_text())
            (root / "NativeCallbackRoot.cs").write_text(
                (runtime.parent.parent / "Callback/NativeCallbackRoot.cs").read_text()
            )
            (root / "ValueArray.cs").write_text(
                (runtime.parent.parent / "ValueArray.cs").read_text()
            )
            (root / "NativeValues.cs").write_text(
                (runtime.parent.parent / "Struct/NativeValues.cs").read_text()
            )
            # The probe compiles the generated raw types, and declares the
            # diagnostic and handle interface that the runtime supplies.
            (root / "NativeMethods.cs").write_text(
                (runtime.parent.parent / "C/NativeMethods.cs").read_text()
            )
            (root / "NativeTypes.g.cs").write_text(
                dotnet_native.generate(compile_api(api))["Internal/C/NativeTypes.g.cs"]
            )
            errors = runtime.parent.parent.parent / "Error"
            (root / "MaplibreException.cs").write_text(
                (errors / "MaplibreException.cs").read_text()
            )
            status = next(
                source
                for path, source in dotnet.emit(api).files.items()
                if path.endswith("/Status.g.cs")
            )
            (root / "Status.g.cs").write_text(status)
            (root / "Program.cs").write_text(
                """
using Maplibre.NativeFfi.Internal;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using static Maplibre.NativeFfi.Internal.C.mln_camera_field;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;
namespace Maplibre.NativeFfi.Internal.C {
  static partial class NativeMethods {
    public static mln_camera mln_camera_default() => new() {
      fields = MLN_CAMERA_CENTER | MLN_CAMERA_ZOOM,
      center = new mln_lat_lng { latitude = 80, longitude = 90 }, zoom = 99
    };
  }
}
"""
                + declarations
                + "\nunsafe class Probe {\n"
                + methods
                + """
  static void Check(bool test) { if (!test) throw new Exception("conversion contract failed"); }
  static void Main() {
    var absent = NativeCamera(new Camera());
    Check(absent.fields == 0 && absent.abi_size == sizeof(mln_camera));
    var zero = NativeCamera(new Camera { Center = new LatLng(0, 0), Zoom = 0 });
    Check((ulong)zero.fields == ((1UL << 40) | 2) && zero.center.latitude == 0 && zero.center.longitude == 0 && zero.zoom == 0);
    var coordinates = stackalloc mln_lat_lng[2];
    coordinates[0] = new mln_lat_lng { latitude = 4, longitude = 5 };
    coordinates[1] = new mln_lat_lng { latitude = 6, longitude = 7 };
    var raw = new mln_snapshot {
      coordinates = coordinates, coordinate_count = 2,
      generation = 42,
      camera = new mln_camera {
        fields = MLN_CAMERA_CENTER, center = new mln_lat_lng { latitude = 13, longitude = -9 }, zoom = 55
      }
    };
    var copy = CopySnapshot(raw);
    raw.camera.center.latitude = 100;
    Check(copy.Generation == 42 && copy.Camera.Center == new LatLng(13, -9) && copy.Camera.Zoom == null);
    coordinates[0].latitude = 99;
    Check(copy.Coordinates[0] == new LatLng(4, 5) && copy.Coordinates[1] == new LatLng(6, 7));
    using var scope = new NativeCallScope();
    var encoded = NativeSnapshot(copy, scope);
    Check(encoded.coordinate_count == 2 && encoded.coordinates[0].latitude == 4);
    // A record struct's reference member can still be null, which encoding rejects.
    try {
      NativeSnapshot(copy with { Camera = null! }, scope);
      Check(false);
    } catch (Maplibre.NativeFfi.Error.MaplibreException error) {
      Check(error.Diagnostic == "Snapshot.Camera must not be null.");
    }
    // A field named after a C# keyword keeps its value in both directions.
    Check(CopyMetric(new mln_metric { @event = 4 }).Event == 4);
    Check(NativeMetric(new Metric(5)).@event == 5);
  }
}
"""
            )
            # The repository's global.json selects the SDK.
            require_tool(self, "dotnet", ROOT / "bindings/dotnet").run(
                self,
                "run",
                "--project",
                str(root / "Probe.csproj"),
                "--verbosity",
                "quiet",
                cwd=ROOT / "bindings/dotnet",
                timeout=180,
            )
