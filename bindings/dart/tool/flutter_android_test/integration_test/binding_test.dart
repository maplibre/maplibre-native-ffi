import 'package:integration_test/integration_test.dart';

import '../../../test/delivery_queue_test.dart' as delivery_queue;
import '../../../test/lifecycle_test.dart' as lifecycle;
import '../../../test/native_library_test.dart' as native_library;
import '../../../test/render_test.dart' as render;
import '../../../test/resource_transform_test.dart' as resource_transform;
import '../../../test/values_test.dart' as values;

// Mirrors and standalone VM process fixtures remain in the desktop suite.
void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();
  native_library.main();
  values.main();
  lifecycle.main();
  delivery_queue.main();
  resource_transform.main();
  render.main();
}
