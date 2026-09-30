"""Managed emitters must reject contracts their runtime cannot represent."""

import re
import subprocess
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import parse

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters import dart, dotnet, kotlin
from tools.bindgen.emitters.dotnet_values import Values
from tools.bindgen.schema import validate


class ManagedEmitterTests(unittest.TestCase):
    def test_kotlin_owner_retains_its_receiver_as_parent(self):
        api = self.parse(
            """
typedef unsigned long long mln_measurement_handle BIND("kind=handle;release=mln_measurement_destroy;dispose=mln_measurement_destroy;parent=mln_map");
BIND("execution=immediate")
mln_status mln_map_measure(mln_map map, mln_measurement_handle *out_owner BIND("direction=out;ownership=owned"), mln_diagnostic *out_diagnostic);
BIND("execution=immediate")
void mln_measurement_destroy(mln_measurement_handle owner);
""",
            map_handle=True,
        )
        self.assertEqual(kotlin.coverage(api)["unsupported"], {})
        (output,) = compile_api(api).operations_by_name["mln_map_measure"].owned_outputs
        self.assertEqual(
            (output.parent_parameter, output.handle.parent), ("map", "mln_map")
        )

    def test_kotlin_conditional_registration_roots_in_its_receiver_until_native_release(
        self,
    ):
        api = self.parse(
            """
typedef void (*mln_release)(void *context BIND("kind=context;lifetime=owner")) BIND("thread=native;failure=contain");
typedef void (*mln_watch)(void *context BIND("kind=context;lifetime=owner")) BIND("reentry=protocol;reentry_owner=registration;reentry_calls=mln_map_close;thread=native;failure=contain");
BIND("execution=immediate;registration=callback;user_data=context;release_callback=release;accepted_unless=done")
mln_status mln_map_watch(mln_map map, mln_watch callback, void *context BIND("kind=context;ownership=borrowed"), mln_release release, _Bool *done BIND("direction=out"), mln_diagnostic *out_diagnostic);
""",
            map_handle=True,
        )
        self.assertEqual(kotlin.coverage(api)["unsupported"], {})
        bound = compile_api(api)
        (registration,) = bound.operations_by_name["mln_map_watch"].direct_registrations
        # Native keeps nothing it reports through the condition.
        self.assertEqual(registration.accepted_unless, "done")
        self.assertTrue(bound.callbacks["mln_watch"].reentry_policy.registration_owner)

    def parse(self, source="", header="metrics.h", map_handle=False, groups=()):
        # Kotlin generates operations only for receivers that are handles.
        source = re.sub(
            r'BIND\("([^"\n]*)"\)(\s+mln_status mln_map_\w+\(mln_map map)',
            r'BIND("receiver=map;\1")\2',
            source,
        )
        api = parse(
            source,
            groups=groups,
            defines=("MLN_PROTOCOL_MAP_CLOSE",) if map_handle else (),
            header=header,
        )
        validate(api)
        return api

    def test_reserved_parameter_names_are_escaped_or_rejected(self):
        for name in ("class", "completion_value", "arena"):
            parameter = "completion" if name == "completion_value" else name
            source = f"""
BIND("execution=command;result=void;shape=none;ownership=value")
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

    def test_public_method_name_collisions_are_rejected(self):
        header = """
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_map_read_scale(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_map_readScale(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        api = self.parse(header)
        self.assertEqual(dotnet.coverage(api)["generated"], [])
        owned = self.parse(header, map_handle=True)
        for emitter in (dart, kotlin):
            self.assertEqual(
                set(emitter.coverage(owned)["generated"]) - {"mln_map_close"}, set()
            )

    def test_dart_commands_generate_on_any_owner(self):
        api = self.parse("""
typedef unsigned long long mln_measurement BIND("kind=handle;release=mln_measurement_close;dispose=mln_measurement_close;parent=none");
BIND("execution=immediate")
void mln_measurement_close(mln_measurement owner);
BIND("receiver=measurement;execution=command;result=void;shape=none;ownership=value")
mln_status mln_measurement_change(mln_measurement measurement, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertIn("mln_measurement_change", dart.coverage(api)["generated"])

    def test_dart_registers_deferred_callbacks_through_the_generated_adapter(self):
        source = """
typedef void (*mln_notice_release)(void *context BIND("kind=context;lifetime=owner")) BIND("thread=native;failure=contain");
typedef unsigned (*mln_notice_callback)(void *context BIND("kind=context;lifetime=owner"), int code, const char *text BIND("length=nul;encoding=utf8;lifetime=call")) BIND("thread=native;failure=0;deferred=1");
BIND("execution=immediate;registration=callback;user_data=context;release_callback=release")
mln_status mln_notice_set_callback(mln_notice_callback callback, void *context BIND("kind=context;ownership=borrowed"), mln_notice_release release, mln_diagnostic *out_diagnostic);
"""
        api = self.parse(source)
        self.assertIn("mln_notice_set_callback", dart.coverage(api)["generated"])
        # Without the annotation, Dart has no way to answer the callback.
        synchronous = self.parse(source.replace(";deferred=1", ""))
        self.assertIn(
            "mln_notice_set_callback", dart.coverage(synchronous)["unsupported"]
        )

    def test_reserved_method_identifiers_are_escaped(self):
        source = """
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_map_class(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        owned = self.parse(source, map_handle=True)
        for emitter in (dart, kotlin):
            with self.subTest(emitter=emitter.__name__):
                self.assertIn("mln_map_class", emitter.coverage(owned)["generated"])

    def test_nullable_input_cannot_be_silently_required(self):
        header = """
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_set_label(mln_map map,
  mln_buffer_view text BIND("encoding=utf8;lifetime=call;nullable=true"),
  const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        api = self.parse(header)
        for emitter in (dotnet,):
            self.assertEqual(emitter.coverage(api)["generated"], [])

        owned = self.parse(header, map_handle=True)
        for emitter in (dart, kotlin):
            with self.subTest(emitter=emitter.__name__):
                self.assertIn("mln_map_set_label", emitter.coverage(owned)["generated"])

    def test_optional_buffer_array_cannot_silently_erase_absence(self):
        for absence in ("nullable=true", "optional=empty"):
            api = self.parse(f"""
BIND("execution=query;result=mln_buffer_view;shape=array;ownership=borrowed;encoding=utf8;{absence}")
mln_status mln_map_labels(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
            for emitter in (dotnet,):
                with self.subTest(absence=absence, emitter=emitter.__name__):
                    self.assertEqual(emitter.coverage(api)["generated"], [])

        header = """
BIND("execution=query;result=mln_buffer_view;shape=array;ownership=borrowed;encoding=utf8;nullable=true")
mln_status mln_map_labels(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        owned = self.parse(header, map_handle=True)
        for emitter in (dart, kotlin):
            with self.subTest(emitter=emitter.__name__):
                self.assertIn("mln_map_labels", emitter.coverage(owned)["generated"])

    def test_binary_optional_result_requires_empty_conversion(self):
        header = """
BIND("execution=query;result=mln_buffer_view;shape=value;ownership=borrowed;encoding=bytes;optional=empty")
mln_status mln_map_bytes(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        api = self.parse(header)
        for emitter in (dotnet,):
            self.assertEqual(emitter.coverage(api)["generated"], [])
        owned = self.parse(header, map_handle=True)
        for emitter in (dart, kotlin):
            with self.subTest(emitter=emitter.__name__):
                self.assertIn("mln_map_bytes", emitter.coverage(owned)["generated"])

    def test_dotnet_new_record_generates_in_its_header_namespace(self):
        api = self.parse("""
typedef struct mln_metric { double event; } mln_metric;
BIND("execution=query;result=mln_metric;shape=value;ownership=borrowed")
mln_status mln_map_metric(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_set_metric(mln_map map, mln_metric metric, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        # The record's header names its public namespace.
        self.assertIn("Metrics/Metric.g.cs", dotnet.generate(api))
        self.assertEqual(len(dotnet.coverage(api)["generated"]), 2)

    def test_dotnet_record_field_name_collisions_are_rejected(self):
        api = self.parse("""
typedef struct mln_metric { double some_value; double someValue; } mln_metric;
BIND("execution=query;result=mln_metric;shape=value;ownership=borrowed")
mln_status mln_map_metric(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(dotnet.coverage(api)["generated"], [])

    def test_dotnet_generates_boolean_outputs(self):
        api = self.parse("""
BIND("execution=immediate")
mln_status mln_map_ready(mln_map map, bool *ready BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(dotnet.emit(api).functions, ("mln_map_ready",))

    def test_counted_output_is_not_treated_as_one_scalar(self):
        api = self.parse("""
BIND("execution=immediate")
mln_status mln_map_samples(mln_map map, unsigned count,
  double *samples BIND("direction=out;length=count"), mln_diagnostic *out_diagnostic);
""")
        for emitter in (dotnet, dart, kotlin):
            self.assertEqual(emitter.coverage(api)["generated"], [])

    def test_dotnet_record_pointer_count_cannot_be_erased(self):
        source = """
typedef struct mln_coordinate { double latitude; double longitude; } mln_coordinate;
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_move(mln_map map,
  const mln_coordinate *coordinates BIND("length=1"), unsigned count,
  const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        self.assertIn("mln_map_move", dotnet.coverage(self.parse(source))["generated"])
        counted = self.parse(source.replace('BIND("length=1")', 'BIND("length=count")'))
        self.assertIn("mln_map_move", dotnet.coverage(counted)["generated"])
        missing = self.parse(source.replace('BIND("length=1")', ""))
        self.assertNotIn("mln_map_move", dotnet.coverage(missing)["generated"])

    def test_dotnet_scalar_absence_cannot_disappear(self):
        source = """
typedef struct mln_metric { double value; } mln_metric;
BIND("execution=query;result=mln_metric;shape=value;ownership=borrowed")
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
BIND("execution=query;result=mln_metric;shape=value;ownership=borrowed")
mln_status mln_map_metric(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertNotIn("mln_map_metric", dotnet.coverage(api)["generated"])

    def test_dotnet_nested_values_preserve_grouped_presence_and_override_defaults(self):
        api = self.parse(groups=("presence_mask",))
        values = Values(api)
        values.record("mln_snapshot")
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
                Path(__file__).resolve().parents[2]
                / "bindings/dotnet/src/Maplibre.NativeFfi/Internal/Memory/NativeCallScope.cs"
            )
            (root / "NativeCallScope.cs").write_text(runtime.read_text())
            (root / "NativeCallbackRoot.cs").write_text(
                (runtime.parent.parent / "Callback/NativeCallbackRoot.cs").read_text()
            )
            (root / "NativeCallbackOwner.cs").write_text(
                (runtime.parent.parent / "Callback/NativeCallbackOwner.cs").read_text()
            )
            (root / "NativeCallbackGuard.cs").write_text(
                (runtime.parent.parent / "Callback/NativeCallbackGuard.cs").read_text()
            )
            (root / "ValueEquality.cs").write_text(
                (runtime.parent.parent / "ValueEquality.cs").read_text()
            )
            (root / "Raw.cs").write_text(
                "namespace Maplibre.NativeFfi.Internal.C { internal unsafe struct mln_buffer_view { public void* data; public nuint size; } }"
            )
            (root / "Program.cs").write_text(
                """
using Maplibre.NativeFfi.Internal.Memory;
enum mln_camera_field : ulong { MLN_CAMERA_CENTER = 1UL << 40, MLN_CAMERA_ZOOM = 2 }
struct mln_lat_lng { public double latitude, longitude; }
struct mln_camera { public uint abi_size; public ulong fields; public double latitude, longitude, zoom; }
unsafe struct mln_snapshot { public mln_camera camera; public ulong generation; public mln_lat_lng* coordinates; public nuint coordinate_count; }
static class NativeMethods {
  public static mln_camera mln_camera_default() => new() {
    fields = (1UL << 40) | 2, latitude = 80, longitude = 90, zoom = 99
  };
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
    Check(zero.fields == ((1UL << 40) | 2) && zero.latitude == 0 && zero.longitude == 0 && zero.zoom == 0);
    var coordinates = stackalloc mln_lat_lng[2];
    coordinates[0] = new mln_lat_lng { latitude = 4, longitude = 5 };
    coordinates[1] = new mln_lat_lng { latitude = 6, longitude = 7 };
    var raw = new mln_snapshot {
      coordinates = coordinates, coordinate_count = 2,
      generation = 42,
      camera = new mln_camera { fields = 1UL << 40, latitude = 13, longitude = -9, zoom = 55 }
    };
    var copy = CopySnapshot(raw);
    raw.camera.latitude = 100;
    Check(copy.Generation == 42 && copy.Camera.Center == new LatLng(13, -9) && copy.Camera.Zoom == null);
    coordinates[0].latitude = 99;
    Check(copy.Coordinates[0] == new LatLng(4, 5) && copy.Coordinates[1] == new LatLng(6, 7));
    using var scope = new NativeCallScope();
    var encoded = NativeSnapshot(copy, scope);
    Check(encoded.coordinate_count == 2 && encoded.coordinates[0].latitude == 4);
  }
}
"""
            )
            result = subprocess.run(
                [
                    "mise",
                    "exec",
                    "--no-deps",
                    "--",
                    "dotnet",
                    "run",
                    "--project",
                    str(root / "Probe.csproj"),
                    "--verbosity",
                    "quiet",
                ],
                capture_output=True,
                text=True,
                timeout=90,
                check=False,
                cwd=Path(__file__).resolve().parents[2] / "bindings/dotnet",
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
