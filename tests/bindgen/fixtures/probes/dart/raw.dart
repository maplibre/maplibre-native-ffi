// The probe takes its records from the generated declarations, and looks its
// functions up in the stub library, because the generated `@Native` externals
// resolve the binding's code asset, which the probe does not build.
// ignore_for_file: non_constant_identifier_names
import 'dart:ffi';
import 'dart:io';

import 'maplibre_native_c.g.dart' hide mln_keyword_combine, mln_probe_hooks_default, mln_probe_read_level, mln_probe_roundtrip, mln_probe_settings_check, mln_probe_settings_default;
export 'maplibre_native_c.g.dart' hide mln_keyword_combine, mln_probe_hooks_default, mln_probe_read_level, mln_probe_roundtrip, mln_probe_settings_check, mln_probe_settings_default;

final _library = DynamicLibrary.open(Platform.environment['MLN_PROBE_LIBRARY']!);

final mln_probe_roundtrip = _library.lookupFunction<
    Int32 Function(
        mln_probe_options, Pointer<mln_probe_options>, Pointer<mln_diagnostic>),
    int Function(mln_probe_options, Pointer<mln_probe_options>,
        Pointer<mln_diagnostic>)>('mln_probe_roundtrip');

final mln_keyword_combine = _library.lookupFunction<
    Int32 Function(Double, Double, Double, Double, Pointer<mln_keyword_entry>,
        Pointer<mln_diagnostic>),
    int Function(double, double, double, double, Pointer<mln_keyword_entry>,
        Pointer<mln_diagnostic>)>('mln_keyword_combine');

final mln_probe_read_level = _library.lookupFunction<
    Int32 Function(Pointer<Double>, Pointer<mln_diagnostic>),
    int Function(
        Pointer<Double>, Pointer<mln_diagnostic>)>('mln_probe_read_level');

final mln_probe_settings_default = _library.lookupFunction<
    mln_probe_settings Function(),
    mln_probe_settings Function()>('mln_probe_settings_default');

final mln_probe_settings_check = _library.lookupFunction<
    Int32 Function(mln_probe_settings, Pointer<mln_diagnostic>),
    int Function(mln_probe_settings,
        Pointer<mln_diagnostic>)>('mln_probe_settings_check');

final mln_probe_hooks_default = _library.lookupFunction<
    mln_probe_hooks Function(),
    mln_probe_hooks Function()>('mln_probe_hooks_default');
