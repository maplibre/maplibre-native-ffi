import 'package:flutter/widgets.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

void main() {
  runApp(
    Directionality(
      textDirection: TextDirection.ltr,
      child: Center(child: Text('C ABI ${Maplibre.cVersion()}')),
    ),
  );
}
