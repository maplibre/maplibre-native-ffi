const path = require("node:path");

// The app compiles the binding's TypeScript sources from the workspace. React
// Native's preset strips Flow types from every file, which rejects the
// binding's `declare` class fields unless that plugin allows them, and it
// leaves out static class blocks. The npm package ships compiled JavaScript,
// so an app that installs it needs neither.
const binding = path.resolve(__dirname, "../../bindings/typescript/src");
const plugin = (name, from) =>
  require.resolve(name, { paths: [require.resolve(from)] });
const flowStripTypes = plugin(
  "@babel/plugin-transform-flow-strip-types",
  "@react-native/babel-preset",
);
const classStaticBlock = plugin(
  "@babel/plugin-transform-class-static-block",
  "@babel/preset-env",
);

module.exports = {
  presets: ["module:@react-native/babel-preset"],
  overrides: [
    {
      test: (file) => file !== undefined && file.startsWith(binding),
      plugins: [
        [flowStripTypes, { allowDeclareFields: true }],
        classStaticBlock,
      ],
    },
  ],
};
