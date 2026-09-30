// FFI declarations for the values and keywords groups of protocols.h, which the
// Dart binding would otherwise take from ffigen.
// ignore_for_file: non_constant_identifier_names, camel_case_types
import 'dart:ffi';
import 'dart:io';

final class mln_buffer_view extends Struct {
  external Pointer<Void> data;
  @Size()
  external int size;
}

final class mln_diagnostic extends Struct {
  @Uint32()
  external int size;
  @Array(4096)
  external Array<Char> message;
}

final class mln_probe_point extends Struct {
  @Double()
  external double type;
  @Double()
  external double gain;
}

final class mln_probe_options extends Struct {
  external mln_buffer_view title;
  @Bool()
  external bool has_point;
  external mln_probe_point point;
  external Pointer<mln_probe_point> left;
  @Uint16()
  external int left_count;
  external Pointer<mln_probe_point> right;
  @Uint32()
  external int right_count;
}

final class mln_keyword_entry extends Struct {
  @Double()
  external double type;
  @Double()
  external double defer;
  @Double()
  external double raw;
}

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
