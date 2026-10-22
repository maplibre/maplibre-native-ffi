package org.maplibre.nativeffi.testhost

import android.content.Context
import android.os.Bundle
import io.flutter.embedding.android.FlutterActivity

class MainActivity : FlutterActivity() {
  private external fun initialize(context: Context): String?

  override fun onCreate(savedInstanceState: Bundle?) {
    System.loadLibrary("maplibre-native-c")
    System.loadLibrary("maplibre_dart_test")
    initialize(applicationContext)?.let { error(it) }
    super.onCreate(savedInstanceState)
  }
}
