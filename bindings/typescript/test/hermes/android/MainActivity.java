package org.maplibre.nativeffi.hermestest;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends Activity {
  private static native int run(Context context);

  @Override
  public void onCreate(Bundle state) {
    super.onCreate(state);
    String runId = getIntent().getStringExtra("runId");
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
