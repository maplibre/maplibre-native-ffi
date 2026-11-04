// Load the host before the app: it installs the binding that the app imports.
import "react-native-example-host";
import { AppRegistry } from "react-native";

import { name } from "./app.json";
import App from "./src/App";

AppRegistry.registerComponent(name, () => App);
