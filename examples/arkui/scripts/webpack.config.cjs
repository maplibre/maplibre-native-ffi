// Wraps ets-loader's webpack configuration, which build-hap.sh runs from
// ets-loader's directory, and passes it the app's API floor as
// --env compatibleSdkVersion=<api>.
//
// ets-loader runs the ArkTS linter in standard mode, which reports violations
// as errors that fail the build, only when the project's compatibleSdkVersion
// is 10 or later. hvigor sets it on the project. The webpack configuration
// doesn't read it from build.json or the environment, so without it the linter
// runs in compatible mode and only warns.
const path = require("node:path");

const loader = process.cwd();
const { projectConfig } = require(path.join(loader, "main.js"));
const config = require(path.join(loader, "webpack.config.js"));

module.exports = (env, argv) => {
  projectConfig.compatibleSdkVersion = Number(env.compatibleSdkVersion);
  return config(env, argv);
};
