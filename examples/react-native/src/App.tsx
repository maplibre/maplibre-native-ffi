import { useEffect, useState } from "react";
import { StatusBar, StyleSheet, Text, View } from "react-native";
import { MapSurfaceView } from "react-native-example-host";

import { mapGestures } from "./gestures";
import { MapController, type Status } from "./MapController";

export default function App() {
  const [status, setStatus] = useState<Status>({ message: "", error: false });
  // The view can report its surface before the effect runs, so the controller
  // exists from the first render and starts in the effect.
  const [controller] = useState(() => new MapController(setStatus));
  const [gestures] = useState(() => mapGestures(() => controller.map));

  useEffect(() => {
    controller.start();
    return () => controller.dispose();
  }, [controller]);

  return (
    <View style={styles.root}>
      <StatusBar barStyle="dark-content" />
      <MapSurfaceView
        style={StyleSheet.absoluteFill}
        onLayout={(event) => {
          const { width, height } = event.nativeEvent.layout;
          controller.setSize({
            width: Math.max(1, Math.round(width)),
            height: Math.max(1, Math.round(height)),
          });
        }}
        onSurfaceAvailable={(event) =>
          controller.surfaceAvailable(event.nativeEvent.surface)
        }
        onSurfaceLost={(event) =>
          controller.surfaceLost(event.nativeEvent.surface)
        }
        {...gestures.panHandlers}
      />
      {status.message === "" ? null : (
        <View
          pointerEvents="none"
          style={[styles.status, status.error && styles.error]}
        >
          <Text style={[styles.statusText, status.error && styles.errorText]}>
            {status.message}
          </Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: "#f2efe9" },
  status: {
    position: "absolute",
    alignSelf: "center",
    bottom: 48,
    maxWidth: "90%",
    paddingHorizontal: 14,
    paddingVertical: 8,
    borderRadius: 8,
    backgroundColor: "rgba(255, 255, 255, 0.92)",
  },
  error: { backgroundColor: "#fdecea" },
  statusText: { color: "#222", fontSize: 15 },
  errorText: { color: "#a1261b" },
});
