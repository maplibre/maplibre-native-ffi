package org.maplibre.nativeffi.example.compose

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.AndroidExternalSurface
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.wrapContentWidth
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel

class MainActivity : ComponentActivity() {
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    enableEdgeToEdge()
    setContent { MapScreen() }
  }
}

// The controller lives in a view model, so the map and its session outlive the activity when a
// configuration change, such as a rotation, recreates it. Only the surface comes and goes.
@Composable
fun MapScreen(controller: MapController = viewModel()) {
  Box(Modifier.fillMaxSize().background(Color(0xfff2efe9))) {
    AndroidExternalSurface(Modifier.fillMaxSize().mapGestures { controller.map }) {
      onSurface { surface, width, height ->
        controller.surfaceCreated(surface, width, height)
        surface.onChanged { newWidth, newHeight -> controller.surfaceChanged(newWidth, newHeight) }
        surface.onDestroyed { controller.surfaceDestroyed(this) }
      }
    }
    StatusOverlay(controller.status)
  }
}

@Composable
private fun StatusOverlay(status: Status) {
  if (status.message.isEmpty()) return
  Box(
    Modifier.fillMaxSize().navigationBarsPadding().padding(bottom = 48.dp),
    contentAlignment = Alignment.BottomCenter,
  ) {
    BasicText(
      status.message,
      Modifier.fillMaxWidth(0.9f)
        .wrapContentWidth()
        .background(
          if (status.error) Color(0xfffdecea) else Color(0xebffffff),
          RoundedCornerShape(8.dp),
        )
        .padding(horizontal = 14.dp, vertical = 8.dp),
      style =
        TextStyle(
          color = if (status.error) Color(0xffa1261b) else Color(0xff222222),
          fontSize = 15.sp,
        ),
    )
  }
}
