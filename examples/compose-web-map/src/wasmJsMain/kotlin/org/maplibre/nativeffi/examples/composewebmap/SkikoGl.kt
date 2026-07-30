@file:JsModule("./skiko.mjs")
@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.examples.composewebmap

import kotlin.js.JsAny

@JsName("GL") external val skikoGl: JsAny
