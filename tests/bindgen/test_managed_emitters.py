"""Managed emitters must reject contracts their runtime cannot represent."""

import re
import subprocess
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from tools.bindgen.emitters import dart, dotnet, kotlin
from tools.bindgen.emitters.dotnet_values import Values
from tools.bindgen.frontend import parse_headers
from tools.bindgen.schema import validate

PRELUDE = """
#define BIND(x) __attribute__((annotate("mln:" x)))
typedef unsigned long long mln_map;
typedef int mln_status;
typedef struct mln_diagnostic { unsigned int size; char message[4096]; } mln_diagnostic;
typedef struct mln_completion { void *state; } mln_completion;
typedef struct mln_buffer_view { const void *data; unsigned long size; } mln_buffer_view;
"""
# Kotlin generates operations only for receivers that are handles.
MAP_HANDLE = (
    'typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_close;'
    'dispose=mln_map_close;parent=none");'
)


class ManagedEmitterTests(unittest.TestCase):
    def dart_map(self, source):
        return dart.generate(self.parse(source, map_handle=True))

    def kotlin_map(self, source, platform="jvmMain"):
        api = self.parse(source, map_handle=True)
        return kotlin.generate(api)[
            f"src/{platform}/kotlin/org/maplibre/nativeffi/generated/GeneratedMapOperations.kt"
        ]

    def test_dotnet_new_owner_uses_shared_release_and_copy_reservation(self):
        api = self.parse("""
typedef unsigned long long mln_measurement BIND("kind=handle;release=mln_measurement_close;dispose=mln_measurement_close;parent=none");
BIND("execution=immediate")
mln_status mln_measurement_create(mln_measurement *out_owner BIND("direction=out;ownership=owned"), mln_diagnostic *out_diagnostic);
BIND("execution=immediate")
void mln_measurement_close(mln_measurement owner);
BIND("execution=immediate")
mln_status mln_measurement_read(mln_measurement owner, double *out_value BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        emitted = dotnet.emit(api)
        self.assertEqual(emitted.unsupported, {})
        source = emitted.files["Metrics/MeasurementHandle.Operations.g.cs"]
        self.assertIn("MeasurementHandle Create()", source)
        self.assertIn("NativeMethods.mln_measurement_close(live)", source)
        self.assertIn("using var read = state.Borrow();", source)
        self.assertIn(
            "NativeMethods.mln_measurement_read(read.Handle, &outValue, NativeDiagnostic.Prepare(&diagnostic))",
            source,
        )
        self.assertIn("MlnMeasurement", emitted.files["Internal/C/Handles.g.cs"])

    def test_kotlin_owner_retains_its_parent_and_closes_through_release(self):
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
        files = kotlin.generate(api)
        generated = "src/{}/kotlin/org/maplibre/nativeffi/generated/{}.kt"
        common = files[generated.format("commonMain", "MeasurementHandle")]
        self.assertIn(
            "class MeasurementHandle : GeneratedMeasurementHandleOperations, AutoCloseable",
            common,
        )
        owner = files[generated.format("jvmMain", "MeasurementHandle")]
        self.assertIn("parent: MapHandle,", owner)
        self.assertIn('HandleStateCore("MeasurementHandle", handle, parent,', owner)
        self.assertIn("override fun close() { measurementDestroy() }", owner)
        operations = files[generated.format("jvmMain", "GeneratedMapOperations")]
        self.assertIn(
            "MeasurementHandle(it, this@GeneratedMapOperations as org.maplibre.nativeffi.generated.MapHandle)",
            operations,
        )
        # A release named close() is inherited instead of wrapped.
        self.assertNotIn("fun close()", files[generated.format("jvmMain", "MapHandle")])

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
        files = kotlin.generate(api)
        generated = "src/{}/kotlin/org/maplibre/nativeffi/generated/{}.kt"
        self.assertIn(
            "fun mapWatch(callback: Watch): Boolean",
            files[generated.format("commonMain", "GeneratedMapOperations")],
        )
        operation = files[generated.format("jvmMain", "GeneratedMapOperations")]
        self.assertIn("bindingReadMap { raw ->", operation)
        self.assertIn(
            "registrations.register(GeneratedWatchRegistration(callback), raw.toLong())",
            operation,
        )
        self.assertIn(
            "MapLibreNativeC.mln_map_watch(raw, GeneratedDirectCallbacks.WatchStub, MemorySegment.ofAddress(token), GeneratedDirectCallbacks.WatchReleaseStub, out, diagnostic)",
            operation,
        )
        # Native keeps nothing it reports through the condition, so the scope frees that root.
        self.assertIn(
            "if (!done) registrations.accept(bindingCallbacks); done", operation
        )
        values = files[generated.format("jvmMain", "GeneratedValues")]
        self.assertIn("CallbackAdmission.scope(root.owner,", values)
        self.assertIn("CallbackRoots.release(context.address())", values)

    def parse(self, source, header="metrics.h", map_handle=False):
        prelude = PRELUDE
        if map_handle:
            prelude = prelude.replace("typedef unsigned long long mln_map;", MAP_HANDLE)
            prelude += 'BIND("execution=immediate") void mln_map_close(mln_map map);\n'
        source = re.sub(
            r'BIND\("([^"\n]*)"\)(\s+mln_status mln_map_\w+\(mln_map map)',
            r'BIND("receiver=map;\1")\2',
            source,
        )
        with TemporaryDirectory() as directory:
            path = Path(directory)
            (path / header).write_text(prelude + source)
            api = parse_headers(path)
        validate(api)
        return api

    def test_reserved_parameter_and_generated_local_names_are_rejected(self):
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

            kotlin_source = self.kotlin_map(source)
            local = "`class`" if parameter == "class" else parameter + "Value"
            self.assertIn(f"{local}: Double", kotlin_source)
            self.assertIn(f"bindingMapHandle(), {local}, completion", kotlin_source)
            dart_source = self.dart_map(source)
            self.assertIn(f"double {parameter}Value", dart_source)
            self.assertIn(
                "raw.mln_map_run(_handle.raw, "
                + parameter
                + "Value, completion, nativeDiagnostic)",
                dart_source,
            )

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

    def test_dart_commands_on_any_owner_share_the_receipt_helper(self):
        api = self.parse("""
typedef unsigned long long mln_measurement BIND("kind=handle;release=mln_measurement_close;dispose=mln_measurement_close;parent=none");
BIND("execution=immediate")
void mln_measurement_close(mln_measurement owner);
BIND("receiver=measurement;execution=command;result=void;shape=none;ownership=value")
mln_status mln_measurement_change(mln_measurement measurement, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertIn("mln_measurement_change", dart.coverage(api)["generated"])
        source = dart.generate(api)
        self.assertIn("Future<CommandCompletion> change() => _startCommand(", source)
        self.assertNotIn("_startCommand(NativeCompletionStart", source)

    def test_dart_registers_deferred_callbacks_through_the_generated_adapter(self):
        source = """
typedef void (*mln_notice_release)(void *context BIND("kind=context;lifetime=owner")) BIND("thread=native;failure=contain");
typedef unsigned (*mln_notice_callback)(void *context BIND("kind=context;lifetime=owner"), int code, const char *text BIND("length=nul;encoding=utf8;lifetime=call")) BIND("thread=native;failure=0;deferred=1");
BIND("execution=immediate;registration=callback;user_data=context;release_callback=release")
mln_status mln_notice_set_callback(mln_notice_callback callback, void *context BIND("kind=context;ownership=borrowed"), mln_notice_release release, mln_diagnostic *out_diagnostic);
"""
        api = self.parse(source)
        self.assertIn("mln_notice_set_callback", dart.coverage(api)["generated"])
        generated = dart.generate(api)
        self.assertIn("void noticeSetCallback(NoticeCallback callback)", generated)
        self.assertIn("_globalCallbackPorts.registerDeferred(", generated)
        self.assertIn("mln_adapter_deferred_callback_function(", generated)
        self.assertIn(
            "arguments.text.cast<Utf8>().toDartString()",
            generated,
        )
        # Without the annotation, Dart has no way to answer the callback.
        synchronous = self.parse(source.replace(";deferred=1", ""))
        self.assertIn(
            "mln_notice_set_callback", dart.coverage(synchronous)["unsupported"]
        )

    def test_dart_owners_come_from_handle_plans(self):
        api = self.parse("""
typedef unsigned long long mln_measurement BIND("kind=handle;release=mln_measurement_close;dispose=mln_measurement_close;parent=none");
typedef unsigned long long mln_sample_handle BIND("kind=handle;release=mln_sample_close;dispose=mln_sample_close;parent=mln_measurement");
BIND("execution=immediate")
void mln_measurement_close(mln_measurement owner);
BIND("execution=immediate")
void mln_sample_close(mln_sample_handle sample);
BIND("receiver=measurement;execution=immediate")
mln_status mln_measurement_take_sample(mln_measurement measurement, mln_sample_handle *out_sample BIND("direction=out;ownership=owned"), mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(dart.coverage(api)["unsupported"], {})
        source = dart.generate(api)
        # The owner name drops the handle suffix, and the child keeps its parent.
        self.assertIn("final class SampleHandle implements Finalizable", source)
        self.assertIn("SampleHandle._(this._parent, NativeSample handle)", source)
        self.assertIn("final MeasurementHandle _parent;", source)
        self.assertIn("SampleHandle._(this, NativeSample(outSample.value))", source)
        self.assertEqual(source.count("BigInt get identity"), 2)

    def test_reserved_method_identifiers_are_rejected(self):
        source = """
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_map_class(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        self.assertIn("fun `class`()", self.kotlin_map(source, "commonMain"))
        self.assertIn("classValue()", self.dart_map(source))

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

        source = self.kotlin_map(header)
        self.assertIn("text: String?", source)
        self.assertIn("GeneratedValues.optionalStringView(arena, text)", source)
        source = self.dart_map(header)
        self.assertIn("String? text", source)
        self.assertIn("text == null ? arena<raw.mln_buffer_view>().ref", source)

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
        nullable = self.parse(header)
        kotlin_source = self.kotlin_map(header)
        self.assertIn("Deferred<List<String>?>", kotlin_source)
        self.assertIn(
            "mln_completion_result.value(result).address() == 0L", kotlin_source
        )
        self.assertIn("Future<List<String>?>", dart.generate(nullable))
        self.assertIn("result.value == nullptr ? null", dart.generate(nullable))

    def test_binary_optional_result_requires_empty_conversion(self):
        header = """
BIND("execution=query;result=mln_buffer_view;shape=value;ownership=borrowed;encoding=bytes;optional=empty")
mln_status mln_map_bytes(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        api = self.parse(header)
        for emitter in (dotnet,):
            self.assertEqual(emitter.coverage(api)["generated"], [])
        self.assertIn(".size == 0", dart.generate(api))
        self.assertIn(".takeIf { it.isNotEmpty() }", self.kotlin_map(header))

    def test_dotnet_new_record_namespace_and_keyword_fields_are_generated(self):
        api = self.parse("""
typedef struct mln_metric { double event; } mln_metric;
BIND("execution=query;result=mln_metric;shape=value;ownership=borrowed")
mln_status mln_map_metric(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_set_metric(mln_map map, mln_metric metric, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        output = dotnet.generate(api)
        self.assertIn("Metrics/Metric.g.cs", output)
        methods = output["Map/MapHandle.Operations.g.cs"]
        self.assertIn("using Maplibre.NativeFfi.Metrics;", methods)
        converters = output["Internal/Struct/GeneratedValues.g.cs"]
        self.assertIn("value.@event", converters)
        self.assertIn("@event = value.Event", converters)
        self.assertEqual(len(dotnet.coverage(api)["generated"]), 2)

    def test_dotnet_record_field_name_collisions_are_rejected(self):
        api = self.parse("""
typedef struct mln_metric { double some_value; double someValue; } mln_metric;
BIND("execution=query;result=mln_metric;shape=value;ownership=borrowed")
mln_status mln_map_metric(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(dotnet.coverage(api)["generated"], [])

    def test_dotnet_boolean_output_uses_blittable_pointer(self):
        api = self.parse("""
BIND("execution=immediate")
mln_status mln_map_ready(mln_map map, bool *ready BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        emitted = dotnet.emit(api)
        self.assertEqual(emitted.functions, ("mln_map_ready",))
        code = emitted.files["Map/MapHandle.Operations.g.cs"]
        self.assertIn("bool ready = default", code)
        self.assertIn("return ready;", code)

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
        emitted = dotnet.generate(counted)["Map/MapHandle.Operations.g.cs"]
        self.assertIn("Coordinate[] coordinates", emitted)
        self.assertIn("coordinates.Length", emitted)
        self.assertNotIn("uint count", emitted)
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
        api = self.parse("""
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef enum BIND("kind=bitmask") mln_camera_field : unsigned long long {
  MLN_CAMERA_CENTER = 1ULL << 40, MLN_CAMERA_ZOOM = 2
} mln_camera_field;
typedef struct mln_lat_lng { double latitude; double longitude; } mln_lat_lng;
typedef struct mln_camera {
  uint32_t abi_size BIND("kind=size;default=sizeof");
  uint64_t fields BIND("kind=presence_mask;enum=mln_camera_field");
  double latitude BIND("mask=fields;bit=MLN_CAMERA_CENTER;group_type=mln_lat_lng");
  double longitude BIND("mask=fields;bit=MLN_CAMERA_CENTER;group_type=mln_lat_lng");
  double zoom BIND("mask=fields;bit=MLN_CAMERA_ZOOM");
} mln_camera BIND("default=mln_camera_default");
typedef unsigned long size_t;
typedef struct mln_snapshot {
  mln_camera camera;
  uint64_t generation;
  const mln_lat_lng* coordinates BIND("length=coordinate_count;ownership=borrowed");
  size_t coordinate_count BIND("kind=count");
} mln_snapshot;
BIND("execution=immediate") mln_camera mln_camera_default(void);
BIND("execution=query;result=mln_snapshot;shape=value;ownership=borrowed")
mln_status mln_map_snapshot(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
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
