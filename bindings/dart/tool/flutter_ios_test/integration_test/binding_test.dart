import 'package:integration_test/integration_test.dart';

import '../../../test/internal_support_test.dart' as internal_support;
import '../../../test/lifecycle_test.dart' as lifecycle;
import '../../../test/native_library_test.dart' as native_library;
import '../../../test/render_test.dart' as render;
import '../../../test/resource_transform_test.dart' as resource_transform;
import '../../../test/values_test.dart' as values;

// Suites that collect garbage through the VM service or run standalone VM
// processes remain in the desktop suite.
void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();
  native_library.main();
  values.main();
  internal_support.main();
  lifecycle.main();
  resource_transform.main();
  render.main();
}
