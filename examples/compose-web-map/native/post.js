Module.importWebGLContext = function (context) {
  const attributes = context.getContextAttributes() || {};
  attributes.majorVersion = 2;
  attributes.enableExtensionsByDefault = true;
  return GL.registerContext(context, attributes);
};

Module.importWebGLTexture = function (texture) {
  const id = GL.getNewId(GL.textures);
  GL.textures[id] = texture;
  return id;
};

Module.unregisterWebGLTexture = function (id) {
  GL.textures[id] = null;
};
