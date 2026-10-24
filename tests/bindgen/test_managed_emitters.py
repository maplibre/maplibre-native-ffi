"""Managed emitters must reject contracts their runtime cannot represent."""

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    ROOT,
    parse,
    parse_sources,
    protocol_header,
    real_api,
    require_tool,
)

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

    def test_a_dispose_that_is_the_release_closes_with_a_completion_and_disposes_without(
        self,
    ):
        api = self.parse("""
typedef unsigned long long mln_measurement BIND("kind=handle;release=mln_measurement_dispose;dispose=mln_measurement_dispose");
BIND("execution=lifecycle")
mln_status mln_measurement_dispose(mln_measurement measurement, const mln_completion *completion, mln_diagnostic *out_diagnostic);
mln_status mln_measurement_create(mln_measurement *out_measurement BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        generated = dart.generate(api)
        # The release passes a completion, as an asynchronous release does.
        self.assertIn(
            "Future<void> close() => _state.closeAsync((handle) => _submit((arena, completion) => "
            "raw.mln_measurement_dispose(handle.raw, completion, nativeDiagnostic)));",
            generated,
        )
        # The dispose and every disposer pass none.
        self.assertIn(
            "void dispose() => _state.close((handle) => "
            "raw.mln_measurement_dispose(handle.raw, nullptr, nativeDiagnostic));",
            generated,
        )
        self.assertIn(
            "_check(raw.mln_measurement_dispose(handle, nullptr, nativeDiagnostic));",
            generated,
        )

        operations = dotnet.generate(api)["Metrics/MeasurementHandle.Operations.g.cs"]
        self.assertIn("IDisposable, IAsyncDisposable", operations)
        self.assertIn("public Task CloseAsync()", operations)
        self.assertIn(
            "public ValueTask DisposeAsync() => state.Close() ? new(teardown) : default;",
            operations,
        )
        self.assertIn(
            "NativeMethods.mln_measurement_dispose(handle, "
            "NativeCompletion.StartUnit(&nativeCompletion, out var nativePending), Diagnostic);",
            operations,
        )
        # Dispose, finalization, and adoption retire through the abandon disposer.
        self.assertIn("public void Dispose() { state.Retire(); }", operations)
        self.assertIn(
            "Abandon(MlnMeasurement live, mln_diagnostic* diagnostic) => "
            "NativeMethods.mln_measurement_dispose(live, null, diagnostic);",
            operations,
        )

    def test_every_dotnet_owner_has_the_same_handle_members(self):
        generated = dotnet.generate(real_api())
        for path, event_source in (
            ("Runtime/RuntimeHandle.Operations.g.cs", True),
            ("Map/MapHandle.Operations.g.cs", True),
            ("RenderSession/RenderSessionHandle.Operations.g.cs", False),
            ("RenderSession/AcquiredFrameHandle.Operations.g.cs", False),
            ("Projection/MapProjectionHandle.Operations.g.cs", False),
        ):
            operations = generated[path]
            self.assertIn("public ulong Id => state.Handle.Value;", operations)
            self.assertIn("public bool IsClosed => state.IsClosed;", operations)
            self.assertIn(
                "public override string ToString() => state.ToString();", operations
            )
            # Only the handles that runtime events name are their source.
            self.assertEqual(
                event_source,
                "which runtime events report as their" in operations,
                path,
            )

    def test_dart_generates_callback_registrations_only_with_queue_delivery(
        self,
    ):
        text = protocol_header(groups=("delivery", "owned_argument"))
        api = parse_sources({"api.h": text})
        generated = dart.coverage(api)["generated"]
        self.assertIn("mln_notice_set_callback", generated)
        self.assertIn("mln_map_set_query_handler", generated)
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
        self.run_dotnet_probe(
            api,
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
    } catch (ArgumentException error) {
      Check(error.Message == "Snapshot.Camera must not be null.");
    }
    // A field named after a C# keyword keeps its value in both directions.
    Check(CopyMetric(new mln_metric { @event = 4 }).Event == 4);
    Check(NativeMetric(new Metric(5)).@event == 5);
  }
}
""",
        )

    def test_dotnet_native_registrations_cross_unchanged_and_are_consumed_once(self):
        # A wake and a registration with a synchronous callback accept a
        # native registration; a registration whose callback takes an argument
        # does not.
        api = self.parse(
            """
typedef void (*mln_probe_release)(void *user_data);
typedef void (*mln_ping_callback)(void *user_data);
typedef struct mln_ping {
  mln_ping_callback callback BIND("nullable=true");
  void *user_data BIND("kind=context");
  mln_probe_release release_user_data;
} mln_ping BIND("kind=callback_registration;release=release_user_data");
BIND("synchronous=true")
typedef void (*mln_hold_callback)(void *user_data);
typedef struct mln_hold {
  mln_hold_callback lock BIND("nullable=true");
  mln_hold_callback unlock BIND("nullable=true");
  void *user_data BIND("kind=context");
  mln_probe_release release_user_data;
} mln_hold BIND("kind=callback_registration;release=release_user_data");
typedef void (*mln_tally_callback)(void *user_data, double count);
typedef struct mln_tally {
  mln_tally_callback callback BIND("nullable=true");
  void *user_data BIND("kind=context");
  mln_probe_release release_user_data;
} mln_tally BIND("kind=callback_registration;release=release_user_data");
typedef struct mln_hooks { mln_ping ping; mln_hold hold; mln_tally tally; } mln_hooks;
mln_status mln_map_set_hooks(mln_map map, const mln_hooks *hooks, mln_diagnostic *out_diagnostic);
""",
            # The runtime's array helpers take a size_t length.
            groups=("standard_types",),
        )
        values = Values(api)
        plans = [values.record(name) for name in ("mln_ping", "mln_hold", "mln_tally")]
        declarations = "\n".join(values.declaration(plan) for plan in plans)
        methods = "\n".join(
            values.callback_methods(plan) + values.encoder(plan) for plan in plans
        )
        self.run_dotnet_probe(
            api,
            """
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Internal.Memory;
namespace Maplibre.NativeFfi.Internal.Callback {
  static class NativeCallbackFailure { internal static void Report(string callback, Exception error) { } }
}
"""
            + declarations
            + "\nunsafe class Probe {\n"
            + methods
            + """
  static int releases;
  static nint released;
  [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
  static void Call(void* context) { }
  [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
  static void Release(void* context) { releases++; released = (nint)context; }
  static NativePointer Address(nint address) => NativePointer.FromBorrowedAddress(address);
  static void Check(bool test) { if (!test) throw new Exception("native registration contract failed"); }
  static void Main() {
    var call = (nint)(delegate* unmanaged[Cdecl]<void*, void>)&Call;
    var release = (nint)(delegate* unmanaged[Cdecl]<void*, void>)&Release;
    // An accepted call copies every address unchanged, and native code owns the context.
    var ping = Ping.Native(Address(call), Address(7), Address(release));
    var copy = ping;
    using (var scope = new NativeCallScope()) {
      var native = NativePing(ping, scope);
      Check((nint)native.callback == call && (nint)native.user_data == 7 && (nint)native.release_user_data == release);
      scope.Accept();
    }
    Check(releases == 0);
    // A copy shares the claim, so a second call fails before it encodes anything.
    try {
      using var scope = new NativeCallScope();
      NativePing(copy, scope);
      Check(false);
    } catch (MaplibreException error) {
      Check(error.Status == Status.InvalidState);
    }
    Check(releases == 0);
    // The binding releases the context of a call that native code does not accept.
    var hold = Hold.Native(Address(call), Address(release), Address(9), Address(release));
    using (var scope = new NativeCallScope()) {
      var native = NativeHold(hold, scope);
      Check((nint)native.@lock == call && (nint)native.unlock == release && (nint)native.user_data == 9);
    }
    Check(releases == 1 && released == 9);
    // A registration whose callback takes an argument keeps only its delegate form.
    Check(typeof(Tally).GetMethod("Native") is null);
  }
}
""",
        )

    def run_dotnet_probe(self, api, program: str) -> None:
        """Compiles `program` with the generated raw types and the runtime that
        generated values call, and runs it."""
        with TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "Probe.csproj").write_text("""
<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup>
<TargetFramework>net10.0</TargetFramework><OutputType>Exe</OutputType>
<AllowUnsafeBlocks>true</AllowUnsafeBlocks><ImplicitUsings>enable</ImplicitUsings>
<Nullable>enable</Nullable>
</PropertyGroup>
<!-- The binding's GlobalUsings.g.cs imports this namespace. -->
<ItemGroup><Using Include="System.Collections.Immutable" /></ItemGroup></Project>
""")
            source = ROOT / "bindings/dotnet/src/Maplibre.NativeFfi"
            for path in (
                "Internal/Memory/NativeCallScope.cs",
                "Internal/Callback/NativeCallbackRoot.cs",
                "Internal/Callback/NativeRegistration.cs",
                "Internal/ValueArray.cs",
                "Internal/Struct/NativeValues.cs",
                # The probe compiles the generated raw types, and declares the
                # diagnostic and handle interface that the runtime supplies.
                "Internal/C/NativeMethods.cs",
                "MaplibreException.cs",
                "NativePointer.cs",
            ):
                (root / Path(path).name).write_text((source / path).read_text())
            (root / "NativeTypes.g.cs").write_text(
                dotnet_native.generate(compile_api(api))["Internal/C/NativeTypes.g.cs"]
            )
            status = next(
                source
                for path, source in dotnet.emit(api).files.items()
                if path.endswith("/Status.g.cs")
            )
            (root / "Status.g.cs").write_text(status)
            (root / "Program.cs").write_text(program)
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
