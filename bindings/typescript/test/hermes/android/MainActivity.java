package org.maplibre.nativeffi.hermestest;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;
import android.util.Log;

public class MainActivity extends Activity {
  private static native int run(Context context);

  @Override
  public void onCreate(Bundle state) {
    super.onCreate(state);
    String runId = getIntent().getStringExtra("runId");
    // The host reads the render backend under test from its environment,
    // which an activity does not inherit from the shell that starts it.
    String backend = getIntent().getStringExtra("backend");
    if (backend != null) {
      try {
        Os.setenv("MLN_FFI_TEST_BACKEND", backend, true);
      } catch (ErrnoException error) {
        throw new IllegalStateException(error);
      }
    }
    new Thread(
            () -> {
              System.loadLibrary("hermes-test");
              int status = run(getApplicationContext());
              Log.i("HermesTest", "HermesTest exit " + runId + " " + status);
              runOnUiThread(this::finish);
            })
        .start();
  }
}
