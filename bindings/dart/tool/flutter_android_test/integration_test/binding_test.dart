import 'dart:ffi';

import 'package:ffi/ffi.dart';
import 'package:integration_test/integration_test.dart';
import 'package:jni/jni.dart';
import 'package:jni_flutter/jni_flutter.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

import '../../../test/lifecycle_test.dart' as lifecycle;
import '../../../test/native_library_test.dart' as native_library;
import '../../../test/render_test.dart' as render;
import '../../../test/resource_transform_test.dart' as resource_transform;
import '../../../test/values_test.dart' as values;

// Mirrors and standalone VM process fixtures remain in the desktop suite.
void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();
  final context = androidApplicationContext;
  initAndroid(
    NativePointer(_jniEnv().address),
    NativePointer(context.reference.pointer.address),
  );
  native_library.main();
  values.main();
  lifecycle.main();
  resource_transform.main();
  render.main();
}

typedef _GetEnv =
    Int32 Function(Pointer<Void> vm, Pointer<Pointer<Void>> env, Int32 version);

// package:jni attaches this thread to the JVM before any JNI call. JNI's
// invocation interface puts GetEnv at index 6 of the JavaVM's function table.
Pointer<Void> _jniEnv() => using((arena) {
  final vm = arena<Pointer<Void>>();
  if (Jni.env.GetJavaVM(vm.cast()) != 0) {
    throw StateError('JNI has no JavaVM.');
  }
  final getEnv = vm.value
      .cast<Pointer<Pointer<NativeFunction<_GetEnv>>>>()
      .value[6]
      .asFunction<int Function(Pointer<Void>, Pointer<Pointer<Void>>, int)>();
  final env = arena<Pointer<Void>>();
  const jniVersion1_6 = 0x00010006;
  if (getEnv(vm.value, env, jniVersion1_6) != 0) {
    throw StateError('This thread is not attached to the JVM.');
  }
  return env.value;
});
