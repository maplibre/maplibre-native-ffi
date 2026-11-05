// Creates a runtime and releases it, then releases it again and expects the
// library's error for a released handle, through the installed VAPI.

async int run () {
	try {
		var options = new MaplibreNativeFfi.RuntimeOptions ();
		options.set_asset_path (".");
		var runtime = new MaplibreNativeFfi.Runtime.create (options);
		yield runtime.release_async ();
		try {
			yield runtime.release_async ();
			stderr.printf ("a second release succeeded\n");
			return 1;
		} catch (MaplibreNativeFfi.Error error) {
			// The domain's codes are the native statuses, which are negative.
			if (!(error is MaplibreNativeFfi.Error.INVALID_ARGUMENT) || error.code >= 0) {
				stderr.printf ("a second release failed with %s\n", error.message);
				return 1;
			}
		}
		// A runtime that is never released is disposed with its object.
		new MaplibreNativeFfi.Runtime.create (null);
	} catch (GLib.Error error) {
		stderr.printf ("%s\n", error.message);
		return 1;
	}
	return 0;
}

int main () {
	var loop = new MainLoop ();
	var status = 1;
	run.begin ((_, result) => {
		status = run.end (result);
		loop.quit ();
	});
	loop.run ();
	return status;
}
